
/* Copyright (c) 2015, Human Brain Project
 *                     Daniel.Nachbaur@epfl.ch
 */

#include <zerobuf/ConstVector.h>
#include <zerobuf/NonMovingSubAllocator.h>
#include <zerobuf/Schema.h>
#include <zerobuf/jsoncpp/json/json.h>
#include <type_traits>

#include <iostream>

namespace zerobuf
{
namespace detail
{

void toJSONValue( const Allocator*, const Schema&, Json::Value& );
void fromJSONValue( Allocator*, const Schema&, const Json::Value& );

/**
 * Convert type T value from its type to the corresponding JSON type.
 */
template< typename T >
void toJSON( const T& value, Json::Value& jsonValue )
{
     if( std::is_floating_point<T>::value)
        jsonValue = Json::Value((double)value);
     else if( std::is_same<bool,T>::value )
        jsonValue = Json::Value((bool)value);
     else if( std::is_signed<T>::value )
        jsonValue = Json::Value((Json::Int64)value);
     else
        jsonValue = Json::Value((Json::UInt64)value);
}

/** Specialization for servus::uint128_t, represented as an object in JSON */
template<>
void toJSON<servus::uint128_t>( const servus::uint128_t& value,
                                Json::Value& jsonValue )
{
    jsonValue["high"] = Json::UInt64(value.high( ));
    jsonValue["low"] = Json::UInt64(value.low( ));
}

/** Specialization for std::string, represented as an object in JSON */
template<>
void toJSON<std::string>( const std::string& value, Json::Value& jsonValue )
{
    jsonValue = Json::Value( value );
}

/**
 * Extract the static value from the ZeroBuf allocator as described by
 * valueSchema, performs necessary conversion to create a JSON value
 */
template< class T >
void convertStaticToJSONValue( const Allocator* allocator,
                               const Schema::Field& valueSchema,
                               Json::Value& value )
{
    const size_t offset =
        std::get< Schema::FIELD_DATAOFFSET >( valueSchema );
    const size_t size = std::get< Schema::FIELD_SIZE >( valueSchema );

    if( size == 0 )
        toJSON< T >( allocator->getItem< T >( offset ), value );
    else if( std::get< Schema::FIELD_ISSTATIC >( valueSchema )) // static array
    {
        const T* ptr = allocator->getItemPtr< T >( offset );
        value.resize( size );
        for( size_t i = 0; i < size; ++i )
            toJSON( ptr[i], value[uint32_t(i)] );
    }
}

/**
 * Extracts the value from the ZeroBuf allocator as described by valueSchema,
 * performs necessary conversion to create a JSON value
 */
template< class T >
void convertToJSONValue( const Allocator* allocator,
                         const Schema::Field& valueSchema,
                         Json::Value& value )
{
    const size_t offset =
        std::get< Schema::FIELD_DATAOFFSET >( valueSchema );

    if( std::get< Schema::FIELD_ISSTATIC >( valueSchema ))
       convertStaticToJSONValue<T>( allocator, valueSchema, value );
    else /* dynamic array */
    {
        ConstVector< T > values( allocator, offset );
        for( size_t i = 0; i < values.size(); ++i )
            toJSON( values[i], value[uint32_t(i)] );
    }
}

/* Spetialization for string extraction from the allocator */
template<>
void convertToJSONValue<char>( const Allocator* allocator,
                               const Schema::Field& valueSchema,
                               Json::Value& value )
{
    const size_t offset =
        std::get< Schema::FIELD_DATAOFFSET >( valueSchema );

    if( std::get< Schema::FIELD_ISSTATIC >( valueSchema ))
       convertStaticToJSONValue<char>( allocator, valueSchema, value );
    else /* dynamic array */
    {
        const size_t size = std::get< Schema::FIELD_SIZE >( valueSchema );
        const uint8_t* ptr =
            allocator->getDynamic< const uint8_t >( offset );
        const std::string strValue(
            ptr, ptr + allocator->getItem< uint64_t >( size ));
        if( !strValue.empty( ))
            toJSON< std::string >( strValue, value );
    }
}


/**
 * Converts the JSON value to a given typename T
 */
template< typename T >
void fromJSON( T& value, const Json::Value& jsonValue )
{
    if( std::is_floating_point<T>( ))
        value = (T)jsonValue.asDouble();
    else if( std::is_same< bool, T >::value )
        value = jsonValue.asBool();
    else if( std::is_signed<T>::value )
        value = (T)jsonValue.asInt64();
    else
        value = (T)jsonValue.asUInt64();
}

/** Specialization for servus::uint128_t, represented as an object in JSON */
template<>
void fromJSON<servus::uint128_t>( servus::uint128_t& value, const Json::Value& jsonValue )
{
    value = servus::uint128_t( jsonValue["high"].asUInt64(),
                               jsonValue["low"].asUInt64( ));
}

/**
 * Gets the value from JSON value, performs necessary conversions and sets the
 * static value into the ZeroBuf allocator, as described by valueSchema.
 */
template< class T >
void convertStaticFromJSONValue( Allocator* allocator,
                                 const Schema::Field& valueSchema,
                                 const Json::Value& value )
{
    const size_t size = std::get< Schema::FIELD_SIZE >( valueSchema );
    const size_t offset =
        std::get< Schema::FIELD_DATAOFFSET >( valueSchema );
    const std::string key =
        std::get< Schema::FIELD_NAME >( valueSchema );

    if( size == 0 )
        fromJSON< T >( allocator->getItem< T >( offset ), value );
    else
    if( std::get< Schema::FIELD_ISSTATIC >( valueSchema ))
    {
        if( value.size() > size )
        {
            std::cerr << "fromJSON: JSON array '" << key
                      << "' too big; got " << value.size()
                      << ", allowed is " << size << std::endl;
        }
        else
        {
            size_t i = 0;
            for( const auto& jsonVal : value )
              fromJSON< T >( allocator->getItemPtr< T >( offset )[i++], jsonVal );
        }
    }
}

/**
 * Gets the value from JSON value, performs necessary conversions and sets the
 * value(s) into the ZeroBuf allocator, as described by valueSchema.
 */
template< class T >
void convertFromJSONValue( Allocator* allocator,
                           const Schema::Field& valueSchema,
                           const Json::Value& value )
{
    const size_t offset =
        std::get< Schema::FIELD_DATAOFFSET >( valueSchema );

    if( std::get< Schema::FIELD_ISSTATIC >( valueSchema ))
        convertStaticFromJSONValue<T>( allocator, valueSchema, value );
    else
    {
        T* array = reinterpret_cast< T* >(
            allocator->updateAllocation( offset, value.size() *
                                              sizeof( T )));
        size_t i = 0;
        for( const auto& jsonValue : value )
            fromJSON< T >(  array[i++], jsonValue );
    }
}


/* string specialization for converting from JSON value */
template<>
void convertFromJSONValue<char>( Allocator* allocator,
                                 const Schema::Field& valueSchema,
                                 const Json::Value& value )
{

    /* static variable(s) */
    if( std::get< Schema::FIELD_ISSTATIC >( valueSchema ) )
        convertStaticFromJSONValue<char>( allocator, valueSchema, value );
    else
    {
        const size_t offset =
            std::get< Schema::FIELD_DATAOFFSET >( valueSchema );
        const size_t staticSize =
            std::get< Schema::FIELD_SIZE >( valueSchema );

        NonMovingBaseAllocator* parentAllocator =
                dynamic_cast<NonMovingBaseAllocator*>( allocator );

        if( !parentAllocator )
            throw std::runtime_error( "Only NonMovingBaseAllocator is supported" );

        NonMovingSubAllocator subAlloc( parentAllocator, offset, 1, staticSize );
        const std::string& valueStr = value.asString();
        subAlloc.copyBuffer( valueStr.c_str(), valueStr.length( ));
    }
}

template<>
void convertStaticToJSONValue< Zerobuf >( const Allocator* allocator,
                                          const Schema::Field& valueSchema,
                                          Json::Value& value )
{
    const size_t offset = std::get< Schema::FIELD_DATAOFFSET >( valueSchema );
    const size_t size = std::get< Schema::FIELD_SIZE >( valueSchema );
    const Schema::SchemaFunction& schemaFunc =
            std::get< Schema::FIELD_SCHEMAFUNC >( valueSchema );

    const Schema& schema = schemaFunc();
    const size_t staticSize = schema.staticSize;

    const NonMovingBaseAllocator* parentAllocator =
               static_cast< const NonMovingBaseAllocator* >( allocator );

    if( size == 0 )
    {
        ConstSubAllocator subAlloc( parentAllocator, offset,
                                     schema.staticSize );
        toJSONValue( &subAlloc, schema, value );
    }
    else if( std::get< Schema::FIELD_ISSTATIC >( valueSchema )) // static array
    {
        for( size_t i = 0; i < size; ++i )
        {
            ConstSubAllocator subAlloc( parentAllocator,
                                        offset + i * staticSize,
                                        schema.staticSize );
            Json::Value jsonVal;
            toJSONValue( &subAlloc, schema, jsonVal );
            value.append( jsonVal );
        }
    }
}

template<>
void convertToJSONValue< Zerobuf >( const Allocator* allocator,
                                    const Schema::Field& valueSchema,
                                    Json::Value& value )
{
    if( std::get< Schema::FIELD_ISSTATIC >( valueSchema ))
    {
       convertStaticToJSONValue<Zerobuf>( allocator, valueSchema, value );
       return;
    }
    // else dynamic variables

    const size_t offset = std::get< Schema::FIELD_DATAOFFSET >( valueSchema );
    const Schema::SchemaFunction& schemaFunc =
        std::get< Schema::FIELD_SCHEMAFUNC >( valueSchema );
    const Schema& schema = schemaFunc();
    const NonMovingBaseAllocator* parentAllocator =
        static_cast< const NonMovingBaseAllocator* >( allocator );

    const size_t size = parentAllocator->getDynamicSize( offset ) /
                        schema.staticSize;
    const uint64_t dynOff = parentAllocator->getDynamicOffset( offset );

    for( size_t i = 0; i < size; ++i )
    {
        ConstSubAllocator subAlloc( parentAllocator,
                                    dynOff + i * schema.staticSize,
                                    schema.staticSize );
        toJSONValue( &subAlloc, schema,  value[ uint32_t( i )]);
    }
}

template<>
void convertStaticFromJSONValue< Zerobuf >( Allocator* allocator,
                                            const Schema::Field& valueSchema,
                                            const Json::Value& value )
{
    const size_t offset = std::get< Schema::FIELD_DATAOFFSET >( valueSchema );
    const size_t size = std::get< Schema::FIELD_SIZE >( valueSchema );
    const Schema::SchemaFunction& schemaFunc =
           std::get< Schema::FIELD_SCHEMAFUNC >( valueSchema );
    const Schema& schema = schemaFunc();
    const size_t staticSize = schema.staticSize;

    NonMovingBaseAllocator* parentAllocator =
               static_cast< NonMovingBaseAllocator* >( allocator );

    if( size == 0 )
    {
        NonMovingSubAllocator nonMovingAllocator( parentAllocator,
                                                  offset,
                                                  0,
                                                  schema.staticSize );

        fromJSONValue( &nonMovingAllocator, schema, value );
    }
    /* static array */
    else if( std::get< Schema::FIELD_ISSTATIC >( valueSchema ))
    {
        if( value.size() > size )
        {
            const std::string& key = std::get< Schema::FIELD_NAME >( valueSchema );
            std::cerr << "fromJSON: JSON array '" << key
                      << "' too big; got " << value.size()
                      << ", allowed is " << size << std::endl;
        }
        else
        {
            size_t i = 0;
            for( const auto& jsonVal : value )
            {
                NonMovingSubAllocator nonMovingAllocator( parentAllocator,
                                                          offset + i * staticSize,
                                                          0, schema.staticSize );
                fromJSONValue( &nonMovingAllocator, schema, jsonVal );
                ++i;
            }
        }
    }
}

template<>
void convertFromJSONValue< Zerobuf >( Allocator* allocator,
                                      const Schema::Field& valueSchema,
                                      const Json::Value& value )
{
    // static variable(s)
    if( std::get< Schema::FIELD_ISSTATIC >( valueSchema ))
    {
        convertStaticFromJSONValue<Zerobuf>( allocator, valueSchema, value );
        return;
    }
    // else dynamic variables
    NonMovingBaseAllocator* parentAllocator =
        static_cast< NonMovingBaseAllocator* >( allocator );

    const size_t offset = std::get< Schema::FIELD_DATAOFFSET >( valueSchema );
    const Schema::SchemaFunction& schemaFunc =
        std::get< Schema::FIELD_SCHEMAFUNC >( valueSchema );
    const Schema& schema = schemaFunc();
    const size_t size = parentAllocator->getDynamicSize( offset ) /
                        schema.staticSize;

    for( size_t i = 0; i < size; ++i )
    {
        NonMovingSubAllocator nonMovingAllocator( parentAllocator, i, size,
                                                  schema.staticSize );
        fromJSONValue( &nonMovingAllocator, schema, value[ uint32_t(i) ] );
    }
}

typedef std::map< std::string, std::function< void( const Allocator*,
                                                    const Schema::Field&,
                                                    Json::Value& ) >> ToJSONFunctionMap;

typedef std::map< std::string, std::function< void( Allocator*,
                                                    const Schema::Field&,
                                                    const Json::Value& ) >> FromJSONFunctionMap;

/**
 * Keep the function pointers for JSON conversion for types.
 *
 * The keys are type names in JSON. Each type should provide
 * convertToJSONValue<T> and convertFromJSONValue<T> function. For simplicity we
 * assume the function names to be convertToJSONValue, convertFromJSONValue.
 */
struct JSONConverter
{
    ToJSONFunctionMap toJSONFunctionMap;
    FromJSONFunctionMap fromJSONFunctionMap;

