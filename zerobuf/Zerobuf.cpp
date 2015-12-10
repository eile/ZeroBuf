
/* Copyright (c) 2015, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#include "Zerobuf.h"

#include "ConstVector.h"
#include "NonMovingSubAllocator.h"
#include "Schema.h"
#include "StaticSubAllocator.h"
#include "Vector.h"
#include "detail/JsonConverter.h"
#include <zerobuf/version.h>

#include <iostream>

namespace zerobuf
{

class Zerobuf::Impl
{
public:
    Impl( Allocator* alloc )
        : allocator( alloc )
    {}

    const void* getZerobufData() const
    {
        return allocator ? allocator->getData() : 0;
    }

    size_t getZerobufSize() const
    {
        return allocator ? allocator->getSize() : 0;
    }

    void setZerobufData( const void* data, size_t size )
    {
        if( !allocator )
            throw std::runtime_error(
                "Can't copy data into empty Zerobuf object" );
        allocator->copyBuffer( data, size );
    }

    Impl& operator = ( const Impl& rhs )
    {
        if( !allocator )
            throw std::runtime_error(
                "Can't copy data into empty Zerobuf object" );

        const Allocator* from = rhs.allocator;
        if( !from )
            throw std::runtime_error(
                "Can't copy data from empty Zerobuf object" );

        allocator->copyBuffer( from->getData(), from->getSize( ));
        return *this;
    }

    void setZerobufArray( const void* data, const size_t size,
                          const size_t arrayNum )
    {
        if( !allocator )
            throw std::runtime_error(
                "Can't copy data into empty Zerobuf object" );

        void* array = allocator->updateAllocation( arrayNum, size );
        ::memcpy( array, data, size );
    }

    Allocator* allocator;
};

template<>
void Vector< Zerobuf >::push_back( const Zerobuf& value )
{
    const size_t size_ =  Super::_getSize();
    const uint8_t* oldPtr = reinterpret_cast<const uint8_t*>( data( ));
    uint8_t* newPtr = Vector::_parent->updateAllocation( Super::_index,
                                                         size_ + value.getZerobufSize( ));
    if( oldPtr != newPtr )
        ::memcpy( newPtr, oldPtr, size_ );

    ::memcpy( newPtr + size_,
              value.getZerobufData(),
              value.getZerobufSize());
}

Zerobuf::Zerobuf()
    : _impl( new Zerobuf::Impl( *this, 0 ))
{}

Zerobuf::Zerobuf( Allocator* allocator )
    : _impl( new Zerobuf::Impl( *this, allocator ))
{
    uint32_t& version = allocator->getItem< uint32_t >( 0 );
    version = ZEROBUF_VERSION_ABI;
}

Zerobuf::Zerobuf( Zerobuf&& from )
    : _impl( std::move( from._impl ))
{
    from._impl.reset( new Zerobuf::Impl( from, 0 ));
}

Zerobuf::~Zerobuf()
{}

Zerobuf& Zerobuf::operator = ( const Zerobuf& rhs )
{
    if( this != &rhs )
        *_impl = *rhs._impl;
    return *this;
}

Zerobuf& Zerobuf::operator = ( Zerobuf&& rhs )
{
    if( this == &rhs )
        return *this;
    if( getZerobufType() != rhs.getZerobufType( ))
        throw std::runtime_error( "Can't assign Zerobuf of a different type" );

    *_impl = std::move( *rhs._impl );
    rhs._impl.reset( new Zerobuf::Impl( rhs, 0 ));
    return *this;
}

const void* Zerobuf::getZerobufData() const
{
    return _impl->getZerobufData();
}

size_t Zerobuf::getZerobufSize() const
{
    return _impl->getZerobufSize();
}

void Zerobuf::setZerobufData( const void* data, size_t size )
{
    notifyChanging();
    _impl->setZerobufData( data, size );
}

std::string Zerobuf::toJSON() const
{
    if( !_impl->allocator )
        throw std::runtime_error( "Can't convert empty Zerobuf object to JSON");

    Json::Value rootJSON;
    JSONConverter converter( getSchemas( ));

    if( !converter.fromJSON( allocator, rootJSON ))
        return std::string();
    return rootJSON.toStyledString();
}

bool Zerobuf::fromJSON( const std::string& json )
{
    if( !allocator )
        throw std::runtime_error(
            "Can't convert empty Zerobuf object from JSON" );

    notifyChanging();

    Json::Value rootJSON;
    Json::Reader reader;
    reader.parse( json, rootJSON );

    JSONConverter converter( getSchemas( ));
    return converter.fromJSON( allocator, rootJSON );
}

bool Zerobuf::operator == ( const Zerobuf& rhs ) const
{
    if( this == &rhs )
        return true;
    return toJSON() == rhs.toJSON();
}

bool Zerobuf::operator != ( const Zerobuf& rhs ) const
{
    return !(*this == rhs);
}

Allocator* Zerobuf::getAllocator()
{
    notifyChanging();
    return _impl->allocator;
}

const Allocator* Zerobuf::getAllocator() const
{
    return _impl->allocator;
}

void Zerobuf::_setZerobufArray( const void* data, const size_t size,
                                const size_t arrayNum )
{
    notifyChanging();
    _impl->setZerobufArray( data, size, arrayNum );
}

}
