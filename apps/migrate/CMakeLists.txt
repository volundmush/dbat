add_executable(dbat_migrate)

target_compile_features(dbat_migrate PUBLIC cxx_std_23)

target_sources(dbat_migrate
    PRIVATE
        src/main.cpp
)

target_link_libraries(dbat_migrate
    PUBLIC
    dbat::serde
    ${MATH_LIB}
)

# Ensure we link using the C++ linker driver (for libstdc++ and friends)
set_target_properties(dbat_migrate PROPERTIES LINKER_LANGUAGE CXX)
