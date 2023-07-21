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

CPMAddPackage("gh:fmtlib/fmt#9.1.0")
CPMAddPackage("gh:SRombauts/SQLiteCpp#3.3.0")
CPMAddPackage("gh:nlohmann/json#v3.11.2")
CPMAddPackage("gh:gabime/spdlog#v1.11.0")
CPMAddPackage("gh:Roblox/luau#0.570")
CPMAddPackage("gh:kunitoki/LuaBridge3#3.0-rc3")

SET(LUABRIDGE_TESTING OFF)
SET(LUAU_BUILD_CLI OFF)
SET(LUAU_BUILD_TESTS OFF)
SET(LUAU_BUILD_BENCHMARKS OFF)

find_package(ZLIB REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS system program_options regex)

set(CIRCLE_INCLUDE_DIRS
        include
        ${Boost_INCLUDE_DIRS}
        ${fmt_SOURCE_DIR}/include
        ${eff_random_SOURCE_DIR}/include
        ${SQLiteCpp_SOURCE_DIR}/include
        ${nlohmann_json_SOURCE_DIR}/single_include
        ${spdlog_SOURCE_DIR}/include
        ${luau_SOURCE_DIR}/Common/include
        ${luau_SOURCE_DIR}/VM/include
        ${luau_SOURCE_DIR}/Compiler/include
        ${LuaBridge_SOURCE_DIR}/Source
        )

file(GLOB_RECURSE CIRCLE_INCLUDE include/*.h)
file(GLOB_RECURSE CIRCLE_SRC src/*.cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-fpermissive -w")

find_library(LIBBSD NAMES bsd libbsd REQUIRED)
add_compile_definitions(HAVE_LIBBSD)

set(BUILD_TESTING OFF)

set(CIRCLE_LINK_LIBRARIES
        ZLIB::ZLIB fmt::fmt ${SQLite3_LIBRARIES} ${Boost_LIBRARIES}
        SQLiteCpp sqlite3 bsd
        Luau.VM Luau.Common Luau.Compiler
        )

# this is the core library we're making.
add_library(circlemud ${CIRCLE_INCLUDE} ${CIRCLE_SRC})
# everything we make will link to it.
target_link_libraries(circlemud ${CIRCLE_LINK_LIBRARIES})
target_compile_definitions(circlemud PUBLIC USING_CMAKE=1 CIRCLE_UNIX=1 POSIX=1)
target_include_directories(circlemud PUBLIC ${CIRCLE_INCLUDE_DIRS})

# the various binaries
add_executable(circle apps/circle.cpp)
target_link_libraries(circle circlemud ${CIRCLE_LINK_LIBRARIES})
target_include_directories(circle PUBLIC ${CIRCLE_INCLUDE_DIRS})

add_executable(dbconv apps/dbconv.cpp)
target_link_libraries(dbconv circlemud ${CIRCLE_LINK_LIBRARIES})
target_include_directories(dbconv PUBLIC ${CIRCLE_INCLUDE_DIRS})

add_executable(migrate apps/migrate.cpp)
target_link_libraries(migrate circlemud ${CIRCLE_LINK_LIBRARIES})
target_include_directories(migrate PUBLIC ${CIRCLE_INCLUDE_DIRS})

SET(circle_link ${CMAKE_INSTALL_PREFIX}/bin/)