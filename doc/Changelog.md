# Changelog {#Changelog}

# Release 0.5 (23-05-2017)

* [80](https://github.com/HBPVIS/ZeroBuf/pull/80):
  Fix const ZeroBuf vector iterator
* [79](https://github.com/HBPVIS/ZeroBuf/pull/79):
  Add to zerobuf::Vector: resize(), begin(), end()
* [78](https://github.com/HBPVIS/ZeroBuf/pull/78):
  Pretty-format generated JSON schemas
* [77](https://github.com/HBPVIS/ZeroBuf/pull/77):
  Create CamelCase names for C++ functions from potential snake_case names
* [75](https://github.com/HBPVIS/ZeroBuf/pull/75):
  Handle dynamic arrays of enums, fix JSON schemas for enum arrays

# Release 0.4 (09-12-2016)

* [73](https://github.com/HBPVIS/ZeroBuf/pull/73):
  Support assignment of enum values in FBS
* [72](https://github.com/HBPVIS/ZeroBuf/pull/72):
  Fix const-access for ZeroBuf vector on ConstAllocator parent, needed after #71
* [71](https://github.com/HBPVIS/ZeroBuf/pull/71):
  Fix update of ZeroBuf objects stored in a ZeroBuf vector after toJSON()
* [69](https://github.com/HBPVIS/ZeroBuf/pull/69):
  Allow empty ZeroBufs with empty JSON payload from HTTP server PUT requests
* [68](https://github.com/HBPVIS/ZeroBuf/pull/68):
  Support partial JSON updates
* [67](https://github.com/HBPVIS/ZeroBuf/pull/67):
  * Fix compilation error when having multiple FBS enums in one namespace
  * Maintain order of FBS fields in JSON schema
* [65](https://github.com/HBPVIS/ZeroBuf/pull/65):
  * JSON schema generated from FBS, available in ZeroBuf::getSchema() and
    ZeroBuf::ZEROBUF_SCHEMA
  * enum class for enums instead of 'old' enums; breaks existing enum names
  * fix wrong JSON value for empty arrays (was 'null', is '[]' now)

# Release 0.3 (30-06-2016)

* [59](https://github.com/HBPVIS/ZeroBuf/pull/59):
  Add ConstAllocator and static functions create(), ZEROBUF_TYPE_NAME(),
  ZEROBUF_TYPE_IDENTIFIER()
* [58](https://github.com/HBPVIS/ZeroBuf/pull/58):
  Move vocabulary to new project Lexis
* [54](https://github.com/HBPVIS/ZeroBuf/pull/54):
  Remove more string-related set/get from array/vector types
* [53](https://github.com/HBPVIS/ZeroBuf/pull/53):
  Remove set/getString method for non-string dynamic members
* [52](https://github.com/HBPVIS/ZeroBuf/pull/52):
  Emit Qt signals plus their new values for PODs and strings in _fromBinary()
* [51](https://github.com/HBPVIS/ZeroBuf/pull/51):
  Fix a bug in the Allocator with multiple reallocs
* [49](https://github.com/HBPVIS/ZeroBuf/pull/49):
  Added the exit event
* [48](https://github.com/HBPVIS/ZeroBuf/pull/48):
  Fix notifyUpdated usage. Generate doxygen documentation
* [42](https://github.com/HBPVIS/ZeroBuf/pull/42):
  zerobufCxx.py can generate QObjects
* [40](https://github.com/HBPVIS/ZeroBuf/pull/40):
  Add data.Progress event and logic
* [38](https://github.com/HBPVIS/ZeroBuf/pull/38):
  Add data.Frame event
* [35](https://github.com/HBPVIS/ZeroBuf/pull/35):
  Add LookOut and render.Frame events

# Release 0.2 (08-03-2016)

* [32](https://github.com/HBPVIS/ZeroBuf/pull/32):
  Add JSON conversion to code generator
* [30](https://github.com/HBPVIS/ZeroBuf/pull/30):
  Add ImageJPEG event

# Release 0.1 (28-01-2016)

* [27](https://github.com/HBPVIS/ZeroBuf/pull/27):
  Support default values for sub-structs
* [25](https://github.com/HBPVIS/ZeroBuf/pull/25):
  Use base64 encoding for byte and ubyte arrays (static and dynamic)
* [19](https://github.com/HBPVIS/ZeroBuf/pull/19):
  Allow compaction of existing allocation
* [19](https://github.com/HBPVIS/ZeroBuf/pull/19):
  Implemented support for dynamic sub-objects
* [17](https://github.com/HBPVIS/ZeroBuf/pull/17):
  Implemented support for static sub-objects
* [17](https://github.com/HBPVIS/ZeroBuf/pull/17):
  Implemented support for arrays of static sub-objects
* [17](https://github.com/HBPVIS/ZeroBuf/pull/17):
  Implemented support for enumerations
