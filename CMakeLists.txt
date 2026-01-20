cmake_minimum_required(VERSION 4.1.1)
project(dbat LANGUAGES C CXX)

# ---------------- basics (yours) ----------------
set(CPM_DOWNLOAD_VERSION 0.42.0)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
  message(STATUS "Downloading CPM.cmake")
  file(DOWNLOAD
       https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
       ${CPM_DOWNLOAD_LOCATION})
endif()
include(${CPM_DOWNLOAD_LOCATION})

# Pick a single C++ standard and use it everywhere
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(DBAT_TIME_TRACE "Enable clang -ftime-trace output" ON)

set(CPM_SOURCE_CACHE ${PROJECT_SOURCE_DIR}/dependencies)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/compiled)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/compiled)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/compiled)

# (Consider guarding these by platform if you plan to make wheels later)
set(CMAKE_CXX_FLAGS "-Wno-write-strings -Wno-format-truncation -Wno-multichar -Wno-unused-result -Wno-deprecated-declarations -fpermissive -march=native")
if(DBAT_TIME_TRACE)
  add_compile_options(-ftime-trace)
endif()
set(CMAKE_EXE_LINKER_FLAGS     "${CMAKE_EXE_LINKER_FLAGS}     -fuse-ld=mold")
set(CMAKE_MODULE_LINKER_FLAGS  "${CMAKE_MODULE_LINKER_FLAGS}  -fuse-ld=mold")
set(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS}  -fuse-ld=mold")

set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
set(CMAKE_C_COMPILER_LAUNCHER   ccache)

set(BUILD_TESTING OFF)
# ---------------- 
# ---------------- deps (yours) ----------------
find_package(OpenSSL REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS regex iostreams url)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

if(UNIX AND NOT APPLE)
  set(MATH_LIB m)
endif()

CPMAddPackage(NAME fmt                   GITHUB_REPOSITORY fmtlib/fmt            GIT_TAG 12.1.0)
CPMAddPackage(NAME spdlog                GITHUB_REPOSITORY gabime/spdlog         VERSION 1.17.0)
CPMAddPackage(NAME nlohmann_json         GITHUB_REPOSITORY nlohmann/json         VERSION 3.12.0)
CPMAddPackage(NAME enchantum            GITHUB_REPOSITORY ZXShady/enchantum    GIT_TAG 0.3.0)
#CPMAddPackage(NAME EnTT                  GITHUB_REPOSITORY skypjack/entt         VERSION 3.15.0)
CPMAddPackage(NAME effolkronium_random   GITHUB_REPOSITORY volundmush/random     GIT_TAG master
               OPTIONS "Random_BuildTests OFF")


# ---------------- targets ----------------
add_subdirectory(libs/logging)
add_subdirectory(libs/jwt)
add_subdirectory(libs/valid)
add_subdirectory(libs/dotenv)
add_subdirectory(libs/web)
add_subdirectory(libs/telnet)
add_subdirectory(libs/net)
add_subdirectory(libs/config)
add_subdirectory(libs/api)
add_subdirectory(libs/dbat)
add_subdirectory(libs/serde)

add_subdirectory(apps/migrate)
add_subdirectory(apps/server)
add_subdirectory(apps/portal)
