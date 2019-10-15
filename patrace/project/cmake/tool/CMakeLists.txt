###########################################################################
## generate version header
###########################################################################

configure_file (
    "${COMMON_INCLUDE_DIRS}/tool/config.hpp.in"
    "${COMMON_INCLUDE_DIRS}/tool/config.hpp"
    )

###########################################################################
## common settings
###########################################################################

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LIBRARY_MM_SYSTEM dl rt)
endif ()

add_custom_command(
    OUTPUT ${SRC_ROOT}/helper/paramsize.cpp
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/helper/paramsize.py
    DEPENDS
    ${SPECS_SCRIPTS}
    ${SRC_ROOT}/specs/glesparams.py
    ${SRC_ROOT}/helper/paramsize.py
    WORKING_DIRECTORY ${SRC_ROOT}/helper
)

add_custom_command(
    OUTPUT ${SRC_ROOT}/specs/pa_func_to_version.cpp
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/specs/glxml_to_extensions.py
    DEPENDS
    ${SPECS_SCRIPTS}
    ${SRC_ROOT}/specs/glxml_to_extensions.py
    ${THIRDPARTY_INCLUDE_DIRS}/opengl-registry/xml/gl.xml
    WORKING_DIRECTORY ${SRC_ROOT}/specs
)

set (LIBRARIES_FOR_TOOLS
    common_eglstate
    common_image
    common_system
    common
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${SNAPPY_LIBRARIES}
    ${LIBRARIES_FOR_TOOLS_SYSTEM}
    jsoncpp
)

set (SRC_FOR_TOOLS
    ${SRC_ROOT}/tool/trace_interface.cpp
    ${SRC_ROOT}/common/call_parser.cpp
    ${SRC_ROOT}/common/trace_model.cpp
)

set_source_files_properties (
    ${SRC_ROOT}/common/call_parser.cpp
    PROPERTIES
        GENERATED True
)

include_directories(
    ${COMMON_INCLUDE_DIRS}
    ${SRC_ROOT}
    ${SRC_ROOT}/common
    ${SRC_ROOT}/dispatch
)

###########################################################################
## modifiers
###########################################################################

