cmake_minimum_required(VERSION 3.4)
project(SuperTuxSmash_Game)

################################################################################

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build")

find_package(SQEE REQUIRED)

################################################################################

if (SQEE_GNU OR SQEE_CLANG)

    string(APPEND CMAKE_CXX_FLAGS " -std=c++17 -Wall -Wextra")

    string(APPEND CMAKE_CXX_FLAGS " -Wno-unused-parameter -Wno-strict-aliasing")
    string(APPEND CMAKE_CXX_FLAGS " -Wno-comment -Wno-misleading-indentation")

    if (SQEE_WINDOWS)
        string(APPEND CMAKE_CXX_FLAGS " -mwindows -Wa,-mbig-obj")
    endif ()

elseif (SQEE_MSVC)

    string(APPEND CMAKE_CXX_FLAGS " /std:c++17 /bigobj /W3 /wd4201")

endif ()

################################################################################

include("cmake/sources.cmake")

file(GLOB_RECURSE SHADERS "shaders/*.glsl")

################################################################################

add_executable(sts-game ${SOURCES} ${SHADERS})

include_directories(${SQEE_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src)

target_link_libraries(sts-game ${SQEE_LIBRARY})

################################################################################

file(COPY "assets" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY "shaders" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

file(COPY "player1.json" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY "player2.json" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
