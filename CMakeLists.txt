cmake_minimum_required(VERSION 3.22)
project(SuperTuxSmash CXX)

################################################################################

set(SQEE_BUILD_ROOT CACHE PATH "The build directory for the SQEE Library")

if (NOT SQEE_BUILD_ROOT)
    message(FATAL_ERROR "SQEE_BUILD_ROOT not specified")
endif ()

set(CMAKE_PREFIX_PATH ${SQEE_BUILD_ROOT})

find_package(sqee REQUIRED)

include(sqeeHelpers)

################################################################################

file(GLOB_RECURSE HEADERS "${PROJECT_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(sts-game ${HEADERS} ${SOURCES})

target_set_output_directory(sts-game "")

file(GLOB_RECURSE VERT_SHADERS "${PROJECT_SOURCE_DIR}/shaders/*.vert")
file(GLOB_RECURSE GEOM_SHADERS "${PROJECT_SOURCE_DIR}/shaders/*.geom")
file(GLOB_RECURSE FRAG_SHADERS "${PROJECT_SOURCE_DIR}/shaders/*.frag")
file(GLOB_RECURSE GLSL_HEADERS "${PROJECT_SOURCE_DIR}/shaders/*.glsl")

target_add_shaders(sts-game ${VERT_SHADERS} ${GEOM_SHADERS} ${FRAG_SHADERS})

file(GLOB_RECURSE JSON_FILES "${PROJECT_SOURCE_DIR}/*.json")
file(GLOB_RECURSE WREN_FILES "${PROJECT_SOURCE_DIR}/*.wren")

target_sources(sts-game PRIVATE ${VERT_SHADERS} ${GEOM_SHADERS} ${FRAG_SHADERS} ${GLSL_HEADERS})
target_sources(sts-game PRIVATE ${JSON_FILES})
target_sources(sts-game PRIVATE ${WREN_FILES})

source_group("0 Headers" FILES ${HEADERS})
source_group("1 Sources" FILES ${SOURCES})
source_group("2 GLSL" FILES ${VERT_SHADERS} ${GEOM_SHADERS} ${FRAG_SHADERS} ${GLSL_HEADERS})
source_group("3 JSON" FILES ${JSON_FILES})
source_group("4 Wren" FILES ${WREN_FILES})

target_include_directories(sts-game PRIVATE "${PROJECT_SOURCE_DIR}/src")

set_property(TARGET sts-game PROPERTY CXX_STANDARD 20)
set_property(TARGET sts-game PROPERTY CXX_STANDARD_REQUIRED True)

target_link_or_copy_directory(sts-game "assets")
target_link_or_copy_directory(sts-game "config")
target_link_or_copy_directory(sts-game "wren")

################################################################################

if (SQEE_GNU OR SQEE_CLANG)
    target_compile_options(sts-game PRIVATE -Wall -Wextra -Wpedantic)
elseif (SQEE_MSVC)
    target_compile_options(sts-game PRIVATE /W3 /wd4251)
    target_set_msvc_options(sts-game)
endif ()

# this will automatically link dependencies and add include paths
target_link_libraries(sts-game sqee)

################################################################################

#set_property(TARGET sts-game PROPERTY CXX_INCLUDE_WHAT_YOU_USE
#    "include-what-you-use;-Xiwyu;--mapping_file=/storage/Projects/SQEE/sqee/extras/iwyu-mapping.imp")

#target_precompile_headers(sts-game PUBLIC <setup.hpp>)
