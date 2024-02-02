cmake_minimum_required(VERSION 3.22)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(circle)

file(GLOB_RECURSE CIRCLE_INCLUDE include/dbat/*.h)
file(GLOB_RECURSE CIRCLE_SRC src/*.cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-fpermissive -w")

set(BUILD_TESTING OFF)

# this is the core library we're making.
add_library(circlemud ${CIRCLE_INCLUDE} ${CIRCLE_SRC})
# everything we make will link to it.
target_include_directories(circlemud PUBLIC include)

SET(circle_link ${CMAKE_INSTALL_PREFIX}/bin/)
