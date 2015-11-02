#!/usr/bin/env python

# TODO:
# * configurable allocator class name
# WIP:
# * nested static/dynamic tables
# * get/setJSON()

# UUID is MD5 hash of namespace::namespace[<cxxtype>|<cxxtype>*|<cxxtype><size>]
#
# Zerobuf binary format is:
#   [dynamic storage headers][static storage][dynamic storage]
#     dynamic storage headers: 8b offset, 8b size
#       for all dynamic arrays in order of spec
#       later dynamic nested classes
#     static storage: 1,2,4,8,16b (* static array size)
#       for all static arrays and variables in order of spec
#       see emit.types for size
#       later also for static nested classes
#     dynamic storage layout is an implementation detail of the Allocator
#
# String arrays '[string]' are stored as an array of 'offset, size' tuples.

import argparse
import hashlib
from pyparsing import *
from sys import stdout, stderr, argv, exit
from os import path

fbsBaseType = oneOf( "int uint float double byte short ubyte ushort ulong uint8_t uint16_t " +
                     "uint32_t uint64_t uint128_t int8_t int16_t int32_t int64_t bool string" )

# namespace foo.bar
fbsNamespaceName = Group( ZeroOrMore( Word( alphanums ) + Suppress( '.' )) +
                          Word( alphanums ))
fbsNamespace = Group( Keyword( "namespace" ) + fbsNamespaceName +
                      Suppress( ';' ))

# enum EventDirection : ubyte { Subscriber, Publisher, Both }
fbsEnumValue = ( Word( alphanums ) + Suppress( Optional( ',' )))
fbsEnum = Group( Keyword( "enum" ) + Word( alphanums ) + Suppress( ':' ) +
                 fbsBaseType + Suppress( '{' ) + OneOrMore( fbsEnumValue ) +
                 Suppress( '}' ))

# value:[type] = defaultValue; entries in table
# TODO: support more default values other than numbers and booleans
fbsType = ( fbsBaseType ^ Word( alphanums ))
fbsTableArray = ( ( Literal( '[' ) + fbsType + Literal( ']' )) ^
                  ( Literal( '[' ) + fbsType + Literal( ':' ) + Word( nums ) +
                    Literal( ']' )) )
fbsTableValue = ((fbsType ^ fbsTableArray) + ZeroOrMore(Suppress('=') +
                Or([Word("true"), Word("false"), Word(nums+".")])))
fbsTableEntry = Group( Word( alphanums+"_" ) + Suppress( ':' ) + fbsTableValue +
                       Suppress( ';' ))
fbsTableSpec = ZeroOrMore( fbsTableEntry )

# table Foo { entries }
fbsTable = Group( Keyword( "table" ) + Word( alphas, alphanums ) +
                  Suppress( '{' ) + fbsTableSpec + Suppress( '}' ))

# root_type foo;
fbsRootType = Group( Keyword( "root_type" ) + Word( alphanums ) +
                     Suppress( ";" ))

# namespace, table(s), root_type
fbsObject = ( Optional( fbsNamespace ) + ZeroOrMore( fbsEnum ) +
              OneOrMore( fbsTable ) + Optional( fbsRootType ))

fbsComment = cppStyleComment
fbsObject.ignore( fbsComment )

#fbsTableArray.setDebug()
#fbsTableValue.setDebug()
#fbsTableEntry.setDebug()

def isDynamic( spec ):
    isString = ( spec[1] == "string" )
    if (len( spec ) == 2 or len( spec ) == 3) and not isString:
        return False
    if len( spec ) == 6: # static array
        return False
    return True

def countDynamic( specs ):
    numDynamic = 0
    for spec in specs:
        if isDynamic( spec ):
            numDynamic += 1
    return numDynamic

def emitFunction( retVal, function, body, static=False, explicit=False ):
    if retVal: # '{}'-less body
        header.write( "    {0}{1} {2};\n".
                      format( "static " if static else "", retVal, function ))
        impl.write( "\n" + retVal + " " + emit.table +
                    "::" + function + "\n{\n    " + body +
                    "\n}\n\n" )
    else:      # ctor '[initializer list]{ body }'
        header.write( "    {0}{1};\n".
                      format( "explicit " if explicit else "", function ))
        impl.write( "\n" + emit.table +
                    "::" + function + "\n    " + body + "\n\n" )

