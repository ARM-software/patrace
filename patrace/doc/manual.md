PATrace
=======

PATrace is software for capturing GLES calls of an application and replaying them on a different device, keeping the GPU workload the same. It's similar to the open source APITrace project, but optimised for performance measurements. This means it tries hard to make the GPU the bottleneck and has low CPU usage. It achieves this by using a compact format for trace files and by allowing preloading, which means parsing and loading the trace data into RAM beforehand, then capturing the performance data while replaying the GLES calls from RAM

Installation
------------

You can build the latest trunk from source. First check it out, then make sure you have fetched all submdules:

    git submodule update --recursive --init

**Note**: master branch should not be used in production systems. Trace files produced with the trunk build might be incompatible in the future. Trace files produced by release builds will always be supported in future releases.

### Building for desktop Linux

Building for desktop Linux is useful if you want to replay trace files on your desktop using the GLES emulator or if you want to inspect the trace files using TraceView, which is a trace browsing GUI. Install these build dependencies:

    apt install build-essential cmake libx11-dev libtiff-dev default-jdk g++-multilib gcc-multilib libhdf5-dev libpq-dev libpq5 subversion qt5-default python3-setuptools python3-dev python3-pip python3-venv python3-wheel swig

Then build with the build.py script:

    ./scripts/build.py patrace x11_x64 release

If you need to run patrace under a `no_mali=1` Mali DDK (this is only available to ARM and ARM Mali partners), then you need to build for fbdev instead:

    scripts/build.py patrace fbdev_x64 release

If you need patrace python tools, you can install them like this:

    cd patrace/python
    pip install ./pypatrace/dist/pypatrace-*.whl
    pip install ./patracetools/dist/patracetools-*.whl

We recommend using and installing these packages inside a python **virtualenv**.

### Building for ARM Linux fbdev

For running on most ARMv7 embedded devices, such as Arndale or Firefly:

    ./scripts/build.py patrace fbdev_arm_hardfloat release

For running on ARMv8 embedded devices, such as most Juno board configurations:

    ./scripts/build.py patrace fbdev_aarch64 release

For embedded devices that do not use hardfloat (rare these days):

    ./scripts/build.py patrace fbdev_arm release

To link libgcc and libstdc++ statically into patrace:
    ./scripts/build.py patrace fbdev_aarch64 release --static true
    ./scripts/build.py patrace fbdev_arm_hardfloat --static true

### Building for Android (Not supported since r3p1)

Build dependencies: You must install the Android SDK and NDK with support for the **android** command as well as the **ndk-build** command (deprecated in later SDK versions). You can get the SDK tools at https://dl.google.com/android/adt/adt-bundle-linux-x86_64-20140702.zip and the NDK at https://dl.google.com/android/repository/android-ndk-r13b-linux-x86_64.zip. Add the NDK installation dir to your path environment variable, or to the NDK environment variable. Add the tools and platform-tools folders located in the android SDK installation folder to your path.

The SDK you just downloaded supports Android-20 out-of-the-box, but this has been seen to cause issues. You should therefore upgrade the SDK to Android-23. To do this, enter the *<path to SDK download>/sdk/tools* folder and run *./android*. This will open up an SDK manager. Select the Android-23 SDK and hit install.

Make sure you have java and ant installed:

    apt install openjdk-7-jdk ant

Note:
Ubuntu 18.04 does not provide openjdk-7-jdk, it provides openjdk-11-jdk and openjdk-8-jdk instead. In this case, version 8 should be used as version 11 will cause compilation errors.

You may also have to install these (if it complains about missing 'aapt' program):

    apt install lib32stdc++6 lib32z1

Building:

    ./scripts/build.py patrace android release

If you have a different android target installed than what the build script expects, then you will get build errors. To fix this, grep for the `ANDROID_TARGET` variable in the ./scripts directory and change it to your android target.


### Building for Android with Gradle (since r3p1)

Build dependencies: You must install the Android SDK and NDK with Android Studio or Android command line tools, you can get them at https://developer.android.com/studio.

If you use Android Studio, run bin/studio.sh, follow the setup wizard to install Android SDK and SDK Platform.
If you've already install Android Studio, follow these steps:
1. In Android Studio, open a project, click **File > Settings**, then navigate to **Appearance & Behavior > System Settings > Android SDK**.
2. Under **SDK Platforms** tab, select **Android 11.0 (R) (API Level 30)**.
3. Under **SDK Tools** tab, select **Android SDK Build-Tools** and **Android SDK Platform-Tools**.
4. Click **Apply > OK > Accept > Next**. When installation is complete, Click **Finish**.

To install NDK, follow the **step 1** above first, click the **SDK Tools** tab and select **Show Package Details** to display all the version available of the packages. Then select **21.1.6352462** under **NDK (Side by side)** as your NDK version.


If you do not need Android Studio, you can download the basic Android command line tools. You can use the included sdkmanager to install SDK and NDK packages.

List installed and available packages:

    sdkmanager --sdk_root=<SDK_ROOT> --list

Install the latest platform tools and the SDK tools for API level 30:

    sdkmanager --sdk_root=<SDK_ROOT> "platform-tools" "platforms;android-30" "build-tools;30.0.3"

Install NDK:

    sdkmanager --sdk_root=<SDK_ROOT> --install "ndk;21.1.6352462"

Add the tools and platform-tools folders located in the android SDK installation folder to .bashrc:

    export ANDROID_HOME=<SDK_ROOT>
    export PATH=$PATH:${ANDROID_HOME}/tools
    export PATH=$PATH:${ANDROID_HOME}/platform-tools

Make sure you have java installed:

    apt-get install openjdk-8-jdk

You may also have to install these (if it complains about missing 'aapt' program):

    apt-get install lib32stdc++6 lib32z1

No need to install Gradle as it will be installed automatically when you run build first time.

Building:

    ./scripts/build.py patrace android release


### Build Known Issues

The Python tree can get stale and fail to rebuild. You can clean the Python build parts with:

    git clean -fxd patrace/python

