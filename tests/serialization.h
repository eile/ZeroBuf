
/* Copyright (c) 2015, Human Brain Project
 *                     Daniel Nachbaur <danielnachbaur@epfl.ch>
 */

#include <testSchema.h>
#include <zerobuf/ConstVector.h>
#include <zerobuf/Vector.h>

#define SETVALUES(type, name) \
    const std::vector< type > name##Vector { type(1), type(1), type(2), type(3) }; \
    const type name##value( 42 ); \
    \
    object.set##name##dynamic( name##Vector ); \
    object.set##name##array( name##Vector ); \
    object.set##name##value( name##value );

#define TESTVALUES( type, name )                                        \
    const std::vector< type > expected##name##Vector { type(1),         \
            type(1), type(2), type(3) };                                \
    const type expected##name##value( 42 );                             \
                                                                        \
    const std::vector< type >& name##Dynamic(                           \
        object.get##name##dynamicVector( ));                            \
    const std::vector< type >& name##Array(                             \
        object.get##name##arrayVector( ));                              \
    const type& name##Value( object.get##name##value( ));               \
                                                                        \
    BOOST_CHECK_EQUAL( expected##name##Vector.size(),                   \
                       name##Dynamic.size( ));                          \
    BOOST_CHECK_EQUAL( expected##name##Vector.size(),                   \
                       name##Array.size( ));                            \
    BOOST_CHECK_EQUAL_COLLECTIONS( expected##name##Vector.begin(),      \
                                   expected##name##Vector.end(),        \
                                   name##Dynamic.begin(),               \
                                   name##Dynamic.end( ));               \
    BOOST_CHECK_EQUAL_COLLECTIONS( expected##name##Vector.begin(),      \
                                   expected##name##Vector.end(),        \
                                   name##Array.begin(),                 \
                                   name##Array.end( ));                 \
    BOOST_CHECK_EQUAL( expected##name##value, name##Value );


test::TestSchema getTestObject()
{
    test::TestSchema object;
    SETVALUES(int32_t, Int);
    SETVALUES(uint32_t, Uint);
    SETVALUES(float, Float);
    SETVALUES(double, Double);
    SETVALUES(int8_t, Byte);
    SETVALUES(int16_t, Short);
    SETVALUES(uint8_t, Ubyte);
    SETVALUES(uint16_t, Ushort);
    SETVALUES(uint64_t, Ulong);
    SETVALUES(uint8_t, Uint8_t);
    SETVALUES(uint16_t, Uint16_t);
    SETVALUES(uint32_t, Uint32_t);
    SETVALUES(uint64_t, Uint64_t);
    SETVALUES(::zerobuf::uint128_t, Uint128_t);
    SETVALUES(int8_t, Int8_t);
    SETVALUES(int16_t, Int16_t);
    SETVALUES(int32_t, Int32_t);
    SETVALUES(int64_t, Int64_t);
    object.setBoolvalue( true );
    object.setStringvalue( "testmessage" );

    object.setEnumeration( test::TestEnum_SECOND );
    const std::vector<test::TestEnum> testEnums = { test::TestEnum_FIRST,
                                                    test::TestEnum_SECOND };
    object.setEnumerations( testEnums );

    int32_t intMagic = 42;
    uint32_t uintMagic = 43;

    // Write nested table using a stack object
    test::TestNested nested;
    nested.setIntvalue( intMagic - 1 );
    nested.setUintvalue( uintMagic - 1 );
    BOOST_CHECK_EQUAL( nested.getIntvalue(), intMagic - 1 );
    BOOST_CHECK_EQUAL( nested.getUintvalue(), uintMagic - 1 );

    object.setNested( nested );
    BOOST_CHECK_EQUAL( object.getNested().getIntvalue(), intMagic - 1 );
    BOOST_CHECK_EQUAL( object.getNested().getUintvalue(), uintMagic - 1 );

    const test::TestSchema& constObject = object;
    BOOST_CHECK_EQUAL( constObject.getNested().getIntvalue(), intMagic - 1  );
    BOOST_CHECK_EQUAL( constObject.getNested().getUintvalue(), uintMagic - 1 );

    // Writable copy of the table is acquired from parent schema
    test::TestNested& mutableNested = object.getNested();
    mutableNested.setIntvalue( intMagic );
    mutableNested.setUintvalue( uintMagic );
    BOOST_CHECK_EQUAL( mutableNested.getIntvalue(), intMagic  );
    BOOST_CHECK_EQUAL( mutableNested.getUintvalue(), uintMagic  );
    BOOST_CHECK_EQUAL( object.getNested().getIntvalue(), intMagic  );
    BOOST_CHECK_EQUAL( object.getNested().getUintvalue(), uintMagic  );

    // Non writable copy of the table is acquired from parent schema
    const test::TestNested& constNested = constObject.getNested();
    BOOST_CHECK_EQUAL( constNested.getIntvalue(), intMagic  );
    BOOST_CHECK_EQUAL( constNested.getUintvalue(), uintMagic  );

    // Copy of non writable object should write into it's own memory
    test::TestNested nestedCopy = constObject.getNested();
    BOOST_CHECK_EQUAL( nestedCopy.getIntvalue(), intMagic  );
    BOOST_CHECK_EQUAL( nestedCopy.getUintvalue(), uintMagic  );

    nestedCopy.setIntvalue( 2 * intMagic );
    nestedCopy.setUintvalue( 2 * uintMagic );
    BOOST_CHECK_EQUAL( nestedCopy.getIntvalue(), 2 * intMagic  );
    BOOST_CHECK_EQUAL( nestedCopy.getUintvalue(), 2 * uintMagic  );
    BOOST_CHECK_EQUAL( object.getNested().getIntvalue(), intMagic  );
    BOOST_CHECK_EQUAL( object.getNested().getUintvalue(), uintMagic  );

#if 0 // unsupported in JSON right now
    // Writable copy of the table is acquired from parent schema
    std::array< test::TestNested, 4 >& nestedArray = object.getNestedarray();
    BOOST_CHECK_EQUAL( nestedArray.size(), 4 );

    intMagic = 42;
    uintMagic = 43;
    for( test::TestNested& inner : nestedArray )
    {
        inner.setIntvalue( intMagic++ );
        inner.setUintvalue( uintMagic++ );
        BOOST_CHECK_EQUAL(
            object.getNestedarray()[ intMagic - 43 ].getIntvalue(),
            intMagic - 1 );
    }

    // Non writable copy of the tables are acquired from parent schema
    const std::array< test::TestNested, 4 >& constArray =
                                                   constObject.getNestedarray();
    intMagic = 42;
    uintMagic = 43;
    for( const test::TestNested& inner : constArray )
    {
        BOOST_CHECK_EQUAL( inner.getIntvalue(), intMagic++ );
        BOOST_CHECK_EQUAL( inner.getUintvalue(), uintMagic++  );

        test::TestNested copy( inner );
        copy.setIntvalue( 42 );
        BOOST_CHECK_EQUAL( copy.getIntvalue(), 42 );
        BOOST_CHECK_EQUAL(
            object.getNestedarray()[ intMagic - 43 ].getIntvalue(),
            intMagic - 1 );
    }

    intMagic = 42;
    uintMagic = 43;

    // Writable nested tables
    std::array< test::TestNested, 4 > nesteds;
    for( test::TestNested& inner : nesteds )
    {
        inner.setIntvalue( intMagic++ );
        inner.setUintvalue( uintMagic++ );
    }
    object.setNestedarray( nesteds );

    intMagic = 42;
    uintMagic = 43;

    // Setting dynamic tables
    std::vector< test::TestNested > nestedDyn;
    for( size_t i = 0; i < 2; ++i  )
    {
        test::TestNested inner;
        inner.setIntvalue( intMagic++ );
        inner.setUintvalue( uintMagic++ );
        nestedDyn.push_back( inner );
    }
    object.setNesteddynamic( nestedDyn );
#endif

    return object;
}

void checkTestObject( const test::TestNested& nested )
{
    BOOST_CHECK_EQUAL( nested.getIntvalue(), 42 );
    BOOST_CHECK_EQUAL( nested.getUintvalue(), 43 );
}

void checkTestObject( const test::TestSchema& object )
{
    TESTVALUES(int32_t, Int);
    TESTVALUES(uint32_t, Uint);
    TESTVALUES(float, Float);
    TESTVALUES(double, Double);
    TESTVALUES(int8_t, Byte);
    TESTVALUES(int16_t, Short);
    TESTVALUES(uint8_t, Ubyte);
    TESTVALUES(uint16_t, Ushort);
    TESTVALUES(uint64_t, Ulong);
    TESTVALUES(uint8_t, Uint8_t);
    TESTVALUES(uint16_t, Uint16_t);
    TESTVALUES(uint32_t, Uint32_t);
    TESTVALUES(uint64_t, Uint64_t);
    TESTVALUES(::zerobuf::uint128_t, Uint128_t);
    TESTVALUES(int8_t, Int8_t);
    TESTVALUES(int16_t, Int16_t);
    TESTVALUES(int32_t, Int32_t);
    TESTVALUES(int64_t, Int64_t);
    BOOST_CHECK( object.getBoolvalue( ));
    BOOST_CHECK_EQUAL( object.getStringvalueString(), "testmessage" );

    checkTestObject( object.getNested( ));

#if 0 // unsupported in JSON right now
    // Test retrieved tables
    const auto& tables = object.getNestedarray();
    int32_t intMagic = 42;
    uint32_t uintMagic = 43;
    for( const auto& inner : tables )
    {
        BOOST_CHECK_EQUAL( inner.getIntvalue(), intMagic++ );
        BOOST_CHECK_EQUAL( inner.getUintvalue(), uintMagic++  );
    }

    // Test retrieved dynamic tables
    intMagic = 42;
    uintMagic = 43;
    ::zerobuf::ConstVector< test::TestNested > dynamicTables =
          object.getNesteddynamic();
    for( size_t i = 0; i < dynamicTables.size(); ++i )
    {
        const test::TestNested& inner = dynamicTables[i];
        BOOST_CHECK_EQUAL( inner.getIntvalue(), intMagic++ );
        BOOST_CHECK_EQUAL( inner.getUintvalue(), uintMagic++  );
    }

    BOOST_REQUIRE_THROW( dynamicTables.data(), std::runtime_error );
#endif
}
