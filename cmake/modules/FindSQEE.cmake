# SQEE CMake Find Module

include(FindPackageHandleStandardArgs)

find_path(
    SQEE_INCLUDE_DIR
    NAMES "sqee/setup.hpp"
    PATH_SUFFIXES "include"
    PATHS ${SQEE_ROOT}
)

find_library(
    SQEE_LIBRARY
    NAMES "sqee" "libsqee"
    PATH_SUFFIXES "lib" "lib64"
    PATHS ${SQEE_ROOT}
)

find_package_handle_standard_args(
    SQEE DEFAULT_MSG
    SQEE_INCLUDE_DIR SQEE_LIBRARY
)