def emitDynamic( spec ):
    if not isDynamic( spec ):
        return;

    cxxName = spec[0][0].upper() + spec[0][1:]
    cxxtype = "char"
    isString = ( spec[1] == "string" )
    if not isString:
        cxxtype = emit.types[ spec[2] ][1]

    emit.md5.update( cxxtype.encode('utf-8') + b"Vector" )

    header.write( "    typedef ::zerobuf::Vector< " + cxxtype + " > " + cxxName + ";\n" )
    header.write( "    typedef ::zerobuf::ConstVector< " + cxxtype + " > Const" + cxxName + ";\n" )
    # non-const, const pointer
    if( cxxtype in emit.tables ):
        emitFunction( "typename " + emit.table + "::" + cxxName,
                      "get" + cxxName + "()",
                      "notifyChanging();\n    " +
                      "return " + cxxName + "( getAllocator(), " + str( emit.currentDyn ) + " );" )
        emitFunction( "typename " + emit.table + "::Const" + cxxName,
                      "get" + cxxName + "() const",
                      "return Const" + cxxName + "( getAllocator(), " +
                      str( emit.currentDyn ) + ", " + cxxtype + "::schema().staticSize );" )
        # vector
        emitFunction( "std::vector< " + cxxtype + " >",
                      "get" + cxxName + "Vector() const",
                      "const Const" + cxxName + "& vec = get" + cxxName + "();\n" +
                      "    std::vector< " + cxxtype + " > ret;\n" +
                      "    ret.reserve( vec.size( ));\n" +
                      "    for( size_t i = 0; i < vec.size(); ++i )\n" +
                      "        ret.push_back( std::move( vec[i] ));\n" +
                      "    return ret;\n" )
        emitFunction( "void",
                      "set" + cxxName + "( const std::vector< " +
                      cxxtype + " >& values )",
                      "notifyChanging();\n    " +
                      "::zerobuf::Vector< ::zerobuf::Zerobuf > dynamic( getAllocator(), " +
                      str(emit.currentDyn) + ");\n" +
                      "    for( const " + cxxtype + "& data : values )\n" +
                      "        dynamic.push_back(data);\n" )
    else:
        emitFunction( "typename " + emit.table + "::" + cxxName,
                      "get" + cxxName + "()",
                      "return " + cxxName + "( getAllocator(), " + str( emit.currentDyn ) + " );" )
        emitFunction( "typename " + emit.table + "::Const" + cxxName,
                      "get" + cxxName + "() const",
                      "return Const" + cxxName + "( getAllocator(), " + str( emit.currentDyn ) + " );" )
        emitFunction( "void",
                      "set" + cxxName + "( " + cxxtype +
                      " const * value, size_t size )",
                      "notifyChanging();\n    " +
                      "_setZerobufArray( value, size * sizeof( " + cxxtype +
                      " ), " + str( emit.currentDyn ) + " );" )
        # vector
        emitFunction( "std::vector< " + cxxtype + " >",
                      "get" + cxxName + "Vector() const",
                      "const Const" + cxxName + "& vec = get" + cxxName + "();\n    return std::vector< " + cxxtype +
                      " >( vec.data(), vec.data() + vec.size( ));" )
        emitFunction( "void",
                      "set" + cxxName + "( const std::vector< " +
                      cxxtype + " >& value )",
                      "notifyChanging();\n    " +
                      "_setZerobufArray( value.data(), value.size() * sizeof( " + cxxtype +
                      " ), " + str( emit.currentDyn ) + " );" )
        # string
        emitFunction( "std::string",
                      "get" + cxxName + "String() const",
                      "const uint8_t* ptr = getAllocator()->template getDynamic< " +
                      "const uint8_t >( " + str( emit.currentDyn ) + " );\n" +
                      "    return std::string( ptr, ptr + " +
                      "getAllocator()->template getItem< uint64_t >( " +
                      str( emit.offset + 8 ) + " ));" )
        emitFunction( "void",
                      "set" + cxxName + "( const std::string& value )",
                      "notifyChanging();\n    " +
                      "_setZerobufArray( value.c_str(), value.length(), " +
                      str( emit.currentDyn ) + " );" )
    # schema entry
    cxxBaseType = cxxtype
    schemaPtr = "::zerobuf::Schema::SchemaFunction( )"
    if( cxxtype in emit.tables ):
        cxxBaseType = "Zerobuf"
        schemaPtr = "::zerobuf::Schema::SchemaFunction( &" + cxxtype + "::schema )"
    if( cxxtype in emit.enums ):
        cxxBaseType = "uint32_t"
    emit.schema.append( "std::make_tuple( \"{0}\", \"{1}\", {2}, {3}, {4}, {5})".
                        format( spec[0], cxxBaseType, emit.currentDyn, emit.offset + 8,
                                "false", schemaPtr ))

    emit.offset += 16 # 8b offset, 8b size
    emit.currentDyn += 1
    header.write( "\n" )
    impl.write( "\n" )

