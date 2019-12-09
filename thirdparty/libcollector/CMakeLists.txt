project(collector)

cmake_minimum_required(VERSION 2.8)

set(SRC_ROOT ".")

add_definitions(-std=c++11 -Wall -fno-strict-aliasing -ggdb)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# --- library ---

add_library(collector
        ${SRC_ROOT}/interface.cpp
        ${SRC_ROOT}/collectors/collector_utility.cpp
        ${SRC_ROOT}/collectors/cputemp.cpp
        ${SRC_ROOT}/collectors/debug.cpp
        ${SRC_ROOT}/collectors/rusage.cpp
        ${SRC_ROOT}/collectors/streamline.cpp
        ${SRC_ROOT}/collectors/streamline_annotate.cpp
        ${SRC_ROOT}/collectors/memory.cpp
        ${SRC_ROOT}/collectors/perf.cpp
        ${SRC_ROOT}/collectors/perf_record.cpp
        ${SRC_ROOT}/collectors/gpufreq.cpp
        ${SRC_ROOT}/collectors/power.cpp
        ${SRC_ROOT}/collectors/procfs_stat.cpp
        ${SRC_ROOT}/collectors/cpufreq.cpp
        ${SRC_ROOT}/collectors/hwcpipe.cpp
        ${SRC_ROOT}/collectors/mali_counters.cpp
        ${SRC_ROOT}/collectors/ferret.cpp
        ${SRC_ROOT}/thirdparty/jsoncpp/json_reader.cpp
        ${SRC_ROOT}/thirdparty/jsoncpp/json_value.cpp
        ${SRC_ROOT}/thirdparty/jsoncpp/json_writer.cpp
)

target_include_directories(collector
    PUBLIC ${SRC_ROOT}
    PRIVATE ${SRC_ROOT}/collectors
    PUBLIC ${SRC_ROOT}/thirdparty
    PRIVATE ${SRC_ROOT}/thirdparty/jsoncpp
)
target_link_libraries(collector rt)
set_target_properties(collector PROPERTIES LINK_FLAGS "-pthread" COMPILE_FLAGS "-pthread")

# --- test app ---

add_executable(test_app ${SRC_ROOT}/test.cpp)
target_link_libraries(test_app collector)
set_target_properties(test_app PROPERTIES LINK_FLAGS "-pthread" COMPILE_FLAGS "-pthread")

add_executable(burrow ${SRC_ROOT}/burrow.cpp)
target_link_libraries(burrow collector)
set_target_properties(burrow PROPERTIES LINK_FLAGS "-pthread" COMPILE_FLAGS "-pthread")
