
/* Copyright (c) 2015, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 *                     grigori.chevtchenko@epfl.ch
 */

#include "NonMovingSubAllocator.h"
#include <zerobuf/version.h>

#include <cstring>

namespace zerobuf
{
template< class A >
NonMovingSubAllocatorBase< A >::NonMovingSubAllocatorBase(
    A* parent, const size_t index, const size_t numDynamic,
    const size_t staticSize  )
    : NonMovingBaseAllocator( staticSize, numDynamic )
    , _parent( parent )
    , _index( index )
{}

template< class A >
NonMovingSubAllocatorBase< A >::NonMovingSubAllocatorBase(
    const NonMovingSubAllocatorBase< A >& from )
    : NonMovingBaseAllocator( from )
    , _parent( from._parent )
    , _index( from._index )
{}

template< class A > NonMovingSubAllocatorBase< A >::~NonMovingSubAllocatorBase()
{}

template< class A >
NonMovingSubAllocatorBase< A >& NonMovingSubAllocatorBase< A >::operator = (
    const NonMovingSubAllocatorBase< A >& rhs )
{
    if( this == &rhs )
        return *this;

    NonMovingBaseAllocator::operator = ( rhs );
    _parent = rhs._parent;
    _index = rhs._index;
    return *this;
}

template< class A > uint8_t* NonMovingSubAllocatorBase< A >::getData()
{
    return _parent->template getDynamic< uint8_t >( _index );
}

template<> uint8_t* NonMovingSubAllocatorBase< const Allocator >::getData()
{
    throw std::runtime_error( "Non-const access on const allocator" );
    return nullptr;
}

template< class A >
const uint8_t* NonMovingSubAllocatorBase< A >::getData() const
{
    return _parent->template getDynamic< uint8_t >( _index );
}

template< class A > size_t NonMovingSubAllocatorBase< A >::getSize() const
{
    return _parent->getDynamicSize( _index );
}

template< class A >
void NonMovingSubAllocatorBase< A >::copyBuffer( const void* data,
                                                 const size_t size )
{
    void* to = _parent->updateAllocation( _index, size );
    ::memcpy( to, data, size );
}

template<> void
NonMovingSubAllocatorBase< const Allocator >::copyBuffer( const void*,
                                                          const size_t )
{
    throw std::runtime_error( "Non-const access on const allocator" );
}

template< class A >
void NonMovingSubAllocatorBase< A >::_resize( const size_t newSize )
{
    const size_t oldSize = _parent->getDynamicSize( _index );
    void* oldPtr = getData();
    void* newPtr = _parent->updateAllocation( _index, newSize );
    if( oldPtr != newPtr )
        ::memcpy( newPtr, oldPtr, std::min( newSize, oldSize ));
}

template<>
void NonMovingSubAllocatorBase< const Allocator >::_resize( const size_t )
{
    throw std::runtime_error( "Non-const access on const allocator" );
}

template class NonMovingSubAllocatorBase< Allocator >;
template class NonMovingSubAllocatorBase< const Allocator >;

}
