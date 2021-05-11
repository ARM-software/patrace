cmake_minimum_required(VERSION 3.6.0)

set(PROJECT_DIR ${CMAKE_SOURCE_DIR}/../../../)
set(THIRD_PARTY_DIR ${PROJECT_DIR}/thirdparty)
set(SOURCE_DIR ${PROJECT_DIR}/collectors)

add_definitions(-DANDROID_STL="c++_shared")
add_definitions(-DANDROID_CPP_FEATURES="exceptions")

set(COLLECTOR_SOURCES
    ${PROJECT_DIR}/interface.cpp
    ${SOURCE_DIR}/collector_utility.cpp
    ${SOURCE_DIR}/cputemp.cpp
    ${SOURCE_DIR}/ferret.cpp
    ${SOURCE_DIR}/rusage.cpp
    ${SOURCE_DIR}/streamline.cpp
    ${SOURCE_DIR}/streamline_annotate.cpp
    ${SOURCE_DIR}/memory.cpp
    ${SOURCE_DIR}/cpufreq.cpp
    ${SOURCE_DIR}/gpufreq.cpp
    ${SOURCE_DIR}/perf.cpp
    ${SOURCE_DIR}/power.cpp
    ${SOURCE_DIR}/procfs_stat.cpp
    ${SOURCE_DIR}/hwcpipe.cpp
    ${SOURCE_DIR}/mali_counters.cpp
    ${THIRD_PARTY_DIR}/jsoncpp/json_writer.cpp
    ${THIRD_PARTY_DIR}/jsoncpp/json_reader.cpp
    ${THIRD_PARTY_DIR}/jsoncpp/json_value.cpp)

set(BURROW_SOURCES ${PROJECT_DIR}/burrow.cpp)

set(TARGET_INCLUDE_DIRS
    ${SOURCE_DIR}
    ${THIRD_PARTY_DIR}
    ${THIRD_PARTY_DIR}/jsoncpp
    ${THIRD_PARTY_DIR}/jsoncpp/json
    ${PROJECT_DIR})

include_directories(${TARGET_INCLUDE_DIRS})

add_library(app-glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

add_library(collector_android STATIC ${COLLECTOR_SOURCES})
target_link_libraries(collector_android app-glue)

find_library(log-lib log)
find_library(android-lib android)

add_executable(burrow ${BURROW_SOURCES})
target_link_libraries(burrow app-glue ${log-lib} ${android-lib} ${app-glue} collector_android)
