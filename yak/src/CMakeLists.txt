# ENABLE_IF_SUPPORTED(CMAKE_CXX_FLAGS "-Wall")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE debug)
ENABLE_IF_SUPPORTED(CMAKE_CXX_FLAGS "-std=c++20")
ENABLE_IF_SUPPORTED(CMAKE_CXX_FLAGS "-pthread")

# Find the packages we need.
find_package(benchmark REQUIRED)
find_package(spdlog REQUIRED)

# Add the list of include paths to be used to search for include files
#include_directories(/usr/local/include)
#include_directories(/opt/homebrew/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(/usr/local/include)

# Search all the .cpp files in the directory where CMakeLists lies and set them to ${SOURCE_FILES}
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp) 

# Search all the .h files in the directory where CMakeLists lies and set them to ${INCLUDE_FILES}
file(GLOB_RECURSE INCLUDE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# Add the executable Example to be built from the source files
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${INCLUDE_FILES})

# Add extra lib directories from the library folder
#link_directories(/usr/local/lib)
#link_directories(/opt/homebrew/lib)

# Link the executable to the libraries.
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC benchmark::benchmark)
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC benchmark::benchmark_main)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} LINK_PUBLIC spdlog::spdlog) 
endif()

if(UNIX AND NOT APPLE)
    find_library(SPDLOG_LIB NAMES libspdlog.a PATHS /usr/local/lib/)
    target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${SPDLOG_LIB})
endif()