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

# ---------------- deps (yours) ----------------
find_package(Boost CONFIG REQUIRED COMPONENTS regex iostreams)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

if(UNIX AND NOT APPLE)
  set(MATH_LIB m)
endif()

CPMFindPackage(NAME fmt                   GITHUB_REPOSITORY fmtlib/fmt            GIT_TAG 11.2.0)
CPMFindPackage(NAME nlohmann_json         GITHUB_REPOSITORY nlohmann/json         VERSION 3.12.0)
CPMFindPackage(NAME magic_enum            GITHUB_REPOSITORY Neargye/magic_enum    VERSION 0.9.7)
CPMFindPackage(NAME EnTT                  GITHUB_REPOSITORY skypjack/entt         VERSION 3.15.0)
CPMFindPackage(NAME effolkronium_random   GITHUB_REPOSITORY volundmush/random     GIT_TAG master
               OPTIONS "Random_BuildTests OFF")

# ---------------- your native targets ----------------
add_subdirectory(libs/dbat)
add_subdirectory(apps/migrate)

# =====================================================================
# Python/Cython extension (built when scikit-build-core drives the build,
# or if you pass -DDBAT_BUILD_PYTHON=ON manually)
# =====================================================================
# scikit-build / scikit-build-core set SKBUILD=ON; default the option from that.
option(DBAT_BUILD_PYTHON "Build the Python extension" ${SKBUILD})

if(DBAT_BUILD_PYTHON)
  message(STATUS "Configuring Python/Cython extension")

  # Python dev + module support (gives Python_add_library and Python::Module)
  find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)  # CMake's FindPython

  # Cython helpers (from cython-cmake). You can keep it as a build req, or vendor the CMake files.
  # If you want to vendor later: `pipx run cython-cmake vendor cmake` and then
  #   list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
  find_package(Cython MODULE REQUIRED)     # provides Cython::Cython and UseCython.cmake
  include(UseCython)                       # provides cython_transpile(...)

  # Transpile the .pyx to a C++ source in the build dir.
  # LANGUAGE CXX ensures a .cxx is produced; OUTPUT_VARIABLE gives us the file path.
  cython_transpile(${CMAKE_SOURCE_DIR}/dbat_ext/core.pyx
                   LANGUAGE CXX
                   OUTPUT_VARIABLE DBAT_EXT_CXX)

  # Build a Python extension module with the correct SOABI/suffix
  Python_add_library(dbat_ext MODULE "${DBAT_EXT_CXX}" WITH_SOABI)

  # Keep the usual target wiring – link to your CMake targets, not raw -l flags
  target_link_libraries(dbat_ext
    PRIVATE
      dbat::dbat
      fmt::fmt
      Boost::regex
      Boost::iostreams
      ZLIB::ZLIB
  )

  # Make sure we compile with the same standard/features as the rest
  target_compile_features(dbat_ext PRIVATE cxx_std_23)

  # If dbat exports public headers properly, you don't need this.
  # If not, add your include dirs here, e.g.:
  # target_include_directories(dbat_ext PRIVATE ${PROJECT_SOURCE_DIR}/libs/dbat/include)

  # On some platforms CMake will add "lib" to MODULE libs; keep the Pythonic name.
  set_target_properties(dbat_ext PROPERTIES PREFIX "" OUTPUT_NAME "dbat_ext")

  # Install into the wheel at the package root (importable as `import dbat_ext`)
  install(TARGETS dbat_ext DESTINATION ${SKBUILD_PROJECT_NAME})
endif()