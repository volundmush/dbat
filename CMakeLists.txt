cmake_minimum_required(VERSION 2.8)

project(circle)

file(GLOB CIRCLE_INCLUDE src/*.h)
#file(GLOB CIRCLE_SRC src/*.cpp)
file(GLOB CIRCLE_SRC src/*.c)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-fpermissive")

add_executable(circle ${CIRCLE_INCLUDE} ${CIRCLE_SRC})
target_include_directories(circle PUBLIC include)

target_compile_definitions(circle PUBLIC USING_CMAKE=1 CIRCLE_UNIX=1)

find_package(ZLIB)
if(ZLIB_FOUND)
    message("Compiling with ZLIB support.")
    target_compile_definitions(circle PUBLIC HAVE_ZLIB_H=1)
    target_link_libraries(circle ZLIB::ZLIB)
endif()

find_package(SQLite3 REQUIRED)
if(SQLite3_FOUND)
    message("Found SQLite3.")
    target_link_libraries(circle SQLite::SQLite3)
endif()

find_package(OpenSSL REQUIRED)
if(OpenSSL_FOUND)
    message("Found OpenSSL.")
    target_link_libraries(circle OpenSSL::Crypto)
endif()

# Support for FMT
target_include_directories(circle PUBLIC libs/fmt/include)
target_compile_definitions(circle PUBLIC FMT_HEADER_ONLY=1)

# Support for SQLITEPP
target_include_directories(circle PUBLIC libs/sqlitepp/include)

# Support for ASIO
target_include_directories(circle PUBLIC libs/asio/asio/include)

#set_property(TARGET circle PROPERTY CXX_STANDARD 17)
set_property(TARGET circle PROPERTY C_STANDARD 11)


SET(circle_link ${CMAKE_INSTALL_PREFIX}/bin/)