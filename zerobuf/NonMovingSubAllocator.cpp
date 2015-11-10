
/* Copyright (c) 2015, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 *                     grigori.chevtchenko@epfl.ch
 */

#include "NonMovingSubAllocator.h"
#include <zerobuf/version.h>

#include <cstring>

namespace zerobuf
{
NonMovingSubAllocator::NonMovingSubAllocator( Allocator* parent,
                                              const size_t index,
                                              const size_t numDynamic,
                                              const size_t staticSize  )
    : NonMovingBaseAllocator( staticSize, numDynamic )
    , _parent( parent )
    , _index( index )
{}

NonMovingSubAllocator::NonMovingSubAllocator( const NonMovingSubAllocator& from)
    : NonMovingBaseAllocator( from )
    , _parent( from._parent )
    , _index( from._index )
{}

NonMovingSubAllocator::~NonMovingSubAllocator()
{}

NonMovingSubAllocator& NonMovingSubAllocator::operator = (
    const NonMovingSubAllocator& rhs )
{
    if( this == &rhs )
        return *this;

    NonMovingBaseAllocator::operator = ( rhs );
    _parent = rhs._parent;
    _index = rhs._index;
    return *this;
}

uint8_t* NonMovingSubAllocator::getData()
{
    return _parent->getDynamic< uint8_t >( _index );
}

const uint8_t* NonMovingSubAllocator::getData() const
{
    return _parent->getDynamic< uint8_t >( _index );
}

size_t NonMovingSubAllocator::getSize() const
{
    return _parent->getDynamicSize( _index );
}

void NonMovingSubAllocator::copyBuffer( const void* data, const size_t size )
{
    void* to = _parent->updateAllocation( _index, size );
    ::memcpy( to, data, size );
}

void NonMovingSubAllocator::_resize( const size_t newSize )
{
    const size_t oldSize = _parent->getDynamicSize( _index );
    void* oldPtr = getData();
    void* newPtr = _parent->updateAllocation( _index, newSize );
    if( oldPtr != newPtr )
        ::memcpy( newPtr, oldPtr, std::min( newSize, oldSize ));
}

Allocator* NonMovingSubAllocator::clone() const
{
    return new NonMovingSubAllocator( *this );
}

}
