add_executable(dbat_migrate)

target_sources(dbat_migrate
    PRIVATE
        src/main.cpp
)

target_link_libraries(dbat_migrate
    PUBLIC
        dbat::dbat
)