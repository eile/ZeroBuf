
/* Copyright (c) 2015, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#include <zerobuf/Schema.h>

#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

namespace zerobuf
{
namespace
{
const uint128_t& _getType( const Schema::Field& field )
{
    return std::get< Schema::FIELD_TYPE >( field );
}

const std::string& _getName( const Schema::Field& field )
{
    return std::get< Schema::FIELD_NAME >( field );
}

bool _isStatic( const Schema::Field& field )
{
    return std::get< Schema::FIELD_SIZE >( field ) > 0;
}

size_t _getOffset( const Schema::Field& field )
{
    return std::get< Schema::FIELD_OFFSET >( field );
}

size_t _getIndex( const Schema::Field& field )
{
    if( _isStatic( field ))
        throw std::runtime_error( "Static elements don't have an index" );

    const size_t offset = _getOffset( field );
    if( offset < 4 || (( offset - 4 ) % 16 ) != 0 )
        throw std::runtime_error( "Not an offset of a dynamic member" );
    return (offset - 4) / 16;
}

size_t _getSize( const Schema::Field& field )
{
    return std::get< Schema::FIELD_SIZE >( field );
}

size_t _getNElements( const Schema::Field& field )
{
    return std::get< Schema::FIELD_ELEMENTS >( field );
}

class JSONConverter
{
public:
    JSONConverter( const Schemas& schemas )
        : _schemas( schemas ) // copy since lifetime of param is not defined
    {
        if( schemas.empty( ))
            throw std::runtime_error( "No schemas given" );

        addConverter< int8_t >( "int8_t" );
        addConverter< uint8_t >( "uint8_t" );
        addConverter< int16_t >( "int16_t" );
        addConverter< uint16_t >( "uint16_t" );
        addConverter< int32_t >( "int32_t" );
        addConverter< uint32_t >( "uint32_t" );
        addConverter< int64_t >( "int64_t" );
        addConverter< uint64_t >( "uint64_t" );
        addConverter< servus::uint128_t >( "servus::uint128_t" );
        addConverter< float >( "float" );
        addConverter< double >( "double" );
        addConverter< bool >( "bool" );
        addConverter< char >( "char" );
        for( const Schema& schema : _schemas )
            addConverter( schema );
    }

    bool toJSON( const Allocator* allocator, Json::Value& value ) const
    {
        const Schema& root = _schemas.front();
        const Schema::Field field( "root", root.type, root.staticSize, 0, 1 );
        return _fromZeroBuf( allocator, field, root, value );
    }

    bool fromJSON( Allocator* allocator, const Json::Value& value ) const
    {
        const Schema& root = _schemas.front();
        const Schema::Field field( "root", root.type, root.staticSize, 0, 1 );
        return _toZeroBuf( allocator, field, root, value );
    }

private:
    typedef std::function< bool( const Allocator*, const Schema::Field&,
                                 Json::Value& ) > ToJSON_f;
    typedef std::function< bool( const Allocator*, const Schema::Field&,
                                 Json::Value& ) const > ToJSONConst_f;
    typedef std::function< bool( Allocator*, const Schema::Field&,
                                 const Json::Value& ) > FromJSON_f;

    typedef std::unordered_map< uint128_t, ToJSON_f > ToJSONMap;
    typedef std::unordered_map< uint128_t, FromJSON_f > FromJSONMap;

    ToJSONMap _toJSONMap;
    FromJSONMap _fromJSONMap;
    Schemas _schemas;

    void addConverter( const Schema& schema )
    {
        _toJSONMap[ schema.type ] = std::bind( &JSONConverter::_fromZeroBuf,
                                               this,
                                               std::placeholders::_1,
                                               std::placeholders::_2, schema,
                                               std::placeholders::_3 );
        _fromJSONMap[ schema.type ] = std::bind( &JSONConverter::_toZeroBuf,
                                                 this,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2, schema,
                                                 std::placeholders::_3 );
    }

    template< class T > void addConverter( const std::string& name )
    {
        const uint128_t& type = servus::make_uint128( name );
        _toJSONMap[ type ] = std::bind( &JSONConverter::_toJSON< T >,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3 );
        _fromJSONMap[ type ] = std::bind( &JSONConverter::_fromJSON< T >, this,
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          std::placeholders::_3 );
    }

    bool _toJSONFunc( const Allocator* allocator, const Schema::Field& field,
                      Json::Value& value ) const
    {
        const uint128_t& type = _getType( field );
        const auto& func = _toJSONMap.find( type );
        if( func == _toJSONMap.end( ))
        {
            std::cerr << "Missing converter for type " << _getName( field )
                      << std::endl;
            return false;
        }
        return func->second( allocator, field, value );
    }

    bool _fromJSONFunc( Allocator* allocator, const Schema::Field& field,
                       const Json::Value& value ) const
    {
        const uint128_t& type = _getType( field );
        const auto& func = _fromJSONMap.find( type );
        if( func == _fromJSONMap.end( ))
        {
            std::cerr << "Missing converter for type " << _getName( field )
                      << std::endl;
            return false;
        }
        return func->second( allocator, field, value );
    }

    // ZeroBuf object conversion
    bool _fromZeroBuf( const Allocator* allocator, const Schema::Field& field,
                       const Schema& schema, Json::Value& value ) const
    {
        switch( _getNElements( field ))
        {
        case 0: // dynamic arroy
            throw std::runtime_error(
                "Dynamic arrays of ZeroBuf objects not implemented" );
        case 1:
        {
            if( _isStatic( field ))
            {
                const ConstStaticSubAllocator subAllocator( allocator,
                                                            _getOffset( field ),
                                                            _getSize( field ));
                return _fromZeroBufItem( &subAllocator, schema, value );
            }

            const bool isRoot = _getOffset( field ) == 0;
            const size_t index = isRoot ? 0 : _getIndex( field );
            const ConstNonMovingSubAllocator subAllocator( allocator, index,
                                                           schema.numDynamic,
                                                           schema.staticSize );
            if( !isRoot )
                allocator = &subAllocator;
            return _fromZeroBufItem( allocator, schema, value );
        }

        default: // static array

        }
    }

    bool _fromZeroBufItem( const Allocator* allocator, const Schema& schema,
                           Json::Value& value ) const
    {
        for( const Schema::Field& field : schema.fields )
        {
            const std::string& name = _getName( field );
            value[ name ] = Json::Value();
            if( !_toJSONFunc( allocator, field, value[ name ]))
                return false;
        }
        return true;
    }

    bool _toZeroBuf( Allocator* allocator, const Schema::Field& field,
                    const Schema& schema, const Json::Value& value ) const
    {
        switch( _getNElements( field ))
        {
        case 0: // dynamic arroy
            assert(); // not implemented
            return false;

        case 1:
        {
            if( _isStatic( field ))
            {
                const ConstSubAllocator subAllocator( allocator,
                                                      _getOffset( field ),
                                                      _getSize( field ));
                return _toZeroBufItem( &subAllocator, value, schema );
            }

            const bool isRoot = _getOffset( field ) == 0;
            const size_t index = isRoot ? 0 : _getIndex( field );
            const NonMovingSubAllocator subAllocator( allocator, index,
                                                      schema.numDynamic,
                                                      schema.staticSize );
            if( !isRoot )
                allocator = &subAllocator;
            return _toZeroBufItem( allocator, schema, value );
        }

        default: // static array
        }
    }

    bool _toZeroBufItem( Allocator* allocator, const Schema& schema,
                         const Json::Value& value ) const
    {
        for( const Schema::Field& field : schema.fields )
        {
            const std::string& name = _getName( field );
            if( !_fromJSONFunc( allocator, field, value[ name ]))
                return false;
        }
        return true;
    }

    // builtin conversions
    template< class T > bool _toJSON( const Allocator* allocator,
                                      const Schema::Field& field,
                                      Json::Value& value ) const
    {
        const size_t offset = _getOffset( field );

        switch( size )
        {
        case 0: // dynamic array
            return _toJSONDynamic< T >( allocator, field, value );

        case 1: // single element
            return _toJSONItem< T >( allocator->getItem< T >( offset ), value );

        default: // static array
        {
            const T* data = allocator->getItemPtr< T >( offset );
            const size_t size = _getSize( field );
            value.resize( size );
            for( size_t i = 0; i < size; ++i )
                if( !_toJSONItem< T >( data[i], value[ uint32_t( i )]))
                    return false;
            return true;
        }
        }
    }

    template< class T > bool _toJSONItem( const T& value,
                                          Json::Value& jsonValue )
    {
        if( std::is_floating_point< T >::value )
            jsonValue = Json::Value( double( value ));
        else if( std::is_same< bool, T >::value )
            jsonValue = Json::Value( value );
        else if( std::is_signed< T >::value )
            jsonValue = Json::Value( Json::Int64( value ));
        else
            jsonValue = Json::Value( Json::UInt64( value ));
        return true;
    }
    template< class T > bool _toJSONDynamic( const Allocator* allocator,
                                             const Schema::Field& field,
                                             Json::Value& value )
    {
        if( _isStatic( field ))
            throw( std::runtime_error( "Internal JSON converter error" ));

        const size_t index = _getIndex( field );
        ConstVector< T > values( allocator, index );
        for( size_t i = 0; i < values.size(); ++i )
            if( !_toJSONItem( values[i], value[ uint32_t( i )]))
                return false;
        return true;
    }

    template< class T > bool _fromJSON( const Allocator* allocator,
                                        const Schema::Field& field,
                                        Json::Value& value )
    {
        const size_t offset = _getOffset( field );

        switch( _getNElements( field ))
        {
        case 0: // dynamic arroy
            return _fromJSONDynamic< T >( allocator, field, value );

        case 1: // single element
            return _fromJSONItem< T >( allocator->getItem<T>( offset ), value );

        default: // static array
        {
            const size_t size = _getSize( field );
            if( value.size() != size )
            {
                std::cerr << "JSON array '" << _getName( field ) << "' of size "
                          << value.size() << " does not match array size "
                          << size << std::endl;
                return false;
            }

            T* data = allocator->getItemPtr< T >( offset );
            for( size_t i = 0; i < size; ++i )
                if( !_fromJSONItem< T >( data[i], value[ uint32_t( i )]))
                    return false;
            return true;
        }
        }
    }

    template< class T > bool _fromJSONItem( T& value,
                                            const Json::Value& jsonValue )
    {
        if( std::is_floating_point< T >( ))
            value = T( jsonValue.asDouble( ));
        else if( std::is_same< bool, T >::value )
            value = jsonValue.asBool();
        else if( std::is_signed< T >::value )
            value = T( jsonValue.asInt64( ));
        else
            value = T( jsonValue.asUInt64( ));
        return true;
    }

    template< class T > bool _fromJSONDynamic( Allocator* allocator,
                                               const Schema::Field& field,
                                               const Json::Value& value )
    {
        if( _isStatic( field ))
            throw( std::runtime_error( "Internal JSON converter error" ));

        const size_t index = _getIndex( field );
        T* data = reinterpret_cast< T* >(
            allocator->updateAllocation( index, value.size() * sizeof( T )));
        for( size_t i = 0; i < size; ++i )
            if( !_fromJSONItem< T >( data[i], value[ uint32_t9 i )))
                return false;
        return true;
    }
};

template<> bool ZeroBuf::_toJSONItem< servus::uint128_t >(
    const uint128_t& value, Json::Value& jsonValue )
{
    jsonValue["high"] = Json::UInt64( value.high( ));
    jsonValue["low"] = Json::UInt64( value.low( ));
    return true;
}

template<> bool ZeroBuf_toJSONItem< std::string >( const std::string& value,
                                                   Json::Value& jsonValue )
{
    jsonValue = Json::Value( value );
    return true;
}

template<> bool ZeroBuf::_toJSONItem< char >( const Allocator* allocator,
                                              const Schema::Field& field,
                                              Json::Value& value )
{
    const size_t index = _getIndex( field );
    const uint8_t* ptr = allocator->getDynamic< const uint8_t >( index );
    const std::string string( ptr, ptr + allocator->getDynamicSize( index ));
    return _toJSONItem< std::string >( string, value );
}

template<> bool ZeroBuf::_fromJSONItem< servus::uint128_t >( uint128_t& value,
                                                  const Json::Value& jsonValue )
{
    value = servus::uint128_t( jsonValue["high"].asUInt64(),
                               jsonValue["low"].asUInt64( ));
    return true;
}

template<> bool ZeroBuf::_fromJSONItem< std::string >( const std::string& value,
                                                       Json::Value& jsonValue )
{
    value = jsonValue.asString();
    return true;
}

template<> bool ZeroBuf::_fromJSONDynamic< char >( Allocator* allocator,
                                                   const Schema::Field& field,
                                                   const Json::Value& value )
{
    const size_t index = _getIndex( field );
    const std::string& string = value.asString();
    void* data = allocator->updateAllocation( index, string.length( ));
    if( !string.empty( ))
        ::memcpy( data, string.data(), string.length( ));
    return true;
}

}
} // zerobuf
