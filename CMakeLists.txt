cmake_minimum_required(VERSION 3.22)

set(CPM_DOWNLOAD_VERSION 0.35.5)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

project(circle)

find_package(ZLIB REQUIRED)


set(CMAKE_C_STANDARD 23)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_compile_options(-Wno-format-overflow)
add_compile_options(-Wno-format-truncation)
add_compile_options(-Wno-multichar)

find_library(LIBBSD NAMES bsd libbsd)
add_compile_definitions(HAVE_LIBBSD)


set(BUILD_TESTING OFF)

add_library(circlemud STATIC)
target_include_directories(circlemud
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

file(GLOB_RECURSE CIRCLE_INCLUDE include/*.h)
file(GLOB_RECURSE CIRCLE_SRC src/*.c)

target_sources(circlemud
    PRIVATE
        ${CIRCLE_SRC}
    PUBLIC
        FILE_SET HEADERS
      BASE_DIRS include
      FILES
        ${CIRCLE_INCLUDE}
)

target_link_libraries(circlemud
    PUBLIC
    bsd
    ZLIB::ZLIB
)

target_compile_definitions(circlemud PUBLIC USING_CMAKE=1 CIRCLE_UNIX=1 POSIX=1)

# the various binaries
#add_executable(circle)
#target_sources(circle PRIVATE apps/circle.cpp)
#target_link_libraries(circle PRIVATE circlemud)



SET(circle_link ${CMAKE_INSTALL_PREFIX}/bin/)