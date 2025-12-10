cmake_minimum_required(VERSION 4.1.1)
project(circle LANGUAGES C CXX)

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
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CPM_SOURCE_CACHE ${PROJECT_SOURCE_DIR}/dependencies)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/compiled)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/compiled)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/compiled)

# (Consider guarding these by platform if you plan to make wheels later)
set(CMAKE_CXX_FLAGS "-Wno-write-strings -Wno-format-truncation -Wno-multichar -Wno-unused-result -Wno-deprecated-declarations -fpermissive -march=native")
set(CMAKE_EXE_LINKER_FLAGS     "${CMAKE_EXE_LINKER_FLAGS}     -fuse-ld=mold")
set(CMAKE_MODULE_LINKER_FLAGS  "${CMAKE_MODULE_LINKER_FLAGS}  -fuse-ld=mold")
set(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS}  -fuse-ld=mold")

set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
set(CMAKE_C_COMPILER_LAUNCHER   ccache)

set(BUILD_TESTING OFF)

option(DBAT_BUILD_PROFILE "Emit compiler timing/profiling information" OFF)
if(DBAT_BUILD_PROFILE)
  message(STATUS "DBAT_BUILD_PROFILE=ON (compiler timing enabled)")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-ftime-report)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-ftime-trace)
  endif()
endif()

# ---------------- deps (yours) ----------------
find_package(Boost CONFIG REQUIRED COMPONENTS regex iostreams)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

if(UNIX AND NOT APPLE)
  set(MATH_LIB m)
endif()

# Fallback: if the detected GNU C++ toolchain doesn't list libstdc++ among
# its implicit link libraries (common with partial/manual installs), add it
# to the standard C++ link line so executables link cleanly without per-target
# workarounds. This has no effect on toolchains that already inject it.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  list(FIND CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "stdc++" _has_stdcxx)
  if(_has_stdcxx EQUAL -1)
    message(STATUS "GNU toolchain missing implicit libstdc++; adding -lstdc++ to CMAKE_CXX_STANDARD_LIBRARIES")
    set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lstdc++")
  endif()
endif()

CPMAddPackage(NAME fmt                   GITHUB_REPOSITORY fmtlib/fmt            GIT_TAG 11.2.0)
CPMAddPackage(NAME spdlog                GITHUB_REPOSITORY gabime/spdlog         VERSION 1.15.3)
CPMAddPackage(NAME nlohmann_json         GITHUB_REPOSITORY nlohmann/json         VERSION 3.12.0)
CPMAddPackage(NAME magic_enum            GITHUB_REPOSITORY Neargye/magic_enum    VERSION 0.9.7)
CPMAddPackage(NAME EnTT                  GITHUB_REPOSITORY skypjack/entt         VERSION 3.15.0)
CPMAddPackage(NAME effolkronium_random   GITHUB_REPOSITORY volundmush/random     GIT_TAG master
               OPTIONS "Random_BuildTests OFF")


# ---------------- your native targets ----------------
add_subdirectory(libs/dbat)
add_subdirectory(apps/migrate)

# =====================================================================
# Python/Cython extension (built when scikit-build-core drives the build,
# or if you pass -DDBAT_BUILD_PYTHON=ON manually)
# =====================================================================
# Prefer enabling by default when building under scikit-build-core. Some
# frontends set SKBUILD as an environment variable; consult both env and var.
set(_dbat_build_python_default OFF)
if(DEFINED SKBUILD AND SKBUILD)
  set(_dbat_build_python_default ON)
elseif(DEFINED ENV{SKBUILD} AND NOT "$ENV{SKBUILD}" STREQUAL "")
  set(_dbat_build_python_default ON)
endif()
option(DBAT_BUILD_PYTHON "Build the Python extension" ${_dbat_build_python_default})
message(STATUS "DBAT_BUILD_PYTHON=${DBAT_BUILD_PYTHON}")

if(DBAT_BUILD_PYTHON)
  message(STATUS "Adding subdirectory libs/ext for Python/Cython extension")
  add_subdirectory(libs/ext)
endif()