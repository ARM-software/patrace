What
====

libcollector is a library for making performance measurements.

The tool burrow is included to make 200Hz CPU Load measurements for subsequent analysis by
ferret.py.

Build
=====

Linux
-----

mkdir build
cd build
cmake ..
make -j 8


Android
-------

You can build for android using ndk-build (deprecated for later android version) or with gradle.
Both require a local checkout of the android-ndk, you should set the ANDROID_NDK environment variable to point to your local android-ndk folder.


Android with ndk-build
-------

cd android
ndk-build -j 8


Android with gradle
-------
cd android/gradle
./gradlew assembleRelease


Usage
=====

Direct function calls
---------------------

JSON interface
--------------

Example:

{
    "cpufreq" : {
        "required" : true,
        "threaded" : true,
        "rate": 100,
    }
}
