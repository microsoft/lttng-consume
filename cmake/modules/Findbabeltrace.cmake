#
# Input variables:
#   BABELTRACE_PREFIX
#
# Output variables
#   BABELTRACE_FOUND
#   BABELTRACE_LIBRARIES
#   BABELTRACE_INCLUDE_DIRS
#

#if (BABELTRACE_INCLUDE_DIRS AND BABELTRACE_LIBRARIES)
#    set(BABELTRACE_FIND_QUIETLY TRUE)
#else ()
    find_path(BABELTRACE_INCLUDE_DIRS NAMES babeltrace/babeltrace.h)

    # You really don't want a static version of this because it loads a bunch of plugins.
    find_library(BABELTRACE_LIBRARIES NAMES libbabeltrace.so)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(babeltrace DEFAULT_MSG BABELTRACE_LIBRARIES BABELTRACE_INCLUDE_DIRS)

 #   if (NOT BABELTRACE_FOUND)
 #       message(FATAL_ERROR "babeltrace not found")
 #   endif ()

    mark_as_advanced(BABELTRACE_LIBRARIES BABELTRACE_INCLUDE_DIRS)
#endif()

add_library(babeltrace INTERFACE)
target_include_directories(babeltrace INTERFACE ${BABELTRACE_INCLUDE_DIRS})
target_link_libraries(babeltrace INTERFACE ${BABELTRACE_LIBRARIES} glib-2.0 gmodule-2.0)

#might need to eventually include glib2, libpopt, uuid?