def emitStaticMember( spec ):
    cxxName = spec[0][0].upper() + spec[0][1:]
    cxxtype = emit.types[ spec[1] ][1]
    elemSize = emit.types[ spec[1] ][0]
    if len(spec) == 3:
        emit.defaultValues += "set{0}({1});\n".format(cxxName,spec[2])

    emit.md5.update( cxxtype.encode('utf-8') )

    if( cxxtype in emit.tables ):
        emitFunction( cxxtype, "get" + cxxName + "() const",
                      "const uint8_t* base = getAllocator()->getData();\n" +
                      "    ::zerobuf::NonMovingAllocator* alloc = new ::zerobuf::NonMovingAllocator( " + str( elemSize ) + ", 0 );\n" +
                      "    alloc->copyBuffer( base + " + str( emit.offset ) + ", " + str( elemSize ) + " );\n" +
                      "    return " + cxxtype + "( alloc );" )
        emitFunction( cxxtype, "get" + cxxName + "() ",
                      "return " + cxxtype + "( new ::zerobuf::StaticSubAllocator( getAllocator(), " + str( emit.offset ) + ", " + str( elemSize ) + " ));" )
        emitFunction( "void",
                      "set"  + cxxName + "( const " + cxxtype + "& value )",
                      "notifyChanging();\n    " +
                      "void* data = getAllocator()->getData() + " + str( emit.offset ) + ";\n" +
                      "    ::memcpy( data, value.getZerobufData(), " + str( elemSize ) + ");" )

    else:
        emitFunction( cxxtype, "get" + cxxName + "() const",
                      "return getAllocator()->template getItem< " + cxxtype +
                      " >( " + str( emit.offset ) + " );" )
        emitFunction( "void",
                      "set"  + cxxName + "( " + cxxtype + " value )",
                      "notifyChanging();\n    " +
                      "getAllocator()->template getItem< " + cxxtype + " >( " +
                      str( emit.offset ) + " ) = value;" )

    # schema entry
    cxxBaseType = cxxtype
    schemaPtr = "::zerobuf::Schema::SchemaFunction( )"
    if( cxxtype in emit.tables ):
        cxxBaseType = "Zerobuf"
        schemaPtr = "::zerobuf::Schema::SchemaFunction( &" + cxxtype + "::schema )"
    if( cxxtype in emit.enums ):
        cxxBaseType = "uint32_t"
    emit.schema.append("std::make_tuple( \"{0}\", \"{1}\", {2}, {3}, {4}, {5})".
                       format( spec[0], cxxBaseType, emit.offset, 0, "true",
                               schemaPtr ))
    emit.offset += elemSize

