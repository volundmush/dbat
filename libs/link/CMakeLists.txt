add_library(dbat_link STATIC)
add_library(dbat::link ALIAS dbat_link)

target_compile_features(dbat_link PUBLIC cxx_std_23)

target_include_directories(dbat_link
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dbat/link/*.hpp
)

target_sources(dbat_link
    PRIVATE
        ${SRC}
    PUBLIC
        FILE_SET HEADERS
      BASE_DIRS include
      FILES
        ${INCLUDE}
)

target_precompile_headers(dbat_link
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private/pch.hpp
)

target_link_libraries(dbat_link
  PUBLIC
        dbat::serde
        pqxx
)
