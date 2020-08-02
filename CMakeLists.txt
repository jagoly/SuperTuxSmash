cmake_minimum_required(VERSION 3.12)
project(SuperTuxSmash CXX)

################################################################################

set(SQEE_BUILD_ROOT CACHE PATH "The build directory for the SQEE Library")

################################################################################

file(GLOB_RECURSE HEADERS "${PROJECT_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(sts-game ${HEADERS} ${SOURCES})

target_include_directories(sts-game PRIVATE "${PROJECT_SOURCE_DIR}/src")

set_property(TARGET sts-game PROPERTY CXX_STANDARD 17)
set_property(TARGET sts-game PROPERTY CXX_STANDARD_REQUIRED True)

#set_property(TARGET sts-game PROPERTY CXX_INCLUDE_WHAT_YOU_USE
#    "iwyu;-Xiwyu;any;-Xiwyu;iwyu;-Xiwyu;--mapping_file=/storage/Projects/SQEE/sqee/extras/iwyu-mapping.imp")

################################################################################

# todo: proper support for finding sqee would be good

if (SQEE_BUILD_ROOT)
    include("${SQEE_BUILD_ROOT}/sqee-exports.cmake")
elseif (EXISTS "../sqee/sqee-exports.cmake")
    include("../sqee/sqee-exports.cmake")
else ()
    message(FATAL_ERROR "Could not find SQEE, try specifying SQEE_BUILD_ROOT")
endif ()

# todo: these variables should come from finding sqee

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(SQEE_LINUX True)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(SQEE_WINDOWS True)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(SQEE_GNU True)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(SQEE_CLANG True)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(SQEE_MSVC True)
endif ()

################################################################################

if (SQEE_GNU OR SQEE_CLANG)
    target_compile_options(sts-game PRIVATE -Wall -Wextra)
elseif (SQEE_MSVC)
    target_compile_options(sts-game PRIVATE /W3 /wd4251)
endif ()

# this will automatically link dependencies and add include paths
target_link_libraries(sts-game sqee)

target_compile_definitions(sts-game PRIVATE SQEE_GLOBAL_BUILTINS)

################################################################################

if (NOT CMAKE_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)

    add_custom_command(TARGET sts-game POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${PROJECT_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/assets")

    add_custom_command(TARGET sts-game POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${PROJECT_SOURCE_DIR}/shaders" "${CMAKE_BINARY_DIR}/shaders")

    add_custom_command(TARGET sts-game POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${PROJECT_SOURCE_DIR}/config" "${CMAKE_BINARY_DIR}/config")

endif ()
