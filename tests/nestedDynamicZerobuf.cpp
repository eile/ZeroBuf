
/* Copyright (c) 2015, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#define BOOST_TEST_MODULE nestedDynamicZerobuf

#include <boost/test/unit_test.hpp>
#include <testSchema.h>

#include <zerobuf/ConstVector.h>
#include <zerobuf/Generic.h>
#include <zerobuf/Vector.h>
#include <utility>

BOOST_AUTO_TEST_CASE(emptyTestNestedZerobuf)
{
    const test::TestNestedZerobuf testNestedZerobuf;
    BOOST_CHECK( testNestedZerobuf.getNested().empty( ));
}

BOOST_AUTO_TEST_CASE(copyConstructTestNestedZerobuf)
{
    test::TestNestedZerobuf temporary;
    temporary.getNested().push_back( test::TestNested( 1, 2 ));
    BOOST_CHECK_EQUAL( temporary.getNested().size(), 1 );

    const test::TestNestedZerobuf testNestedZerobuf( temporary );
    BOOST_CHECK_EQUAL( testNestedZerobuf.getNested().size(), 1 );
    BOOST_CHECK( testNestedZerobuf.getNested()[0] == test::TestNested( 1, 2 ));
    BOOST_CHECK( temporary == testNestedZerobuf );
}

BOOST_AUTO_TEST_CASE(moveConstructTestNestedZerobuf)
{
    test::TestNestedZerobuf temporary;
    temporary.getNested().push_back( test::TestNested( 1, 2 ));
    const test::TestNestedZerobuf testNestedZerobuf( std::move( temporary ));
    BOOST_CHECK( testNestedZerobuf.getNested().size() == 1 );
    BOOST_CHECK( testNestedZerobuf.getNested()[0] == test::TestNested( 1, 2 ));
    BOOST_CHECK( temporary != testNestedZerobuf );
    BOOST_CHECK( temporary.getNested().empty( ));
}

BOOST_AUTO_TEST_CASE(changeTestNestedZerobuf)
{
    test::TestNestedZerobuf object;
    object.getNested().push_back( test::TestNested( 1, 2 ));
    // object.getNested()[0] = test::TestNested( 3, 4 );
    // BOOST_CHECK( object.getNested()[0] == test::TestNested( 3, 4 ));

    object.setNested( { test::TestNested( 6, 7 ) });;
    BOOST_CHECK( object.getNested()[0] == test::TestNested( 6, 7 ));

    // object.getNested()[0].setIntvalue( 5 );
    // BOOST_CHECK( object.getNested()[0] == test::TestNested( 5, 7 ));
}

const std::string expectedJSON =
    "{\n"
    "   \"nested\" : [\n"
    "      {\n"
    "         \"intvalue\" : 1,\n"
    "         \"uintvalue\" : 2\n"
    "      }\n"
    "   ]\n"
    "}\n";

BOOST_AUTO_TEST_CASE(testNestedZerobufToGeneric)
{
    test::TestNestedZerobuf testNestedZerobuf;
    testNestedZerobuf.getNested().push_back( test::TestNested( 1, 2 ));
    const void* data = testNestedZerobuf.getZerobufData();
    const size_t size = testNestedZerobuf.getZerobufSize();
    const zerobuf::Schemas& schemas = test::TestNestedZerobuf::schemas();

    zerobuf::Generic generic( schemas );
    generic.copyZerobufData( data, size );
    const std::string& json = generic.toJSON();
    BOOST_CHECK_EQUAL( json, expectedJSON );
}

BOOST_AUTO_TEST_CASE(genericToTestNestedZerobuf)
{
    const zerobuf::Schemas& schemas = test::TestNestedZerobuf::schemas();
    zerobuf::Generic generic( schemas );
    generic.fromJSON( expectedJSON );

    test::TestNestedZerobuf expectedTestNestedZerobuf;
    expectedTestNestedZerobuf.getNested().push_back( test::TestNested( 1, 2 ));
    const test::TestNestedZerobuf testNestedZerobuf( generic );
    BOOST_CHECK( testNestedZerobuf == expectedTestNestedZerobuf );
    BOOST_CHECK_EQUAL( testNestedZerobuf.getZerobufNumDynamics(),
                       generic.getZerobufNumDynamics( ));
    BOOST_CHECK_EQUAL( testNestedZerobuf.getZerobufStaticSize(),
                       generic.getZerobufStaticSize( ));
}

BOOST_AUTO_TEST_CASE(testNestedZerobufJSON)
{
    test::TestNestedZerobuf testNestedZerobuf;
    testNestedZerobuf.getNested().push_back( test::TestNested( 1, 2 ));

    const std::string& json = testNestedZerobuf.toJSON();
    BOOST_CHECK_EQUAL( json, expectedJSON );

    test::TestNestedZerobuf testNestedZerobuf2;
    testNestedZerobuf2.fromJSON( json );
    BOOST_CHECK( testNestedZerobuf == testNestedZerobuf2 );
}
