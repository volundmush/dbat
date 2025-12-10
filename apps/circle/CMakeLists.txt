add_executable(circle)

target_sources(circle
    PRIVATE
        src/main.c
)

target_link_libraries(circle
    PUBLIC
    dbat::dbat
    ${MATH_LIB}
)
