cmake_minimum_required(VERSION 3.16)
project(SuperTuxSmash CXX)

################################################################################

set(SQEE_BUILD_ROOT CACHE PATH "The build directory for the SQEE Library")

if (NOT SQEE_BUILD_ROOT)
    message(FATAL_ERROR "SQEE_BUILD_ROOT not specified")
endif ()

################################################################################

set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")

function (target_add_shaders TARGET)
    foreach (SHADER ${ARGN})

        string( REGEX REPLACE
                "${PROJECT_SOURCE_DIR}/shaders/(.*)\.(vert|geom|frag)"
                "${SHADER_OUTPUT_DIR}/\\1\.\\2\.spv"
                OUTPATH ${SHADER} )

        if (${SHADER} STREQUAL ${OUTPATH})
            message(FATAL_ERROR "bad shader path '${SHADER}'")
        endif ()

        get_filename_component(OUTDIR ${OUTPATH} DIRECTORY)
        file(MAKE_DIRECTORY ${OUTDIR})

        add_custom_command( OUTPUT ${OUTPATH}
                            #COMMAND glslc ${SHADER} -o ${OUTPATH}
                            COMMAND glslc ${SHADER} -o - | spirv-opt -O - -o ${OUTPATH}
                            DEPENDS ${SHADER}
                            IMPLICIT_DEPENDS CXX ${SHADER}
                            VERBATIM )

        set_source_files_properties(${OUTPATH} PROPERTIES GENERATED True)
        target_sources(${TARGET} PRIVATE ${OUTPATH})

    endforeach ()
endfunction ()

################################################################################

file(GLOB_RECURSE HEADERS "${PROJECT_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(sts-game ${HEADERS} ${SOURCES})

file(GLOB_RECURSE VERT_SHADERS "${PROJECT_SOURCE_DIR}/shaders/*.vert")
file(GLOB_RECURSE GEOM_SHADERS "${PROJECT_SOURCE_DIR}/shaders/*.geom")
file(GLOB_RECURSE FRAG_SHADERS "${PROJECT_SOURCE_DIR}/shaders/*.frag")
file(GLOB_RECURSE GLSL_HEADERS "${PROJECT_SOURCE_DIR}/shaders/*.glsl")

target_add_shaders(sts-game ${VERT_SHADERS} ${GEOM_SHADERS} ${FRAG_SHADERS})

target_sources(sts-game PRIVATE ${VERT_SHADERS} ${GEOM_SHADERS} ${FRAG_SHADERS} ${GLSL_HEADERS})
source_group("GLSL" FILES ${VERT_SHADERS} ${GEOM_SHADERS} ${FRAG_SHADERS} ${GLSL_HEADERS})

target_include_directories(sts-game PRIVATE "${PROJECT_SOURCE_DIR}/src")

set_property(TARGET sts-game PROPERTY CXX_STANDARD 17)
set_property(TARGET sts-game PROPERTY CXX_STANDARD_REQUIRED True)

# force use of the static msvc runtime so we don't need a bunch of extra dlls
set_property(TARGET sts-game PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)

################################################################################

#set_property(TARGET sts-game PROPERTY CXX_INCLUDE_WHAT_YOU_USE
#    "iwyu;-Xiwyu;any;-Xiwyu;iwyu;-Xiwyu;--mapping_file=/storage/Projects/SQEE/sqee/extras/iwyu-mapping.imp")

#target_precompile_headers(sts-game PUBLIC <setup.hpp>)

################################################################################

set(CMAKE_PREFIX_PATH APPEND ${SQEE_BUILD_ROOT})

find_package(sqee REQUIRED)

################################################################################

if (SQEE_GNU OR SQEE_CLANG)
    target_compile_options(sts-game PRIVATE -Wall -Wextra -Wpedantic)
elseif (SQEE_MSVC)
    target_compile_options(sts-game PRIVATE /W3 /wd4251)
endif ()

# this will automatically link dependencies and add include paths
target_link_libraries(sts-game sqee)

################################################################################

if (NOT CMAKE_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)

    add_custom_command(TARGET sts-game POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${PROJECT_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/assets")

    add_custom_command(TARGET sts-game POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${PROJECT_SOURCE_DIR}/wren" "${CMAKE_BINARY_DIR}/wren")

    add_custom_command(TARGET sts-game POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${PROJECT_SOURCE_DIR}/config" "${CMAKE_BINARY_DIR}/config")

endif ()
