#!/bin/bash

### Run release integration testing
#

set -e

### Sanity checking the test
#

if [ "$1" == "" ]; then
	echo "Usage: $0 <null driver path>"
	exit 1
fi
if [ "$(pwd|sed 's/.*scripts.*/scripts/')" == "scripts" ]; then
	echo "Must be run from source root directory"
	exit 1
fi

### Defines and functions
#

OUT=tmp/testfiles
DDK_PATH=$1
FB_PATRACE_ROOT="$(pwd)/install/patrace/fbdev_x64/sanitizer"
FB_PATRACE_LIB="${FB_PATRACE_ROOT}/lib"
FB_PATRACE_BIN="${FB_PATRACE_ROOT}/bin"
FB_PATRACE_TEST="${FB_PATRACE_ROOT}/tests"
X11_PATRACE_ROOT="$(pwd)/install/patrace/x11_x64/sanitizer"
X11_PATRACE_BIN="${X11_PATRACE_ROOT}/bin"
TESTTRACE1=$OUT/indirectdraw_1.1.pat
TESTTRACE2=$OUT/khr_blend_equation_advanced.1.pat
TESTTRACE3=$OUT/geometry_shader_1.1.pat
ASANLIB=$(ldd ${FB_PATRACE_BIN}/paretrace | grep libasan | sed 's#.* \(/usr.*\) .*#\1#')
export LSAN_OPTIONS="detect_leaks=0"
export ASAN_OPTIONS="symbolize=1,abort_on_error=1"

function build_test() {
	echo "*** Testing BUILDING"
	NO_PYTHON_BUILD=y scripts/build.py patrace x11_x64 debug
	NO_PYTHON_BUILD=y scripts/build.py patrace x11_x64 release
	NO_PYTHON_BUILD=y scripts/build.py patrace x11_x64 sanitizer
	NO_PYTHON_BUILD=y scripts/build.py patrace fbdev_aarch64 release
	NO_PYTHON_BUILD=y scripts/build.py patrace fbdev_x64 sanitizer
	NO_PYTHON_BUILD=y scripts/build.py patrace x11_x32 release
	NO_PYTHON_BUILD=y scripts/build.py patrace wayland_x64 debug
}

function run() {
	echo
	echo "-- running $1 --"
	set -x
	PADEMO_NULL_RUN=1 LD_LIBRARY_PATH=$DDK_PATH:. ${FB_PATRACE_TEST}/$1
	set +x
}

function trace() {
	echo
	echo "-- tracing with null driver: $1 --"
	set -x
	PADEMO_NULL_RUN=1 OUT_TRACE_FILE=$OUT/$1 LD_PRELOAD=${ASANLIB}:${FB_PATRACE_LIB}/libegltrace.so LD_LIBRARY_PATH=$DDK_PATH:. INTERCEPTOR_LIB=$FB_PATRACE_LIB/libegltrace.so TRACE_LIBEGL=$DDK_PATH/libEGL.so TRACE_LIBGLES1=$DDK_PATH/libGLESv1_CM.so TRACE_LIBGLES2=$DDK_PATH/libGLESv2.so ${FB_PATRACE_TEST}/$1
	set +x
}

function nreplay() {
	echo
	echo "-- replaying with null driver: $1 --"
	set -x
	LD_LIBRARY_PATH=$DDK_PATH:. ${FB_PATRACE_BIN}/paretrace $OUT/$1.1.pat
	set +x
}

function replay() {
	echo
	echo "-- replaying with real driver: $1 --"
	set -x
	${X11_PATRACE_BIN}/paretrace -noscreen -snapshotprefix ${1}_ -framenamesnaps -s */frame -overrideEGL 8 8 8 8 24 8 $OUT/$1.1.pat
	set +x
}

