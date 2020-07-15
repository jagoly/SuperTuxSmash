cmake_minimum_required(VERSION 3.10)
project(SuperTuxSmash_Game)

################################################################################

set(SQEE_BUILD_ROOT CACHE PATH "The build directory for the SQEE Library")

if (NOT SQEE_BUILD_ROOT)
    message(FATAL_ERROR "You must specify the build directory for SQEE")
endif ()

################################################################################

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules/")

################################################################################

file(GLOB_RECURSE HEADERS "${PROJECT_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(sts-game ${HEADERS} ${SOURCES})

target_include_directories(sts-game PRIVATE "${PROJECT_SOURCE_DIR}/src")

set_property(TARGET sts-game PROPERTY CXX_STANDARD 17)
set_property(TARGET sts-game PROPERTY CXX_STANDARD_REQUIRED True)

################################################################################

include("${SQEE_BUILD_ROOT}/sqee-exports.cmake")

target_compile_definitions(sts-game PRIVATE SQEE_GLOBAL_BUILTINS)

target_link_libraries(sts-game sqee)

################################################################################

if (SQEE_GNU OR SQEE_CLANG)

    target_compile_options(sts-game PRIVATE -Wall -Wextra)
    #target_compile_options(sts-game PRIVATE -Wno-unused-parameter -Wno-strict-aliasing)
    #target_compile_options(sts-game PRIVATE -Wno-comment -Wno-misleading-indentation)

    if (SQEE_WINDOWS)
        # todo: actually test this
        target_compile_options(sts-game PRIVATE -mwindows -Wa,-mbig-obj)
    endif ()

elseif (SQEE_MSVC)

    target_compile_options(sts-game PRIVATE /bigobj /W3)
    #target_compile_options(sts-game PRIVATE /wd4201)

endif ()

################################################################################

add_custom_command(TARGET sts-game POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink
    "${PROJECT_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/assets")

add_custom_command(TARGET sts-game POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink
    "${PROJECT_SOURCE_DIR}/shaders" "${CMAKE_BINARY_DIR}/shaders")

add_custom_command(TARGET sts-game POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink
    "${PROJECT_SOURCE_DIR}/player1.json" "${CMAKE_BINARY_DIR}/player1.json")
add_custom_command(TARGET sts-game POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink
    "${PROJECT_SOURCE_DIR}/player2.json" "${CMAKE_BINARY_DIR}/player2.json")
add_custom_command(TARGET sts-game POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink
    "${PROJECT_SOURCE_DIR}/player3.json" "${CMAKE_BINARY_DIR}/player3.json")
add_custom_command(TARGET sts-game POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink
    "${PROJECT_SOURCE_DIR}/player4.json" "${CMAKE_BINARY_DIR}/player4.json")
