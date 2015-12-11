#!/usr/bin/env python

# TODO:
# * nested dynamic tables
# * endian swap method
# * configurable allocator class name ?

# UUID is MD5 hash of namespace::namespace[<cxxtype>|<cxxtype>*|<cxxtype><size>]
# See @ref Binary for a description of the memory layout

import argparse
import hashlib
import re
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
    # remove default params for implementation definition
    implFunc = re.sub( r"=.*?(\(.*?\))?([,)])", r"\2", function )
    implFunc = re.sub( r" final$", "", implFunc ) # remove ' final' keyword

    if retVal: # '{}'-less body
        header.write( "    {0}{1} {2};\n".
                      format( "static " if static else "", retVal, function ))
        impl.write( "\n" + retVal + " " + emit.table +
                    "::" + implFunc + "\n{\n    " + body +
                    "\n}\n" )
    else:      # ctor '[initializer list]{ body }'
        header.write( "    {0}{1};\n".
                      format( "explicit " if explicit else "", function ))
        impl.write( "\n" + emit.table +
                    "::" + implFunc + "\n    " + body + "\n\n" )

def emitDynamic( spec ):
    if not isDynamic( spec ):
        return;

    cxxName = spec[0][0].upper() + spec[0][1:]
    cxxtype = "char"
    isString = ( spec[1] == "string" )
    if not isString:
        cxxtype = emit.types[ spec[2] ][1]

    emit.md5.update( cxxtype.encode('utf-8') + b"Vector" )

    header.write( "    typedef ::zerobuf::Vector< " + cxxtype + " > " +
                  cxxName + ";\n" )
    header.write( "    typedef ::zerobuf::ConstVector< " + cxxtype + " > Const" + cxxName + ";\n" )
    # non-const, const pointer
    if( cxxtype in emit.tables ):
        emitFunction( "typename " + emit.table + "::" + cxxName,
                      "get" + cxxName + "()",
                      "notifyChanging();\n    " +
                      "return " + cxxName + "( getAllocator(), " +
                      str( emit.currentDyn ) + " );" )
        emitFunction( "typename " + emit.table + "::Const" + cxxName,
                      "get" + cxxName + "() const",
                      "return Const" + cxxName + "( getAllocator(), " +
                      str( emit.currentDyn ) + " );" )
        # vector
        emitFunction( "std::vector< " + cxxtype + " >",
                      "get" + cxxName + "Vector() const",
                      "const Const" + cxxName + "& vec = get" + cxxName
                      + "();\n" +
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
                      "return " + cxxName + "( getAllocator(), " +
                      str( emit.currentDyn ) + " );" )
        emitFunction( "typename " + emit.table + "::Const" + cxxName,
                      "get" + cxxName + "() const",
                      "return Const" + cxxName + "( getAllocator(), " +
                      str( emit.currentDyn ) + " );" )
        emitFunction( "void",
                      "set" + cxxName + "( " + cxxtype +
                      " const * value, size_t size )",
                      "notifyChanging();\n    " +
                      "_copyZerobufArray( value, size * sizeof( " + cxxtype +
                      " ), " + str( emit.currentDyn ) + " );" )
        # vector
        emitFunction( "std::vector< " + cxxtype + " >",
                      "get" + cxxName + "Vector() const",
                      "const Const" + cxxName + "& vec = get" + cxxName +
                      "();\n    return std::vector< " + cxxtype +
                      " >( vec.data(), vec.data() + vec.size( ));" )
        emitFunction( "void",
                      "set" + cxxName + "( const std::vector< " +
                      cxxtype + " >& value )",
                      "notifyChanging();\n    " +
                      "_copyZerobufArray( value.data(), value.size() * sizeof( " +
                      cxxtype + " ), " + str( emit.currentDyn ) + " );" )
        # string
        emitFunction( "std::string",
                      "get" + cxxName + "String() const",
                      "const uint8_t* ptr = getAllocator().template getDynamic< " +
                      "const uint8_t >( " + str( emit.currentDyn ) + " );\n" +
                      "    return std::string( ptr, ptr + " +
                      "getAllocator().template getItem< uint64_t >( " +
                      str( emit.offset + 8 ) + " ));" )
        emitFunction( "void",
                      "set" + cxxName + "( const std::string& value )",
                      "notifyChanging();\n    " +
                      "_copyZerobufArray( value.c_str(), value.length(), " +
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
                        format( spec[0], cxxBaseType, emit.currentDyn,
                                emit.offset + 8, "false", schemaPtr ))

    emit.offset += 16 # 8b offset, 8b size
    emit.currentDyn += 1
    header.write( "\n" )
    impl.write( "\n" )

