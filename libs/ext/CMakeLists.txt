# Python dev + module support (gives Python_add_library and Python::Module)
  find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)  # CMake's FindPython

  # Cython helpers (from cython-cmake). You can keep it as a build req, or vendor the CMake files.
  # If you want to vendor later: `pipx run cython-cmake vendor cmake` and then
  #   list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
  find_package(Cython MODULE REQUIRED)     # provides Cython::Cython and UseCython.cmake
  include(UseCython)                       # provides cython_transpile(...)

  # Transpile the .pyx to a C++ source in the build dir.
  # LANGUAGE CXX ensures a .cxx is produced; OUTPUT_VARIABLE gives us the file path.
  message(STATUS "Cython: transpiling ${CMAKE_CURRENT_SOURCE_DIR}/dbat_ext.pyx")
  cython_transpile(dbat_ext.pyx
                   LANGUAGE CXX
                   OUTPUT_VARIABLE DBAT_EXT_CORE)

  # Build a Python extension module with the correct SOABI/suffix
  # Build extension from core.pyx; its Cython module name is "core",
  # so name the produced .so accordingly and drop the "lib" prefix.
  Python_add_library(dbat_ext MODULE ${DBAT_EXT_CORE} WITH_SOABI)
  # Ensure the output filename is exactly 'dbat_ext' without 'lib' prefix
  set_target_properties(dbat_ext PROPERTIES PREFIX "" OUTPUT_NAME "dbat_ext")

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

  # Install the Python extension into the top-level of the wheel so
  # `import dbat_ext` finds it directly. Cover both Unix (.so) and Windows (.pyd).
  install(TARGETS dbat_ext
          LIBRARY DESTINATION .
          RUNTIME DESTINATION .)