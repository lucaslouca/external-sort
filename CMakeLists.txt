CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

########################################################################
# Versioning
########################################################################

SET(CCF_MAJOR_VERSION 0)
SET(CCF_MINOR_VERSION 1)
SET(CCF_PATCH_VERSION 0)

SET(CCF_VERSION ${CCF_MAJOR_VERSION}.${CCF_MINOR_VERSION}.${CCF_PATCH_VERSION})

PROJECT(externalsort
    VERSION
        ${CCF_VERSION}
    DESCRIPTION
        "Very nice project"
    LANGUAGES
        CXX
)

MESSAGE(STATUS "CMAKE_ROOT:           ${CMAKE_ROOT}")
MESSAGE(STATUS "CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")

########################################################################
# Macros and additional flags
########################################################################

INCLUDE( CheckCXXCompilerFlag )

FILE( GLOB filenames "cmake/*.cmake" )
FOREACH( filename ${filenames} )
  INCLUDE( ${filename} )
ENDFOREACH()

########################################################################
# APP
########################################################################

ADD_SUBDIRECTORY(src)

########################################################################
# GIT VERSION
########################################################################

# Add a custom command that produces version.cpp, plus
# a dummy output that's not actually produced, in order
# to force version.cmake to always be re-run before the build
# ADD_CUSTOM_COMMAND(
#     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
#            ${CMAKE_CURRENT_BINARY_DIR}/_version.cpp
#     COMMAND ${CMAKE_COMMAND} -P
#             ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version_header.cmake)

########################################################################
# Tests
########################################################################

# Tell CMake to expose a new target called test. Individual tests may be 
# found in the `tests` subdirectory of the repository.
ENABLE_TESTING()

ADD_SUBDIRECTORY(tests)
