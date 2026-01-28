add_executable(dbat_portal)

target_compile_features(dbat_portal PUBLIC cxx_std_23)

target_include_directories(dbat_portal
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

file(GLOB_RECURSE INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dbat/portal/*.hpp
)

target_sources(dbat_portal
    PRIVATE
        ${SRC}
    PUBLIC
        FILE_SET HEADERS
      BASE_DIRS include
      FILES
        ${INCLUDE}
)

target_precompile_headers(dbat_portal
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private/pch.hpp
)

target_link_libraries(dbat_portal
    PUBLIC
    volcano::portal
    volcano::web
    volcano::telnet
    volcano::config
    dbat::serde
)

set_target_properties(dbat_portal PROPERTIES LINKER_LANGUAGE CXX)
