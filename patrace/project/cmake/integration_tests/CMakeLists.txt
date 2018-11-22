include_directories(${SRC_ROOT}/integration_tests ${SRC_ROOT}/specs ${THIRDPARTY_INCLUDE_DIRS}/paframework/common ${SRC_ROOT} ${SRC_ROOT}/dispatch ${SRC_ROOT}/common)

add_custom_command(
    OUTPUT ${SRC_ROOT}/specs/pa_gl31.cpp ${SRC_ROOT}/specs/pa_gl31.h
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/specs/glxml_to_funcs.py
    DEPENDS
        ${THIRDPARTY_INCLUDE_DIRS}/opengl-registry/xml/gl.xml
        ${SRC_ROOT}/specs/glxml_to_funcs.py
    WORKING_DIRECTORY ${SRC_ROOT}/specs
)
add_custom_target(glxml_to_funcs DEPENDS ${SRC_ROOT}/specs/pa_gl31.cpp)

set(IT_LIBS -Wl,--add-needed paframework -L${TEST_LIBRARY} ${TEST_LIBRARY}/libEGL.so ${TEST_LIBRARY}/libGLESv2.so)
if (WINDOWSYSTEM MATCHES "x11")
	set(IT_LIBS ${IT_LIBS} X11)
endif ()
set(IT_CFLAGS "-std=gnu++0x -Wall")
set(IT_LFLAGS "${FBDEV_EXTRA_OPTS} -L${CMAKE_CURRENT_BINARY_DIR}/../paframework/lib -L${CMAKE_CURRENT_BINARY_DIR}/integration_tests/tests -L${TEST_LIBRARY}")
set(IT_DEPS glxml_to_funcs paframework)
set(IT_FILES ${SRC_ROOT}/integration_tests/pa_demo.cpp ${SRC_ROOT}/specs/pa_gl31.cpp)

