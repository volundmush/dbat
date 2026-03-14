add_library(dbat_serde STATIC)
add_library(dbat::serde ALIAS dbat_serde)

target_compile_features(dbat_serde PUBLIC cxx_std_23)

target_include_directories(dbat_serde
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dbat/serde/*.hpp
)

target_sources(dbat_serde
    PRIVATE
        ${SRC}
    PUBLIC
        FILE_SET HEADERS
      BASE_DIRS include
      FILES
        ${INCLUDE}
)

target_precompile_headers(dbat_serde
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private/pch.hpp
)

target_link_libraries(dbat_serde
  PUBLIC
    dbat::game
    nlohmann_json
  PRIVATE
    ZLIB::ZLIB
    Threads::Threads
)
