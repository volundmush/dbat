add_library(dbat_api STATIC)
add_library(dbat::api ALIAS dbat_api)

target_include_directories(dbat_api
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/api/*.hpp
)

target_sources(dbat_api
    PRIVATE
        ${SRC}
    PUBLIC
        FILE_SET HEADERS
      BASE_DIRS include
      FILES
        ${INCLUDE}
)

target_precompile_headers(dbat_api
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private/pch.hpp
)

target_link_libraries(dbat_api
  PUBLIC
        dbat::serde
        dbat::jwt
        dbat::web
  PRIVATE
    Boost::url
    Boost::iostreams
    OpenSSL::SSL
    OpenSSL::Crypto

)