function integration_test() {
	echo "*** Testing INTEGRATION TESTS"

	rm -rf tmp/png1 tmp/png2
	rm -f tmp/testfiles/*
	mkdir -p $OUT

	# --- STEP 1 : Run normally
	run dummy_1
	run extension_pack
	run drawrange_1
	run drawrange_2
	run compute_1
	run compute_2
	run compute_3
	#run programs_1
	run imagetex_1
	run indirectdraw_1
	run indirectdraw_2
	#run multisample_1
	run vertexbuffer_1
	run bindbufferrange_1
	run geometry_shader_1
	run khr_debug
	run copy_image_1
	run ext_texture_border_clamp
	run ext_texture_buffer
	run ext_texture_cube_map_array
	run ext_texture_sRGB_decode
	run khr_blend_equation_advanced
	run oes_sample_shading
	run oes_texture_stencil8
	run ext_gpu_shader5

	# --- STEP 2 : Trace
	trace dummy_1
	trace drawrange_1
	trace drawrange_2
	trace compute_1
	trace compute_2
	trace compute_3
	#trace programs_1
	trace imagetex_1
	trace indirectdraw_1
	trace indirectdraw_2
	#trace multisample_1
	trace vertexbuffer_1
	trace bindbufferrange_1
	trace geometry_shader_1
	trace khr_debug
	trace copy_image_1
	trace ext_texture_border_clamp
	trace ext_texture_buffer
	trace ext_texture_cube_map_array
	trace ext_texture_sRGB_decode
	trace khr_blend_equation_advanced
	trace oes_sample_shading
	trace oes_texture_stencil8
	trace ext_gpu_shader5

	# --- STEP 3 : Retrace with null driver
	nreplay dummy_1
	nreplay drawrange_1
	nreplay drawrange_2
	nreplay compute_1
	nreplay compute_2
	nreplay compute_3
	#nreplay programs_1
	nreplay imagetex_1
	nreplay indirectdraw_1
	nreplay indirectdraw_2
	#nreplay multisample_1
	nreplay vertexbuffer_1
	nreplay bindbufferrange_1
	nreplay geometry_shader_1
	nreplay khr_debug
	nreplay copy_image_1
	nreplay ext_texture_border_clamp
	nreplay ext_texture_buffer
	nreplay ext_texture_cube_map_array
	nreplay ext_texture_sRGB_decode
	nreplay khr_blend_equation_advanced
	nreplay oes_sample_shading
	nreplay oes_texture_stencil8
	nreplay ext_gpu_shader5

	rm -f *.png
	mkdir -p tmp/png1
	mkdir -p tmp/png2

	# --- STEP 3 : Retrace with real driver
	replay dummy_1
	replay drawrange_1
	replay drawrange_2
	replay compute_1
	replay compute_2
	replay compute_3
	#replay programs_1
	replay imagetex_1
	replay indirectdraw_1
	replay indirectdraw_2
	#replay multisample_1
	replay vertexbuffer_1
	replay bindbufferrange_1
	replay geometry_shader_1
	replay khr_debug
	replay copy_image_1
	replay ext_texture_border_clamp
	replay ext_texture_buffer
	replay ext_texture_cube_map_array
	replay ext_texture_sRGB_decode
	replay khr_blend_equation_advanced
	replay oes_sample_shading
	replay oes_texture_stencil8
	replay ext_gpu_shader5

	mv *.png tmp/png1

	# play image generating tests again to compare
	replay drawrange_1
	replay drawrange_2
	replay indirectdraw_1
	replay indirectdraw_2
	#replay multisample_1
	replay vertexbuffer_1
	replay geometry_shader_1
	replay copy_image_1
	replay ext_texture_border_clamp
	replay ext_texture_buffer
	replay ext_texture_cube_map_array
	replay ext_texture_sRGB_decode
	replay khr_blend_equation_advanced
	replay oes_sample_shading
	replay oes_texture_stencil8
	replay ext_gpu_shader5

	mv *.png tmp/png2

	# will warn if tests do not generate deterministic results -- image based
	( cd tmp/png2 ; find . -type f -exec cmp {} ../png1/{} ';' )

	rm -f mali_this_file_is_for_testing_purposes_only.out mali.*
}

function replayer_test() {
	echo "*** Testing REPLAYER FEATURES"

	set -x
	${X11_PATRACE_BIN}/paretrace -overrideEGL 8 8 8 8 24 8 -noscreen $TESTTRACE2
	LD_LIBRARY_PATH=$DDK_PATH:. ${FB_PATRACE_BIN}/paretrace -offscreen $TESTTRACE2
	${X11_PATRACE_BIN}/paretrace -preload 2 7 -noscreen $TESTTRACE2
	${X11_PATRACE_BIN}/paretrace -overrideEGL 8 8 8 8 24 8 -framerange 0 10 -noscreen $TESTTRACE2
	${X11_PATRACE_BIN}/paretrace -infojson $TESTTRACE2
	${X11_PATRACE_BIN}/paretrace -info $TESTTRACE2
	${X11_PATRACE_BIN}/paretrace -overrideEGL 8 8 8 8 24 8 -noscreen -debug -framerange 3 9 -flush -collect $TESTTRACE3
	set +x
}

function az_subtest() {
	set -x
	${X11_PATRACE_ROOT}/tools/analyze_trace -n -o tmp/$1 -r 1 $OUT/$1.1.pat
	${X11_PATRACE_ROOT}/tools/trace_to_txt $OUT/$1.1.pat tmp/$1.txt
	scripts/validate_analysis.py tmp/$1 0

	${X11_PATRACE_ROOT}/tools/analyze_trace -n -o tmp/$1 -r 1 -f 0 2 -j $OUT/$1.1.pat
	scripts/renderpass_json/validate.py tmp/$1_f1_rp1
	set +x
}

function tools_test() {
	rm -rf aztmp* *.ra

	echo "*** Testing TOOLS"
	echo "Testing shader analyzer"
	set -x
	${X11_PATRACE_BIN}/drawstate --version
	${X11_PATRACE_ROOT}/tools/shader_analyzer -v
	${X11_PATRACE_ROOT}/tools/shader_analyzer --selftest
	${X11_PATRACE_ROOT}/bin/drawstate --overrideEGL 8 8 8 8 24 8 --noscreen $TESTTRACE3 69
	${X11_PATRACE_ROOT}/tools/shader_analyzer --test geometry_shader_1.1_f2_c69/shader_2.vert
	${X11_PATRACE_ROOT}/tools/shader_analyzer --test geometry_shader_1.1_f2_c69/shader_3.frag
	${X11_PATRACE_ROOT}/tools/shader_analyzer --test geometry_shader_1.1_f2_c69/shader_4.geom
	${X11_PATRACE_ROOT}/bin/drawstate --noscreen $TESTTRACE1 67
	${X11_PATRACE_ROOT}/tools/shader_analyzer --test indirectdraw_1.1_f3_c67/shader_2.vert
	${X11_PATRACE_ROOT}/tools/shader_analyzer --test indirectdraw_1.1_f3_c67/shader_3.frag
	set +x
	rm -rf geometry_shader_1.1_f2_c69
	rm -rf indirectdraw_1.1_f3_c67

	echo "*** Testing DEDUPLICATOR"
	set -x
	${X11_PATRACE_ROOT}/tools/deduplicator --all $TESTTRACE3 tmp/dedup.pat
	set +x
	COUNT=$(${X11_PATRACE_ROOT}/tools/trace_to_txt tmp/dedup.pat 2>&1 | grep glUseProgram | wc -l)
	if [ "$COUNT" != "1" ]; then
		echo "Bad glUseProgram count: $COUNT"
		exit 1
	fi
	${X11_PATRACE_ROOT}/tools/deduplicator --all --replace $TESTTRACE3 tmp/dedup.pat
	COUNT=$(${X11_PATRACE_ROOT}/tools/trace_to_txt tmp/dedup.pat 2>&1 | grep glEnable\(cap=0xFF | wc -l)
	if [ "$COUNT" != "10" ]; then
		echo "Bad glEnable(GL_INVALID_INDEX) count: $COUNT"
		exit 1
	fi
	rm -f *.ra tmp/dedup.pat

	echo "*** Testing FASTFORWARDER"
	set -x
	${X11_PATRACE_BIN}/fastforward --version
	${X11_PATRACE_BIN}/fastforward --noscreen --input $TESTTRACE3 --output ff_f4.pat --targetFrame 4 --endFrame 10
	${X11_PATRACE_BIN}/paretrace -snapshotprefix ff_f4_ -framenamesnaps -s 4/frame -noscreen -overrideEGL 8 8 8 8 24 8 ff_f4.pat
	cmp ff_f4_0004*.png tmp/png1/geometry_shader_1_0004*.png
	set +x

	echo "*** Testing ANALYZE_TRACE"
	az_subtest drawrange_1
	az_subtest drawrange_2
	#az_subtest compute_1
	#az_subtest compute_2
	#az_subtest compute_3
	#az_subtest programs_1
	#az_subtest imagetex_1
	az_subtest indirectdraw_1
	az_subtest indirectdraw_2
	#az_subtest multisample_1
	az_subtest vertexbuffer_1
	#az_subtest bindbufferrange_1
	az_subtest geometry_shader_1
	#az_subtest khr_debug
	az_subtest copy_image_1
	az_subtest ext_texture_border_clamp
	az_subtest ext_texture_buffer
	az_subtest ext_texture_cube_map_array
	az_subtest ext_texture_sRGB_decode
	az_subtest khr_blend_equation_advanced
	az_subtest oes_sample_shading
	az_subtest oes_texture_stencil8
	az_subtest ext_gpu_shader5
}

### Run all the tests
#

build_test
integration_test
replayer_test
tools_test

echo
echo "*** ALL DONE - all tests passed ***"
echo

# Cleanup...
rm -f *.ra
