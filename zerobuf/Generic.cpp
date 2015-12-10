
/* Copyright (c) 2015, Human Brain Project
 *                     Daniel.Nachbaur@epfl.ch
 */

#include "Generic.h"
#include <zerobuf/NonMovingAllocator.h>

namespace zerobuf
{

Generic::Generic( const Schemas& schemas )
    : Zerobuf( AllocatorPtr(
                   new NonMovingAllocator( schemas.front().staticSize,
                                           schemas.front().numDynamics )))
    , _schemas( schemas )
{}

uint128_t Generic::getZerobufType() const
{
    return getSchema().type;
}

size_t Generic::getZerobufStaticSize() const
{
    return getSchema().staticSize;
}

size_t Generic::getZerobufNumDynamics() const
{
    return getSchema().numDynamics;
}

Schema Generic::getSchema() const
{
    return _schemas.front();
}

Schemas Generic::getSchemas() const
{
    return _schemas;
}

}
