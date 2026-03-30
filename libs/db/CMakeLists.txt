add_library(dbat_db STATIC)
add_library(dbat::db ALIAS dbat_db)

# target is C23
target_compile_features(dbat_db PUBLIC c_std_23)
set_target_properties(dbat_db PROPERTIES LINKER_LANGUAGE C)
target_compile_options(dbat_db PUBLIC -std=gnu23)

target_include_directories(dbat_db
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private
)

# Global up src/*.cpp
file(GLOB_RECURSE SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

# Also glob up include/*.hpp

file(GLOB_RECURSE INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dbat/db/*.h
)

target_sources(dbat_db
    PRIVATE
        ${SRC}
    PUBLIC
        FILE_SET HEADERS
      BASE_DIRS include
      FILES
        ${INCLUDE}
)
