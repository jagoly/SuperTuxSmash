# SQEE CMake Find Module

include(FindPackageHandleStandardArgs)

################################################################################

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    add_definitions(-DSQEE_DEBUG)
endif ()

if (SQEE_STATIC_LIB)
    add_definitions(-DSQEE_STATIC_LIB)
endif ()

################################################################################

if (CMAKE_SYSTEM_NAME MATCHES "Linux")

    set(SQEE_LINUX True)
    add_definitions(-DSQEE_LINUX)

elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")

    set(SQEE_WINDOWS True)
    add_definitions(-DSQEE_WINDOWS)

endif ()

################################################################################

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")

    set(SQEE_GNU True)
    add_definitions(-DSQEE_GNU)

elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")

    set(SQEE_CLANG True)
    add_definitions(-DSQEE_CLANG)

elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")

    set(SQEE_MSVC True)
    add_definitions(-DSQEE_MSVC)

endif ()

################################################################################

if (EXISTS "${SQEE_ROOT}/include/sqee/setup.hpp")

    message("file exists: ${SQEE_ROOT}/include/sqee/setup.hpp")
    set(SQEE_INCLUDE_DIR "${SQEE_ROOT}/include")

endif ()

################################################################################

if (SQEE_LINUX)

    if (NOT SQEE_STATIC_LIB AND EXISTS "${SQEE_ROOT}/sqee.so")

        message("file exists: ${SQEE_ROOT}/sqee.so")
        set(SQEE_LIBRARY "${SQEE_ROOT}/sqee.so")

    elseif (SQEE_STATIC_LIB AND EXISTS "${SQEE_ROOT}/sqee.a")

        message("file exists: ${SQEE_ROOT}/sqee.a")
        set(SQEE_LIBRARY "${SQEE_ROOT}/sqee.a")

    endif ()

elseif (SQEE_WINDOWS AND EXISTS "${SQEE_ROOT}/sqee.lib")

    message("file exists: ${SQEE_ROOT}/sqee.lib")
    set(SQEE_LIBRARY "${SQEE_ROOT}/sqee.lib")

endif ()

################################################################################

# these don't work for some reason

#find_path(
#    SQEE_INCLUDE_DIR
#    NAMES "sqee/setup.hpp"
#    PATH_SUFFIXES "include"
#    PATHS ${SQEE_ROOT}
#)

#find_library(
#    SQEE_LIBRARY
#    NAMES "sqee" "libsqee"
#    PATH_SUFFIXES "build"
#    PATHS ${SQEE_ROOT}
#)

################################################################################

find_package_handle_standard_args(
    SQEE DEFAULT_MSG
    SQEE_INCLUDE_DIR SQEE_LIBRARY
)
