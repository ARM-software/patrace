include_directories(${SRC_ROOT}/integration_tests ${SRC_ROOT}/specs ${SRC_ROOT} ${SRC_ROOT}/dispatch ${SRC_ROOT}/common)

add_custom_command(
    OUTPUT ${SRC_ROOT}/specs/pa_gl31.cpp ${SRC_ROOT}/specs/pa_gl31.h
    COMMAND ${SRC_ROOT}/specs/glxml_to_funcs
    DEPENDS
        ${THIRDPARTY_INCLUDE_DIRS}/opengl-registry/xml/gl.xml
        ${SRC_ROOT}/specs/glxml_to_funcs.py
    WORKING_DIRECTORY ${SRC_ROOT}/specs
)
add_custom_target(glxml_to_funcs DEPENDS ${SRC_ROOT}/specs/pa_gl31.cpp)

set(IT_LIBS -L${TEST_LIBRARY} ${TEST_LIBRARY}/libEGL.so ${TEST_LIBRARY}/libGLESv2.so pthread)
if (WINDOWSYSTEM MATCHES "x11")
	set(IT_LIBS ${IT_LIBS} X11)
endif ()
set(IT_CFLAGS "-std=c++11 -Wall")
set(IT_LFLAGS "${FBDEV_EXTRA_OPTS} -L${CMAKE_CURRENT_BINARY_DIR}/integration_tests/tests -L${TEST_LIBRARY}")
set(IT_DEPS glxml_to_funcs)
set(IT_FILES ${SRC_ROOT}/integration_tests/pa_demo.cpp ${SRC_ROOT}/specs/pa_gl31.cpp)

function(integration_test test_name)
	add_executable(${ARGV0} ${SRC_ROOT}/integration_tests/${ARGV0}.cpp ${IT_FILES})
	target_link_libraries(${ARGV0} ${IT_LIBS})
	set_target_properties(${ARGV0} PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
	set_target_properties(${ARGV0} PROPERTIES LINK_FLAGS ${IT_LFLAGS})
	add_dependencies(${ARGV0} ${IT_DEPS})
	install(TARGETS ${ARGV0} DESTINATION tests)
endfunction()

integration_test(dummy_1)
integration_test(multisurface_1)
integration_test(multithread_1)
integration_test(multithread_2)
integration_test(multithread_3)
integration_test(bindbufferrange_1)
integration_test(compute_1)
integration_test(compute_2)
integration_test(compute_3)
integration_test(programs_1)
integration_test(imagetex_1)
integration_test(indirectdraw_1)
integration_test(indirectdraw_2)
integration_test(drawrange_1)
integration_test(drawrange_2)
integration_test(directdraw_1)
integration_test(directdraw_2)
integration_test(multisample_1)
integration_test(vertexbuffer_1)
integration_test(khr_debug)
integration_test(extension_pack_es31a)
integration_test(copy_image_1)
integration_test(geometry_shader_1)
integration_test(khr_blend_equation_advanced)
integration_test(oes_sample_shading)
integration_test(ext_texture_border_clamp)
integration_test(oes_texture_stencil8)
integration_test(ext_gpu_shader5)
integration_test(ext_texture_sRGB_decode)
integration_test(ext_texture_buffer)
integration_test(ext_texture_cube_map_array)

install(PROGRAMS
	${SRC_ROOT}/integration_tests/fbdev_test.sh
	${SRC_ROOT}/integration_tests/x11_test.sh
	DESTINATION tests)

add_custom_command(
    OUTPUT ${SRC_ROOT}/specs/stub_gles2.cpp
    COMMAND ${SRC_ROOT}/specs/glxml_to_gles2_stubs
    DEPENDS
        ${THIRDPARTY_INCLUDE_DIRS}/opengl-registry/xml/gl.xml
        ${SRC_ROOT}/specs/glxml_to_gles2_stubs.py
    WORKING_DIRECTORY ${SRC_ROOT}/specs
)
add_custom_target(glxml_to_gles2_stubs DEPENDS ${SRC_ROOT}/specs/stub_gles2.cpp)
add_library(gles2_stub SHARED ${SRC_ROOT}/specs/stub_gles2.cpp)
install(TARGETS gles2_stub DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_custom_command(
    OUTPUT ${SRC_ROOT}/specs/stub_gles1.cpp
    COMMAND ${SRC_ROOT}/specs/glxml_to_gles1_stubs
    DEPENDS
        ${THIRDPARTY_INCLUDE_DIRS}/opengl-registry/xml/gl.xml
        ${SRC_ROOT}/specs/glxml_to_gles1_stubs.py
    WORKING_DIRECTORY ${SRC_ROOT}/specs
)
add_custom_target(glxml_to_gles1_stubs DEPENDS ${SRC_ROOT}/specs/stub_gles1.cpp)
add_library(gles1_stub SHARED ${SRC_ROOT}/specs/stub_gles1.cpp)
install(TARGETS gles1_stub DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_custom_command(
    OUTPUT ${SRC_ROOT}/specs/stub_egl.cpp
    COMMAND ${SRC_ROOT}/specs/glxml_to_egl_stubs
    DEPENDS
        ${THIRDPARTY_INCLUDE_DIRS}/opengl-registry/xml/gl.xml
        ${SRC_ROOT}/specs/glxml_to_egl_stubs.py
    WORKING_DIRECTORY ${SRC_ROOT}/specs
)
add_custom_target(glxml_to_egl_stubs DEPENDS ${SRC_ROOT}/specs/stub_egl.cpp)
add_library(egl_stub SHARED ${SRC_ROOT}/specs/stub_egl.cpp)
install(TARGETS egl_stub DESTINATION ${CMAKE_INSTALL_LIBDIR})

