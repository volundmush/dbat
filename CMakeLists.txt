cmake_minimum_required(VERSION 3.22)

set(CPM_DOWNLOAD_VERSION 0.35.5)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

project(circle)

CPMAddPackage("gh:fmtlib/fmt#10.2.1")
CPMAddPackage("gh:SRombauts/SQLiteCpp#3.3.1")
CPMAddPackage("gh:nlohmann/json#v3.11.3")

set(CIRCLE_INCLUDE_DIRS
        include
        ${nlohmann_json_SOURCE_DIR}/single_include
        ${fmt_SOURCE_DIR}/include
        ${SQLiteCpp_SOURCE_DIR}/include
        )


file(GLOB_RECURSE CIRCLE_INCLUDE include/dbat/*.h include/shared/*.h)
file(GLOB_RECURSE CIRCLE_SRC src/*.cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-fpermissive -w")

set(BUILD_TESTING OFF)

set(CIRCLE_LINK_LIBRARIES
        SQLiteCpp sqlite3
        sodium
        )

if(WIN32)
set(CIRCLE_LINK_LIBRARIES ${CIRCLE_LINK_LIBRARIES} Ws2_32 wsock32)
endif()

# this is the core library we're making.
add_library(circlemud ${CIRCLE_INCLUDE} ${CIRCLE_SRC})
# everything we make will link to it.
target_link_libraries(circlemud ${CIRCLE_LINK_LIBRARIES})
target_include_directories(circlemud PUBLIC ${CIRCLE_INCLUDE_DIRS})

add_executable(migrate apps/migrate.cpp)
target_link_libraries(migrate circlemud ${CIRCLE_LINK_LIBRARIES})
target_include_directories(migrate PUBLIC ${CIRCLE_INCLUDE_DIRS})

add_executable(circle apps/circle.cpp)
target_link_libraries(circle circlemud ${CIRCLE_LINK_LIBRARIES})


target_include_directories(circle PUBLIC ${CIRCLE_INCLUDE_DIRS})

SET(circle_link ${CMAKE_INSTALL_PREFIX}/bin/)
