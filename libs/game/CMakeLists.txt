add_library(dbat_game STATIC)
add_library(dbat::game ALIAS dbat_game)

target_compile_features(dbat_game PUBLIC cxx_std_23)
target_compile_definitions(dbat_game PUBLIC USING_CMAKE=1 CIRCLE_UNIX=1 POSIX=1)

target_include_directories(dbat_game
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private
)

# Global up src/*.cpp
file(GLOB_RECURSE SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*
)

# Also glob up include/*.h*

file(GLOB_RECURSE INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dbat/*.h*
)

target_sources(dbat_game
    PRIVATE
        ${SRC}
    PUBLIC
        FILE_SET HEADERS
      BASE_DIRS include
      FILES
        ${INCLUDE}
)

target_precompile_headers(dbat_game
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private/pch.hpp
)

target_link_libraries(dbat_game
  PUBLIC
    dbat::db
    ZLIB::ZLIB
    effolkronium_random
)