add_executable(dummy_1 ${SRC_ROOT}/integration_tests/dummy_1.cpp ${IT_FILES})
target_link_libraries(dummy_1 ${IT_LIBS})
set_target_properties(dummy_1 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(dummy_1 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
add_dependencies(dummy_1 ${IT_DEPS})
install(TARGETS dummy_1 DESTINATION tests)

add_executable(bindbufferrange_1 ${SRC_ROOT}/integration_tests/bindbufferrange_1.cpp ${IT_FILES})
target_link_libraries(bindbufferrange_1 ${IT_LIBS})
set_target_properties(bindbufferrange_1 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(bindbufferrange_1 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
add_dependencies(bindbufferrange_1 ${IT_DEPS})
install(TARGETS bindbufferrange_1 DESTINATION tests)

add_executable(compute_1 ${SRC_ROOT}/integration_tests/compute_1.cpp ${IT_FILES})
target_link_libraries(compute_1 ${IT_LIBS})
set_target_properties(compute_1 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(compute_1 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
add_dependencies(compute_1 ${IT_DEPS})
install(TARGETS compute_1 DESTINATION tests)

add_executable(compute_2 ${SRC_ROOT}/integration_tests/compute_2.cpp ${IT_FILES})
target_link_libraries(compute_2 ${IT_LIBS})
set_target_properties(compute_2 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(compute_2 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
add_dependencies(compute_2 ${IT_DEPS})
install(TARGETS compute_2 DESTINATION tests)

add_executable(compute_3 ${SRC_ROOT}/integration_tests/compute_3.cpp ${IT_FILES})
target_link_libraries(compute_3 ${IT_LIBS})
set_target_properties(compute_3 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(compute_3 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
add_dependencies(compute_3 ${IT_DEPS})
install(TARGETS compute_3 DESTINATION tests)

add_executable(programs_1 ${SRC_ROOT}/integration_tests/programs_1.cpp ${IT_FILES})
target_link_libraries(programs_1 ${IT_LIBS})
set_target_properties(programs_1 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(programs_1 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
add_dependencies(programs_1 ${IT_DEPS})
install(TARGETS programs_1 DESTINATION tests)

add_executable(imagetex_1 ${SRC_ROOT}/integration_tests/imagetex_1.cpp ${IT_FILES})
target_link_libraries(imagetex_1 ${IT_LIBS})
set_target_properties(imagetex_1 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(imagetex_1 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
add_dependencies(imagetex_1 ${IT_DEPS})
install(TARGETS imagetex_1 DESTINATION tests)

add_executable(indirectdraw_1 ${SRC_ROOT}/integration_tests/indirectdraw_1.cpp ${IT_FILES})
target_link_libraries(indirectdraw_1 ${IT_LIBS})
set_target_properties(indirectdraw_1 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(indirectdraw_1 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
add_dependencies(indirectdraw_1 ${IT_DEPS})
install(TARGETS indirectdraw_1 DESTINATION tests)

add_executable(indirectdraw_2 ${SRC_ROOT}/integration_tests/indirectdraw_2.cpp ${IT_FILES})
target_link_libraries(indirectdraw_2 ${IT_LIBS})
set_target_properties(indirectdraw_2 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(indirectdraw_2 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
add_dependencies(indirectdraw_2 ${IT_DEPS})
install(TARGETS indirectdraw_2 DESTINATION tests)

add_executable(directdraw_1 ${SRC_ROOT}/integration_tests/directdraw_1.cpp ${IT_FILES})
target_link_libraries(directdraw_1 ${IT_LIBS})
set_target_properties(directdraw_1 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(directdraw_1 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
add_dependencies(directdraw_1 ${IT_DEPS})
install(TARGETS directdraw_1 DESTINATION tests)

add_executable(directdraw_2 ${SRC_ROOT}/integration_tests/directdraw_2.cpp ${IT_FILES})
target_link_libraries(directdraw_2 ${IT_LIBS})
set_target_properties(directdraw_2 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(directdraw_2 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
add_dependencies(directdraw_2 ${IT_DEPS})
install(TARGETS directdraw_2 DESTINATION tests)

add_executable(multisample_1 ${SRC_ROOT}/integration_tests/multisample_1.cpp ${IT_FILES})
target_link_libraries(multisample_1 ${IT_LIBS})
set_target_properties(multisample_1 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(multisample_1 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
add_dependencies(multisample_1 ${IT_DEPS})
install(TARGETS multisample_1 DESTINATION tests)

add_executable(vertexbuffer_1 ${SRC_ROOT}/integration_tests/vertexbuffer_1.cpp ${IT_FILES})
target_link_libraries(vertexbuffer_1 ${IT_LIBS})
set_target_properties(vertexbuffer_1 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(vertexbuffer_1 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
install(TARGETS vertexbuffer_1 DESTINATION tests)
add_dependencies(vertexbuffer_1 ${IT_DEPS})

add_executable(khr_debug ${SRC_ROOT}/integration_tests/khr_debug.cpp ${IT_FILES})
target_link_libraries(khr_debug ${IT_LIBS})
set_target_properties(khr_debug PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(khr_debug PROPERTIES LINK_FLAGS ${IT_LFLAGS})
install(TARGETS khr_debug DESTINATION tests)
add_dependencies(khr_debug ${IT_DEPS})

add_executable(extension_pack ${SRC_ROOT}/integration_tests/extension_pack_es31a.cpp ${IT_FILES})
target_link_libraries(extension_pack ${IT_LIBS})
set_target_properties(extension_pack PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(extension_pack PROPERTIES LINK_FLAGS ${IT_LFLAGS})
install(TARGETS extension_pack DESTINATION tests)
add_dependencies(extension_pack ${IT_DEPS})

add_executable(copy_image_1 ${SRC_ROOT}/integration_tests/copy_image_1.cpp ${IT_FILES})
target_link_libraries(copy_image_1 ${IT_LIBS})
set_target_properties(copy_image_1 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(copy_image_1 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
install(TARGETS copy_image_1 DESTINATION tests)
add_dependencies(copy_image_1 ${IT_DEPS})

add_executable(geometry_shader_1 ${SRC_ROOT}/integration_tests/geometry_shader_1.cpp ${IT_FILES})
target_link_libraries(geometry_shader_1 ${IT_LIBS})
set_target_properties(geometry_shader_1 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(geometry_shader_1 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
install(TARGETS geometry_shader_1 DESTINATION tests)
add_dependencies(geometry_shader_1 ${IT_DEPS})

add_executable(khr_blend_equation_advanced ${SRC_ROOT}/integration_tests/khr_blend_equation_advanced.cpp ${IT_FILES})
target_link_libraries(khr_blend_equation_advanced ${IT_LIBS})
set_target_properties(khr_blend_equation_advanced PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(khr_blend_equation_advanced PROPERTIES LINK_FLAGS ${IT_LFLAGS})
install(TARGETS khr_blend_equation_advanced DESTINATION tests)
add_dependencies(khr_blend_equation_advanced ${IT_DEPS})

add_executable(oes_sample_shading ${SRC_ROOT}/integration_tests/oes_sample_shading.cpp ${IT_FILES})
target_link_libraries(oes_sample_shading ${IT_LIBS})
set_target_properties(oes_sample_shading PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(oes_sample_shading PROPERTIES LINK_FLAGS ${IT_LFLAGS})
install(TARGETS oes_sample_shading DESTINATION tests)
add_dependencies(oes_sample_shading ${IT_DEPS})

add_executable(ext_texture_border_clamp ${SRC_ROOT}/integration_tests/ext_texture_border_clamp.cpp ${IT_FILES})
target_link_libraries(ext_texture_border_clamp ${IT_LIBS})
set_target_properties(ext_texture_border_clamp PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(ext_texture_border_clamp PROPERTIES LINK_FLAGS ${IT_LFLAGS})
install(TARGETS ext_texture_border_clamp DESTINATION tests)
add_dependencies(ext_texture_border_clamp ${IT_DEPS})

add_executable(oes_texture_stencil8 ${SRC_ROOT}/integration_tests/oes_texture_stencil8.cpp ${IT_FILES})
target_link_libraries(oes_texture_stencil8 ${IT_LIBS})
set_target_properties(oes_texture_stencil8 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(oes_texture_stencil8 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
install(TARGETS oes_texture_stencil8 DESTINATION tests)
add_dependencies(oes_texture_stencil8 ${IT_DEPS})

add_executable(ext_gpu_shader5 ${SRC_ROOT}/integration_tests/ext_gpu_shader5.cpp ${IT_FILES})
target_link_libraries(ext_gpu_shader5 ${IT_LIBS})
set_target_properties(ext_gpu_shader5 PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(ext_gpu_shader5 PROPERTIES LINK_FLAGS ${IT_LFLAGS})
install(TARGETS ext_gpu_shader5 DESTINATION tests)
add_dependencies(ext_gpu_shader5 ${IT_DEPS})

add_executable(ext_texture_sRGB_decode ${SRC_ROOT}/integration_tests/ext_texture_sRGB_decode.cpp ${IT_FILES})
target_link_libraries(ext_texture_sRGB_decode ${IT_LIBS})
set_target_properties(ext_texture_sRGB_decode PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(ext_texture_sRGB_decode PROPERTIES LINK_FLAGS ${IT_LFLAGS})
install(TARGETS ext_texture_sRGB_decode DESTINATION tests)
add_dependencies(ext_texture_sRGB_decode ${IT_DEPS})

add_executable(ext_texture_buffer ${SRC_ROOT}/integration_tests/ext_texture_buffer.cpp ${IT_FILES})
target_link_libraries(ext_texture_buffer ${IT_LIBS})
set_target_properties(ext_texture_buffer PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(ext_texture_buffer PROPERTIES LINK_FLAGS ${IT_LFLAGS})
install(TARGETS ext_texture_buffer DESTINATION tests)
add_dependencies(ext_texture_buffer ${IT_DEPS})

add_executable(ext_texture_cube_map_array ${SRC_ROOT}/integration_tests/ext_texture_cube_map_array.cpp ${IT_FILES})
target_link_libraries(ext_texture_cube_map_array ${IT_LIBS})
set_target_properties(ext_texture_cube_map_array PROPERTIES COMPILE_FLAGS ${IT_CFLAGS})
set_target_properties(ext_texture_cube_map_array PROPERTIES LINK_FLAGS ${IT_LFLAGS})
install(TARGETS ext_texture_cube_map_array DESTINATION tests)
add_dependencies(ext_texture_cube_map_array ${IT_DEPS})

install(PROGRAMS
	${SRC_ROOT}/integration_tests/android_test.sh
	${SRC_ROOT}/integration_tests/fbdev_test.sh
	${SRC_ROOT}/integration_tests/x11_test.sh
	DESTINATION tests)

add_custom_command(
    OUTPUT ${SRC_ROOT}/specs/stub_gles2.cpp
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/specs/glxml_to_gles2_stubs.py
    DEPENDS
        ${THIRDPARTY_INCLUDE_DIRS}/opengl-registry/xml/gl.xml
        ${SRC_ROOT}/specs/glxml_to_gles2_stubs.py
    WORKING_DIRECTORY ${SRC_ROOT}/specs
)
add_custom_target(glxml_to_gles2_stubs DEPENDS ${SRC_ROOT}/specs/stub_gles2.cpp)
add_library(gles2_stub SHARED ${SRC_ROOT}/specs/stub_gles2.cpp)
install(TARGETS gles2_stub DESTINATION lib)

add_custom_command(
    OUTPUT ${SRC_ROOT}/specs/stub_gles1.cpp
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/specs/glxml_to_gles1_stubs.py
    DEPENDS
        ${THIRDPARTY_INCLUDE_DIRS}/opengl-registry/xml/gl.xml
        ${SRC_ROOT}/specs/glxml_to_gles1_stubs.py
    WORKING_DIRECTORY ${SRC_ROOT}/specs
)
add_custom_target(glxml_to_gles1_stubs DEPENDS ${SRC_ROOT}/specs/stub_gles1.cpp)
add_library(gles1_stub SHARED ${SRC_ROOT}/specs/stub_gles1.cpp)
install(TARGETS gles1_stub DESTINATION lib)

add_custom_command(
    OUTPUT ${SRC_ROOT}/specs/stub_egl.cpp
    COMMAND ${PYTHON_EXECUTABLE} ${SRC_ROOT}/specs/glxml_to_egl_stubs.py
    DEPENDS
        ${THIRDPARTY_INCLUDE_DIRS}/opengl-registry/xml/gl.xml
        ${SRC_ROOT}/specs/glxml_to_egl_stubs.py
    WORKING_DIRECTORY ${SRC_ROOT}/specs
)
add_custom_target(glxml_to_egl_stubs DEPENDS ${SRC_ROOT}/specs/stub_egl.cpp)
add_library(egl_stub SHARED ${SRC_ROOT}/specs/stub_egl.cpp)
install(TARGETS egl_stub DESTINATION lib)

