cmake_minimum_required(VERSION 3.16)
include(FetchContent)

include_guard()

project(mmtmhasparserlib VERSION 1.0.0)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(mmtmhasparserlib_BUILD_BINARIES OFF CACHE BOOL "Build demo executables")
set(mmtmhasparserlib_BUILD_DOC      OFF CACHE BOOL "Build doxygen doc")

FetchContent_Declare(
  ilo
  GIT_REPOSITORY https://github.com/Fraunhofer-IIS/ilo.git
  GIT_TAG        r1.0.0
)

FetchContent_Declare(
  mmtisobmff
  GIT_REPOSITORY https://github.com/Fraunhofer-IIS/mmtisobmff.git
  GIT_TAG        r1.0.0
)

FetchContent_Declare(
  mmtaudioparser
  GIT_REPOSITORY https://github.com/Fraunhofer-IIS/mmtaudioparser.git
  GIT_TAG        r1.0.0
)

add_subdirectory(src)

if(mmtmhasparserlib_BUILD_BINARIES)
  add_subdirectory(demo)
endif()

if(mmtmhasparserlib_BUILD_DOC)
  add_subdirectory(doc)
endif()
