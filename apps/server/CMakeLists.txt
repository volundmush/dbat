add_executable(dbat_server)

target_sources(dbat_server
    PRIVATE
        src/main.cpp
)

target_link_libraries(dbat_server
    PUBLIC
    dbat::netbind
    dbat::serde
)

# Ensure we link using the C++ linker driver (for libstdc++ and friends)
set_target_properties(dbat_server PROPERTIES LINKER_LANGUAGE CXX)