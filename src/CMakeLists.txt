# CMakeList.txt : CMake project for lttng-consume, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.12)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
find_package(uuid REQUIRED)
find_package(babeltrace REQUIRED)

set(LC_PrivateHeaders BabelPtr.h LttngConsumerImpl.h FailureHelpers.h JsonBuilderSink.h LttngJsonReader.h)
set(LC_SourceFiles LttngConsumer.cpp LttngConsumerImpl.cpp JsonBuilderSink.cpp)

set(PublicHeaderSrcPrefix ${PROJECT_SOURCE_DIR}/include/lttng-consume)
set(LC_PublicHeaders ${PublicHeaderSrcPrefix}/LttngConsumer.h)

# Add source to this project's executable.
add_library(lttng-consume ${LC_SourceFiles} ${LC_PrivateHeaders} ${LC_PublicHeaders})
target_compile_options(lttng-consume PRIVATE -Wextra -Wall -pedantic)
target_include_directories(lttng-consume PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(lttng-consume PUBLIC jsonbuilder PRIVATE babeltrace)
target_compile_features(lttng-consume PUBLIC cxx_std_11)

# TODO: Add tests and install targets if needed.

install(TARGETS lttng-consume DESTINATION lib)
install(FILES ${LC_PublicHeaders} DESTINATION include/lttng-consume)
