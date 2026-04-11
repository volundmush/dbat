add_executable(dbat_server)

target_compile_features(dbat_server PUBLIC cxx_std_23)

target_sources(dbat_server
    PRIVATE
        src/main.c
)

target_link_libraries(dbat_server
    PUBLIC
    dbat::db
    dbat::game
)

# Ensure we link using the C++ linker driver (for libstdc++ and friends)
set_target_properties(dbat_server PROPERTIES LINKER_LANGUAGE CXX)
