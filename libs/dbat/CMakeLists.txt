add_library(dbat STATIC)
add_library(dbat::dbat ALIAS dbat)

target_compile_features(dbat PUBLIC c_std_23)

# Flags shared by every C file in the server. Keep in sync with c_flags in build.zig.
set(DBAT_C_FLAGS
    -g -fno-omit-frame-pointer
    -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wmissing-prototypes
    -Wno-format-truncation  # snprintf truncating is the intended, bounded behaviour
    -Wno-unused-result      # 45 unchecked fscanf/fgets/system results; tracked separately
    -Werror=format-security -Werror=return-type -Werror=implicit-function-declaration
    -Wshadow -Wredundant-decls -Wwrite-strings
    # GCC-only checks for conditions that are always true/false or branches that are identical
    $<$<C_COMPILER_ID:GNU>:-Wlogical-op -Wduplicated-cond -Wduplicated-branches>
    CACHE INTERNAL "")
target_compile_options(dbat PRIVATE ${DBAT_C_FLAGS})

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
