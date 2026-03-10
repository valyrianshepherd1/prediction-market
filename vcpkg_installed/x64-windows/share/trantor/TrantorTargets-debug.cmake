#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Trantor::Trantor" for configuration "Debug"
set_property(TARGET Trantor::Trantor APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(Trantor::Trantor PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/trantor.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_DEBUG "c-ares::cares"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/trantor.dll"
  )

list(APPEND _cmake_import_check_targets Trantor::Trantor )
list(APPEND _cmake_import_check_files_for_Trantor::Trantor "${_IMPORT_PREFIX}/debug/lib/trantor.lib" "${_IMPORT_PREFIX}/debug/bin/trantor.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