If you have strange errors and wish to reset the build system, try this:

    rm -rf builds/*
    git clean -fxd patrace/python  
    git submodule update --init  

If you do not wish to build the Python code (not doing so speeds up Linux builds considerably), you can do this by setting the environment variable

    NO_PYTHON_BUILD=y

Tracing
-------

### Installing fakedriver on Android

Make sure you have a rooted device where developer mode is enabled and connect it to your desktop. You then need to remount the system directory to install files there.

    adb shell  
    su  
    mount -o rw,remount /system  

When it comes to installing the fakedriver, the method differs depending on which device you have. Android will load the first driver it finds that goes by the name `libGLES_*.so` that resides in

    /system/lib/egl/

for 32-bit and

    /system/lib64/egl/

for 64-bit applications, with a fallback to `/system/vendor/lib[64]/egl`. If a vendor driver resides in this location, move it to the corresponding `/system/vendor/lib[64]/egl` directory to make sure Android loads the fakedriver rather than the system driver.

We have some hardcoded driver names that we check, if these do not work, try renaming the vendor drivers to `wrapped_libGLES.so` and place it in the `/system/vendor/lib[64]/egl` directory. (Note that if you do this, and then remove the fakedriver without renaming the system driver back, the phone will suddenly become unusable!)

By convention, we call our fakedriver `libGLES_wrapper.so`.

Example instructions for **Galaxy S7** (32-bit apps):

    adb push fakedriver/libGLES_wrapper_arm.so /sdcard/  
    adb shell su -c mount -o rw,remount /system  
    adb shell su -c cp /sdcard/libGLES_wrapper_arm.so /system/lib/egl/libGLES.so  
    adb shell su -c rm /sdcard/libGLES_wrapper_arm.so  
    adb shell su -c chmod 755 /system/lib/egl/libGLES.so  
    adb shell su -c mount -o ro,remount /system  

### Installing interceptor on Android

Install the interceptor and configure it:

    adb push install/patrace/android/release/egltrace/libinterceptor_patrace_arm.so /sdcard/  
    adb push install/patrace/android/release/egltrace/libinterceptor_patrace_arm64.so /sdcard/  
    adb shell  
    su  
    mount -o rw,remount /system  

    cp /sdcard/libinterceptor_patrace_arm.so /system/lib/egl/libinterceptor_patrace_arm.so  
    cp /sdcard/libinterceptor_patrace_arm64.so /system/lib64/egl/libinterceptor_patrace_arm64.so  

    chmod 755 /system/lib/egl/libinterceptor_patrace_arm.so  
    chmod 755 /system/lib64/egl/libinterceptor_patrace_arm64.so  

    echo "/system/lib/egl/libinterceptor_patrace_arm.so" > /system/lib/egl/interceptor.cfg  
    echo "/system/lib64/egl/libinterceptor_patrace_arm64.so" > /system/lib64/egl/interceptor.cfg  

    chmod 644 /system/lib/egl/interceptor.cfg  
    chmod 644 /system/lib64/egl/interceptor.cfg  

    mkdir /data/apitrace  
    chmod 777 /data/apitrace

Sometimes you will need to store the traces on the SD card instead:

    adb shell su -c mkdir /sdcard/patrace  
    adb shell su -c chmod 777 /sdcard/patrace  
    adb shell su -c ln -s /sdcard/Android/data /data/apitrace  

For Android before version 4.4, you need to update `egl.cfg`. Either update you `egl.cfg` manually, or use the provided one:

    adb push patrace/project/android/fakedriver/egl.cfg /system/lib/egl/

### Installing fakedriver on Android 8

Tracing on an Android 8 device is similar with the above. But some Project Treble enabled Android 8 devices forbid us to load libraries from /system/lib(64)/egl, so we need to install fakedrivers to /vendor/lib(64)/egl instead.

You can check if your Android 8 device supports Treble in adb shell:

    getprop ro.treble.enabled

Return value is "true" means your device supports Treble. Then you need to use the following steps to install fakedriver (32-bit apps):

    adb push fakedriver/libGLES_wrapper_arm.so /sdcard/  

    adb shell  
    su  
    mount -o rw,remount /vendor  

    cp /sdcard/libGLES_wrapper_arm.so /vendor/lib/egl/  
    rm /sdcard/libGLES_wrapper_arm.so  
    chmod 755 /vendor/lib/egl/libGLES_wrapper_arm.so  
    mv /vendor/lib/egl/libGLES_mali.so /vendor/lib/egl/lib_mali.so  
    mount -o ro,remount /vendor  

### Installing interceptor on Android 8

Similarly, install the interceptor to /vendor/lib(64)/egl and configure it:

    adb push install/patrace/android/release/egltrace/libinterceptor_patrace_arm.so /sdcard/  
    adb push install/patrace/android/release/egltrace/libinterceptor_patrace_arm64.so /sdcard/  
    adb shell  
    su  
    mount -o rw,remount /vendor  

    cp /sdcard/libinterceptor_patrace_arm.so /vendor/lib/egl/libinterceptor_patrace_arm.so  
    cp /sdcard/libinterceptor_patrace_arm64.so /vendor/lib64/egl/libinterceptor_patrace_arm64.so  

    chmod 755 /vendor/lib/egl/libinterceptor_patrace_arm.so  
    chmod 755 /vendor/lib64/egl/libinterceptor_patrace_arm64.so  

    echo "/vendor/lib/egl/libinterceptor_patrace_arm.so" > /vendor/lib/egl/interceptor.cfg  
    echo "/vendor/lib64/egl/libinterceptor_patrace_arm64.so" > /vendor/lib64/egl/interceptor.cfg  

    chmod 644 /vendor/lib/egl/interceptor.cfg  
    chmod 644 /vendor/lib64/egl/interceptor.cfg  

    mkdir /data/apitrace  
    chmod 777 /data/apitrace  

### Installing fakedriver on Android 9

Tracing on an Android 9 device is similar with tracing on Android 8. But some Project Treble enabled Android 9 devices forbid our fakedriver to load libstdc++.so from /system/lib(64)/egl, so we need to copy them to /vendor/lib(64)/ first.

You can check if your Android 8 device supports Treble in adb shell:

    getprop ro.treble.enabled

Return value is "true" means your device supports Treble. So you need to do the following copying first:

    cp /system/lib64/libstdc++.so /vendor/lib64/  
    cp /system/lib/libstdc++.so /vendor/lib/  

Then you can follow the steps in "Installing fakedriver on Android 8".

### Installing interceptor on Android 9

The same with "Installing interceptor on Android 8".

### Installing fakedriver on Android 10

Tracing on an Android 10 device is similar with tracing on Android 8.
But DO NOT rename libGLES_mali.so to lib_mali.so, Android UI will not start.
DO NOT do the following:

    mv /vendor/lib/egl/libGLES_mali.so /vendor/lib/egl/lib_mali.so

### Installing interceptor on Android 10

The same with "Installing interceptor on Android 8".

### Installing fakedriver on Android 11

Tracing on an Android 11 device is similar with tracing on Android 10.
You should do the following steps.

Comment the following line in /vendor/build.prop:

    #ro.hardware.egl=mali

Rename the fakedriver libGLES_wrapper_arm64.so:

    mv libGLES_wrapper_arm64.so libGLES_arm_wrapper64.so
    mv libGLES_wrapper_arm.so   libGLES_arm_wrapper.so


### Running Vulkan apps after installing fakedriver on Android 8 and 9

The Android Vulkan loader uses 32-bit and 64-bit Vulkan drivers here:

    /vendor/lib/hw/vulkan.<ro.product.platform>.so  
    /vendor/lib64/hw/vulkan.<ro.product.platform>.so  

If these 2 files are symbolic links to real DDK(`libGLES_mali.so`) and you renamed the real DDK for installing fakedriver, you need to recreate these symbolic links for Vulkan apps running.

### Tracing on Android

Find out the package name of the app you want to trace (i.e. `com.arm.mali.Timbuktu`, not just `Timbuktu`). The easiest way it to run the app, then run `top -m 5` or `ps`.

Add the name to `/system/lib/egl/appList.cfg` (32 bit) or `/system/lib64/egl/appList.cfg` (64 bit). This is a newline-separated list of the names of the apps that you want to trace (the full package name, not the icon name).

If `/system/lib[64]/egl/` does not exist, you can also use `/system/vendor/lib/egl/` is also OK.

Create the output trace directory in advance, which will be named /data/apitrace/&lt;package name&gt;. Give it 777 permissions.

Example:

    echo com.arm.mali.Timbuktu >> /system/lib/egl/appList.cfg
    chmod 644 /system/lib/egl/appList.cfg  
    mkdir -p /data/apitrace/com.arm.mali.Timbuktu
    chmod 777 /data/apitrace/com.arm.mali.Timbuktu

Make sure `/system/lib[64]/egl/appList.cfg` is world readable.

When you have run the application and want to stop tracing, go to home screen on the Android UI, then kill the application either using "adb shell kill &lt;pid&gt;" (do **not** use -9 here!) or use the Android UI to kill it. You need to make sure the application is properly closed before copying out the file, otherwise the trace file will not work. Do not use "Close all" from Android. After killing from adb shell, grep for "Close trace file " in logcat. If you find it, means the trace file was created properly.

#### Tracing on Android L and M

You may need to do 'setenforce 0' in adb shell before tracing, otherwise you will get file open errors. This may need to be repeated after each reboot of the device. Note that on Android N and later, this may break the device.

#### Tracing non-native resolutions

It is possible to trace non-native resolutions on Android by switching window resolution through adb. Use "adb shell wm size WIDTHxHEIGHT", but you may need to add a few pixels to get the exact right size. Look in the "adb logcat" for the resolution traced, to verify. For example, on Nexus10, you need 2004x1080 resolution to trace 1080p content.

#### Tracing on Samsung S8

More restrictions have been added on Android. You will need to pre-create the target trace directory, adnd use chcon to set the Selinux file permissions on it, for example `chcon u:object_r:app_data_file:s0:c512,c768 /data/apitrace/com.futuremark.dmandroid`, in addition to setting the permissions to 777. You can also no longer 'adb pull' the file directly from this directory, but have to go by the way of /sdcard/.

#### Tracing on Android with Treble enabled

If the fakedriver doesn't seem to be picked up (nothing relevant printed in logcat), it might be because Android has Treble enabled. If so, put libs under `vendor/` and rename `libGLES_mali.so` to `lib_mali.so`.

### Tracing on Linux

Put the libegltrace.so file you built somewhere where you can access it, then run:

    LD_PRELOAD=/my/path/libegltrace.so OUT_TRACE_FILE=myGLB2 ./glbenchmark2

Use the `OUT_TRACE_FILE` variable to set the path and filename of the captured trace. The filename will be appended with an index followed by `.pat`. If `%u` appears in `OUT_TRACE_FILE`, it will be used as a placeholder for the index. E.g. `foo-%u-bar.pat`. If the `OUT_TRACE_FILE` variable is omitted, then the hard coded pattern, `trace.%u.pat`, will be used.

You may also specify the environment variables `TRACE_LIBEGL`, `TRACE_LIBGLES1` and `TRACE_LIBGLES2` to tell the tracer exactly where the various GLES DDK files are found. If you use these, you may be able to use `LD_LIBRARY_PATH` to the fakedriver instead of `LD_PRELOAD` of the tracer directly. In this latter case, set `INTERCEPTOR_LIB` to point to your tracer library.

The fakedriver may be hard to use on desktop Linux. We have poor experience using the Mali emulator as well, and recommend using a proper GLES capable driver.

### Tracer configuration file

The tracer can be configured through a special configuration file `$PWD/tracerparams.cfg` that contains configuration lines containing one keyword and one value which is usually "true" or "false".The following parameters can be specified in it:

-   EnableErrorCheck - Turn on or off saving errors to the trace file
-   UniformBufferOffsetAlignment - Change UBO alignment. By default this is set to the lowest common denominator for all relevant platforms.
-   ShaderStorageBufferOffsetAlignment - Change SSBO alignment. As above.
-   EnableActiveAttribCheck
-   InteractiveIntercept - Debugging tool
-   FilterSupportedExtension - Report only a specified list of extensions to the application.
-   FlushTraceFileEveryFrame - Make sure we save each frame to disk. On by default. You could try turning it off if you really need to speed up tracing performance.
-   StateDumpAfterSnapshot - Debugging tool
-   StateDumpAfterDrawCall - Debugging tool
-   SupportedExtension - Use this to specify which extensions to report to the application. One extension per keyword.
-   DisableErrorReporting - Disable GLES error reporting callbacks. Set DisableErrorReporting to false if debug-callback error occurs, it's a Debug option.
-   EnableRandomVersion  - Enable to append a random to the gl_version when gl_renderer begins with "Mali". Default to True.

The most useful keyword is 'FilterSupportedExtension', which, if set to 'true', will fake the list of supported extensions reported to the application only a limited list of extensions. In this case, put each extension you want to support in the configuration file on a separate line with the 'SupportedExtension' keyword.

### Special features of PaTrace Tracer

You may enable or disable extensions, this is useful to create traces that don't use certain extensions. Limiting the extensions an app 'sees' is useful when you want to create a tracefile that is compatible with devices that don't support a given extension. How it works; when an app calls `glGetString(GL_EXTENSIONS)`, only the ones enabled in `/system/lib/egl/tracerparams.cfg` will be returned to the app. Unfortunately, some applications ignore or don't use this information, and may use certain extensions, anyways.The tracer will ignore some extensions like the binary shader extensions by default. You can override this by explicitly listing the extensions to use in `tracerparams.cfg`, as described 
above.The full list of extensions that the device supports will be saved in the trace header.

We never want binary shaders in the resulting trace file, since it can then only be retraced on the same platform. Since binary shaders are supported in GLES3, merely disabling the binary shader extensions may not be enough. You may have to go into Android app settings, and flush all app caches before you run the app to make sure the shader caches are cleared, 
before tracing it.

### Troubleshooting trace issues

1.  If the traced content uses multiple threads and/or multiple contexts, retracing may fail. Try running the pat-remap-egl python script that comes with patrace. See python tool installation instructions above.
2.  Did you remember to close the application correctly? See tracing instructions above.
3.  The tracer does not write directly to disk; instead it has a in-memory cache that is flushed to disk when full. The cache is currently hardcoded to 70MB, but can be decreased or increased. It was increased from 20MB to 70MB when it was discovered that a game allocating large textures (almost 4k by 4k) used more space that the maxium size of the cache. The downside of increasing the cache size is that we have a limited amount of memory on the devices we create traces on. The cache size is defined in `patrace/src/tracer/egltrace.hpp::TraceOut::WRITE_BUF::LEN`
4.  If you store data on the sdcard, you may have to give the app permissions to write to the sdcard. To do so, you may need to edit the app's platform.xml and add <group gid="media_rw" />" after a <permission name="android.permission.WRITE_EXTERNAL_STORAGE"> line. If there is no such line, you may have to add it.

GLES Layer
---------
From Android Q, Android introduces GLES Layer similar with Vulkan layer that chains calls together between the application and driver. Since new Android uses more and more restrict rule for file system permission, the possible official way to trace GLES content after Android Q is using GLES Layer.

### Enable GLES Layer on Android
First, deploy layer into the hard coded system directory:

    adb push libGLES_layer_arm64.so /data/local/debug/gles
    adb push libGLES_layer_arm.so   /data/local/debug/gles

Then, enable GLES Layer for specified target application (`libGLES_layer_arm.so` for 32bit):

    adb shell settings put global enable_gpu_debug_layers 1
    adb shell settings put global gpu_debug_layers_gles libGLES_layer_arm64.so
    adb shell settings put global gpu_debug_app  application_package_name_wantToTrace

#### Enable GLES Layer on S21
First, enable GLES layer by running `adb shell setprop debug.gles.layers libGLES_layer_arm64.so`.
Then, find which directory to place the GLES layer by running `adb logcat | grep libEGL`, while starting the game. It will list where it is searching for the GLES layer. Copy the GLES layer to this directory, and give it 755 permission.

### Tracing on Android
Create the output trace directory in advance, which will be named `/data/apitrace/<package_name>`. Give it 777 permission, and run `chcon u:object_r:app_data_file:s0:c512,c768 /data/apitrace/<package_name>`
After that, run the application and tracing begins automatically.

Retracing
---------

### Performance measurements

One of PATrace's main targets has all along been about measuring performance, and due to it's fast binary format it's suitable for such tasks. There are a few different things to consider when doing performance measurements:

-   Set measurement range using the framerange option to include only the gameplay / main content, and avoiding any loading frames. The easiest way to find the relevant framerange is to use the -step option of paretrace.
-   Use the preload option to keep the selected framerange in memory to avoid disk IO.
-   If restricted by vsync or your screen is too small, use offscreen mode. Offscreen mode adds the overhead of an additional blit every 100 frames, but when running on silicon devices, it is usually the right option to use.

You can get detailed information saved to disk about each frame in 'results.json' with the 'collectors' options. For the options possible to set with this option, see the 'libcollector documentation' below.

To get more detailed CPU counters on Android, you may have to run

    adb shell setprop security.perf_harden 0

against the device first.

Detailed call statistics about the time spent in each API call can be gathered with the 'callstats' option. The results will end up in a 'callstats.csv' file.

The GL_AMD_performance_monitor will be used on devices that support it, however you may have to set frame ranges to avoid counter data being destroyed on context destruction. Its outputs will end up in the file 'perfmon.csv' in current working directory on Linux and under '/sdcard' on Android. The list of existing counters will be dumped to 'perfmon_counters.csv'. The file 'perfmon.conf' can be used to configure it - the first line sets the counter group, and all other lines set individual counters, all by value.

### Retracing on FPGA

On the FPGA with dummy winsys, you need to specify some extra environment variables. `MALI_EGL_DUMMY_DISPLAY_WIDTH` and `MALI_EGL_DUMMY_DISPLAY_HEIGHT` should be set to "4096". Set `LD_LIBRARY_PATH` to point to the location of your compiled DDK.

### Retracing on Android

-   Install `eglretrace-release.apk`
-   If you need to use the GUI, copy original/modified traces to `/data/apitrace/` or `/sdcard/apitrace/trace_repo`. (they will already be in `/data/apitrace` if they are traces from the device).
-   Run the `PARetrace` app.
-   Select a trace to start retracing it.
-   For extra parameters, such as setting a custom screen resolution, thread ID etc, start the retracer using am as shown below.

To launch the retracer from command line, you can use the following command:

    adb shell am start -n com.arm.pa.paretrace/.Activities.RetraceActivity --es fileName /absolute/path/to/tracefile.pat

For a full set of available parameters, see Command Line Parameters in ADB Shell.

To retrace some traces which contain external YUV textures on Android 7 or later, you need to add libui.so to /etc/public.libraries.txt out

    adb shell  
    su  
    mount -o rw,remount /  
    echo libui.so >> /etc/public.libraries.txt  

If you have a problem opening files on /sdcard, you may need to do this:

    adb shell appops set com.arm.pa.paretrace READ_EXTERNAL_STORAGE allow
    adb shell appops set com.arm.pa.paretrace WRITE_EXTERNAL_STORAGE allow

### Retracing On Desktop Linux

For GLES support on Linux desktop environments there are three options:

-   Use Nvidia's GLES capable driver
-   Use the Mali GLES Emulator
-   Use the MESA GLES libraries provided with Debian/Ubuntu (`libgles1-mesa libgles2-mesa`)

The preferred option is to use the latest Nvidia driver (see Tracing on desktop Linux). Using the MESA's GLES implementation is the least preferred option. Copy the trace file from the device and replay it with:

    paretrace TRACE_FILE.pat

The first time you run the file, it is recommended to add the "-debug" command line switch to see any error messages from the driver, which can reveal many issues.

### Retracing on embedded Linux

-   Just run paretrace &lt;tracefile.pat&gt;
-   If you're interested in the final FPS number then you will need to set a suitable frame range and use preload, i.e. "eglretrace -preload x y tracefile.pat"
-   See paretrace -h for options such as setting window size, retracing only a part of the file, debugging etc.
-   If your use case is more advanced (automation, instrumented data collection) you'll need to pass the parameters to the retracer as a JSON file - see PatracePerframeData for more information.

### Parameter options

There are three different ways to tell the retracer which parameters that should be used: (1) by command line, (2) in adb shell, and (3) by passing a JSON file.

#### Command Line Parameters 

| Parameter                                    | Description                                                                                                                                                                                                                            |
|----------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `-tid THREADID`                              | only the function calls invoked by the given thread ID will be retraced                                                                                                                                                                |
| `-s CALL_SET`                                | take snapshot for the calls in the specific call set. Example `*/frame` for one snapshot for each frame, or `250/frame` to take a snapshot just of frame 250.                                                                          |
| `-step`                                      | For desktop Linux, use F1-F4 to step forward frame by frame, F5-F8 to step forward draw call by draw call. For Linux fbdev, press H to see detailed usage.                                                                                                               |
| `-ores W H`                                  | override the resolution of the final onscreen rendering (FBOs used in earlier renderpasses are not affected!) |
| `-msaa SAMPLES`                              | Enable multi sample anti alias for the final framebuffer |
| `-overrideMSAA SAMPLES`                      | Override any existing MSAA settings for intermediate framebuffers that already use MSAA. |
| `-preload START STOP`                        | preload the trace file frames from START to STOP. START must be greater than zero. Implies -framerange.                                                                                                                                |
| `-all                                        | (since r4p0) run all calls even those with no side-effects. This is useful for CPU load measurements. |
| `-framerange FRAME_START FRAME_END`          | start fps timer at frame start, stop timer and playback at frame end. The default framerange starts at 1, but it can be specified at 0. Usually you want to measure the middle-to-end part of a trace, so you're not measuring time spent for EGL init and loading screens.    |
| `-instrumentation-delay USECONDS`            | Delay in microseconds that the retracer should sleep for after each present call in the measurement range.    |
| `-skipfence start-end,start-end`             | Skip some fence waits calls(eglClientWaitSync, eglWaitSync, eglClientWaitSyncKHR, eglWaitSyncKHR, glWaitSync, glClientWaitSync) when within the measurement frame range.    |
| `-loop TIMES`                                | (since r3p0) Loop the given frame range at least the given number of times. |
| `-looptime SECONDS`                          | (since r3p0) Loop the given frame range at least the given number of seconds. |
| `-singlesurface SURFACE`                     | (since r3p0) Render all surfaces except the given one to pbuffer render target. |
| `-debug`                                     | Output debug messages                                                                                                                                                                                                                  |
| `-debugfull`                                 | Output all of the current invoked gl functons, with callNo, frameNo and skipped or discarded information                                                                                                                               |
| `-singlewindow`                              | Force everything to render in a single window                                                                                                                                                                                          |
| `-offscreen`                                 | Run in offscreen mode                                                                                                                                                                                                                  |
| `-singleframe`                               | Draw only one frame for each buffer swap (offscreen only)                                                                                                                                                                              |
| `-jsonParameters FILE RESULT_FILE TRACE_DIR` | path to a JSON file containing the parameters, the output result file and base trace path                                                                                                                                              |
| `-info`                                      | Show default EGL Config for playback (stored in trace file header). Do not play trace.                                                                                                                                                 |
| `-infojson`                                  | Show JSON header. Do not play trace.                                                                                                                                                                                                   |
| `-instr`                                     | Output the supported instrumentation modes as a JSON file. Do not play trace.                                                                                                                                                          |
| `-overrideEGL`                               | Red Green Blue Alpha Depth Stencil, example: overrideEGL 5 6 5 0 16 8, for 16 bit color and 16 bit depth and 8 bit stencil                                                                                                             |
| `-strict`                                    | Use strict EGL mode (fail unless the specified EGL configuration is valid)                                                                                                                                                             |
| `-forceVRS`                                  | Force the use of VRS for all framebuffers. Valid values: 38566 (1x1), 38567 (1x2), 38568 (2x1), 38569 (2x2), 38572 (4x2) and 38574 (4x4). |
| `-libEGL`                                    | Set the path to the EGL library to load |
| `-libGLESv1`                                 | Set the path to the GLES 1 library to load |
| `-libGLESv2`                                 | Set the path to the GLES 2+ library to load |
| `-version`                                   | Output the version of this program                                                                                                                                                                                                     |
| `-callstats`                                 | (since r2p4) Output GLES API call statistics to disk, time spent in API calls measured in nanoseconds. Required to use with -framerange.                                                                                                                                |
| `-collect`                                   | (since r2p4) Collect performance information and save it to disk. It enables some default libcollector collectors. For fine-grained control over libcollector behaviour, use the JSON interface instead.                               |
| `-perf FRAME_START FRAME_END`                | (since r2p5) Create perf callstacks of the selected frame range and save it to disk. It calls "perf record -g" in a separate thread once your selected frame range begins.                                                             |
| `-perfpath filepath`                         | (since r2p5) Path to your perf binary. Mostly useful on embedded systems.                                                                                                                                                              |
| `-perffreq freq`                             | (since r2p5) Your perf polling frequency. The default is 1000. Can usually go up to 25000.                                                                                                                                             |
| `-perfout filepath`                          | (since r2p5) Destination file for your -perf data                                                                                                                                                                                      |
| `-perfevent event`                           | (since r4p3) Capture custom event |
| `-script scriptPath Frame`                   | (since r3p3) trigger script on the specific frame                                                                                                                                                                                      |
| `-noscreen`                                  | (since r2p4) Render without visual output using a pbuffer render target. This can be significantly slower, but will work on some setups where trying to render to a visual output target will not work.                                |
| `-flush`                                     | (since r2p5) Will try hard to flush all pending CPU and GPU work before starting the selected framerange. This should usually not be necessary.                                                                                        |
| `-flushonswap`                               | (since r2p15) Will try hard to flush all pending CPU and GPU work before starting the next frame. This should usually not be necessary. |
| `-cpumask`                                   | (since r2p15) Lock all work associated with this replay to the specified CPU cores, given as a string of one or zero for each core. |
| `-multithread`                               | Enable to run the calls in all the threads recorded in the pat file. These calls will be dispatched to corresponding work threads and run simultaneously. The execution sequence of calls between different threads is not guaranteed. |
| `-dmasharedmem`                              | (since r2p16) The retracer would use shared memory feature of linux to handle dma buffer. Recommended on model. |
| `-egl_surface_compression_fixed_rate flag`   | (since r3p4)  Set compression control flag on framebuffer. 0: disable fixed rate compression; 1: enable fixed rate compression with default rate; 2: enable fixed rate compression with lowest rate; 3: enable fixed rate compression with highest rate.  |
| `-egl_image_compression_fixed_rate flag`     | (since r3p4)  Set compression control flag on eglImage. 0: disable fixed rate compression; 1: enable fixed rate compression with default rate.  |
| `-gles_texture_compression_fixed_rate flag`  | (since r3p4)  Set compression control flag on texture.  0: disable fixed rate compression; 1: enable fixed rate compression with default rate; 2: enable fixed rate compression with lowest rate; 3: enable fixed rate compression with highest rate.   |
| `-savecache prefix`                          | (since r4p2) Save shaders as binaries to a shader cache. Will add .bin and .idx to the given name. |
| `-loadcache prefix`                          | (since r4p2) Load binary shaders from an existing shader cache created with -savecache. Will add .bin and .idx to the given name. |
| `-cacheonly`                                 | (since r4p2) Skip any calls not needed for populating a shader cache. Can only be used with -savecache. |

    CALL_SET = interval ( '/' frequency )
    interval = '*' | number | start_number '-' end_number
    frequency = divisor | "frame" | "draw"

