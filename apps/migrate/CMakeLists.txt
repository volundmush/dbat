add_executable(dbat_migrate)

target_sources(dbat_migrate
    PRIVATE
        src/main.cpp
)

target_link_libraries(dbat_migrate
    PUBLIC
    dbat::dbat
    fmt::fmt
    Boost::regex
    Boost::iostreams
    ZLIB::ZLIB
    Threads::Threads
    ${MATH_LIB}
)

# Ensure we link using the C++ linker driver (for libstdc++ and friends)
set_target_properties(dbat_migrate PROPERTIES LINKER_LANGUAGE CXX)

# Workaround: some toolchains (observed with GCC 15 on this system) are not
# injecting libstdc++/pthread implicitly during the link step in this VS Code
# CMake/Ninja path. Add them explicitly to avoid massive undefined std::*
# symbols at link time.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(dbat_migrate PRIVATE stdc++)
endif()

# Ensure pthread is linked even if Threads::Threads isn't propagated
target_link_libraries(dbat_migrate PRIVATE Threads::Threads)