    template< class T >
    void addConverterToFunctionMap( const std::string& key )
    {
        toJSONFunctionMap[ key ] = std::bind( &convertToJSONValue< T >,
                                              std::placeholders::_1,
                                              std::placeholders::_2,
                                              std::placeholders::_3 );

        fromJSONFunctionMap[ key ] = std::bind( &convertFromJSONValue< T >,
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3 );
    }

    JSONConverter()
    {
        addConverterToFunctionMap< int8_t >( "int8_t" );
        addConverterToFunctionMap< uint8_t >( "uint8_t" );
        addConverterToFunctionMap< int16_t >( "int16_t" );
        addConverterToFunctionMap< uint16_t >( "uint16_t" );
        addConverterToFunctionMap< int32_t >( "int32_t" );
        addConverterToFunctionMap< uint32_t >( "uint32_t" );
        addConverterToFunctionMap< int64_t >( "int64_t" );
        addConverterToFunctionMap< uint64_t >( "uint64_t" );
        addConverterToFunctionMap< servus::uint128_t >( "servus::uint128_t" );
        addConverterToFunctionMap< float >( "float" );
        addConverterToFunctionMap< double >( "double" );
        addConverterToFunctionMap< bool >( "bool" );
        addConverterToFunctionMap< char >( "char" );
        addConverterToFunctionMap< ::zerobuf::Zerobuf >( "Zerobuf" );

    }
};

namespace{ JSONConverter converter; }

/** Convert a zerobuf::Schema to a JSON value. */
void toJSONValue( const Allocator* allocator, const Schema& schema,
                  Json::Value& rootJSON )
{
    for( const Schema::Field& valueSchema : schema.fields )
    {
        const std::string& type = std::get<Schema::FIELD_TYPE>( valueSchema );
        const std::string& key = std::get<Schema::FIELD_NAME>( valueSchema );
        rootJSON[ key ] = Json::Value();
        converter.toJSONFunctionMap[ type ]( allocator, valueSchema,
                                             rootJSON[ key ]);
    }
}

/** Convert a JSON value into a Zerobuf buffer. */
void fromJSONValue( Allocator* allocator, const Schema& schema,
                    const Json::Value& rootJSON )
{
    for( const Schema::Field& valueSchema : schema.fields )
    {
        const std::string& type = std::get<Schema::FIELD_TYPE>( valueSchema );
        const std::string& key = std::get<Schema::FIELD_NAME>( valueSchema );
        converter.fromJSONFunctionMap[ type ]( allocator, valueSchema,
                                               rootJSON[ key ]);
    }
}

} // detail
} // zerobuf