#### Retrace Command Line Parameters in ADB Shell

| Command                    | Description                                                                                                                                                                                                                                                                                                                                                                  |
|----------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `--es fileName`            | /path/to/tracefile                                                                                                                                                                                                                                                                                                                                                           |
| `--ei oresW`               | width                                                                                                                                                                                                                                                                                                                                                                        |
| `--ei oresH`               | height                                                                                                                                                                                                                                                                                                                                                                       |
|                            | Override the resolution of the final onscreen rendering (FBOs used in earlier renderpasses are not affected!) |
| `--ez forceOffscreen`      | true/false. When enabled, 100 frames will be rendered per visible frame, and rendering is no longer vsync limited. Frames are visible as 10x10 tiles.                                                                                                                                                                                                                        |
| `--ez offscreenSingleTile` | true/false. Draw only one frame for each buffer swap in offscreen mode.                                                                                                                                                                                                                                                                                                      |
| `--es snapshotPrefix`      | /path/to/snapshots/prefix- Must contain full path and a prefix, resulting screenshots will be named prefix-callnumber.png                                                                                                                                                                                                                                                    |
| `--es snapshotCallset`     | call begin - call end / frequency, example: '10-100/draw' or '10-100/frame' or '10-100' (snapshot after every call in range!)                                                                                                                                                                                                                                                |
| `--ei frame_start`         | Start measure fps from this frame. The default framerange starts at 1.                                                                                                                                                                                                                                                                                                       |
| `--ei frame_end`           | Stop fps measure, and stop playback                                                                                                                                                                                                                                                                                                                                          |
| `--ei instrumentationDelay`           | Delay in microseconds that the retracer should sleep for after each present call in the measurement range.                                                                                                                                                                                                                                                                                                                                          |
| `--ei loop`                | times. Loop the given frame range at least the given number of times.                                                                                                                                                                                                                                                                                                        |
| `--ei loopSeconds`         | seconds. Loop the given frame range at least the given number of seconds.                                                                                                                                                                                                                                                                                                    |
| `--ez runAllCalls`         | True/False(default). run all calls even those with no side-effects. This is useful for CPU load measurements.                                                                                                                                                                                                                                                                |
| `--es skipfence`           | start-end,start-end. Skip some fence waits calls(eglClientWaitSync, eglWaitSync, eglClientWaitSyncKHR, eglWaitSyncKHR, glWaitSync, glClientWaitSync) when within the measurement frame range.                                                                                                                                                                                                                                                                                                               |
| `--ez callstats`           | true/false(default)  Output GLES API call statistics to callstats.csv under /sdcard for Android, or under the current dir, time spent in API calls measured in nanoseconds.                                                                                                                                                                                                  |
| `--ei singlesurface`       | SURFACE. (since r3p0) Render all surfaces except the given one to pbuffer render target.                                                                                                                                                                                                                                                                                     |
| `--ei perfstart`           | Start perf record from this frame. Frame number must be 1 or higher.  |
| `--ei perfend`             | Stop perf record and save perf data. Frame number must be greater than `perfstart` frame  |
| `--es perfpath`            | Path to your perf binary. The default is `/system/bin/simpleperf`   |
| `--ei perffreq`            | Your perf polling frequency. The default is 1000.  |
| `--es perfout`             | Destination file for your perf data. The default is `/sdcard/perf.data`          |
| `--es perfevent`           | Event you want to capture. The default is "".  |
| `--ez noscreen`            | true/false(default). Render without visual output using a pbuffer render target. This can be significantly slower, but will work on some setups where trying to render to a visual output target will not work.                                                                                                                                                              |
| `--ez finishBeforeSwap`    | True/False(default). Will try hard to flush all pending CPU and GPU work before starting the next frame. This should usually not be necessary.                                                                                                                                                                                                                               |
| `--ez flushWork`           | True/False(default). Will try hard to flush all pending CPU and GPU work before starting the selected framerange. This should usually not be necessary.                                                                                                                                                                                                                      |
| `--ez preload`             | True/False(default) Loads calls for frames between `frame_start` and `frame_end` into memory. Useful when playback is IO-bound.                                                                                                                                                                                                                                              |
|                            | The following options may be used to override onscreen EGL config stored in trace header.                                                                                                                                                                                                                                                                                    |
| `--ei colorBitsRed`        | bits                                                                                                                                                                                                                                                                                                                                                                         |
| `--ei colorBitsGreen`      | bits                                                                                                                                                                                                                                                                                                                                                                         |
| `--ei colorBitsBlue`       | bits                                                                                                                                                                                                                                                                                                                                                                         |
| `--ei colorBitsAlpha`      | bits                                                                                                                                                                                                                                                                                                                                                                         |
| `--ei depthBits`           | bits                                                                                                                                                                                                                                                                                                                                                                         |
| `--ei stencilBits`         | bits                                                                                                                                                                                                                                                                                                                                                                         |
| `--ez antialiasing`        | true/false(default) Enable 4x MSAA.                                                                                                                                                                                                                                                                                                                                          |
| `--ei msaa`                | SAMPLES. Enable multi sample anti alias for the final framebuffer                                                                                                                                                                                                                                                                                                            |
| `--ei overrideMSAA`        | SAMPLES. Override any existing MSAA settings for intermediate framebuffers that already use MSAA.                                                                                                                                                                                                                                                                            |
| `--ei forceVRS`            | VRS. Force the use of VRS for all framebuffers. Valid values: 38566 (1x1), 38567 (1x2), 38568 (2x1), 38569 (2x2), 38572 (4x2) and 38574 (4x4).                                                                                                                                                                                                                               |
| `--ez removeUnusedAttributes`| true/false(default) Modify the shader in runtime by removing attributes that were not enabled during tracing. When this is enabled, 'storeProgramInformation' is automatically turned on.                                                                                                                                                                                  |
| `--ez storeProgramInfo`    | true/false(default) In the result file, store information about a program after each glLinkProgram. Such as, active attributes and compile errors.                                                                                                                                                                                                                           |
| `--es cpumask`             | Lock all work associated with this replay to the specified CPU cores, given as a string of one or zero for each core.                                                                                                                                                                                                                                                        |
| `--es jsonData`            | path to a JSON file containing parameters, e.g. /data/apitrace/input.json. Only works together with traceFilePath and resultFile, any other options don't work anymore                                                                                                                                                                                                       |
| `--es traceFilePath`       | base path to trace file storage, e.g. /data/apitrace                                                                                                                                                                                                                                                                                                                         |
| `--es resultFile`          | path to output result file, e.g. /data/apitrace/result.json                                                                                                                                                                                                                                                                                                                  |
| `--ez multithread`         | true/false(default) Multithread execution mode.                                                                                                                                                                                                                                                                                                                          |
| `--ez enOverlay`           | If true(default), enable overlay all the surfaces when there is more then one surface created. If false, all the surfaces will be splited horizontally in a slider container.                                                                                                                                                                                                |
| `--ei transparent`         | The alpha value of each surface, when using Overlay layout. The default is 100 (opaque).                                                                                                                                                                                                                                                                                     |
| `--ez forceSingleWindow`   | True/False(default) to force render all the calls onto a single surface. This can't be true with multithread mode enabled.                                                                                                                                                                                                                                                   |
| `--ez enFullScreen`        | True/False(default) to hide the system navigator and control bars.                                                                                                                                                                                                                                                                                                           |
| `--ez debug`               | True/False(default) Output debug messages.                                                                                                                                                                                                                                                                                                                                   |
| `--ei eglSurfaceCompressionFixedRate`    | (since r3p4) Set compression control flag on framebuffer. 0: disable fixed rate compression; 1: enable fixed rate compression with default rate; 2: enable fixed rate compression with lowest rate; 3: enable fixed rate compression with highest rate.  |
| `--ei eglImageCompressionFixedRate`      | (since r3p4) Set compression control flag on eglImage. 0: disable fixed rate compression; 1: enable fixed rate compression with default rate.   |
| `--ei glesTextureCompressionFixedRate`   | (since r3p4) Set compression control flag on texture. 0: disable fixed rate compression; 1: enable fixed rate compression with default rate; 2: enable fixed rate compression with lowest rate; 3: enable fixed rate compression with highest rate.  |

