
/* Copyright (c) 2015, Human Brain Project
 *                     Daniel.Nachbaur@epfl.ch
 */

#include "Generic.h"
#include <zerobuf/NonMovingAllocator.h>

namespace zerobuf
{

Generic::Generic( const Schema& schema )
    : Zerobuf( AllocatorPtr( new NonMovingAllocator( schema.staticSize,
                                                     schema.numDynamics )))
    , _schema( schema )
{}

servus::uint128_t Generic::getZerobufType() const
{
    return _schema.type;
}

size_t Generic::getZerobufStaticSize() const
{
    return _schema.staticSize;
}

size_t Generic::getZerobufNumDynamics() const
{
    return _schema.numDynamics;
}

Schema Generic::getSchema() const
{
    return _schema;
}

}
