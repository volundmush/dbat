add_executable(dbat_migrate)

target_compile_features(dbat_migrate PUBLIC cxx_std_23)

target_include_directories(dbat_game
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private
)

# Global up src/*.cpp
file(GLOB_RECURSE SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Also glob up include/*.hpp

file(GLOB_RECURSE INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dbat/*.hpp
)

target_sources(dbat_migrate
    PRIVATE
        ${SRC}
    PUBLIC
        FILE_SET HEADERS
      BASE_DIRS include
      FILES
        ${INCLUDE}
)

target_link_libraries(dbat_migrate
    PUBLIC
    dbat::game
    ${MATH_LIB}
)

# Ensure we link using the C++ linker driver (for libstdc++ and friends)
set_target_properties(dbat_migrate PROPERTIES LINKER_LANGUAGE CXX)
