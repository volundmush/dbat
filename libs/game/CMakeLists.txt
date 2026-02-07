add_library(dbat_game STATIC)
add_library(dbat::game ALIAS dbat_game)

target_compile_features(dbat_game PUBLIC cxx_std_23)

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
    fmt::fmt
    enchantum::enchantum
    effolkronium_random
    dbat::valid
    Boost::regex
    Threads::Threads
    volcano::util
    volcano::circle
    volcano::log
    volcano::mud
    bsd
    ${MATH_LIB}
  PRIVATE
    Boost::iostreams
)