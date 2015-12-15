
/* Copyright (c) 2015, Human Brain Project
 *                     Daniel Nachbaur <danielnachbaur@epfl.ch>
 */

#define BOOST_TEST_MODULE camera

#include <boost/test/unit_test.hpp>

#include <zerobuf/render/camera.h>


BOOST_AUTO_TEST_CASE(emptyCamera)
{
    const zerobuf::render::Camera camera;
    const zerobuf::render::Vector3f empty;
    BOOST_CHECK( camera.getOrigin() == empty );
    BOOST_CHECK( camera.getLookAt() == empty );
    BOOST_CHECK( camera.getUp() == empty );
}

BOOST_AUTO_TEST_CASE(initializeCamera)
{
    const zerobuf::render::Camera camera( zerobuf::render::Vector3f( 1, 0, 0 ),
                                          zerobuf::render::Vector3f( -1, 1, 0 ),
                                          zerobuf::render::Vector3f( 0, 0, 1 ));
    BOOST_CHECK( camera.getOrigin() == zerobuf::render::Vector3f( 1, 0, 0 ));
    BOOST_CHECK( camera.getLookAt() == zerobuf::render::Vector3f( -1, 1, 0 ));
    BOOST_CHECK( camera.getUp() == zerobuf::render::Vector3f( 0, 0, 1 ));
}

BOOST_AUTO_TEST_CASE(changeCamera)
{
    zerobuf::render::Camera camera;
    camera.setOrigin( zerobuf::render::Vector3f( 1.f, 0.f, 0.f ));
    camera.setLookAt( zerobuf::render::Vector3f( 0.f, 1.f, 0.f ));
    camera.setUp( zerobuf::render::Vector3f( 0.f, 0.f, 1.f ));
    BOOST_CHECK_EQUAL( camera.getOrigin().getX(), 1.f );
    BOOST_CHECK_EQUAL( camera.getLookAt().getY(), 1.f );
    BOOST_CHECK_EQUAL( camera.getUp().getZ(), 1.f );
}

BOOST_AUTO_TEST_CASE(cameraJSON)
{
    zerobuf::render::Camera camera;
    camera.setOrigin( zerobuf::render::Vector3f( 1.f, 0.f, 0.f ));
    camera.setLookAt( zerobuf::render::Vector3f( 0.f, 1.f, 0.f ));
    camera.setUp( zerobuf::render::Vector3f( 0.f, 0.f, 1.f ));

    const std::string expectedJSON = "{\n"
                                     "   \"lookAt\" : {\n"
                                     "      \"x\" : 0,\n"
                                     "      \"y\" : 1,\n"
                                     "      \"z\" : 0\n"
                                     "   },\n"
                                     "   \"origin\" : {\n"
                                     "      \"x\" : 1,\n"
                                     "      \"y\" : 0,\n"
                                     "      \"z\" : 0\n"
                                     "   },\n"
                                     "   \"up\" : {\n"
                                     "      \"x\" : 0,\n"
                                     "      \"y\" : 0,\n"
                                     "      \"z\" : 1\n"
                                     "   }\n"
                                     "}\n";
    const std::string& json = camera.toJSON();
    BOOST_CHECK_EQUAL( json, expectedJSON );

    zerobuf::render::Camera camera2;
    camera2.fromJSON( json );
    BOOST_CHECK( camera == camera2 );
}
