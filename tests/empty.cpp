
/* Copyright (c) 2015, Human Brain Project
 *                     Daniel Nachbaur <danielnachbaur@epfl.ch>
 */

#define BOOST_TEST_MODULE empty

#include <boost/test/unit_test.hpp>
#include <testSchema.h>

BOOST_AUTO_TEST_CASE(empty)
{
    const test::TestEmpty empty1;
    test::TestEmpty empty2( empty1 );
    test::TestEmpty empty3 = empty1;;

    BOOST_CHECK_EQUAL( empty1.getZerobufStaticSize(), 0 );
    BOOST_CHECK_EQUAL( empty1.getZerobufNumDynamics(), 0 );
    BOOST_CHECK_EQUAL( empty1.getZerobufSize(), 0 );
    BOOST_CHECK( empty1.getZerobufData() == nullptr );
    BOOST_CHECK( empty1 == empty2 );
    BOOST_CHECK( empty3 == empty2 );
}