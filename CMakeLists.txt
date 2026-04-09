cmake_minimum_required(VERSION 3.31)
project(dbat LANGUAGES C CXX)

set(CPM_DOWNLOAD_VERSION 0.42.1)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CPM_SOURCE_CACHE ${PROJECT_SOURCE_DIR}/dependencies)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-fpermissive -w")

CPMAddPackage(
        NAME json
        GITHUB_REPOSITORY nlohmann/json
        VERSION 3.12.0
)

CPMAddPackage(
        NAME eff_random
        GITHUB_REPOSITORY ilqvya/random
        VERSION 1.5.0
        OPTIONS "Random_BuildTests OFF"
)

CPMAddPackage(
        NAME cppcodec
        GITHUB_REPOSITORY tplgy/cppcodec
        VERSION 0.2
        OPTIONS "BUILD_TESTING OFF"
)

CPMAddPackage(
        NAME fmt
        GITHUB_REPOSITORY fmtlib/fmt
        GIT_TAG 9.1.0
)

CPMAddPackage(
        name sqlitepp
        GITHUB_REPOSITORY mpaland/sqlitepp
        GIT_TAG 1.6.0
)

CPMAddPackage(
        NAME spdlog
        GITHUB_REPOSITORY gabime/spdlog
        VERSION 1.10.0
)

CPMAddPackage(
        name argh
        GITHUB_REPOSITORY adishavit/argh
        VERSION 1.3.2
        OPTIONS "BUILD_EXAMPLES OFF" "BUILD_TESTS OFF"
)

find_package(ZLIB REQUIRED)
find_package(Boost REQUIRED)

set(BUILD_TESTING OFF)

add_subdirectory(libs/game)
add_subdirectory(apps/server)