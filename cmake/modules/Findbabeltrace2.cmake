# Output target
#   babeltrace2::babeltrace2

include(AliasPkgConfigTarget)

if (TARGET babeltrace2::babeltrace2)
    return()
endif()

# First try and find with PkgConfig
find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(babeltrace2 QUIET IMPORTED_TARGET babeltrace2)
    if (TARGET PkgConfig::babeltrace2)
        alias_pkg_config_target(babeltrace2::babeltrace2 PkgConfig::babeltrace2)
        return()
    endif ()
endif ()

# If that doesn't work, try again with old fashioned path lookup, with some caching
if (NOT babeltrace2_LIBRARY)
    find_path(babeltrace2_INCLUDE_DIR
        NAMES babeltrace2/babeltrace.h)

    find_library(babeltrace2_LIBRARY
        NAMES babeltrace2)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(babeltrace2 DEFAULT_MSG
        babeltrace2_INCLUDE_DIR babeltrace2_LIBRARY)

    mark_as_advanced(babeltrace2_INCLUDE_DIR babeltrace2_LIBRARY)
endif()

add_library(babeltrace2::babeltrace2 UNKNOWN IMPORTED)
set_target_properties(babeltrace2::babeltrace2 PROPERTIES
    IMPORTED_LOCATION "${babeltrace2_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${babeltrace2_INCLUDE_DIR}")

set(babeltrace2_FOUND TRUE)
