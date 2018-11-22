#!/bin/bash

# Run integration tests on android platform.

# Set $DEVICE (eg '-s 01234456789ABCDEF') on calling this script if multiple android units are connected.
# Set $PATRACE_INSTALL to install from some other location.

INSTALL_ROOT=${PATRACE_INSTALL:-"../../../install"}

mkdir -p results
rm -f results/*

function install()
{
	adb ${DEVICE} uninstall com.arm.pa.$1
	adb ${DEVICE} install -r ${INSTALL_ROOT}/patrace/android/release/integration_tests/app/$1/$1-release.apk
	adb ${DEVICE} shell mkdir -p /data/apitrace/com.arm.pa.$1
	adb ${DEVICE} shell chmod 777 /data/apitrace/com.arm.pa.$1
}

function run()
{
	adb ${DEVICE} logcat -c
	adb ${DEVICE} shell am start -n com.arm.pa.$1/android.app.NativeActivity
	adb ${DEVICE} logcat -d *:E,F
	sleep 3
	adb ${DEVICE} shell am force-stop com.arm.pa.$1
}

function trace()
{
	# the two lines below are for testing apitrace-like wrapping
	TNAME=`echo wrap.com.arm.pa.$1 | cut -c 1-31` # truncated for android property limits
	#adb ${DEVICE} shell setprop $TNAME LD_PRELOAD=/system/lib/egl/libinterceptor_patrace_arm.so

	adb ${DEVICE} shell rm -f /data/apitrace/com.arm.pa.$1/*
	adb ${DEVICE} logcat -c
	adb ${DEVICE} shell "echo com.arm.pa.$1 > /system/lib/egl/appList.cfg"
	adb ${DEVICE} shell am start -n com.arm.pa.$1/android.app.NativeActivity
	adb ${DEVICE} logcat -d *:E,F
	sleep 3
	adb ${DEVICE} shell am kill com.arm.pa.$1
	sleep 2
	adb ${DEVICE} shell am force-stop com.arm.pa.$1
	adb ${DEVICE} shell chmod 777 /data/apitrace/com.arm.pa.$1/com.arm.pa.$1.1.pat
	adb ${DEVICE} pull /data/apitrace/com.arm.pa.$1/com.arm.pa.$1.1.pat results/

	adb ${DEVICE} shell setprop $TNAME ""
}

function replay()
{
	adb ${DEVICE} logcat -c
	adb ${DEVICE} shell am start -n com.arm.pa.paretrace/.Activities.RetraceActivity --ez debug true --es fileName /data/apitrace/com.arm.pa.$1/com.arm.pa.$1.1.pat
	adb ${DEVICE} logcat -d *:E,F
	sleep 2
	adb ${DEVICE} shell am kill com.arm.pa.paretrace
	sleep 1
}

TESTS="dummy_1 compute_1 compute_2 compute_3 programs_1 indirectdraw_1 indirectdraw_2 vertexbuffer_1 multisample_1 imagetex_1 bindbufferrange_1"

for test in ${TESTS}; do
	echo "Installing $test"
	install $test
done

for test in ${TESTS}; do
	echo "Running $test"
	run $test
done

for test in ${TESTS}; do
	echo "Tracing $test"
	trace $test
done

for test in ${TESTS}; do
	echo "Replaying $test"
	replay $test
done
