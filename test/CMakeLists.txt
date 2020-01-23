# CMakeList.txt : CMake project for lttng-consume, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.7)

# Add source to this project's executable.
add_executable(lttng-consumeTest TestTracepoint.cpp TestTraceLogging.cpp TestTraceLoggingKeywords.cpp Test-Tracepoint.h Test-Tracepoint.cpp CatchMain.cpp)
target_compile_features(lttng-consumeTest PRIVATE cxx_std_11)
target_include_directories(lttng-consumeTest PRIVATE .)
target_link_libraries(lttng-consumeTest PRIVATE tracelogging lttng-consume Catch2::Catch2 pthread dl lttng-ust lttng-ust-tracepoint)

include(CTest)
include(Catch)
catch_discover_tests(lttng-consumeTest)

install(TARGETS lttng-consumeTest DESTINATION bin)
