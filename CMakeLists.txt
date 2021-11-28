cmake_minimum_required(VERSION 3.17)

set(CPM_DOWNLOAD_VERSION 0.34.0)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

project(circle)

CPMAddPackage(
        NAME json
        GITHUB_REPOSITORY nlohmann/json
        VERSION 3.10.4
)

CPMAddPackage(
        NAME eff_random
        GITHUB_REPOSITORY effolkronium/random
        VERSION 1.4.0
)

CPMAddPackage(
        NAME fmt
        GITHUB_REPOSITORY fmtlib/fmt
        GIT_TAG 8.0.1
)

CPMAddPackage(
        name sqlitepp
        GITHUB_REPOSITORY mpaland/sqlitepp
        GIT_TAG 1.6.0
)

CPMAddPackage(
        name argh
        GITHUB_REPOSITORY adishavit/argh
        VERSION 1.3.1
)

include_directories(PUBLIC
        include
        ${json_SOURCE_DIR}/single_include
        ${fmt_SOURCE_DIR}/include
        ${eff_random_SOURCE_DIR}/include
        ${sqlitepp_SOURCE_DIR}
        ${argh_SOURCE_DIR}
)

file(GLOB CIRCLE_INCLUDE src/*.h)
file(GLOB CIRCLE_SRC src/*.cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-fpermissive")

find_library(ZLIB NAMES z libz REQUIRED)
find_library(LIBBSD NAMES bsd libbsd REQUIRED)
add_compile_definitions(HAVE_LIBBSD)
link_libraries(${ZLIB} fmt::fmt ${LIBBSD})

# this is the core library we're making.
add_library(circlemud ${CIRCLE_INCLUDE} ${CIRCLE_SRC})
# everything we make will link to it.
link_libraries(circlemud)

# the various binaries
add_executable(circle apps/circle.cpp)
add_executable(test apps/test.cpp)

target_compile_definitions(circlemud PUBLIC USING_CMAKE=1 CIRCLE_UNIX=1 POSIX=1)

SET(circle_link ${CMAKE_INSTALL_PREFIX}/bin/)