| `--ez step`               | true/false(default) Enable step mode. Use "input keyevent <keyname>" to step forward frames/draws. Input N/M/L to step forward 1/10/100 frame, input D to step forward 1 draw call.  |

#### Parameters from JSON file

A JSON file can be passed to the retracer via the -jsonParameters option. In this mode, the retracer will output a result file when the retracing has finished. The content should be a JSON object that contains the following keys:

| Key                          | Type       | Optional | Description                                                                                                                                                                                                                            |
|------------------------------|------------|----------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| colorBitsAlpha               | int        | yes      |                                                                                                                                                                                                                                        |
| colorBitsBlue                | int        | yes      |                                                                                                                                                                                                                                        |
| colorBitsGreen               | int        | yes      |                                                                                                                                                                                                                                        |
| colorBitsRed                 | int        | yes      |                                                                                                                                                                                                                                        |
| depthBits                    | int        | yes      |                                                                                                                                                                                                                                        |
| stencilBits                  | int        | yes      | |
| msaa                         | int        | yes      | Enable multi sample anti alias for the final framebuffer |
| file                         | string     | no       | The file name of the .pat file.                                                                                                                                                                                                        |
| frames                       | string     | no       | The frame range delimited with '-'. The default frames range starts at 1.                                                                                                                                                                |
| overrideMSAA                 | int        | yes      | Override any existing MSAA settings for intermediate framebuffers that already use MSAA. |
| forceVRS                     | int        | yes      | Force the use of VRS for all framebuffers. Valid values: 38566 (1x1), 38567 (1x2), 38568 (2x1), 38569 (2x2), 38572 (4x2) and 38574 (4x4). |
| loopTimes                    | int        | yes      | (since r3p0) Loop the given frame range at least the given number of times. |
| loopSeconds                  | int        | yes      | (since r3p0) Loop the given frame range at least the given number of seconds. |
| singlesurface                | int        | yes      | (since r3p0) Render all surfaces except the given one to pbuffer render target. |
| instrumentation              | list       | yes      | **(deprecated since r2p4)** See PATrace performance measurements setup for more information                                                                                                                                            |
| callStats                    | boolean    | yes      | Output GLES API call statistics to callstats.csv under /sdcard for Android, or under the current dir, time spent in API calls measured in nanoseconds.                                                                                 |
| collectors                   | dictionary | yes      | (since r2p4) Dictionary of libcollector collectors to enable, and their configuration options. <br> Example:                              <br>                                                                            {                                                                                                                                                                                                                                                                                              "cpufreq": { "required": true },<br>                                                                                                                                                                                                 "rusage": {}<br>                                                                                                                                                                                                                                                                               } <br>                                                                                                                                                                                                                                 For description of the various collectors, see the libcollector documentation below.                                                                                                               |
| perfrange                    | string     | yes      | The frame range delimited with '-'. The first frame must be 1 or higher. |
| perfpath                     | string     | yes      | Path to your perf binary. Mostly useful on embedded systems.   |
| perffreq                     | int        | yes      | Your perf polling frequency. The default is 1000. Can usually go up to 25000.     |
| perfout                      | string     | yes      | Destination file for your perf data      |
| perfevent                    | string     | yes      | Event you want to capture.   |
| instrumentationDelay         | int        | yes      | Delay in microseconds that the retracer should sleep for after each present call in the measurement range. |
| scriptpath                   | string     | yes      | (since r3p3) The script file with path to be executed.           |
| scriptframe                  | int        | yes      | (since r3p3) The frame number when script begin to execute.      |
| landscape                    | boolean    | yes      | Override the orientation                                                                                                                                                                                                               |
| offscreen                    | boolean    | yes      | Render the trace offscreen                                                                                                                                                                                                             |
| noscreen                     | boolean    | yes      | Render without visual output using a pbuffer render target. This can be significantly slower, but will work on some setups where trying to render to a visual output target will not work.                             |
| overrideHeight               | int        | yes      | Override height in pixels                                                                                                                                                                                                              |
| overrideResolution           | boolean    | yes      | If true then the resolution is overridden                                                                                                                                                                                              |
| overrideWidth                | int        | yes      | Override width in pixels                                                                                                                                                                                                               |
| preload                      | boolean    | yes      | Preloads the trace                                                                                                                                                                                                                     |
| runAllCalls                  | boolean    | yes      | (since r4p0) Run all calls even those with no side-effects. This is useful for CPU load measurements. |
| snapshotCallset              | string     | yes      | call begin - call end / frequency, example: '10-100/draw' or '10-100/frame' (snapshot after every call in range!). The snapshot is saved under the current directory by default.                                                       |
| snapshotPrefix               | string     | yes      | Contain a path and a prefix, resulting screenshots will be named prefix-callnumber.png                                                                                                                                                |
| skipfence                    | string     | yes      | Skip some fence waits calls(eglClientWaitSync, eglWaitSync, eglClientWaitSyncKHR, eglWaitSyncKHR, glWaitSync, glClientWaitSync) when within the measurement frame range.                                                                                            |
| removeUnusedVertexAttributes | boolean    | yes      | Modify the shader in runtime by removing attributes that were not enabled during tracing. When this is enabled, 'storeProgramInformation' is automatically turned on.                                                                  |
| flushWork                    | boolean    | yes      | Will try hard to flush all pending CPU and GPU work before starting running the selected framerange. This should usually not be necessary.                                                                                             |
| finishBeforeSwap             | boolean    | yes      | Will try hard to flush all pending CPU and GPU work before every call to swap the backbuffer. This should usually not be necessary.                                                                                                    |
| debug                        | boolean    | yes      | Output debug messages                                                                                                                                                                                                                  |
| storeProgramInformation      | boolean    | yes      | In the result file, store information about a program after each glLinkProgram. Such as, active attributes and compile errors.                                                                                                         |
| threadId                     | int        | yes      | Retrace this specified thread id. **DO NOT USE** except for debugging!                                                                                                                                                                 |
| offscreenSingleTile          | boolean    | yes      | Draw only one frame for each buffer swap in offscreen mode.                                                                                                                                                                            |
| multithread                  | boolean    | yes      | Enable to run the calls in all the threads recorded in the pat file. These calls will be dispatched to corresponding work threads and run simultaneously. The execution sequence of calls between different threads is not guaranteed. |
| forceSingleWindow            | boolean    | yes      | Force render all the calls onto a single surface. This can't be true with multithread mode enabled.                                                                                                                                    |
| cpumask                      | string     | yes      | See 'cpumask' command line option above. |
| dmaSharedMem                 | bool       | yes      | If it is true, the retracer would use shared memory feature of linux to handle dma buffer. Recommended on model.|
| eglSurfaceCompressionFixedRate  | int     | yes      | (since r3p4) Set compression control flag on framebuffer. 0: disable fixed rate compression; 1: enable fixed rate compression with default rate; 2: enable fixed rate compression with lowest rate; 3: enable fixed rate compression with highest rate.  |
| eglImageCompressionFixedRate | int        | yes      | (since r3p4) Set compression control flag on eglImage. 0: disable fixed rate compression; 1: enable fixed rate compression with default rate.  |
| glesTextureCompressionFixedRate | int     | yes      | (since r3p4) Set compression control flag on texture. 0: disable fixed rate compression; 1: enable fixed rate compression with default rate; 2: enable fixed rate compression with lowest rate; 3: enable fixed rate compression with highest rate.  |
| loadShaderCache              | string     | yes      | (since r4p2) See 'loadcache' command line option above. |
| saveShaderCache              | string     | yes      | (since r4p2) See 'savecache' command line option above. |
| cacheOnly                    | boolean    | yes      | (since r4p2) See 'cacheonly' command line option above. |
| step                    | boolean    | yes      | (since r4p3) See 'step' option above for desktop Linux and Android.Press H to see detailed usage on uDriver and fbdev. |