add_executable (texture_modifier
    ${COMMON_INCLUDE_DIRS}/tool/texture_modifier.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries (texture_modifier
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies (texture_modifier call_parser_src_generation)
install (TARGETS texture_modifier DESTINATION tools)

add_executable (shader_modifier
    ${COMMON_INCLUDE_DIRS}/tool/shader_modifier.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries (shader_modifier
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies (shader_modifier call_parser_src_generation)
install (TARGETS shader_modifier DESTINATION tools)

add_executable (client_side_buffer_modifier
    ${COMMON_INCLUDE_DIRS}/tool/client_side_buffer_modifier.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries (client_side_buffer_modifier
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies (client_side_buffer_modifier call_parser_src_generation)
install (TARGETS client_side_buffer_modifier DESTINATION tools)

add_executable (trace_looper
    ${COMMON_INCLUDE_DIRS}/tool/trace_looper.cpp
    ${COMMON_INCLUDE_DIRS}/tool/trace_looper.hpp
    ${THIRDPARTY_INCLUDE_DIRS}/md5/md5.c
    ${THIRDPARTY_INCLUDE_DIRS}/md5/md5.h
    ${SRC_FOR_TOOLS}
)
target_link_libraries (trace_looper
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies (trace_looper call_parser_src_generation)
install (TARGETS trace_looper DESTINATION tools)

###########################################################################
## dumpers
###########################################################################

add_executable (shader_dumper
    ${COMMON_INCLUDE_DIRS}/tool/shader_dumper.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries (shader_dumper
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies (shader_dumper call_parser_src_generation)
install (TARGETS shader_dumper DESTINATION tools)

add_executable (texture_dumper
    ${COMMON_INCLUDE_DIRS}/tool/texture_dumper.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries (texture_dumper
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies (texture_dumper call_parser_src_generation)
install (TARGETS texture_dumper DESTINATION tools)

###########################################################################

add_executable(flatten_threads
    ${SRC_ROOT}/tool/flatten_threads.cpp
    ${SRC_ROOT}/tool/utils.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries(flatten_threads
    md5
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies(flatten_threads call_parser_src_generation)
install(TARGETS flatten_threads DESTINATION tools)

###

add_executable(single_surface
    ${SRC_ROOT}/tool/single_surface.cpp
    ${SRC_ROOT}/tool/utils.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries(single_surface
    md5
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies(single_surface call_parser_src_generation)
install(TARGETS single_surface DESTINATION tools)

###

add_executable(shader_grep
    ${SRC_ROOT}/tool/shader_grep.cpp
    ${SRC_ROOT}/common/analysis_utility.cpp
    ${SRC_ROOT}/tool/parse_interface.cpp
    ${SRC_ROOT}/tool/glsl_parser.cpp
    ${SRC_ROOT}/tool/glsl_lookup.cpp
    ${SRC_ROOT}/tool/glsl_utils.cpp
    ${SRC_ROOT}/specs/pa_func_to_version.cpp
    ${SRC_FOR_TOOLS}
)
target_compile_definitions(shader_grep PRIVATE RETRACE GLES_CALLCONVENTION= TOOL_BUILD)
target_link_libraries(shader_grep
    md5
    dl
    common
    common_eglstate
    ${SNAPPY_LIBRARIES}
    md5
    ${LIBRARY_MM_SYSTEM}
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies(shader_grep
    call_parser_src_generation
)
install(TARGETS shader_grep DESTINATION tools)

###

add_executable(shader_repacker
    ${SRC_ROOT}/tool/shader_repacker.cpp
    ${SRC_ROOT}/common/analysis_utility.cpp
    ${SRC_ROOT}/tool/parse_interface.cpp
    ${SRC_ROOT}/tool/glsl_parser.cpp
    ${SRC_ROOT}/tool/glsl_lookup.cpp
    ${SRC_ROOT}/tool/glsl_utils.cpp
    ${SRC_ROOT}/specs/pa_func_to_version.cpp
    ${SRC_ROOT}/tool/utils.cpp
    ${SRC_FOR_TOOLS}
)
target_compile_definitions(shader_repacker PRIVATE RETRACE GLES_CALLCONVENTION= TOOL_BUILD)
target_link_libraries(shader_repacker
    md5
    dl
    common
    common_eglstate
    ${SNAPPY_LIBRARIES}
    md5
    ${LIBRARY_MM_SYSTEM}
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies(shader_repacker
    call_parser_src_generation
)
install(TARGETS shader_repacker DESTINATION tools)

###

add_executable(shader_analyzer
    ${SRC_ROOT}/tool/shader_analyzer.cpp
    ${SRC_ROOT}/common/analysis_utility.cpp
    ${SRC_ROOT}/tool/parse_interface.cpp
    ${SRC_ROOT}/tool/glsl_parser.cpp
    ${SRC_ROOT}/tool/glsl_lookup.cpp
    ${SRC_ROOT}/tool/glsl_utils.cpp
    ${SRC_ROOT}/specs/pa_func_to_version.cpp
    ${SRC_ROOT}/tool/utils.cpp
    ${SRC_FOR_TOOLS}
)
target_compile_definitions(shader_analyzer PRIVATE RETRACE GLES_CALLCONVENTION= TOOL_BUILD)
target_link_libraries(shader_analyzer
    md5
    dl
    common
    common_eglstate
    ${SNAPPY_LIBRARIES}
    md5
    ${LIBRARY_MM_SYSTEM}
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies(shader_analyzer
    call_parser_src_generation
)
install(TARGETS shader_analyzer DESTINATION tools)

###

add_executable(analyze_trace
    ${SRC_ROOT}/tool/analyze_trace.cpp
    ${SRC_ROOT}/common/analysis_utility.cpp
    ${SRC_ROOT}/tool/parse_interface.cpp
    ${SRC_ROOT}/tool/glsl_parser.cpp
    ${SRC_ROOT}/tool/glsl_lookup.cpp
    ${SRC_ROOT}/tool/glsl_utils.cpp
    ${SRC_ROOT}/specs/pa_func_to_version.cpp
    ${SRC_ROOT}/tool/parse_interface_retracing.cpp
    ${SRC_FOR_TOOLS}
    ${SRC_ROOT}/common/trace_model_utility.cpp
    ${SRC_ROOT}/dispatch/eglproc_auto.hpp
    ${SRC_ROOT}/dispatch/eglproc_auto.cpp
    ${SRC_ROOT}/dispatch/eglproc_retrace.cpp
    ${SRC_ROOT}/retracer/retracer.cpp
    ${SRC_ROOT}/retracer/retrace_api.cpp
    ${SRC_ROOT}/retracer/retrace_gles_auto.cpp
    ${SRC_ROOT}/retracer/retrace_egl.cpp
    ${SRC_ROOT}/retracer/eglconfiginfo.cpp
    ${SRC_ROOT}/retracer/glws.cpp
    ${SRC_ROOT}/retracer/glws_egl.cpp
    ${SRC_ROOT}/retracer/glws_egl_${WINDOWSYSTEM}.cpp
    ${SRC_ROOT}/retracer/state.cpp
    ${SRC_ROOT}/retracer/forceoffscreen/offscrmgr.cpp
    ${SRC_ROOT}/retracer/forceoffscreen/quad.cpp
    ${SRC_ROOT}/retracer/glstate_images.cpp
    ${SRC_ROOT}/retracer/trace_executor.cpp
    ${SRC_ROOT}/retracer/dma_buffer/dma_buffer.cpp
    ${SRC_ROOT}/helper/states.cpp
    ${SRC_ROOT}/helper/shaderutility.cpp
    ${SRC_ROOT}/helper/depth_dumper.cpp
    ${SRC_ROOT}/helper/paramsize.cpp
    ${SRC_ROOT}/helper/shadermod.cpp
    ${SRC_ROOT}/tool/utils.cpp
)
target_compile_definitions(analyze_trace PRIVATE RETRACE GLES_CALLCONVENTION= TOOL_BUILD)
set_source_files_properties(
    ${SRC_ROOT}/dispatch/eglproc_auto.hpp
    ${SRC_ROOT}/dispatch/eglproc_auto.cpp
    ${SRC_ROOT}/retracer/retrace_gles_auto.cpp
    PROPERTIES
        GENERATED True
)
target_link_libraries(analyze_trace
    md5
    dl
    common
    common_eglstate
    ${SNAPPY_LIBRARIES}
    md5
    ${LIBRARY_MM_SYSTEM}
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    jsoncpp
    ${LIBRARIES_FOR_TOOLS}
    ${CMAKE_BINARY_DIR}/libcollector/libcollector.a
)
set_target_properties(analyze_trace PROPERTIES LINK_FLAGS "-pthread" COMPILE_FLAGS "-pthread")
if (${WINDOWSYSTEM} MATCHES "wayland")
    message(STATUS "link with wayland: " ${wayland_LIBRARIES})
    target_compile_options(analyze_trace PRIVATE ${wayland_CFLAGS})
    target_link_libraries(analyze_trace ${wayland_LDFLAGS})
endif()
if (${WINDOWSYSTEM} MATCHES "x11")
    message(STATUS "link with x11: " ${X11_X11_LIB})
    target_link_libraries(analyze_trace ${X11_X11_LIB})
endif()
add_dependencies(analyze_trace
    call_parser_src_generation
    retrace_gles_auto_src_generation
    eglproc_auto_src_generation
)
install(TARGETS analyze_trace DESTINATION tools)

###

add_executable(rename_call
    ${SRC_ROOT}/tool/rename_call.cpp
    ${SRC_ROOT}/tool/utils.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries(rename_call
    md5
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies(rename_call call_parser_src_generation)
install(TARGETS rename_call DESTINATION tools)

###

add_executable(trim
    ${SRC_ROOT}/tool/trim.cpp
    ${SRC_ROOT}/tool/utils.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries(trim
    md5
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies(trim call_parser_src_generation)
install(TARGETS trim DESTINATION tools)

###

add_executable(strip
    ${SRC_ROOT}/tool/strip.cpp
    ${SRC_ROOT}/tool/utils.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries(strip
    md5
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies(strip call_parser_src_generation)
install(TARGETS strip DESTINATION tools)

###

add_executable(insert_swap_before_terminate
    ${SRC_ROOT}/tool/insert_swap_before_terminate.cpp
    ${SRC_ROOT}/tool/utils.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries(insert_swap_before_terminate
    md5
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies(insert_swap_before_terminate call_parser_src_generation)
install(TARGETS insert_swap_before_terminate DESTINATION tools)

###

add_executable(resize
    ${SRC_ROOT}/tool/resize.cpp
    ${SRC_ROOT}/tool/utils.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries(resize
    md5
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies(resize call_parser_src_generation)
install(TARGETS resize DESTINATION tools)

###

add_executable (header_patcher
    ${SRC_ROOT}/tool/header_patcher.cpp
)
target_link_libraries (header_patcher
    jsoncpp
    common
)
install (TARGETS header_patcher DESTINATION tools)

###

add_executable(vr_pp
    ${SRC_ROOT}/tool/vr_postprocessing.cpp
    ${SRC_ROOT}/tool/utils.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries(vr_pp
    md5
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies(vr_pp call_parser_src_generation)
install(TARGETS vr_pp DESTINATION tools)

###

add_executable(replace_shader
    ${SRC_ROOT}/tool/replace_shader.cpp
    ${SRC_ROOT}/tool/utils.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries(replace_shader
    md5
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies(replace_shader call_parser_src_generation)
install(TARGETS replace_shader DESTINATION tools)

###

add_executable(rgba_to_yuv
    ${SRC_ROOT}/tool/rgba_to_yuv.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries(rgba_to_yuv
    md5
    jsoncpp
    common_eglstate
    common_image
    common_system
    common
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${SNAPPY_LIBRARIES}
    ${LIBRARIES_FOR_TOOLS}
    ${LIBRARIES_FOR_TOOLS_SYSTEM}
)
add_dependencies(rgba_to_yuv call_parser_src_generation)
install(TARGETS rgba_to_yuv DESTINATION tools)

###

add_executable(APIremap_post_processing
    ${SRC_ROOT}/tool/APIremap_post_processing.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries(APIremap_post_processing
    md5
    ${LIBRARIES_FOR_TOOLS}
)
add_dependencies(APIremap_post_processing call_parser_src_generation)
install(TARGETS APIremap_post_processing DESTINATION tools)

###

set(SRC_TRACE_TO_TXT
    ${SRC_ROOT}/tool/trace_to_txt.cpp
    ${SRC_ROOT}/common/trace_model.cpp
    ${SRC_ROOT}/common/call_parser.cpp
    ${SRC_ROOT}/common/api_info.cpp
)

set_source_files_properties (
    ${SRC_ROOT}/common/call_parser.cpp
    PROPERTIES
        GENERATED True
)

include_directories(
    ${SRC_ROOT}
    ${SRC_ROOT}/common
)

# must define RETRACE for glCalls to be defined in dispatch/eglproc.hpp
add_definitions(-DRETRACE -DGLES_CALLCONVENTION=)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors" )
add_executable(trace_to_txt
    ${SRC_TRACE_TO_TXT}
)

target_link_libraries (trace_to_txt
    #libpng
    #zlib
    common
    ${SNAPPY_LIBRARIES}
    md5
    dl #libdl, for dlopen
    rt #librt, realtime clock, mutexes
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    jsoncpp
    common_eglstate
)
add_dependencies (trace_to_txt call_parser_src_generation)
install (TARGETS trace_to_txt DESTINATION tools)

###

add_executable(remove_crop
    ${SRC_ROOT}/tool/remove_crop.cpp
    ${SRC_FOR_TOOLS}
)
target_link_libraries(remove_crop
    md5
    jsoncpp
    common_eglstate
    common_image
    common_system
    common
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${SNAPPY_LIBRARIES}
    ${LIBRARIES_FOR_TOOLS}
    ${LIBRARIES_FOR_TOOLS_SYSTEM}
)
add_dependencies(remove_crop call_parser_src_generation)
install(TARGETS remove_crop DESTINATION tools)

###########################################################################
