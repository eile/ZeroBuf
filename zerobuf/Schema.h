
/* Copyright (c) 2015, Human Brain Project
 *                     Daniel.Nachbaur@epfl.ch
 */

#ifndef ZEROBUF_SCHEMA_H
#define ZEROBUF_SCHEMA_H

#include <zerobuf/api.h>
#include <zerobuf/NonMovingAllocator.h>
#include <servus/uint128_t.h>
#include <tuple>

namespace zerobuf
{

/**
 * A runtime and optimized representation of the schema file input, which
 * describes the data layout of a ZeroBuf object.
 */
struct Schema
{
    /**
     * The static size of the object.
     * @sa NonMovingBaseAllocator
     */
    const size_t staticSize;

    /**
     * The number of dynamic fields in the object.
     * @sa NonMovingBaseAllocator
     */
    const size_t numDynamics;

    /**
     * The type identifier of the object.
     * @sa ZeroBuf::getZerobufType()
     */
    const uint128_t type;

    /** Access enums for the field tuple in the object.*/
    enum FieldEnum
    {
        FIELD_NAME,       //!< name of the field
        FIELD_TYPE,       //!< type identifier of the field
        FIELD_OFFSET,     //!< offset in bytes in the zerobuf
        FIELD_SIZE,       //!< size of the field (0 for dynamic elements)
        FIELD_ELEMENTS    //!< number of elements (0 for dynamic arrays)
    };

    /**
     * Contains the information to describe one Zerobuf member.
     * @sa FieldEnum
     */
    typedef std::tuple< std::string, uint128_t, size_t, size_t, size_t > Field;

    /** All the fields in object */
    const std::vector< Field > fields;
};



}

#endif