This is an example of a JSON parameter file:

    {
     "colorBitsAlpha": 0,
     "colorBitsBlue": 8,
     "colorBitsGreen": 8,
     "colorBitsRed": 8,
     "depthBits": 24,
     "file": "./egypt_hd_10fps.orig.std.etc1.gles2.pat",
     "frames": "4-200",
     "snapshotCallset": "4-5/frame",
     "snapshotPrefix": "/data/apitrace/snap/frame_",
     "landscape": true,
     "offscreen": false,
     "overrideHeight": 720,
     "overrideResolution": true,
     "overrideWidth": 1280,
     "preload": true,
     "removeUnusedVertexAttributes": true,
     "stencilBits": 0,
     "storeProgramInformation": true
    }

For using it with the Linux retracer, use the following command line:

    paretrace -jsonParameters yourParameterFile.json result.json .

### Retracing multithread trace

Patrace does not encapsulate "multithread" into file head during tracing, so multithread is false by default.

For trace in trace repo, there's no need to add the multithread option, as it will be added to trace head and set to true if needed.

For trace made by yourself, if wanting to enable multithread, you could edit file head and set multithread to true, or use the mulithread parameter option.

### Looping

The looping functionality in the replayer is very basic. Do not simply assume that it will work, always test the frame range first. One simple way to test it
is to loop twice with the screenshot option set to snap the first frame of the frame range. In this case it will capture two screenshots, of the initial run and
of the loop run, and then you can compare the two to see if looping works properly.

