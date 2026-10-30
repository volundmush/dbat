add_executable(dbat_server)

target_compile_features(dbat_server PUBLIC c_std_23)

target_sources(dbat_server
    PRIVATE
        src/main.c
)

target_link_libraries(dbat_server
    PUBLIC
    dbat::db
    dbat::game
)