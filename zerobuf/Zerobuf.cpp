
/* Copyright (c) 2015, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#include "Zerobuf.h"

#include "NonMovingSubAllocator.h"
#include "Schema.h"
#include "StaticSubAllocator.h"
#include "detail/JsonSerialization.h"
#include <zerobuf/version.h>

#include <iostream>

namespace zerobuf
{

class Zerobuf::Impl
{
public:
    Impl( Zerobuf& zerobuf, AllocatorPtr allocator )
        : alloc( std::move( allocator ))
        , _zerobuf( zerobuf )
    {}

    ~Impl()
    {}

    const void* getZerobufData() const
    {
        return alloc ? alloc->getData() : 0;
    }

    size_t getZerobufSize() const
    {
        return alloc ? alloc->getSize() : 0;
    }

    void setZerobufData( const void* data, size_t size )
    {
        if( !alloc )
            throw std::runtime_error(
                "Can't copy data into empty Zerobuf object" );
        alloc->copyBuffer( data, size );
    }

    std::string toJSONString() const
    {
        if( !alloc )
            throw std::runtime_error(
                "Can't convert empty Zerobuf object to JSON" );

        Json::Value rootJSON;
        detail::toJSONValue( alloc.get(), _zerobuf.getSchema(), rootJSON );
        return rootJSON.toStyledString();
    }

    void fromJSON( const std::string& json )
    {
        if( !alloc )
            throw std::runtime_error(
                "Can't convert empty Zerobuf object from JSON" );

        Json::Value rootJSON;
        Json::Reader reader;
        reader.parse( json, rootJSON );
        detail::fromJSONValue( alloc.get(), _zerobuf.getSchema(), rootJSON );
    }

    Impl& operator = ( const Impl& rhs )
    {
        if( !alloc || !rhs.alloc )
            return *this;

        alloc->copyBuffer( rhs.alloc->getData(), rhs.alloc->getSize( ));
        return *this;
    }

    void setZerobufArray( const void* data, const size_t size,
                          const size_t arrayNum )
    {
        if( !alloc )
            throw std::runtime_error(
                "Can't copy data into empty Zerobuf object" );

        void* array = alloc->updateAllocation( arrayNum, size );
        ::memcpy( array, data, size );
    }

    AllocatorPtr alloc;
    Zerobuf& _zerobuf;
};

Zerobuf::Zerobuf()
    : _impl( new Zerobuf::Impl( *this, 0 ))
{}

Zerobuf::Zerobuf( AllocatorPtr alloc )
    : _impl( new Zerobuf::Impl( *this, std::move( alloc )))
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
    rhs._impl.reset( new Zerobuf::Impl( rhs,
        AllocatorPtr( new NonMovingAllocator( getZerobufStaticSize(),
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

void Zerobuf::setZerobufData( const void* data, size_t size )
{
    notifyChanging();
    _impl->setZerobufData( data, size );
}

std::string Zerobuf::toJSON() const
{
    return _impl->toJSONString();
}

void Zerobuf::fromJSON( const std::string& json )
{
    notifyChanging();
    _impl->fromJSON( json );
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

Allocator& Zerobuf::getAllocator()
{
    notifyChanging();
    return *_impl->alloc;
}

const Allocator& Zerobuf::getAllocator() const
{
    return *_impl->alloc;
}

void Zerobuf::_setZerobufArray( const void* data, const size_t size,
                                const size_t arrayNum )
{
    notifyChanging();
    _impl->setZerobufArray( data, size, arrayNum );
}

}