### Fastforwarding on Android

Fastforward is a function to generate a trace that skips a range of unnecessary frames. It is integrated as an activity of paretrace application on Android and can be launched with the following command:

    adb shell am start -n com.arm.pa.paretrace/.Activities.FastforwardActivity --es input /absolute/path/to/original/tracefile.pat --es output /absolute/path/to/modified/tracefile.pat --ei targetFrame 100

### Parameter Options

On Android, you can set parameter of fastforward by ADB shell or a JSON file.

#### Fastforward Command Line Parameters in ADB Shell

| Command                    | Optional |Description                                                                                                                                                                                                                                                                                                                                                                   |
|----------------------------|----------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| --es input                 | no       | Target trace to fastforward                                                                                                                                                                                                                                                                                                                                                 |
| --es output                | no       | Where to write fastforwarded trace file                                                                                                                                                                                                                                                                                                                                                        |
| --ei targetFrame           | no       | The frame number that should be fastforwarded to                                                                                                                                                                                                                                                                                                                                                          |
| --ei targetDrawCallNo      | no       | The draw call number that should be fastforwarded to                                                                                                                                                                                                                                                                                                                                                          |
| --ei endFrame              | yes      | The frame number that should be ended (by default fastforward to the last frame)                                                                                                                                                                                                                                                                                                                                                      |
| --ez multithread           | yes      | Run in multithread mode                                                                                                                                                                                                                                                                                                                                                        |
| --ez offscreen             | yes      | Run in offscreen mode                                                                                                                                                                                                                                                                                                                                                        |
| --ez noscreen              | yes      | Run in pbuffer output mode                                                                                                                                                                                                                                                                                                                                                        |
| --ez norestoretex          | yes      | When generating a fastforward trace, don't inject commands to restore the contents of textures to what the would've been when retracing the original. (NOTE: NOT RECOMMEND)                                                                                                                                                                            |
| --ez version               | yes      | Output the version of this program                                                                                                                                                                                                                                                                                                                                                     |
| --ei restorefbo0           | yes      | Repeat to inject a draw call commands and swapbuffer the given number of times to restore the last default FBO. Suggest repeating 3~4 times if set DamageRegionKHR, else repeating 1 time.                                                                                                                                                                                 |
| --ez txu                   | yes      | Remove the unused textures and related function calls                                                                                                                                                                                                                                                                                                                                             |

#### Example of a JSON file

    {
        "input": "data/example_trace.pat",
        "output": "data/example_trace_ff.pat",
        "targetFrame": 1000,
        "endFrame": 1100,
        "multithread": false,
        "offscreen": false,
        "noscreen": false,
        "norestoretex": false,
        "restorefbo0": 0
        "txu": false
    }

Other
-----

### PaTrace File Format

The latest .pat file format has the following structure:

1. Fixed size header containing that contains among other things;
    -   length of json string
    -   file offset where json string ends (from where we can begin reading sigbook and calls)
2. Variable length json string "header" described below.
3. A function signature book (or list) (sigbook), which maps EGL and GLES function names to id's (a number) used per intercepted call. This list is generated from khronos headers when compiling the tracer. When playing back a tracefile, the retracer reads the sigbook. The sigbook is compressed using the 'snappy' compression algorithm.
4. Finally the real content: intercepted EGL and GLES calls, which are also compressed with "snappy".
 
