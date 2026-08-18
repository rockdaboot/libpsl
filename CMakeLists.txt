# Note: this isn't a full CMakeLists.txt for this project.
# This is just building a static library.

cmake_minimum_required(VERSION 3.10)

set(VERSION 0.21.5) # must match version.txt
set(VERSION_NUMBER 0x001505) # must match version.txt converted to hexadecimal

project(psl
  VERSION ${VERSION}
  LANGUAGES C
  DESCRIPTION "C library to handle the Public Suffix List"
)
set(LIBPSL_VERSION_NUMBER ${VERSION_NUMBER})
set(LIBPSL_VERSION_MAJOR ${psl_VERSION_MAJOR})
set(LIBPSL_VERSION_MINOR ${psl_VERSION_MINOR})
set(LIBPSL_VERSION_PATCH ${psl_VERSION_PATCH})
set(LIBPSL_VERSION "${psl_VERSION_MAJOR}.${psl_VERSION_MINOR}.${psl_VERSION_PATCH}")

set(LIBPSL_STANDALONE_BUILD OFF)
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(LIBPSL_STANDALONE_BUILD ON)
endif()
option(LIBPSL_ENABLE_INSTALL "${PROJECT_NAME}: Enable install" ${LIBPSL_STANDALONE_BUILD})

option(ENABLE_BUILTIN "Generate built-in PSL data" ON)
set(ENABLE_RUNTIME "" CACHE STRING "Enable runtime PSL data (libicu, libidn, libidn2)")

add_definitions(
  -DPACKAGE_VERSION=\"${LIBPSL_VERSION}\"
)

if(ENABLE_BUILTIN)
  add_definitions(-DENABLE_BUILTIN=1)
endif()
if(ENABLE_RUNTIME STREQUAL "libicu")
  add_definitions(-DWITH_LIBICU=1)
endif()
if(ENABLE_RUNTIME STREQUAL "libidn")
  add_definitions(-DWITH_LIBIDN=1)
endif()
if(ENABLE_RUNTIME STREQUAL "libidn2")
  add_definitions(-DWITH_LIBIDN2=1)
endif()

set(PSL_FILE "${CMAKE_SOURCE_DIR}/list/public_suffix_list.dat")

find_program(
  PYTHON python
  NAMES python2 python3 python3.7
  HINTS /usr/pkg/bin
  REQUIRED
)
add_custom_target(
  "suffixes_dafsa.h"
  ALL
  "${PYTHON}"
  "${CMAKE_SOURCE_DIR}/src/psl-make-dafsa"
  --output-format=cxx+
  "${PSL_FILE}"
  "${CMAKE_BINARY_DIR}/suffixes_dafsa.h"
)

configure_file(
  "${CMAKE_SOURCE_DIR}/include/libpsl.h.in"
  "${CMAKE_BINARY_DIR}/libpsl.h"
)

add_library(${PROJECT_NAME} STATIC
  src/lookup_string_in_fixed_set.c
  src/psl.c
)

include_directories(
  "${CMAKE_BINARY_DIR}"
)

add_dependencies(
  "${PROJECT_NAME}"
  "suffixes_dafsa.h"
)

if(LIBPSL_ENABLE_INSTALL)
  install(TARGETS ${PROJECT_NAME} DESTINATION lib)
  install(FILES "${CMAKE_BINARY_DIR}/libpsl.h" DESTINATION include)
endif()

