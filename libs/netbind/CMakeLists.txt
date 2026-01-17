add_library(dbat_netbind STATIC)
add_library(dbat::netbind ALIAS dbat_netbind)

target_include_directories(dbat_netbind
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/netbind/*.hpp
)

target_sources(dbat_netbind
    PRIVATE
        ${SRC}
    PUBLIC
        FILE_SET HEADERS
      BASE_DIRS include
      FILES
        ${INCLUDE}
)

target_link_libraries(dbat_netbind
  PUBLIC
    dbat::dbat
  PRIVATE
    Boost::url
    Boost::iostreams
    OpenSSL::SSL
    OpenSSL::Crypto
)