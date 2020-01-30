cmake_minimum_required(VERSION 3.7)

add_executable(lttng-consumeTest
    TestTracepoint.cpp
    TestTraceLogging.cpp
    Test-Tracepoint.cpp
    CatchMain.cpp)
target_compile_features(lttng-consumeTest PRIVATE cxx_std_17)
target_include_directories(lttng-consumeTest PRIVATE .)

target_link_libraries(lttng-consumeTest
    PRIVATE
        lttng-consume
        tracelogging::tracelogging
        Catch2::Catch2
        pthread)

include(CTest)
include(Catch)
catch_discover_tests(lttng-consumeTest)
