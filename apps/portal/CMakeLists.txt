add_executable(dbat_portal)

target_compile_features(dbat_portal PUBLIC cxx_std_23)

target_sources(dbat_portal
    PRIVATE
        src/main.cpp
)

target_link_libraries(dbat_portal
    PUBLIC
    dbat::net
    dbat::telnet
    dbat::config
)

set_target_properties(dbat_portal PROPERTIES LINKER_LANGUAGE CXX)