def emitStaticMember( spec ):
    cxxname = spec[0]
    cxxName = cxxname[0].upper() + cxxname[1:]
    cxxtype = emit.types[ spec[1] ][1]
    elemSize = emit.types[ spec[1] ][0]
    if len(spec) == 3:
        emit.defaultValues += "set{0}({1});\n".format(cxxName,spec[2])

    emit.md5.update( cxxtype.encode('utf-8') )

    if( cxxtype in emit.tables ):
        emitFunction( "const {0}&".format( cxxtype ),
                      "get" + cxxName + "() const",
                      "return _{0};".format( cxxname ))
        emitFunction( "{0}&".format( cxxtype ), "get" + cxxName + "()",
                      "return _{0};".format( cxxname ))
        emitFunction( "void",
                      "set"  + cxxName + "( const " + cxxtype + "& value )",
                      "_{0} = value;".format( cxxname ))
        emit.members.append( "{0} _{1};".format( cxxtype, cxxname ));
        emit.initializers.append(
            "_{0}( ::zerobuf::AllocatorPtr( new ::zerobuf::StaticSubAllocator( getAllocator(), {1}, {2} )))".
            format( cxxname, emit.offset, elemSize ))
    else:
        emitFunction( cxxtype, "get" + cxxName + "() const",
                      "return getAllocator().template getItem< " + cxxtype +
                      " >( " + str( emit.offset ) + " );" )
        emitFunction( "void",
                      "set"  + cxxName + "( " + cxxtype + " value )",
                      "notifyChanging();\n    " +
                      "getAllocator().template getItem< " + cxxtype + " >( " +
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
    cxxname = spec[0]
    cxxName = cxxname[0].upper() + cxxname[1:]
    cxxtype = emit.types[ spec[2] ][1]
    elemSize = int( emit.types[ spec[2] ][0] )
    nElems = int( spec[4] )
    nBytes = elemSize * nElems

    emit.md5.update( (cxxtype + str( nElems )).encode('utf-8') )

    if( cxxtype in emit.tables ):
        header.write( "    typedef std::array< {0}, {1} > {2};\n".
                      format( cxxtype, nElems, cxxName ))
        emitFunction( "const {0}::{1}&".format( emit.table, cxxName ),
                      "get" + cxxName + "() const",
                      "return _{0};".format( cxxname ))
        emitFunction( "{0}::{1}&".format( emit.table, cxxName ),
                      "get" + cxxName + "()",
                      "return _{0};".format( cxxname ))
#        emitFunction( "void",
#                      "set{0}( const {1}& value )".format( cxxName, cxxtype ),
#                      "notifyChanging();\n    " +
#                      "_{0} = value;".format( cxxname ))
        emit.members.append( "{0} _{1};".format( cxxName, cxxname ));
        initializer = "_{0}{1}".format( cxxname, "{{" );
        for i in range( 0, nElems ):
            initializer += "\n        {0}( ::zerobuf::AllocatorPtr( new ::zerobuf::StaticSubAllocator( getAllocator(), {1}, {2} ))){3} ".format( cxxtype, emit.offset + i * elemSize, elemSize, "}}" if i == nElems - 1 else "," )
        emit.initializers.append( initializer );

    else:
        emitFunction( cxxtype + "*", "get" + cxxName + "()",
                      "notifyChanging();\n    " +
                      "return getAllocator().template getItemPtr< " + cxxtype +
                      " >( " + str( emit.offset ) + " );" )
        emitFunction( "const " + cxxtype + "*",
                      "get" + cxxName + "() const",
                      "return getAllocator().template getItemPtr< " + cxxtype +
                      " >( " + str( emit.offset ) + " );" )
        emitFunction( "std::vector< " + cxxtype + " >",
                      "get" + cxxName + "Vector() const",
                      "const " + cxxtype + "* ptr = getAllocator().template " +
                      "getItemPtr< " + cxxtype + " >( " + str( emit.offset ) +
                      " );\n    return std::vector< " + cxxtype +
                      " >( ptr, ptr + " + str( nElems ) + " );" )
        emitFunction( "void",
                      "set"  + cxxName + "( " + cxxtype + " value[ " +
                      spec[4] + " ] )",
                      "notifyChanging();\n    " +
                      "::memcpy( getAllocator().template getItemPtr< " +
                      cxxtype + " >( " + str( emit.offset ) + " ), value, " +
                      spec[4] + " * sizeof( " + cxxtype + " ));" )
        emitFunction( "void",
                      "set" + cxxName + "( const std::vector< " +
                      cxxtype + " >& value )",
                      "notifyChanging();\n    " +
                      "if( " + str( nElems ) + " >= value.size( ))\n" +
                      "        ::memcpy( getAllocator().template getItemPtr<" +
                      cxxtype + ">( " + str( emit.offset ) +
                      " ), value.data(), value.size() * sizeof( " + cxxtype +
                      "));" )
        emitFunction( "void",
                      "set" + cxxName + "( const std::string& value )",
                      "notifyChanging();\n    " +
                      "if( " + str( nBytes ) + " >= value.length( ))\n" +
                      "        ::memcpy( getAllocator().template getItemPtr<" +
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
        emit.members = []
        emit.initializers = []
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
            header.write( "    " + item[1] + "() : ::zerobuf::Zerobuf( ::zerobuf::AllocatorPtr( )) {}\n" )
            header.write( "    " + item[1] + "( const " + item[1] +
                          "& ) : ::zerobuf::Zerobuf( ::zerobuf::AllocatorPtr( )) {}\n" )
            header.write( "    virtual ~" + item[1] + "() {}\n\n" )
            header.write( "    " + item[1] + "& operator = ( const " +
                          item[1] + "& ) { return *this; }\n\n" )
        else:
            initializers = ""
            if emit.initializers:
                initializers = "    , {0}\n".format( '\n    , '.join( emit.initializers ))

            emitFunction( None,
                          "{0}()".format( item[1] ),
                          ": {0}( ::zerobuf::AllocatorPtr( new ::zerobuf::NonMovingAllocator( {1}, {2} )))\n"
                          "{{\n{3}}}\n".format( item[1], emit.offset, emit.numDynamic,emit.defaultValues ))
            emitFunction( None,
                          item[1] + "( const " + item[1] +"& from )",
                          ": {0}( ::zerobuf::AllocatorPtr( new ::zerobuf::NonMovingAllocator( {1}, {2} )))\n".format( item[1], emit.offset, emit.numDynamic ) +
                          "{\n" +
                          "    *this = from;\n" +
                          "}\n",
                          explicit = False )
            emitFunction( None,
                          item[1] + "( const ::zerobuf::Zerobuf& from )",
                          ": {0}( ::zerobuf::AllocatorPtr( new ::zerobuf::NonMovingAllocator( {1}, {2} )))\n".format( item[1], emit.offset, emit.numDynamic ) +
                          "{\n" +
                          "    *this = from;\n" +
                          "}\n",
                          explicit = False )

            # Zerobuf object owns allocator!
            emitFunction( None, "{0}( ::zerobuf::AllocatorPtr allocator )".format( item[1] ),
                          ": ::zerobuf::Zerobuf( std::move( allocator ))\n{0}    {{}}\n".format( initializers ))

            header.write( "    virtual ~" + item[1] + "() {}\n\n" )
            header.write( "    " + item[1] + "& operator = ( const " + item[1] + "& rhs )\n"+
                          "        { ::zerobuf::Zerobuf::operator = ( rhs ); return *this; }\n\n" )

        # introspection
        header.write( "    size_t getZerobufStaticSize() const final\n"+
                      "        {{ return {0}; }}\n".format( emit.offset ))
        header.write( "    size_t getZerobufNumDynamics() const final\n"+
                      "        {{ return {0}; }}\n".format( emit.numDynamic ))

        # zerobufType
        digest = emit.md5.hexdigest()
        high = digest[ 0 : len( digest ) - 16 ]
        low  = digest[ len( digest ) - 16: ]
        zerobufType = "servus::uint128_t( 0x{0}ull, 0x{1}ull )".format( high,
                                                                        low )
        header.write( "    servus::uint128_t getZerobufType() const final\n"+
                      "        { return " + zerobufType + "; }\n" )
        header.write( "\n" )

        # schema
        schema = "{{ {0}, {1},\n        {2},\n        {{\n         {3}\n         }} }}".format( emit.offset,
                emit.currentDyn, zerobufType, ',\n         '.join( emit.schema ))
        emitFunction( "::zerobuf::Schema", "schema()",
                      "return " + schema + ";", True )
        emitFunction( "::zerobuf::Schema", "getSchema() const final",
                      "{ return schema(); }\n" );
        header.write( "\n" )

        # closing statements
        header.write( "private:\n    {0}\n".
                      format( '\n    '.join( emit.members )))
        header.write( "};\n\n" );
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
    header.write( "#include <zerobuf/Zerobuf.h> // base class\n" )
    header.write(
        "#include <zerobuf/NonMovingAllocator.h> // inline default param\n" )
    header.write( "#include <array>\n" )
    header.write( "\n" )
    impl.write( "// Generated by zerobufCxx.py\n\n" )
    impl.write( "#include \"" + headerbase + ".h\"\n\n" )
    impl.write( "#include <zerobuf/ConstVector.h>\n" )
    impl.write( "#include <zerobuf/NonMovingSubAllocator.h>\n" )
    impl.write( "#include <zerobuf/Schema.h>\n" )
    impl.write( "#include <zerobuf/StaticSubAllocator.h>\n" )
    impl.write( "#include <zerobuf/Vector.h>\n" )

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
