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


Using as a layer (Vulkan only)
==============================

Once built, the layer and json manifest will be in <build_dir>/implicit_layer.d

Set the following env. vars to enable the layer on linux:

export VK_LAYER_PATH=<build_dir>/implicit_layer.d
export VK_INSTANCE_LAYERS=VK_LAYER_ARM_libcollector
export VK_DEVICE_LAYERS=VK_LAYER_ARM_libcollector
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$VK_LAYER_PATH

Then set the following env var to point to your libcollector JSON configuration file:

export VK_LIBCOLLECTOR_CONFIG_PATH=<path_to_json>

Then run your app as normal. One result file will be created per device in your application, and by default the results are written to the run directory.
To override this, set the "result_file_basename" field in the config json.

JSON interface (layer specific)
---------------------------

When using the layer, the JSON config has a extended set of configurations options.

Note that all parameters are optional except for the "collectors" field. The values below are the Linux default values given if nothing else is specified.

{
	"collectors": {
		"cpufreq" : {
	        "required" : true,
	        "threaded" : true,
	        "rate": 100,
	    }
	},
	"result_file_basename": "results.json", # The basename for each result file. There will be one per. vulkan device, expanded as: results_device_<n>.json
	"frame_trigger_function": "vkQueuePresentKHR", # The function call that will trigger a sample for all synchronous collectors.
	"num_subframes_per_frame": 1, # The number of times the "frame_trigger_function" must be called before a sample is triggered and a frame is counted. (2 would be every other call, 3 every third call etc.)
	"start_frame": 0, # The frame at which sampling will start.
	"end_frame": 99999 # The frame at which sampling will end.
}
