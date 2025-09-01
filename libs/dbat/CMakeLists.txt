add_library(dbat STATIC)
add_library(dbat::dbat ALIAS dbat)

target_include_directories(dbat
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dbat/*.h
)

target_sources(dbat
    PRIVATE
        ${SRC}
    PUBLIC
        FILE_SET HEADERS
      BASE_DIRS include
      FILES
        ${INCLUDE}
)

target_link_libraries(dbat
  PUBLIC
    fmt::fmt
    magic_enum::magic_enum
    effolkronium_random
    nlohmann_json
    ZLIB::ZLIB
  PRIVATE
    Boost::iostreams
)