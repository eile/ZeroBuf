
/* Copyright (c) 2015-2016, Human Brain Project
 *                          Stefan.Eilemann@epfl.ch
 */

#ifndef ZEROBUF_VECTOR_H
#define ZEROBUF_VECTOR_H

#include <zerobuf/DynamicSubAllocator.h> // used inline
#include <zerobuf/Zerobuf.h>             // sfinae type
#include <zerobuf/json.h>                // used inline

#include <cstring>   // memcmp
#include <stdexcept> // std::runtime_error
#include <typeinfo>  // typeid

namespace zerobuf
{
template <class T>
class STLAllocator
{
public:
    typedef T value_type;

    STLAllocator(Allocator& alloc, size_t index) noexcept
        : _alloc(alloc)
        , _index(index)
    {
    }

    const T* allocate(const size_t n) const
    {
        _alloc.updateAllocation(_index, true /*copy*/,
                                n * _getElementSize<T>());
    }
    void deallocate(T* const p, size_t) const noexcept;

private:
    Allocator& _alloc;
    const size_t _index;

    template <class Q = T>
    size_t _getElementSize(
        typename std::enable_if<std::is_base_of<Zerobuf, Q>::value, Q>::type* =
            0) const
    {
        return Q::ZEROBUF_STATIC_SIZE();
    }

    template <class Q = T>
    size_t _getElementSize(
        typename std::enable_if<!std::is_base_of<Zerobuf, Q>::value, Q>::type* =
            0) const
    {
        return sizeof(Q);
    }
};

/**
 * STL vector abstraction for dynamic arrays in a zerobuf.
 *
 * @param T element type
 */
template <class T>
using Vector = std::vector<T, STLAllocator<T>>;
}

#endif
