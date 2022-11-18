cmake_minimum_required(VERSION 3.22)

set(CPM_DOWNLOAD_VERSION 0.35.5)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

project(circle)

CPMAddPackage(
        NAME fmt
        GITHUB_REPOSITORY fmtlib/fmt
        GIT_TAG 9.1.0
)

find_package(ZLIB REQUIRED)

include_directories(PUBLIC
        include
        ${fmt_SOURCE_DIR}/include
        ${eff_random_SOURCE_DIR}/include
        )

file(GLOB_RECURSE CIRCLE_INCLUDE include/*.h)
file(GLOB_RECURSE CIRCLE_SRC src/*.cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-fpermissive -w")

find_library(LIBBSD NAMES bsd libbsd)

if(${LIBBSD})
    link_libraries(${LIBBSD})
    add_compile_definitions(HAVE_LIBBSD)
endif()

set(BUILD_TESTING OFF)

link_libraries(ZLIB::ZLIB fmt::fmt ${SQLite3_LIBRARIES})

# this is the core library we're making.
add_library(circlemud ${CIRCLE_INCLUDE} ${CIRCLE_SRC})
# everything we make will link to it.
link_libraries(circlemud)

# the various binaries
add_executable(circle apps/circle.cpp)
#add_executable(dbconv apps/dbconv.cpp)

target_compile_definitions(circlemud PUBLIC USING_CMAKE=1 CIRCLE_UNIX=1 POSIX=1)

SET(circle_link ${CMAKE_INSTALL_PREFIX}/bin/)