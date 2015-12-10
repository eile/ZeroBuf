
/* Copyright (c) 2015, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#include "Zerobuf.h"

#include "NonMovingSubAllocator.h"
#include "Schema.h"
#include "StaticSubAllocator.h"
#include "detail/JsonConverter.h"
#include <zerobuf/version.h>

#include <iostream>

namespace zerobuf
{

class Zerobuf::Impl
{
public:
    Impl( AllocatorPtr allocator_ )
        : allocator( std::move( allocator_ ))
    {}

    ~Impl()
    {}

    const void* getZerobufData() const
    {
        return allocator ? allocator->getData() : nullptr;
    }

    size_t getZerobufSize() const
    {
        return allocator ? allocator->getSize() : 0;
    }

    void copyZerobufData( const void* data, size_t size )
    {
        if( !allocator )
            throw std::runtime_error(
                "Can't copy data into empty Zerobuf object" );
        allocator->copyBuffer( data, size );
    }

    Impl& operator = ( const Impl& rhs )
    {
        if( !allocator || !rhs.allocator )
            return *this;

        allocator->copyBuffer( rhs.allocator->getData(),
                               rhs.allocator->getSize( ));
        return *this;
    }

    void copyZerobufArray( const void* data, const size_t size,
                          const size_t arrayNum )
    {
        if( !allocator )
            throw std::runtime_error(
                "Can't copy data into empty Zerobuf object" );

        void* array = allocator->updateAllocation( arrayNum, size );
        ::memcpy( array, data, size );
    }

    AllocatorPtr allocator;
};

Zerobuf::Zerobuf( AllocatorPtr alloc )
    : _impl( new Zerobuf::Impl( std::move( alloc )))
{
    if( alloc )
    {
        uint32_t& version = alloc->getItem< uint32_t >( 0 );
        version = ZEROBUF_VERSION_ABI;
    }
}

Zerobuf::Zerobuf( Zerobuf&& rhs )
{
    *this = rhs;
}

Zerobuf::~Zerobuf()
{}

Zerobuf& Zerobuf::operator = ( const Zerobuf& rhs )
{
    if( this == &rhs )
        return *this;
    if( getZerobufType() != rhs.getZerobufType( ))
        throw std::runtime_error( "Can't assign Zerobuf of a different type" );

    notifyChanging();
    *_impl = *rhs._impl;
    return *this;
}

Zerobuf& Zerobuf::operator = ( Zerobuf&& rhs )
{
    if( this == &rhs )
        return *this;
    if( getZerobufType() != rhs.getZerobufType( ))
        throw std::runtime_error( "Can't assign Zerobuf of a different type" );

    notifyChanging();
    rhs.notifyChanging();
    _impl = std::move( rhs._impl );
    rhs._impl.reset( new Zerobuf::Impl( AllocatorPtr(
                           new NonMovingAllocator( getZerobufStaticSize(),
                                                   getZerobufNumDynamics( )))));
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

void Zerobuf::copyZerobufData( const void* data, size_t size )
{
    notifyChanging();
    _impl->copyZerobufData( data, size );
}

std::string Zerobuf::toJSON() const
{
    if( !_impl->allocator )
        throw std::runtime_error( "Can't convert empty Zerobuf object to JSON");

    Json::Value json;
    JSONConverter converter( getSchemas( ));

    if( converter.toJSON( *_impl->allocator, json ))
        return json.toStyledString();

    std::cerr << "Internal error converting to JSON, got so far:\n"
              << json.toStyledString() << std::endl;
    return std::string();
}

bool Zerobuf::fromJSON( const std::string& string )
{
    if( !_impl->allocator )
        throw std::runtime_error(
            "Can't convert empty Zerobuf object from JSON" );

    notifyChanging();

    Json::Value json;
    Json::Reader reader;
    reader.parse( string, json );

    JSONConverter converter( getSchemas( ));
    return converter.fromJSON( *_impl->allocator, json );
}

bool Zerobuf::operator == ( const Zerobuf& rhs ) const
{
    if( this == &rhs ||
        ( !_impl->allocator && getZerobufType() == rhs.getZerobufType( )))
    {
        return true;
    }
    return toJSON() == rhs.toJSON();
}

bool Zerobuf::operator != ( const Zerobuf& rhs ) const
{
    return !(*this == rhs);
}

Allocator& Zerobuf::getAllocator()
{
    if( !_impl->allocator )
        throw std::runtime_error( "Empty Zerobuf has no allocator" );

    notifyChanging();
    return *_impl->allocator;
}

const Allocator& Zerobuf::getAllocator() const
{
    if( !_impl->allocator )
        throw std::runtime_error( "Empty Zerobuf has no allocator" );

    return *_impl->allocator;
}

void Zerobuf::_copyZerobufArray( const void* data, const size_t size,
                                 const size_t arrayNum )
{
    notifyChanging();
    _impl->copyZerobufArray( data, size, arrayNum );
}

}
