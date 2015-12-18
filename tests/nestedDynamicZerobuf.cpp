
/* Copyright (c) 2015, Human Brain Project
 *                     Stefan.Eilemann@epfl.ch
 */

#define BOOST_TEST_MODULE nestedDynamicZerobuf

#include <boost/test/unit_test.hpp>
#include <testSchema.h>

#include <zerobuf/Generic.h>
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

    const test::TestNestedZerobuf::Nested& nested=testNestedZerobuf.getNested();
    BOOST_CHECK( nested[0] == test::TestNested( 1, 2 ));
    BOOST_CHECK( temporary == testNestedZerobuf );
}

BOOST_AUTO_TEST_CASE(moveConstructTestNestedZerobuf)
{
    test::TestNestedZerobuf temporary;
    temporary.getNested().push_back( test::TestNested( 1, 2 ));
    test::TestNestedZerobuf testNestedZerobuf( std::move( temporary ));

    BOOST_CHECK( testNestedZerobuf.getNested().size() == 1 );
    BOOST_CHECK( testNestedZerobuf.getNested()[0] == test::TestNested( 1, 2 ));
    BOOST_CHECK( temporary != testNestedZerobuf );
    BOOST_CHECK( temporary.getNested().empty( ));

    temporary = std::move( testNestedZerobuf );
    BOOST_CHECK( temporary.getNested().size() == 1 );
    BOOST_CHECK( temporary.getNested()[0] == test::TestNested( 1, 2 ));
    BOOST_CHECK( temporary != testNestedZerobuf );
    BOOST_CHECK( testNestedZerobuf.getNested().empty( ));
}

BOOST_AUTO_TEST_CASE(changeTestNestedZerobuf)
{
    test::TestNestedZerobuf object;
    const test::TestNestedZerobuf& constObject = object;
    object.getNested().push_back( test::TestNested( 1, 2 ));

    test::TestNested threeFour( 3, 4 );
    object.getNested()[0] = threeFour;
    BOOST_CHECK( object.getNested()[0] == test::TestNested( 3, 4 ));
    BOOST_CHECK( threeFour == test::TestNested( 3, 4 ));
    BOOST_CHECK( object.getNested()[0] == threeFour );

    object.getNested()[0] = test::TestNested( 8, 9 );
    BOOST_CHECK( object.getNested()[0] == test::TestNested( 8, 9 ));

    object.getNested()[0] = std::move( test::TestNested( 10, 11 ));
    BOOST_CHECK( object.getNested()[0] == test::TestNested( 10, 11 ));

    object.getNested()[0] = std::move( threeFour );
    BOOST_CHECK( object.getNested()[0] == test::TestNested( 3, 4 ));
    BOOST_CHECK( object.getNested()[0] != threeFour );

    object.setNested( { test::TestNested( 6, 7 ) });;
    BOOST_CHECK( constObject.getNested()[0] == test::TestNested( 6, 7 ));
    BOOST_CHECK( object.getNested()[0] == test::TestNested( 6, 7 ));

    object.getNested()[0].setIntvalue( 5 );
    BOOST_CHECK( constObject.getNested()[0] == test::TestNested( 5, 7 ));
    BOOST_CHECK( object.getNested()[0] == test::TestNested( 5, 7 ));
}

const std::string expectedJSON =
    "{\n"
    "   \"nested\" : [\n"
    "      {\n"
    "         \"intvalue\" : 1,\n"
    "         \"uintvalue\" : 2\n"
    "      },\n"
    "      {\n"
    "         \"intvalue\" : 10,\n"
    "         \"uintvalue\" : 20\n"
    "      }\n"
    "   ]\n"
    "}\n";

BOOST_AUTO_TEST_CASE(testNestedZerobufToGeneric)
{
    test::TestNestedZerobuf testNestedZerobuf;
    testNestedZerobuf.getNested().push_back( test::TestNested( 1, 2 ));
    testNestedZerobuf.getNested().push_back( test::TestNested( 10, 20 ));
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

    test::TestNestedZerobuf expected;
    expected.getNested().push_back( test::TestNested( 1, 2 ));
    expected.getNested().push_back( test::TestNested( 10, 20 ));
    const test::TestNestedZerobuf testNestedZerobuf( generic );
    BOOST_CHECK( testNestedZerobuf == expected );
    BOOST_CHECK_EQUAL( testNestedZerobuf.getZerobufNumDynamics(),
                       generic.getZerobufNumDynamics( ));
    BOOST_CHECK_EQUAL( testNestedZerobuf.getZerobufStaticSize(),
                       generic.getZerobufStaticSize( ));
}

BOOST_AUTO_TEST_CASE(testNestedZerobufJSON)
{
    test::TestNestedZerobuf testNestedZerobuf;
    testNestedZerobuf.getNested().push_back( test::TestNested( 1, 2 ));
    testNestedZerobuf.getNested().push_back( test::TestNested( 10, 20 ));

    const std::string& json = testNestedZerobuf.toJSON();
    BOOST_CHECK_EQUAL( json, expectedJSON );

    test::TestNestedZerobuf testNestedZerobuf2;
    testNestedZerobuf2.fromJSON( json );
    BOOST_CHECK( testNestedZerobuf == testNestedZerobuf2 );
}
