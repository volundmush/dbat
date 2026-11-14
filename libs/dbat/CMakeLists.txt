add_library(dbat STATIC)
add_library(dbat::dbat ALIAS dbat)

target_compile_features(dbat PUBLIC cxx_std_26)

option(DBAT_WERROR "Treat warnings as errors (the debug and asan presets turn this on)" OFF)

# Flags shared by every source file in the server.
set(DBAT_CXX_FLAGS
    -g -fno-omit-frame-pointer
    -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wmissing-declarations
    -Wno-missing-field-initializers  # designated initializers leave the rest zeroed on purpose
    -Wno-format-truncation  # snprintf truncating is the intended, bounded behaviour
    -Wno-unused-result      # 45 unchecked fscanf/fgets/system results; tracked separately
    -Wformat=2 -Wno-format-nonliteral  # shop, guild and auction formats come from files, checked on load
    -Werror=format-security -Werror=return-type
    -Wshadow -Wredundant-decls -Wcast-qual -Wextra-semi -Wnull-dereference -Wundef -Wvla -Wzero-as-null-pointer-constant
    # GCC-only: conditions that are always true/false, identical branches, and stricter forms of the above
    $<$<CXX_COMPILER_ID:GNU>:-Wlogical-op -Wduplicated-cond -Wduplicated-branches
                             -Wformat-signedness -Wcast-align=strict -Wimplicit-fallthrough=5 -Wuseless-cast>
    $<$<CXX_COMPILER_ID:Clang>:-Wcast-align -Wimplicit-fallthrough>
    $<$<BOOL:${DBAT_WERROR}>:-Werror>
    CACHE INTERNAL "")
target_compile_options(dbat PRIVATE ${DBAT_CXX_FLAGS})

target_include_directories(dbat
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Global up src/*.cpp
file(GLOB_RECURSE SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Also glob up include/*.hpp
file(GLOB_RECURSE INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/dbat/*.hpp
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

add_executable(dbat_test_game_world tests/test_game_world.cpp)
target_compile_options(dbat_test_game_world PRIVATE ${DBAT_CXX_FLAGS})
target_link_libraries(dbat_test_game_world PRIVATE dbat::dbat)
add_test(NAME game_world COMMAND dbat_test_game_world)

add_library(dbat_test_harness STATIC tests/harness.cpp)
target_compile_options(dbat_test_harness PRIVATE ${DBAT_CXX_FLAGS})
target_compile_definitions(dbat_test_harness PRIVATE DBAT_TEST_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/world")
target_link_libraries(dbat_test_harness PUBLIC dbat::dbat)

add_executable(dbat_test_world tests/test_world.cpp)
target_compile_options(dbat_test_world PRIVATE ${DBAT_CXX_FLAGS})
target_link_libraries(dbat_test_world PRIVATE dbat_test_harness)
add_test(NAME world COMMAND dbat_test_world)

add_executable(dbat_test_bad_data tests/test_bad_data.cpp)
target_compile_options(dbat_test_bad_data PRIVATE ${DBAT_CXX_FLAGS})
target_link_libraries(dbat_test_bad_data PRIVATE dbat_test_harness)
add_test(NAME bad_data COMMAND dbat_test_bad_data)

add_executable(dbat_test_gameplay tests/test_gameplay.cpp)
target_compile_options(dbat_test_gameplay PRIVATE ${DBAT_CXX_FLAGS})
target_link_libraries(dbat_test_gameplay PRIVATE dbat_test_harness m)
add_test(NAME gameplay COMMAND dbat_test_gameplay)

add_executable(dbat_test_commands tests/test_commands.cpp)
target_compile_options(dbat_test_commands PRIVATE ${DBAT_CXX_FLAGS})
target_link_libraries(dbat_test_commands PRIVATE dbat_test_harness)
add_test(NAME commands COMMAND dbat_test_commands)
set_tests_properties(commands PROPERTIES LABELS slow TIMEOUT 3600)

add_executable(dbat_test_techniques tests/test_techniques.cpp tests/scenes_offense.cpp tests/scenes_dispatchers.cpp
               tests/scenes_informative.cpp tests/scenes_items_wizard.cpp tests/scenes_other.cpp tests/scenes_misc.cpp tests/scenes_wizard.cpp)
target_compile_options(dbat_test_techniques PRIVATE ${DBAT_CXX_FLAGS})
target_link_libraries(dbat_test_techniques PRIVATE dbat_test_harness)
# in shards, so that ctest -j runs them side by side (under ASan the whole run takes over an hour)
foreach(shard RANGE 3)
  add_test(NAME techniques_${shard} COMMAND dbat_test_techniques --shard ${shard}/4)
  set_tests_properties(techniques_${shard} PROPERTIES LABELS slow TIMEOUT 3600)
endforeach()

add_executable(dbat_test_bot tests/test_bot.cpp)
target_compile_options(dbat_test_bot PRIVATE ${DBAT_CXX_FLAGS})
target_link_libraries(dbat_test_bot PRIVATE dbat_test_harness)
add_test(NAME bot COMMAND dbat_test_bot)
add_test(NAME corpus COMMAND dbat_test_bot --corpus ${CMAKE_CURRENT_SOURCE_DIR}/tests/corpus)

# Coverage-guided fuzzing, for the fuzz preset: see tools/fuzz.sh.
if(DBAT_FUZZ)
  target_compile_definitions(dbat_test_harness PRIVATE DBAT_FUZZ)
  foreach(target load boot script bot)
    add_executable(dbat_fuzz_${target} tests/fuzz/fuzz_${target}.cpp tests/fuzz/fuzz.cpp)
    target_include_directories(dbat_fuzz_${target} PRIVATE tests tests/fuzz)
    target_compile_options(dbat_fuzz_${target} PRIVATE -g)
    target_link_libraries(dbat_fuzz_${target} PRIVATE dbat_test_harness)
    target_link_options(dbat_fuzz_${target} PRIVATE -fsanitize=fuzzer -Wl,--wrap=exit)
  endforeach()
endif()

# Real servers and real clients: needs python3, and takes a minute or two.
add_test(NAME copyover COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/test_copyover.py $<TARGET_FILE:dbat_server>)
set_tests_properties(copyover PROPERTIES LABELS slow TIMEOUT 1200)
