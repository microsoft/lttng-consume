cmake_minimum_required(VERSION 3.7)

project(lttng-consume VERSION 0.1)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

find_package(babeltrace2 REQUIRED)

# Allow submodule of jsonbuilder
if (NOT TARGET jsonbuilder::jsonbuilder)
    if (EXISTS ${PROJECT_SOURCE_DIR}/external/JsonBuilder/CMakeLists.txt)
        add_subdirectory(external/JsonBuilder)
    else ()
        find_package(jsonbuilder REQUIRED)
    endif ()
endif ()

add_compile_options(-Wall -Wextra)

add_subdirectory(src)

# Only include testing stuff if we are the top level, unless specifically opted out
option(LTTNGCONSUME_ENABLE_TESTING "build test dir" ON)
if (${LTTNGCONSUME_ENABLE_TESTING} AND ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    enable_testing()

    if (EXISTS ${PROJECT_SOURCE_DIR}/external/Catch2/CMakeLists.txt)
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/external/Catch2/contrib")
        add_subdirectory(external/Catch2)
    else ()
        find_package(Catch2 REQUIRED)
    endif ()

    if (EXISTS ${PROJECT_SOURCE_DIR}/external/TraceLogging/CMakeLists.txt)
        add_subdirectory(external/TraceLogging)
    else ()
        find_package(tracelogging REQUIRED)
    endif ()

    add_subdirectory(test)
endif ()
