#!/bin/bash

# Run integration tests on fbdev platform.

set -e

rm -f testfiles/*
rm -rf png1 png2

mkdir -p testfiles
DDK_PATH=${DDK_PATH:-"/scratch/pr368/drivers/midgard/5420_t62x_r0p1/trunk_4aa1dd8_f42126/fbdev_instr1_debug1_cl0"}
PATRACE_ROOT=${PARETRACE_PATH:-"/scratch/permat01"}
PATRACE_LIB="${PATRACE_ROOT}/lib"
PATRACE_EXE="${PATRACE_ROOT}/bin"

# --- STEP 1 : Run normally

function run() {
	echo
	echo "-- running $1 --"
	LD_LIBRARY_PATH=$DDK_PATH:. ./$1
}

run compute_1
run compute_2
run compute_3
run programs_1
run imagetex_1
run indirectdraw_1
run indirectdraw_2
run multisample_1
run vertexbuffer_1
run bindbufferrange_1
run dummy_1
#run geometry_shader_1
run khr_debug
run copy_image_1
run ext_texture_border_clamp
run ext_texture_buffer
run khr_blend_equation_advanced
run oes_sample_shading
run oes_texture_stencil8

# --- STEP 2 : Trace

function trace() {
	echo
	echo "-- tracing $1 --"
	OUT_TRACE_FILE=testfiles/$1 LD_PRELOAD=$PATRACE_LIB/libegltrace.so LD_LIBRARY_PATH=$DDK_PATH:. INTERCEPTOR_LIB=$PATRACE_LIB/libegltrace.so TRACE_LIBEGL=$DDK_PATH/libEGL.so TRACE_LIBGLES1=$DDK_PATH/libGLESv1_CM.so TRACE_LIBGLES2=$DDK_PATH/libGLESv2.so ./$1
}

trace compute_1
trace compute_2
trace compute_3
trace programs_1
trace imagetex_1
trace indirectdraw_1
trace indirectdraw_2
trace multisample_1
trace vertexbuffer_1
trace bindbufferrange_1
trace dummy_1
#trace geometry_shader_1
trace khr_debug
trace copy_image_1
trace ext_texture_border_clamp
trace ext_texture_buffer
trace khr_blend_equation_advanced
trace oes_sample_shading
trace oes_texture_stencil8

# --- STEP 3 : Retrace

function replay() {
	echo
	echo "-- replaying $1 --"
	LD_LIBRARY_PATH=$DDK_PATH:. ${PATRACE_EXE}/paretrace -s */frame -debug testfiles/$1.1.pat
}

replay compute_1
replay compute_2
replay compute_3
replay programs_1
replay imagetex_1
replay indirectdraw_1
replay indirectdraw_2
replay multisample_1
replay vertexbuffer_1
replay bindbufferrange_1
replay dummy_1
#replay geometry_shader_1
replay khr_debug
replay copy_image_1
replay ext_texture_border_clamp
replay ext_texture_buffer
replay khr_blend_equation_advanced
replay oes_sample_shading
replay oes_texture_stencil8

mkdir -p png1
mv *.png png1

# play image generating tests again to compare
replay indirectdraw_1
replay indirectdraw_2
replay multisample_1
replay vertexbuffer_1
#replay geometry_shader_1
replay copy_image_1
replay ext_texture_border_clamp
replay ext_texture_buffer
replay khr_blend_equation_advanced
replay oes_sample_shading
replay oes_texture_stencil8

mkdir -p png2
mv *.png png2

# will warn if tests do not generate deterministic results -- image based
( cd png2 ; find . -type f -exec cmp {} ../png1/{} ';' )

# will warn if tests do not generate deterministic results -- state based
( cd testfiles ; find . -name \*.retracelog -type f -exec bash -c 'diff -u ${0} ${0/retracelog/tracelog}' {} ';' )

rm -f mali_this_file_is_for_testing_purposes_only.out mali.*
