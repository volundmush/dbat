add_executable(dbat_server)

target_compile_features(dbat_server PUBLIC c_std_23)
target_compile_options(dbat_server PRIVATE ${DBAT_C_FLAGS})
# Same binary name as the Zig build (zig-out/bin/dbat).
set_target_properties(dbat_server PROPERTIES OUTPUT_NAME dbat)
# Export symbols (-rdynamic) so crash backtraces in the log show function names.
set_target_properties(dbat_server PROPERTIES ENABLE_EXPORTS ON)

target_sources(dbat_server
    PRIVATE
        src/main.c
)

target_link_libraries(dbat_server
    PUBLIC
    dbat::dbat
)