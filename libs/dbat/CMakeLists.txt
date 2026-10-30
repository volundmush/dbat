add_library(dbat STATIC)
add_library(dbat::dbat ALIAS dbat)

target_compile_features(dbat PUBLIC c_std_23)
target_compile_definitions(dbat PUBLIC USING_CMAKE=1 CIRCLE_UNIX=1 POSIX=1)

target_include_directories(dbat
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Global up src/*.c
file(GLOB_RECURSE SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

# Also glob up include/*.h
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
