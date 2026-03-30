add_executable(circle)

target_compile_features(circle PUBLIC cxx_std_23)

target_include_directories(circle
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private
)

# Global up src/*.cpp
file(GLOB_RECURSE SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)


target_sources(circle
    PRIVATE
        ${SRC}
)

target_link_libraries(circle
  PUBLIC
    dbat
)