CONFIGURE_FILE( Base.hh.in ${CMAKE_SOURCE_DIR}/tests/Base.hh )

IF( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
  # Ignore warnings related to the GNU statement expressions if we
  # compile with clang.
  ENABLE_IF_SUPPORTED( CMAKE_CXX_FLAGS "-Wno-gnu-statement-expression" )
ENDIF()

# Optimizes for a useful debugging experience. While the test cases
# might run slightly faster if "real" optimizations were enabled, I
# like this flag for debugging.
ENABLE_IF_SUPPORTED( CMAKE_CXX_FLAGS "-Og" )
ENABLE_IF_SUPPORTED( CMAKE_CXX_FLAGS "-g" )

#
# The tests should always have these nasty flags enabled. What's the
# use of the test without them?
#
# Coincidentally, isn't it nice how "-Wall" will not enable any of
# the flags described afterwards?
#
ENABLE_IF_SUPPORTED( CMAKE_CXX_FLAGS "-Wall" )
ENABLE_IF_SUPPORTED( CMAKE_CXX_FLAGS "-Wextra" )

# There are some issues with user-installed libraries in Mac OS X that
# cause warnings to appear in code that does *not* belong to Aleph, so
# I rather want this switch to be disabled.
#
# The automated tests under Linux will still have this setting though,
# so I should not miss too many issues.
IF( NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
  ENABLE_IF_SUPPORTED( CMAKE_CXX_FLAGS "-Werror" )
ENDIF()

ENABLE_IF_SUPPORTED( CMAKE_CXX_FLAGS "-Wconversion" )
ENABLE_IF_SUPPORTED( CMAKE_CXX_FLAGS "-Wnon-virtual-dtor" )
ENABLE_IF_SUPPORTED( CMAKE_CXX_FLAGS "-Wold-style-cast" )
ENABLE_IF_SUPPORTED( CMAKE_CXX_FLAGS "-Woverloaded-virtual" )
ENABLE_IF_SUPPORTED( CMAKE_CXX_FLAGS "-Wself-init" )
ENABLE_IF_SUPPORTED( CMAKE_CXX_FLAGS "-Wunsafe-loop-optimization" )
ENABLE_IF_SUPPORTED( CMAKE_CXX_FLAGS "-pedantic" )

file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp) 
file(GLOB INCLUDE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable(test_${PROJECT_NAME} ${SOURCE_FILES} ${INCLUDE_FILES})
ADD_TEST(barycentric_subdivision test_${PROJECT_NAME})
ADD_TEST(barycentric_subdivision_xxx test_${PROJECT_NAME})




