# Python dev + module support (gives Python_add_library and Python::Module)
  find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)  # CMake's FindPython

  # Try cython-cmake if available; otherwise fall back to calling the cython CLI.
  set(DBAT_EXT_CORE "")
  find_package(Cython MODULE QUIET)
  if(Cython_FOUND)
    include(UseCython)
    message(STATUS "Cython: transpiling ${CMAKE_CURRENT_SOURCE_DIR}/dbat_ext.pyx (UseCython)")
    cython_transpile(dbat_ext.pyx LANGUAGE CXX OUTPUT_VARIABLE DBAT_EXT_CORE)
  else()
    message(STATUS "Cython: cython-cmake not found; using CLI fallback")
    find_program(CYTHON_EXECUTABLE NAMES cython3 cython)
    if(NOT CYTHON_EXECUTABLE)
      message(FATAL_ERROR "Cython executable not found. Please install 'cython' in your environment.")
    endif()
    set(DBAT_EXT_CORE "${CMAKE_CURRENT_BINARY_DIR}/dbat_ext.cxx")
    add_custom_command(
      OUTPUT "${DBAT_EXT_CORE}"
      COMMAND "${CYTHON_EXECUTABLE}" -3 --cplus -o "${DBAT_EXT_CORE}" "${CMAKE_CURRENT_SOURCE_DIR}/dbat_ext.pyx"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/dbat_ext.pyx"
      COMMENT "Cython (fallback): generating ${DBAT_EXT_CORE} from dbat_ext.pyx"
      VERBATIM
    )
  endif()

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
  #target_compile_features(dbat_ext PRIVATE cxx_std_23)

  # Install the Python extension into the top-level of the wheel so
  # `import dbat_ext` finds it directly. Cover both Unix (.so) and Windows (.pyd).
  #install(TARGETS dbat_ext LIBRARY DESTINATION . RUNTIME DESTINATION .)