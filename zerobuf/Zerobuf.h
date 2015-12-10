
/* Copyright (c) 2015, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 *                     grigori.chevtchenko@epfl.ch
 */

#ifndef ZEROBUF_ZEROBUF_H
#define ZEROBUF_ZEROBUF_H

#include <zerobuf/api.h>
#include <zerobuf/Types.h>
#include <servus/uint128_t.h>
#include <memory> // std::unique_ptr

namespace zerobuf
{

/**
 * Base class for all zerobufs.
 *
 * Zerobuf objects can serialize/deserialize directly from their member storage
 * and from and to JSON.
 */
class Zerobuf
{
public:
    Zerobuf( Zerobuf&& from );

    ZEROBUF_API Zerobuf& operator = ( const Zerobuf& rhs );
    ZEROBUF_API Zerobuf& operator = ( Zerobuf&& rhs );

    virtual uint128_t getZerobufType() const = 0;
    virtual size_t getZerobufStaticSize() const = 0;
    virtual size_t getZerobufNumDynamics() const = 0;
    virtual Schema getSchema() const = 0;
    virtual Schemas getSchemas() const = 0;

    /** Called if any data in this object is about to change. */
    virtual void notifyChanging() {}

    /** Called by ZeroEQ subscriber upon receive() of this object. */
    virtual void notifyReceived() {}

    /** @return the pointer to the zerobuf. */
    ZEROBUF_API const void* getZerobufData() const;

    /** @return the size of the zerobuf. */
    ZEROBUF_API size_t getZerobufSize() const;

    /** Copy the raw data into the zerobuf. */
    ZEROBUF_API void copyZerobufData( const void* data, size_t size );

    /** Convert the given instance to a JSON representation. */
    ZEROBUF_API std::string toJSON() const;

    /**
     * Convert the given JSON string into this object.
     *
     * When a parse error occurs, the object may be partially updated by the
     * already parsed data.
     *
     * @return true on success, false on error.
     */
    ZEROBUF_API bool fromJSON( const std::string& json );

    ZEROBUF_API bool operator == ( const Zerobuf& rhs ) const;
    ZEROBUF_API bool operator != ( const Zerobuf& rhs ) const;

    /* @internal */
    const Allocator& getAllocator() const;

protected:
    explicit Zerobuf( AllocatorPtr alloc ); // takes ownership of alloc
    Zerobuf( const Zerobuf& zerobuf ) = delete;
    ZEROBUF_API virtual ~Zerobuf();

    Allocator& getAllocator();

    ZEROBUF_API void _copyZerobufArray( const void* data, size_t size,
                                        size_t arrayNum );
private:
    class Impl;
    std::unique_ptr<Impl> _impl;

    Zerobuf() = delete;
};

}

#endif
