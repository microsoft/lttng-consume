#
# Input variables:
#   UUID_PREFIX
#
# Output variables
#   UUID_FOUND
#   UUID_LIBRARIES
#   UUID_INCLUDE_DIRS
#

if (NOT TARGET uuid)
    find_path(UUID_INCLUDE_DIRS NAMES uuid/uuid.h)
    find_library(UUID_LIBRARIES NAMES uuid)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(uuid DEFAULT_MSG UUID_LIBRARIES UUID_INCLUDE_DIRS)

    mark_as_advanced(UUID_LIBRARIES UUID_INCLUDE_DIRS)

    add_library(uuid INTERFACE)
    target_include_directories(uuid INTERFACE ${UUID_INCLUDE_DIRS})
    target_link_libraries(uuid INTERFACE ${UUID_LIBRARIES})
endif ()
