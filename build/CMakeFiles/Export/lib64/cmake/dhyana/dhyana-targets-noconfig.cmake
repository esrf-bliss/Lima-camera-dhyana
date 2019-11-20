#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "dhyana" for configuration ""
set_property(TARGET dhyana APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(dhyana PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/liblimadhyana.so.0.0.0"
  IMPORTED_SONAME_NOCONFIG "liblimadhyana.so.0.0"
  )

list(APPEND _IMPORT_CHECK_TARGETS dhyana )
list(APPEND _IMPORT_CHECK_FILES_FOR_dhyana "${_IMPORT_PREFIX}/lib64/liblimadhyana.so.0.0.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