def emitStaticArray( spec ):
    cxxName = spec[0][0].upper() + spec[0][1:]
    cxxtype = emit.types[ spec[2] ][1]
    elemSize = emit.types[ spec[2] ][0]
    nElems = spec[4]
    nBytes = int(emit.types[ spec[2] ][0]) * int(nElems)

    emit.md5.update( (cxxtype + nElems).encode('utf-8') )

    if( cxxtype in emit.tables ):
        emitFunction( "std::vector< " + cxxtype + " >",
                      "get" + cxxName + "Vector() const",
                      "std::vector< " + cxxtype + " > ret;\n" +
                      "    const uint8_t* base = getAllocator()->getData() + " +
                      str( emit.offset ) + ";\n" +
                      "    for( size_t i = 0; i < " + str( nElems ) + "; ++i" +
                      " )\n" +
                      "    {\n" +
                      "        ::zerobuf::NonMovingAllocator* alloc = " +
                      "new ::zerobuf::NonMovingAllocator( " + str( elemSize ) +
                      ", 0 );\n" +
                      "        alloc->copyBuffer( base + i * " +
                      str( elemSize ) + ", " + str( elemSize ) + " );\n" +
                      "        ret.push_back( std::move( " +  cxxtype +
                      "( alloc )));\n" +
                      "    }\n" +
                      "    return ret;" )
        emitFunction( "std::vector< " + cxxtype + " >",
                      "get" + cxxName + "Vector()",
                      "std::vector< " + cxxtype + " > ret;\n" +
                      "    for( size_t i = 0; i < " + str(nElems) + "; ++i" +
                      " )\n" +
                      "        ret.push_back( std::move( " +  cxxtype +
                      "( new ::zerobuf::StaticSubAllocator( getAllocator(), " +
                      str( emit.offset ) + " + i * " + str( elemSize ) + ", " +
                      str( elemSize ) + " ))));\n" +
                      "    return ret;" )
        emitFunction( "void",
                      "set" + cxxName + "( const std::vector< " + cxxtype +
                      " >& value )",
                      "notifyChanging();\n    " +
                      "uint8_t* data = getAllocator()->getData() + " +
                      str( emit.offset ) + ";\n" +
                      "     for( size_t i = 0; i < " + str( nElems ) + "; ++i" +
                      " )\n" +
                      "        ::memcpy( data  + i * " + str( elemSize ) +
                      ", value[i].getZerobufData(), " + str( elemSize ) +
                      " );" )
    else:
        emitFunction( cxxtype + "*", "get" + cxxName + "()",
                      "notifyChanging();\n    " +
                      "return getAllocator()->template getItemPtr< " + cxxtype +
                      " >( " + str( emit.offset ) + " );" )
        emitFunction( "const " + cxxtype + "*",
                      "get" + cxxName + "() const",
                      "return getAllocator()->template getItemPtr< " + cxxtype +
                      " >( " + str( emit.offset ) + " );" )
        emitFunction( "std::vector< " + cxxtype + " >",
                      "get" + cxxName + "Vector() const",
                      "const " + cxxtype + "* ptr = getAllocator()->template " +
                      "getItemPtr< " + cxxtype + " >( " + str( emit.offset ) +
                      " );\n    return std::vector< " + cxxtype +
                      " >( ptr, ptr + " + nElems + " );" )
        emitFunction( "void",
                      "set"  + cxxName + "( " + cxxtype + " value[ " +
                      spec[4] + " ] )",
                      "notifyChanging();\n    " +
                      "::memcpy( getAllocator()->template getItemPtr< " +
                      cxxtype + " >( " + str( emit.offset ) + " ), value, " +
                      spec[4] + " * sizeof( " + cxxtype + " ));" )
        emitFunction( "void",
                      "set" + cxxName + "( const std::vector< " +
                      cxxtype + " >& value )",
                      "notifyChanging();\n    " +
                      "if( " + str( nElems ) + " >= value.size( ))\n" +
                      "        ::memcpy( getAllocator()->template getItemPtr<" +
                      cxxtype + ">( " + str( emit.offset ) +
                      " ), value.data(), value.size() * sizeof( " + cxxtype +
                      "));" )
        emitFunction( "void",
                      "set" + cxxName + "( const std::string& value )",
                      "notifyChanging();\n    " +
                      "if( " + str( nBytes ) + " >= value.length( ))\n" +
                      "        ::memcpy( getAllocator()->template getItemPtr<" +
                      cxxtype + ">( " + str( emit.offset ) +
                      " ), value.data(), value.length( ));" )
    # schema entry
    cxxBaseType = cxxtype
    schemaPtr = "::zerobuf::Schema::SchemaFunction( )"
    if( cxxtype in emit.tables ):
        cxxBaseType = "Zerobuf"
        schemaPtr = "::zerobuf::Schema::SchemaFunction( &" + cxxtype + "::schema )"
    if( cxxtype in emit.enums ):
        cxxBaseType = "uint32_t"
    emit.schema.append( "std::make_tuple( \"{0}\", \"{1}\", {2}, {3}, {4}, {5})".
                        format( spec[0], cxxBaseType, emit.offset, nElems,
                                "true", schemaPtr ))
    emit.offset += nBytes