The variable length json "header" always contains:
-   default thread id
-   glesVersion (1,2,3)
-   frame count
-   call count
-   a list of per thread EGL configurations
-   per thread client side buffer use in bytes (non-VBO type data)
-   window width and height (winW, winH) captured from eglCreateWindowSurface

Debugging the interceptor on Android
------------------------------------

This section describes how to debug the tracer while it is tracing a third party Android app. This is solved by using gdb's remote debugging possibilities. Remote debugging with gdb is achieved by setting up a gdb server (`gdbserver`) on the Android device and connect to it with the gdb client (`gdb`) on your local machine. The method described in this section focuses on egltrace. However, this method can be used to debug any native Android library or app.

### Compile the interceptor

The egltrace project is normally built with optimizations and without debugging symbols. In order to be able to debug, optimizations should be disabled and debugging symbols must be generated.In the egltrace project (`content_capturing/patrace/project/android/egltrace`), edit the `jni/Application.mk` file:

-   Change `APP_OPTIM :=` `release` to `APP_OPTIM :=` `debug`
-   Remove the optimization flag, `-O3`, from `APP_CFLAGS` and replace it with `-g`

Compile the project with the `NDK_DEBUG=1` parameter:

    ndk-build NDK_DEBUG=1

### Installing built files

After a successful build, files in the `libs` and `obj` directories are created. These four files are relevant for remote debugging:

|                                                     |                                                                       |
|-----------------------------------------------------|-----------------------------------------------------------------------|
| `./libs/armeabi-v7a/gdb.setup`                      | configuration file for gdb client                                     |
| `./libs/armeabi-v7a/gdbserver`                      | gdbserver to be used on your Android device                           |
| `./libs/armeabi-v7a/libinterceptor_patrace.so`      | library with debug symbols stripped, to be used on the Android device |
| `./obj/local/armeabi-v7a/libinterceptor_patrace.so` | library with debug symbols, to be used by the gdb client              |

Upload `./libs/armeabi-v7a/libinterceptor_patrace.so` to your Android device to `/system/lib/egl`

    adb push ./libs/armeabi-v7a/libinterceptor_patrace.so /system/lib/egl

Upload `./libs/armeabi-v7a/gdbserver` to your Android device to `/data/local`

    adb push ./libs/armeabi-v7a/gdbserver /data/local

`./libs/armeabi-v7a/gdb.setup` is a configuration file for the gdb client. It instructs gdb where to search for libraries and source code. In this case, where to find the unstripped version of `libinterceptor_patrace.so`.

### Setup remote debugging

Forward port 5039 on your PC to port 5039 on your Android device:

    adb forward tcp:5039 tcp:5039

Open an adb shell:

    adb shell

Start the app that you want to trace on Android. Make sure that it is traced with `libinterceptor_patrace.so`. Find the app's process id by using `ps` in the adb shell.Start the gdb server on the Android device:

    /data/local/gdbserver :5039 --attach <YOUR-APP'S-PROCESS-ID>

The gdbserver is now listening for incoming connections.Open a new terminal on your desktop PC, and change directory to where you built the tracer (`content_capturing/patrace/project/android/egltrace`).You cannot use your normal `gdb` in order to remote debug the Android device. Instead, use a compatible gdb that is included in the Android NDK. Tell gdb to use the generated `gdb.setup file`. E.g.

    android-ndk-r8e/toolchains/arm-linux-androideabi-4.7/prebuilt/linux-x86_64/bin/arm-linux-androideabi-gdb -x libs/armeabi-v7a/gdb.setup

Once gdb is started, tell it to connect to port 5039:

    target remote :5039

List all loaded libraries:

    info sharedlibrary

Make sure that you find `libinterceptor_patrace.so` in the list returned. Also make sure that is prepended with the path of where it is located on your PC. Example output:

    ...
     No librs_jni.so
     No libandroid.so
     No libwebcore.so
     No libsoundpool.so
    0x5bd48350 0x5be51dc8 No /work/dev/arm/content_capturing/patrace/project/android/egltrace/obj/local/armeabi-v7a/libinterceptor_patrace.so
     No libUMP.so
     No libMali.so
     No libGLESv2_mali.so
    ...

"No" means that the symbols are not loaded. To load the symbols run the commmand:

    sharedlibrary

Perform the `info sharedlibraries` command again, and make sure the symbols are now loaded for `libinterceptor_patrace.so` To set a breakpoint in `eglSwapBuffers`:

    break eglSwapBuffers

Now continue the execution of the app:

    continue

gdb should now break inside the the eglSwapBuffers.Now you can perform gdb debugging as normal.**Optional:** In order to see function names for system libraries, you can copy them to 
your local machine:

    adb pull /system/bin/ ./obj/local/armeabi-v7a/
    adb pull /system/lib/ ./obj/local/armeabi-v7a/

### CGDB

If you want to debug with `cgdb` use the `-d` parameter to specify the arm compatible gdb:

    cgdb -d <PATH-TO-GDB>/arm-linux-androideabi-gdb -x libs/armeabi-v7a/gdb.setup

### Break on app start

Sometimes it is desirable to break as early as possible in a started app. If an app crashes immediately when you start it, you do not have the time to find its PID, attach the gdbserver to the PID, and connect the gbd client, set break points, and so on.The solution is to add an infinite loop. This makes the app hang and gives you time to find its PID. Add the following code somewhere in the tracer library. For example in the constructor of `DllLoadUloadHandler` in `src/tracer/egltrace.cpp`

    int i = 1;
    while (i) {}

Perform the following steps that are described in more detail above:

1.  Compile the library
2.  Upload it to the device
3.  Start the app on the device
4.  Find the PID of the app
5.  Start the gdbserver on the device
6.  Connect the gdb client the gdb server
7.  Load sharedlibaries on the gdb client

The app is stuck in the infinite loop. In order to continue the execution of app, perform these steps in the gdb client

1.  `continue`
2.  Ctrl+C
3.  `set var i = 0`
4.  `next`

The `next` command will make you end up on the statement **after** the infinite loop. Now it is possible to debug as normal.

libcollector documentation
==========================

libcollector is a library for doing performance measurements.

All collectors can be configured with the following JSON options:

| Option     | What it does                                                                    | Values        | Default                   |
|------------|---------------------------------------------------------------------------------|---------------|---------------------------|
| "required" | Aborts the program if initializing the collector fails.                         | true or false | false                     |
| "threaded" | Run the collector in a separate background thread.                              | true or false | false (true for ferret)   |
| "rate"     | When run in a background thread, how often to collect samples, in milliseconds. | integer       | 100 (200 for ferret)      |

Existing collectors:

| Name                 | What it does                                                                                                                                                                            | Unit             | Options                             
|----------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------|-------------------------------------|
| `perf`               | Gets CPU Counter from perf                                                                                                                                                              | Cycles           | "set", "event", "allthread". See detailed information in perf collector options. |
| `battery_temperature`| Gets the battery temperature                                                                                                                                                            | Celsius          |                                     |                                                                                          |
| `cpufreq`            | CPU frequencies. Each sample is the average frequency used since the last sampling point for each core. Then you also get the max of every core in `highest_avg` for your convenience. | Hz               |                                     
| `memfreq`            | Memory frequency                                                                                                                                                                        |  Hz              |                                     |                                                                                          |
| `memfreqdisplay`     | Memory frequency                                                                                                                                                                        |  Hz |                                     |                                                                                          |
| `memfreqint`         | Memory frequency                                                                                                                                                           |  Hz |                                     |                                                                                          |
| `gpu_active_time`    | Time GPU has been active                                                                                                                                                                |                  |                                     |                                                                                          |
| `gpu_suspended_time` | Time GPU has been suspended                                                                                                                                                             |                  |                                     |                                                                                          |
| `cpufreqtrans`       | Number of frequency transitions the CPU has made                                                                                                                                        | Transitions      |                                     |
| `debug`              | **EXPERIMENTAL**                                                                                                                                                                        |                  |                                     |                                                                                          |
| `streamline`         | Adds streamline markers to the execution                                                                                                                                                |  None            |                                     |
| `memory`             | Track amount of memory used                                                                                                                                                             | Kilobytes        |                                     | 
| `cputemp`            | CPU temperature                                                                                                                                                                         |                  |                                     | 
| `gpufreq`            | GPU frequency                                                                                                                                                                           |                  | 'path' : path to GPU frequency file |
| `procfs`             | Information from /procfs filesystem                                                                                                                                                     | Various          |                                     | 
| `rusage`             | Information from getrusage() system call                                                                                                                                                | Various          |                                     | 
| `power`              | TBD                                                                                                                                                                                     |                  | TBD                                 | 
| `ferret`             | Monitors CPU usage by polling system files. Gives coarse per thread CPU load statistics (cycles consumed, frequencies during the rune etc.) | Various | 'cpus': List of cpus to monitor. Example: cpus: [0, 2, 3, 5, 7], will monitor core 0, 2, 3, 5 and 7. All work done on the other cores will be ignored.<br>This defaults to all cores on the system if not set.<br><br>'enable_postprocessing': Boolean value. If this is set, the sampled results will be postprocessed at shutdown. Giving per. thread derived statistics like estimated CPU fps etc. Defaults to false.<br><br>'banned_threads': Only used when 'enable_postprocessing' is set to true. This is a list of thread names to exclude when generating derived statistics. Defaults to: 'banned_threads': ["ferret"], this will exclude the CPU overhead added by the ferret instrumentation.<br><br>'output_dir': Path to an existing directory where sample data will be stored. |


