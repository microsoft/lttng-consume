cmake_minimum_required(VERSION 3.7)

add_library(lttng-consume 
    LttngConsumer.cpp
    LttngConsumerImpl.cpp
    LttngJsonReader.cpp
    JsonBuilderSink.cpp)

target_include_directories(lttng-consume
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(lttng-consume 
    PUBLIC
        jsonbuilder::jsonbuilder
    PRIVATE
        babeltrace2::babeltrace2)

target_compile_features(lttng-consume PUBLIC cxx_std_17)

set_property(TARGET lttng-consume PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET lttng-consume PROPERTY SOVERSION 0)

add_library(lttng-consume::lttng-consume ALIAS lttng-consume)

include(GNUInstallDirs)

install(TARGETS lttng-consume 
    EXPORT lttng-consume-export
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(LTTNGCONSUME_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/lttng-consume)

install(EXPORT lttng-consume-export
    FILE lttng-consumeTargets.cmake
    NAMESPACE lttng-consume::
    DESTINATION ${LTTNGCONSUME_CMAKE_DIR})

install(FILES ${PROJECT_SOURCE_DIR}/cmake/modules/lttng-consumeConfig.cmake
    DESTINATION ${LTTNGCONSUME_CMAKE_DIR})
