cmake_minimum_required(VERSION 3.18)

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

file(GLOB CIRCLE_INCLUDE src/*.h)
file(GLOB CIRCLE_SRC src/*.cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-fpermissive")

include_directories(libs/asio/asio/include)
include_directories(libs/sqlitepp)

add_subdirectory(libs/fmt)
include_directories(libs/fmt/include)
link_libraries(fmt::fmt)


find_library(LIBBSD NAMES bsd libbsd)

if(${LIBBSD})
    link_libraries(${LIBBSD})
    add_compile_definitions(HAVE_LIBBSD)
endif()

include_directories(PUBLIC include ${eff_random_SOURCE_DIR}/include $${json_SOURCE_DIR}/single_include)


# Support for ASIO
#include_directories(libs/asio/asio/include)

# Support for SQLITEPP
#include_directories(libs/sqlitepp)

# Support for FMT
#include_directories(libs/fmt/include)
#add_compile_definitions(FMT_HEADER_ONLY=1)

#find_package(ZLIB REQUIRED)
#link_libraries(ZLIB::ZLIB)
find_library(ZLIB NAMES z libz REQUIRED)
link_libraries(${ZLIB})

#find_package(SQLite3 REQUIRED)
#if(SQLite3_FOUND)
#    message("Found SQLite3.")
#    link_libraries(SQLite::SQLite3)
#endif()

#find_package(OpenSSL REQUIRED)
#if(OpenSSL_FOUND)
#    message("Found OpenSSL.")
#    link_libraries(OpenSSL::Crypto)
#endif()


# this is the core library we're making.
add_library(circlemud ${CIRCLE_INCLUDE} ${CIRCLE_SRC})
# everything we make will link to it.
link_libraries(circlemud)

# the various binaries
add_executable(circle apps/circle.cpp)
add_executable(test apps/test.cpp)
#add_executable(asciipasswd apps/circle.cpp)
#add_executable(autowiz apps/autowiz.cpp)
#add_executable(delobjs apps/delobjs.cpp)
#add_executable(play2to3 apps/play2to3.cpp)
#add_executable(shopconv apps/shopconv.cpp)
#add_executable(showplay apps/showplay.cpp)
#add_executable(sign apps/sign.cpp)
#add_executable(split apps/split.cpp)
#add_executable(wld2html apps/wld2html.cpp)

target_compile_definitions(circlemud PUBLIC USING_CMAKE=1 CIRCLE_UNIX=1 POSIX=1)

SET(circle_link ${CMAKE_INSTALL_PREFIX}/bin/)