Example with libcollector
-------

The above names should be added as keys under the "collectors" dictionary in the input.json file:

    {
        "collectors": {
            "cpufreq": {
                "required": true,
                "rate": 100,
                "thread": true
            },
            "gpufreq": {
                "path": "/sys/kernel/gpu/gpu_clock"
            },
            "procfs": {},
            "rusage": {}
        },
        "file": "driver2.orig.gles3.pat",
        "frames": "1-191",
        "offscreen": true,
        "preload": true
    }


Generating CPU load with perf collector
------------------------------

A new GLES CPU performance measurement is provided with perf collector since r3p2.

Perf collector can also be configured with the following json options.

| Key                          | Type       | Optional | Description                                                                                                                                                                                                                            |
|------------------------------|------------|----------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| set                          | int        | no       | CPU counters set for a group of events. set 0~3 is reserved for specified event counters. 0:default for generalized hardware CPU events  1:CPU cache related  2:CPU bandwidth related  3:CPU bandwidth related on Cortex-A73. 4 and greater:customized CPU counter set.It should be used with "event" option to specify event counters.                                                                                                                                                                                                                             |
| event                        | jsonarray  | yes      | a group of event. Only works with set 4 or greater. set 0~3 will overwrite it with reserved event counter. If "set" is 0~3, just skip this option.                                                                                                                               |
| allthread                    | boolean    | yes      | true(default)/false. If true, the count includes events that happens in replay main threads as well as mali- background threads. If false, the count only includes events in replay main threads. Recommend setting it to true strongly to get data of main thread and background thread at the same time.                                                                                                                                                                                                                                                          |


Event option of perf collector can be configured with the following json options.

| Key                          | Type       | Optional | Description                                                                                                                                                                                                                            |
|------------------------------|------------|----------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| name                         | string     | no       | the PMU event mnemonic named by user. It's a field to classify the event output result in thread_data item.                                                                                                                                                                      |
| type                         | int        | no       | PMU event type. 0:PERF_TYPE_HARDWARE  1:PERF_TYPE_SOFTWARE  2:PERF_TYPE_TRACEPOINT  3:PERF_TYPE_HW_CACHE  4:PERF_TYPE_RAW  5:PERF_TYPE_BREAKPOINT                                                                                                                                |
| config                       | int        | no       | PMU event number. Should be decimal format, not in hex format. Please refer to your CPU reference manaul to get event number.                                                                                                                                                    |
| excludeUser                  | boolean    | yes      | true/false(default). If this is true, the count excludes events that happens in user space.                                                                                                                                                                                      |
| excludeKernel                | boolean    | yes      | true/false(default). If this is true, the count excludes events that happens in kernel space.                                                                                                                                                                                    |
| counterLen64bit              | int        | yes      | 1/0(default). 0 means using 32bit counters, 1 means using 64bit counters.                                                                                                                                                                                                        |


Example with perf collector
------------------------------

perf collector should be added as keys under the "collectors" dictionary in input.json file. "set", "event" and "allthread" should be added as keys under "perf" dictionary in input.json file.

    {
        "collectors": {
            "perf": {
                "set": 4,
                "event": [
                    {
                        "name": "CPUCyclesUser",
                        "type": 4,
                        "config": 17,
                        "excludeKernel": true
                    },
                    {
                        "name": "CPUCyclesKernel",
                        "type": 4,
                        "config": 17,
                        "excludeUser": true,
                    }
                ]
            }
        },
        "file": "driver2.orig.gles3.pat",
        "frames": "1-191",
        "preload": true
    }


Output result of perf collector
------------------------------

PerfCollector samples counter per frame for each thread. To reduce the size of output json file, it merges all replay main threads to a single entry named "replayMainThreads" in result file, also the same way for all background threads to a single entry named "backgroundThreads". Then it merges the main and background threads to a single entry named "allThreads".

    {
        "results" : [
            "frame_data": {
                "perf": {
                    "thread_data": [
                        {
                            "CCthread": "replayMainThreads",
                            "CPUCycleCount": [],
                            "CPUCyclesUser": [],
                            "CPUCyclesKernel": [],
                            "SUM": [
                                "CPUCycleCount": 6564218453,
                                "CPUCyclesUser": 6141323697,
                                "CPUCyclesKernel": 422510306
                            ]
                        },
                        {
                            "CCthread": "backgroundThreads",
                            "CPUCycleCount": [],
                            "CPUCyclesUser": [],
                            "CPUCyclesKernel": [],
                            "SUM": [
                                "CPUCycleCount": 171442785,
                                "CPUCyclesUser": 89874083,
                                "CPUCyclesKernel": 81563989
                            ]
                        },
                        {
                            "CCthread": "allThreads",
                            "CPUCycleCount": [],
                            "CPUCyclesUser": [],
                            "CPUCyclesKernel": [],
                            "SUM": [
                                "CPUCycleCount": 6735661238,
                                "CPUCyclesUser": 6231197780,
                                "CPUCyclesKernel": 504074295
                            ]
                        }
                    ]
                }
            }
        ]
    }


Generating CPU load statistics with ferret
------------------------------

You can get statistics for the CPU load (driver overhead) when running a trace by enabling the ferret libcollector module with postprocessing.

To do this, create a json parameter file similar to the one below (we will refer to it as parameters.json):

    {
        "collectors": {
            "ferret": {
                "enable_postprocessing": true,
                "output_dir": "<my_out_dir>"
            }
        },
        "file": <opath_to_my_trace_file>,
        "frames": "<start_frame>-<end_frame>",
        "offscreen": true,
        "preload": true
    }


Then run paretrace as follows:

    # Linux  
    paretrace -jsonParameters parameters.json results.json .  


    # Android (using adb)  
    adb shell am start -n com.arm.pa.paretrace/.Activities.RetraceActivity --es jsonData parameters.json --es resultFile results.json  


    # Android, using the (on device) android shell  
    am start -n com.arm.pa.paretrace/.Activities.RetraceActivity --es jsonData parameters.json --es resultFile results.json  


Once the run finishes the most relevant CPU statistics will be printed to stdout.

Detailed derived statistics will be available in the results.json file.

In depth (per. sample) data can be found in the `<my_out_dir>` folder specified in the parameters.json file.



The most interesting metrics are the following:

|Metric name stdout  | Metric name json | Calculated as	| Description |
|--------------------|------------------|---------------|-------------|
|CPU full system FPS@3GHz |	cpu_fps_full_system@3GHz | num_frames / (total_cpu_cycles / 3,000,000,000) | This metric represents the total amount of work done by the driver across all cores on the system.<br><br>Specifically, it is the maximum FPS the driver could deliver if it were to run on a single 3GHz CPU core comparable to the ones the CPU data was generated on.<br><br>This includes work done in all threads, and therefore it is not a good measure for the maximum throughput of the driver (as it is expected to be worse for multithreaded DDK's). Use it only as a indicator of how much work the driver does (and not necessarily how fast it does it). |
| CPU main thread FPS@3GHz | cpu_fps_main_thread@3GHz | num_frames / (heaviest_thread_cpu_cycles / 3,000,000,000) | This metric represents the work done by the heaviest thread in the driver.<br><br>This is the number that most accurately represents the maximum throughput CPU side. It will give you the maximum FPS the driver could deliver if the heaviest thread (assumed to also be the bottleneck) was to run on a 3GHz CPU core comparable to the ones the CPU data was generated on.<br><br>This number is the one to use if you are looking for a representation of the maximum FPS the target system could deliver if it was CPU bound. |



Getting robust results
----------------------

In order to get good results (with low error bars) it is essential that the content you are retracing runs for an extended period of time.

If the content in question has a high CPU load, running for 5+ seconds should give you good results. Still, running for longer will improve the quality of the data and reduce error bars.

If the content in question has a low CPU load, running for several minutes might be required.

In general, if the "active" times across all threads (as seen in the results.json file) is less than 0.2 for more than 4 cores (on a system with a clock tick time of 10ms) you should increase the runtime of your application until this is not the case.

A good rule of thumb is to ensure that all active times on all cores for all relevant threads is greater than the one divided by the clock ticks per seconf (`1 / _SC_CLK_TCK` on linux) times 20.

Note that this is rarely an issue for modern content. Ensuring a runtime of 30+ seconds will result in good data most of the time.

Also, you should configure your framerange such that all loading frame work is skipped (as loading assets is not relevant to the driver performance).



Calculating a CPU index number
------------------------------
A CPU index number is a single number that summarizes the CPU performance across multiple content entries. A good way to calculate this is to simply run a set of content entries and calculate the geometric mean of all the cpu_fps_main_thread@3GHz results.

This index can then be used to compare the results from different platforms with different drivers (assuming the content set is the same across all platforms and the CPU cores on them do not differ too much).
