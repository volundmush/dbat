add_library(dbat_net STATIC)
add_library(dbat::net ALIAS dbat_net)

target_include_directories(dbat_net
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/net/*.hpp
)

target_precompile_headers(dbat_net
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private/pch.hpp
)

target_sources(dbat_net
    PRIVATE
        ${SRC}
    PUBLIC
        FILE_SET HEADERS
      BASE_DIRS include
      FILES
        ${INCLUDE}
)

# Add deps as needed
target_link_libraries(dbat_net
  PUBLIC
    dbat::logging
  PRIVATE
    Boost::iostreams
    OpenSSL::SSL
    OpenSSL::Crypto
)