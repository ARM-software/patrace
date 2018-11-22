set(SRC_TRACEVIEW
    ${SRC_ROOT}/tool/traceview/app.cpp
    ${SRC_ROOT}/tool/traceview/ui/main_frame.cpp
    ${SRC_ROOT}/tool/traceview/ui/frame_call_tree.cpp
    ${SRC_ROOT}/tool/traceview/ui/call_detail.cpp
    ${SRC_ROOT}/tool/traceview/ui/texture_frame.cpp
    ${SRC_ROOT}/common/trace_model.cpp
    ${SRC_ROOT}/common/call_parser.cpp
)

set_source_files_properties (
    ${SRC_ROOT}/common/call_parser.cpp
    PROPERTIES
        GENERATED True
)

include_directories(
	${SRC_ROOT}
	${SRC_ROOT}/common
	${SRC_ROOT}/dispatch
	${SRC_ROOT}/tool/traceview
)

add_definitions(-DRETRACE -DGLES_CALLCONVENTION=)

add_executable(traceview
	${SRC_TRACEVIEW}
)

find_package(wxWidgets REQUIRED core base richtext)
message(STATUS ${wxWidgets_LIBRARIES})

execute_process(COMMAND wx-config --cflags OUTPUT_VARIABLE wx_c_flags)
string(STRIP ${wx_c_flags} wx_c_flags)
execute_process(COMMAND wx-config --cxxflags OUTPUT_VARIABLE wx_cxx_flags)
string(STRIP ${wx_cxx_flags} wx_cxx_flags)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${wx_c_flags}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wx_cxx_flags}")

target_link_libraries (traceview
    common
    ${SNAPPY_LIBRARIES}
    md5
    dl
    rt
    ${wxWidgets_LIBRARIES}
    common_eglstate
    common_image
    common_system
    ${PNG_LIBRARIES}
    jsoncpp
    ${ZLIB_LIBRARIES}
)

add_dependencies (traceview call_parser_src_generation)
install(TARGETS traceview
    RUNTIME DESTINATION tools)

