# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required(VERSION 3.7)

project(lttng-consume VERSION 0.1)

set(LTTNGCONSUME_ENABLE_TESTING TRUE CACHE BOOL "build test dir" )

if (NOT TARGET nonstd::string_view-lite)
    add_subdirectory(external/string-view-lite)
endif ()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
#find_package(uuid REQUIRED)
find_package(babeltrace2 REQUIRED)

# populates jsonbuilder::jsonbuilder
find_package(jsonbuilder REQUIRED)
# if (NOT TARGET jsonbuilder)
#     add_subdirectory(external/JsonBuilder)
# endif()

add_compile_options(-Wall -Wextra -Werror)

add_subdirectory(src)

# Only include testing stuff if we are the top level
if (${LTTNGCONSUME_ENABLE_TESTING} AND ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    enable_testing()

    if (NOT TARGET Catch2::Catch2)
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/external/Catch2/contrib")
        add_subdirectory(external/Catch2)
    endif ()

    if (NOT TARGET tracelogging)
        add_subdirectory(external/TraceLogging)
    endif()

    add_subdirectory(test)
endif ()
