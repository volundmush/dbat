cmake_minimum_required(VERSION 3.28)

set(CPM_DOWNLOAD_VERSION 0.40.5)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

project(circle)

CPMAddPackage("gh:fmtlib/fmt#10.2.1")
CPMAddPackage("gh:nlohmann/json#v3.11.3")
CPMAddPackage("gh:gabime/spdlog#v1.14.1")
CPMAddPackage("gh:robinlinden/libsodium-cmake#master")
CPMAddPackage("gh:ilqvya/random#v1.5.0")
CPMAddPackage("gh:Neargye/magic_enum#v0.9.6")
CPMAddPackage("gh:tplgy/cppcodec#v0.2")

find_package(Boost CONFIG REQUIRED COMPONENTS system program_options regex)

set(CIRCLE_INCLUDE_DIRS
        include
        ${ZLIB_INCLUDE_DIRS}
        ${cppcodec_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
        ${fmt_SOURCE_DIR}/include
        ${nlohmann_json_SOURCE_DIR}/single_include
        ${spdlog_SOURCE_DIR}/include
        ${sodium_SOURCE_DIR}/libsodium/src/libsodium/include
        ${effolkronium_random_SOURCE_DIR}/include
        ${magic_enum_SOURCE_DIR}/include
        CACHE INTERNAL "Include directories for CIRCLE"
)


file(GLOB_RECURSE CIRCLE_INCLUDE include/dbat/*.h)
file(GLOB_RECURSE CIRCLE_SRC src/*.cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-fpermissive -w")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=mold")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=mold")

set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
set(CMAKE_C_COMPILER_LAUNCHER ccache)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

set(BUILD_TESTING OFF)

set(CIRCLE_LINK_LIBRARIES
        fmt::fmt ${Boost_LIBRARIES} ${ZLIB_LIBRARIES}
        #Luau.VM Luau.Common Luau.Compiler
        sodium
        effolkronium_random
        CACHE INTERNAL "Include libraries for CIRCLE"
        )

# this is the core library we're making.
add_library(circlemud ${CIRCLE_INCLUDE} ${CIRCLE_SRC})
# everything we make will link to it.
target_link_libraries(circlemud ${CIRCLE_LINK_LIBRARIES})
target_include_directories(circlemud PUBLIC ${CIRCLE_INCLUDE_DIRS})

add_executable(circle apps/circle.cpp)
target_link_libraries(circle circlemud ${CIRCLE_LINK_LIBRARIES})
target_include_directories(circle PUBLIC ${CIRCLE_INCLUDE_DIRS})

add_executable(migrate apps/migrate.cpp)
target_link_libraries(migrate circlemud ${CIRCLE_LINK_LIBRARIES})
target_include_directories(migrate PUBLIC ${CIRCLE_INCLUDE_DIRS})

SET(circle_link ${CMAKE_INSTALL_PREFIX}/bin/)