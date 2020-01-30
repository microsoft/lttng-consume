get_filename_component(LTTNGCONSUME_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

find_dependency(jsonbuilder REQUIRED)

if (NOT TARGET lttng-consume::lttng-consume)
    include("${LTTNGCONSUME_CMAKE_DIR}/lttng-consumeTargets.cmake")
endif ()
