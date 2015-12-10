
/* Copyright (c) 2015, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 *                     grigori.chevtchenko@epfl.ch
 */

#include "StaticSubAllocator.h"
#include <zerobuf/version.h>

#include <cstring>

namespace zerobuf
{
template< class A >
StaticSubAllocatorBase< A >::StaticSubAllocatorBase( A* parent,
                                                     const size_t offset,
                                                     const size_t size )
    : _parent( parent )
    , _offset( offset )
    , _size( size )
{}

template< class A >
StaticSubAllocatorBase< A >::StaticSubAllocatorBase(
    const StaticSubAllocatorBase< A >& from )
    : _parent( from._parent )
    , _offset( from._offset )
    , _size( from._size )
{}

template< class A >
StaticSubAllocatorBase< A >::~StaticSubAllocatorBase()
{}

template< class A >
StaticSubAllocatorBase< A >& StaticSubAllocatorBase< A >::operator = (
    const StaticSubAllocatorBase< A >& rhs )
{
    if( this == &rhs )
        return *this;

    _parent = rhs._parent;
    _offset = rhs._offset;
    _size = rhs._size;
    return *this;
}

template< class A > uint8_t* StaticSubAllocatorBase< A >::getData()
{
    return _parent->template getItemPtr< uint8_t >( _offset );
}

template<> uint8_t* StaticSubAllocatorBase< const Allocator >::getData()
{
    throw std::runtime_error( "Non-const data access on const data" );
    return nullptr;
}

template< class A > const uint8_t* StaticSubAllocatorBase< A >::getData() const
{
    return _parent->template getItemPtr< uint8_t >( _offset );
}

template< class A >
void StaticSubAllocatorBase< A >::copyBuffer( const void* data,
                                              const size_t size )
{
    if( size != _size )
        throw std::runtime_error(
            "Can't copy buffer of different size into a static-sized member" );
    ::memcpy( getData(), data, size );
}

template< class A >
uint8_t* StaticSubAllocatorBase< A >::updateAllocation( size_t, size_t )
{
    throw std::runtime_error(
        "Static-sized Zerobuf does not have dynamic allocations" );
    return nullptr;
}

template< class A >
Allocator* StaticSubAllocatorBase< A >::clone() const
{
    return new StaticSubAllocatorBase( *this );
}

template class StaticSubAllocatorBase< Allocator >;
template class StaticSubAllocatorBase< const Allocator >;
}
