add_library(dbat_ansi STATIC)
add_library(dbat::ansi ALIAS dbat_ansi)

target_compile_features(dbat_ansi PUBLIC cxx_std_23)

target_include_directories(dbat_ansi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private
)

file(GLOB_RECURSE SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dbat/ansi/*.hpp
)

target_precompile_headers(dbat_ansi
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private/pch.hpp
)

target_sources(dbat_ansi
    PRIVATE
        ${SRC}
    PUBLIC
        FILE_SET HEADERS
      BASE_DIRS include
      FILES
        ${INCLUDE}
)

target_link_libraries(dbat_ansi
   PUBLIC
    enchantum::enchantum
)
