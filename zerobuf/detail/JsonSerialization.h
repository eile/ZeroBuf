
/* Copyright (c) 2015, Human Brain Project
 *                     Daniel.Nachbaur@epfl.ch
 */

#include <zerobuf/ConstVector.h>
#include <zerobuf/NonMovingSubAllocator.h>
#include <zerobuf/Schema.h>
#include <zerobuf/jsoncpp/json/json.h>

#include <iostream>
#include <type_traits>
#include <unordered_map>

namespace zerobuf
{
namespace detail
{

void toJSONValue( const Allocator*, const Schema&, Json::Value& ) {}
void fromJSONValue( Allocator*, const Schema&, const Json::Value& ) {}

} // detail
} // zerobuf
