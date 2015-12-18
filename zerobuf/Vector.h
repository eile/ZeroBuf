
/* Copyright (c) 2015, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#ifndef ZEROBUF_VECTOR_H
#define ZEROBUF_VECTOR_H

#include <zerobuf/BaseVector.h> // base class

namespace zerobuf
{
/**
 * Non-const vector
 *
 * @param T element type
 */
template< class T >
class Vector : public BaseVector< Allocator, T >
{
    typedef BaseVector< Allocator, T > Super;

public:
    /**
     * @param alloc The parent allocator that contains the data.
     * @param index Index of the vector in the parent allocator dynamic storage
     */
    Vector( Allocator& alloc, size_t index );
    ~Vector() {}

    template< class Q = T >
    void push_back( const typename
                    std::enable_if<!std::is_base_of<Zerobuf,Q>::value, Q>::type&
                    value )
    {
        const size_t size_ = Super::_getSize();
        const T* oldPtr = size_ == 0 ? nullptr : data();
        T* newPtr = reinterpret_cast< T* >(
            Super::_alloc.updateAllocation( Super::_index,
                                            size_ + sizeof( T )));
        if( oldPtr && oldPtr != newPtr )
            ::memcpy( newPtr, oldPtr, size_ );

        newPtr[ size_ / Super::template _getElementSize< T >() ] = value;
    }

    template< class Q = T >
    void push_back( const typename
                    std::enable_if<std::is_base_of<Zerobuf,Q>::value, Q>::type&
                    value )
    {
        const size_t size_ =  Super::_getSize();
        const uint8_t* oldPtr = size_ == 0 ? nullptr :
                                reinterpret_cast<const uint8_t*>( data( ));
        uint8_t* newPtr = Super::_alloc.updateAllocation( Super::_index,
                                                   size_ +
                                                   value.getZerobufSize( ));
        if( oldPtr && oldPtr != newPtr )
            ::memcpy( newPtr, oldPtr, size_ );

        ::memcpy( newPtr + size_, value.getZerobufData(),
                  value.getZerobufSize( ));
    }

    T* data()
        { return Super::_alloc.template getDynamic< T >( Super::_index ); }

    void clear() { Super::_alloc.updateAllocation( Super::_index, 0 ); }

private:
    Vector();
    void _resize( const size_t size )
        { Super::_alloc.updateAllocation( Super::_index, size ); }
    void copyBuffer( uint8_t* data, size_t size );
};

// Implementation
template< class T > inline
Vector< T >::Vector( Allocator& alloc, const size_t index )
    : BaseVector< Allocator, T >( alloc, index )
{}

template< class T > inline
void Vector< T >::copyBuffer( uint8_t* data_, size_t size )
{
    void* to = Super::_alloc.updateAllocation( Super::_index, size );
    ::memcpy( to, data_, size );
}

}

#endif