def emitStatic( spec ):
    if isDynamic( spec ):
        return;

    if len( spec ) == 2 or len( spec ) == 3:
        emitStaticMember( spec )
    else:
        emitStaticArray( spec )
    header.write( "\n" )
    impl.write( "\n" )

def emit():
    emit.namespace = ""
    emit.offset = 0
    emit.numDynamic = 0;
    emit.currentDyn = 0;
    emit.enums = set()
    emit.tables = set()
    emit.schema = []
    emit.types = { "int" : ( 4, "int32_t" ),
                   "uint" : ( 4, "uint32_t" ),
                   "float" : ( 4, "float" ),
                   "double" : ( 8, "double" ),
                   "byte" : ( 1, "int8_t" ),
                   "short" : ( 2, "int16_t" ),
                   "ubyte" : ( 1, "uint8_t" ),
                   "ushort" : ( 2, "uint16_t" ),
                   "ulong" : ( 8, "uint64_t" ),
                   "uint8_t" : ( 1, "uint8_t" ),
                   "uint16_t" : ( 2, "uint16_t" ),
                   "uint32_t" : ( 4, "uint32_t" ),
                   "uint64_t" : ( 8, "uint64_t" ),
                   "uint128_t" : ( 16, "servus::uint128_t" ),
                   "int8_t" : ( 1, "int8_t" ),
                   "int16_t" : ( 2, "int16_t" ),
                   "int32_t" : ( 4, "int32_t" ),
                   "int64_t" : ( 8, "int64_t" ),
                   "bool" : ( 1, "bool" ),
                   "string" : ( 1, "char*" )
    }

    def namespace():
        for namespace in emit.namespace:
            header.write( "}\n" )
            impl.write( "}\n" )
        emit.namespace = item[1]
        for namespace in emit.namespace:
            header.write( "namespace " + namespace + "\n{\n" )
            impl.write( "namespace " + namespace + "\n{\n" )

    def enum():
        emit.types[ item[1] ] = ( 4, item[1] )
        header.write( "enum " + item[1] + "\n{\n" )
        for enumValue in item[3:]:
            header.write( "    " + item[1] + "_" + enumValue + ",\n" )
        header.write( "};\n\n" )
        emit.enums.add( item[1] )

    def table():
        emit.offset = 4 # 4b version header in host endianness
        emit.numDynamic = countDynamic( item[2:] )
        emit.currentDyn = 0
        emit.defaultValues = ''
        emit.schema = []
        emit.table = item[1]
        emit.md5 = hashlib.md5()
        for namespace in emit.namespace:
            emit.md5.update( namespace.encode('utf-8') + b"::" )
        emit.md5.update( item[1].encode('utf-8') )

        # class header
        header.write( "class " + item[1] + " : public zerobuf::Zerobuf\n" +
                      "{\npublic:\n" )

        # member access
        for member in item[2:]:
            emitDynamic( member )
        for member in item[2:]:
            emitStatic( member )

        # ctors, dtor and assignment operator
        if emit.offset == 4: # OPT: table has no data
            emit.offset = 0
            header.write( "    " + item[1] + "() : Zerobuf() {}\n" )
            header.write( "    " + item[1] + "( const " + item[1] +
                          "& ) : Zerobuf() {}\n" )
            header.write( "    virtual ~" + item[1] + "() {}\n\n" )
            header.write( "    " + item[1] + "& operator = ( const " +
                          item[1] + "& ) { return *this; }\n\n" )
        else:
            emitFunction( None,
                          "{0}( ::zerobuf::Allocator *allocator = "
                          "new ::zerobuf::NonMovingAllocator( {0}, {1} ))"
                          .format(item[1], emit.offset, emit.currentDyn),
                          ": ::zerobuf::Zerobuf( allocator )\n"
                          "{{\n{0}}}\n".format(emit.defaultValues))
            emitFunction( None,
                          item[1] + "( const " + item[1] +"& from )",
                          ": zerobuf::Zerobuf( new ::zerobuf::NonMovingAllocator( " + str(emit.offset) + ", " + str(emit.numDynamic) + " ))\n" +
                          "{\n" +
                          "    *this = from;\n" +
                          "}\n",
                          explicit = False )
            emitFunction( None,
                          item[1] + "( const ::zerobuf::Zerobuf& from )",
                          ": ::zerobuf::Zerobuf( new ::zerobuf::NonMovingAllocator( " + str(emit.offset) + ", " + str(emit.numDynamic) + " ))\n" +
                          "{\n" +
                          "    *this = from;\n" +
                          "}\n",
                          explicit = False )
            header.write( "    virtual ~" + item[1] + "() {}\n\n" )
            header.write( "    " + item[1] + "& operator = ( const " + item[1] + "& rhs )\n"+
                          "        { ::zerobuf::Zerobuf::operator = ( rhs ); return *this; }\n\n" )

        # introspection
        header.write( "    static bool isEmptyZerobuf() { return " +
                   str( emit.offset == 0 ).lower() + "; }\n" )
        header.write( "    static bool isStaticZerobuf() { return " +
                   str( emit.numDynamic == 0 ).lower() + "; }\n" )

        # zerobufType
        digest = emit.md5.hexdigest()
        high = digest[ 0 : len( digest ) - 16 ]
        low  = digest[ len( digest ) - 16: ]
        zerobufType = "servus::uint128_t( 0x{0}ull, 0x{1}ull )".format(high, low)
        header.write( "    servus::uint128_t getZerobufType() const override\n" +
                      "        { return " + zerobufType + "; }\n" )
        header.write( "\n" )

        # schema
        schema = "{{ {0}, {1},\n        {2},\n        {{\n         {3}\n         }} }}".format( emit.offset,
                 emit.currentDyn, zerobufType, ',\n         '.join( emit.schema ))
        emitFunction( "::zerobuf::Schema", "schema()",
                      "return " + schema + ";", True )
        header.write( "    ::zerobuf::Schema getSchema() const override { return schema(); }\n" )
        header.write( "\n" )

        # closing statements
        header.write( "};\n\n" )
        emit.types[ item[1] ] = ( emit.offset, item[1] )
        emit.tables.add( item[1] )

    def root_type():
        header.write( "" )

    rootOptions = { "namespace" : namespace,
                    "enum" : enum,
                    "table" : table,
                    "root_type" : root_type,
                    }

    header.write( "// Generated by zerobufCxx.py\n\n" )
    header.write( "#pragma once\n" )
    header.write( "#include <zerobuf/ConstVector.h>\n" )
    header.write( "#include <zerobuf/NonMovingAllocator.h>\n" )
    header.write( "#include <zerobuf/NonMovingSubAllocator.h>\n" )
    header.write( "#include <zerobuf/Schema.h>\n" )
    header.write( "#include <zerobuf/StaticSubAllocator.h>\n" )
    header.write( "#include <zerobuf/Vector.h>\n" )
    header.write( "#include <zerobuf/Zerobuf.h>\n" )
    header.write( "\n" )
    impl.write( "// Generated by zerobufCxx.py\n\n" )
    impl.write( "#include \"" + headerbase + ".h\"\n\n" )

    for item in schema:
        rootOptions[ item[0] ]()

    for namespace in emit.namespace:
        header.write( "}\n" )
        impl.write( "}\n" )


