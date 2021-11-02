cmake_minimum_required(VERSION 2.8)

project(circle CXX)

file(GLOB CIRCLE_INCLUDE src/*.h)
file(GLOB CIRCLE_SRC src/*.cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-fpermissive")

include_directories(libs/asio/asio/include)
include_directories(libs/sqlitepp)

include_directories(libs/random/include)

include_directories(libs/fmt/include)
add_compile_definitions(FMT_HEADER_ONLY=1)

include_directories(libs/json/single_include)


find_library(LIBBSD NAMES bsd libbsd)
include_directories(include)
if(${LIBBSD})
    link_libraries(${LIBBSD})
    add_compile_definitions(HAVE_LIBBSD)
endif()


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
add_executable(asciipasswd apps/circle.cpp)
add_executable(autowiz apps/autowiz.cpp)
add_executable(delobjs apps/delobjs.cpp)
add_executable(play2to3 apps/play2to3.cpp)
add_executable(shopconv apps/shopconv.cpp)
add_executable(showplay apps/showplay.cpp)
add_executable(sign apps/sign.cpp)
add_executable(split apps/split.cpp)
add_executable(wld2html apps/wld2html.cpp)

target_compile_definitions(circlemud PUBLIC USING_CMAKE=1 CIRCLE_UNIX=1 POSIX=1)

SET(circle_link ${CMAKE_INSTALL_PREFIX}/bin/)