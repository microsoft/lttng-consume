#
# Input variables:
#   BABELTRACE2_PREFIX
#
# Output variables
#   BABELTRACE2_FOUND
#   BABELTRACE2_LIBRARIES
#   BABELTRACE2_INCLUDE_DIRS
#

#if (BABELTRACE2_INCLUDE_DIRS AND BABELTRACE2_LIBRARIES)
#    set(BABELTRACE2_FIND_QUIETLY TRUE)
#else ()
    find_path(BABELTRACE2_INCLUDE_DIRS NAMES babeltrace2/babeltrace.h)

    # You really don't want a static version of this because it loads a bunch of plugins.
    find_library(BABELTRACE2_LIBRARIES NAMES libbabeltrace2.so)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(babeltrace2 DEFAULT_MSG BABELTRACE2_LIBRARIES BABELTRACE2_INCLUDE_DIRS)

 #   if (NOT BABELTRACE_FOUND)
 #       message(FATAL_ERROR "babeltrace not found")
 #   endif ()

    mark_as_advanced(BABELTRACE2_LIBRARIES BABELTRACE2_INCLUDE_DIRS)
#endif()

add_library(babeltrace2 INTERFACE)
target_include_directories(babeltrace2 INTERFACE ${BABELTRACE2_INCLUDE_DIRS})
target_link_libraries(babeltrace2 INTERFACE ${BABELTRACE2_LIBRARIES} glib-2.0 gmodule-2.0)

#might need to eventually include glib2, libpopt, uuid?