if __name__ == "__main__":
    if len(argv) < 2 :
        stderr.write("ERROR - " + argv[0] + " - too few input arguments!")
        exit(-1)

    parser = argparse.ArgumentParser( description =
                                      "zerobufCxx.py: A zerobuf C++ code generator for extended flatbuffers schemas" )
    parser.add_argument( "files", nargs = "*" )
    parser.add_argument( '-o', '--outputdir', action='store', default = "",
                         help = "Prefix directory for all generated files.")

    # Parse, interpret and validate arguments
    args = parser.parse_args()
    if len(args.files) == 0 :
        stderr.write("ERROR - " + argv[0] + " - no input .fbs files given!")
        exit(-1)

    for file in args.files:
        basename = path.splitext( file )[0]
        headerbase = path.basename( basename )
        if args.outputdir:
            if args.outputdir == '-':
                header = stdout
                impl = stdout
            else:
                header = open( args.outputdir + "/" + headerbase + ".h", 'w' )
                impl = open( args.outputdir + "/" + headerbase + ".cpp", 'w' )
        else:
            header = open( basename + ".h" , 'w' )
            impl = open( basename + ".cpp" , 'w' )

        schema = fbsObject.parseFile( file )
        # import pprint
        # pprint.pprint( schema.asList( ))
        emit()
