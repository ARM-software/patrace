
LOCAL_PATH:=$(call my-dir)/../../../../src

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
PA_BUILD_64BIT := -DPLATFORM_64BIT
else
PA_BUILD_64BIT :=
endif

################################################################
# Target: md5
include $(CLEAR_VARS)

LOCAL_MODULE    	:= md5

LOCAL_SRC_FILES 	:= \
    ../../thirdparty/md5/md5.c

LOCAL_C_INCLUDES 	:=
LOCAL_CFLAGS 		:= -frtti -D__arm__ -D__gnu_linux__

include $(BUILD_STATIC_LIBRARY)

################################################################
# Target: jsoncpp
include $(CLEAR_VARS)

LOCAL_MODULE    	:= jsoncpp

LOCAL_SRC_FILES 	:= \
    ../../thirdparty/jsoncpp/src/lib_json/json_writer.cpp \
    ../../thirdparty/jsoncpp/src/lib_json/json_reader.cpp \
    ../../thirdparty/jsoncpp/src/lib_json/json_value.cpp


LOCAL_C_INCLUDES 	:= $(LOCAL_PATH)/../../thirdparty/jsoncpp/include
LOCAL_CFLAGS 		:= -frtti -D__arm__ -D__gnu_linux__ -DJSON_USE_EXCEPTION=0

include $(BUILD_STATIC_LIBRARY)

################################################################
# Target: libpng
include $(CLEAR_VARS)

LOCAL_MODULE        := libpng

LOCAL_SRC_FILES     := \
    ../../thirdparty/libpng/png.c \
    ../../thirdparty/libpng/pngerror.c \
    ../../thirdparty/libpng/pngget.c \
    ../../thirdparty/libpng/pngmem.c \
    ../../thirdparty/libpng/pngpread.c \
    ../../thirdparty/libpng/pngread.c \
    ../../thirdparty/libpng/pngrio.c \
    ../../thirdparty/libpng/pngrtran.c \
    ../../thirdparty/libpng/pngrutil.c \
    ../../thirdparty/libpng/pngset.c \
    ../../thirdparty/libpng/pngtrans.c \
    ../../thirdparty/libpng/pngwio.c \
    ../../thirdparty/libpng/pngwrite.c \
    ../../thirdparty/libpng/pngwtran.c \
    ../../thirdparty/libpng/pngwutil.c

LOCAL_C_INCLUDES    :=
LOCAL_CFLAGS        :=

include $(BUILD_STATIC_LIBRARY)

################################################################
# Target: snappy
include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc
LOCAL_MODULE        := snappy

LOCAL_SRC_FILES     := \
    ../../thirdparty/snappy/snappy.cc \
    ../../thirdparty/snappy/snappy-sinksource.cc \
    ../../thirdparty/snappy/snappy-stubs-internal.cc \
    ../../thirdparty/snappy/snappy-c.cc

LOCAL_C_INCLUDES    :=
LOCAL_CFLAGS        := -frtti

include $(BUILD_STATIC_LIBRARY)

################################################################
# Target: libcollector
# This is copied and adapted from android/jni/Android.mk in libcollector
include $(CLEAR_VARS)

LOCAL_MODULE            := collector_android
LOCAL_SRC_FILES         :=  \
                    ../../thirdparty/libcollector/interface.cpp \
                    ../../thirdparty/libcollector/collectors/collector_utility.cpp \
                    ../../thirdparty/libcollector/collectors/cputemp.cpp \
                    ../../thirdparty/libcollector/collectors/debug.cpp \
                    ../../thirdparty/libcollector/collectors/ferret.cpp \
                    ../../thirdparty/libcollector/collectors/rusage.cpp \
                    ../../thirdparty/libcollector/collectors/streamline.cpp \
                    ../../thirdparty/libcollector/collectors/streamline_annotate.cpp \
                    ../../thirdparty/libcollector/collectors/memory.cpp \
                    ../../thirdparty/libcollector/collectors/perf.cpp \
                    ../../thirdparty/libcollector/collectors/cpufreq.cpp \
                    ../../thirdparty/libcollector/collectors/gpufreq.cpp \
                    ../../thirdparty/libcollector/collectors/power.cpp \
                    ../../thirdparty/libcollector/collectors/procfs_stat.cpp \
                    ../../thirdparty/libcollector/collectors/hwcpipe.cpp \
                    ../../thirdparty/libcollector/collectors/mali_counters.cpp \
                    ../../thirdparty/libcollector/thirdparty/jsoncpp/json_writer.cpp \
                    ../../thirdparty/libcollector/thirdparty/jsoncpp/json_reader.cpp \
                    ../../thirdparty/libcollector/thirdparty/jsoncpp/json_value.cpp

LOCAL_C_INCLUDES        := \
                    $(LOCAL_PATH)/../../thirdparty/libcollector \
                    $(LOCAL_PATH)/../../thirdparty/libcollector/collectors \
                    $(LOCAL_PATH)/../../thirdparty/libcollector/thirdparty \
                    $(LOCAL_PATH)/../../thirdparty/libcollector/thirdparty/jsoncpp

LOCAL_CFLAGS            := -O3 -frtti -D__arm__ -D__gnu_linux__ -pthread
LOCAL_CPPFLAGS          += -std=c++11
LOCAL_LINKFLAGS         += -pthread

ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_CFLAGS            += -Wno-attributes
endif

LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../thirdparty/libcollector/thirdparty/

include $(BUILD_STATIC_LIBRARY)

LOCAL_MODULE := burrow
LOCAL_MODULE_FILENAME := burrow
LOCAL_SRC_FILES := ../../thirdparty/libcollector/burrow.cpp
LOCAL_C_INCLUDES := \
                    $(LOCAL_PATH)/../../thirdparty/libcollector/collectors \
                    $(LOCAL_PATH)/../../thirdparty/libcollector/thirdparty \
                    $(LOCAL_PATH)/../../thirdparty/libcollector/thirdparty/jsoncpp \
                    $(LOCAL_PATH)/../..

LOCAL_LDLIBS    := -L$(SYSROOT)/usr/lib -nodefaultlibs -lc -lm -ldl -llog -latomic
LOCAL_STATIC_LIBRARIES := android_native_app_glue collector_android

include $(BUILD_EXECUTABLE)

################################################################
# Target: common
include $(CLEAR_VARS)

LOCAL_MODULE        := common

LOCAL_SRC_FILES     := \
    common/memory.cpp \
    common/trace_callset.cpp \
    common/os_posix.cpp \
    common/os_thread_linux.cpp \
    common/api_info_auto.cpp \
    common/api_info.cpp \
    common/in_file_mt.cpp \
    common/in_file_ra.cpp \
    common/in_file.cpp \
    common/out_file.cpp \
    common/memoryinfo.cpp \
    common/call_parser.cpp \
    common/image.cpp \
    common/image_bmp.cpp \
    common/image_png.cpp \
    common/image_pnm.cpp \
    common/base64.cpp \
    common/gl_extension_supported.cpp \
    common/library.cpp

LOCAL_C_INCLUDES    := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/common \
    $(LOCAL_PATH)/../../thirdparty \
    $(LOCAL_PATH)/../../thirdparty/egl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/opengl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/opencl-headers \
    $(LOCAL_PATH)/../../thirdparty/snappy \
    $(LOCAL_PATH)/../../thirdparty/libpng
LOCAL_CFLAGS        := -frtti $(PA_BUILD_64BIT) -DRETRACE
LOCAL_CPPFLAGS      += -std=c++11

include $(BUILD_STATIC_LIBRARY)

################################################################
# Target: graphicbuffer
include $(CLEAR_VARS)

LOCAL_MODULE    	:= graphicbuffer

LOCAL_SRC_FILES     := \
	graphic_buffer/DynamicLibrary.cpp \
	graphic_buffer/GraphicBuffer.cpp

LOCAL_C_INCLUDES 	:= \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/../../thirdparty \
    $(LOCAL_PATH)/../../thirdparty/snappy \
    $(LOCAL_PATH)/../../thirdparty/libpng \
    $(LOCAL_PATH)/../../thirdparty/egl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/opencl-headers \
    $(LOCAL_PATH)/../../thirdparty/opengl-registry/api

LOCAL_CFLAGS 		:= -frtti -D__arm__ -D__gnu_linux__ $(PA_BUILD_64BIT) -Wno-attributes
LOCAL_CPPFLAGS          += -std=c++11
LOCAL_CFLAGS_arm   += -U__ARM_ARCH_5__ -U__ARM_ARCH_5T__ \
	      -U__ARM_ARCH_5E__ -U__ARM_ARCH_5TE__ \
	      -march=armv7-a -mfpu=vfp

include $(BUILD_STATIC_LIBRARY)

################################################################
# Target: eglretrace
include $(CLEAR_VARS)

LOCAL_MODULE        := eglretrace

LOCAL_SRC_FILES     := \
    dispatch/eglproc_retrace.cpp \
    dispatch/eglproc_auto.cpp \
    retracer/retracer.cpp \
    retracer/retrace_api.cpp \
    retracer/retrace_gles_auto.cpp \
    retracer/retrace_egl.cpp \
    retracer/eglconfiginfo.cpp \
    retracer/glws.cpp \
    retracer/glws_egl.cpp \
    retracer/glws_egl_android.cpp \
    retracer/state.cpp \
    retracer/forceoffscreen/offscrmgr.cpp \
    retracer/forceoffscreen/quad.cpp \
    retracer/glstate_images.cpp \
    retracer/trace_executor.cpp \
    helper/states.cpp \
    helper/shaderutility.cpp \
    helper/depth_dumper.cpp \
    common/gl_utility.cpp \
    helper/shadermod.cpp \
    ../project/android/eglretrace/jni/NativeAPI.cpp

LOCAL_C_INCLUDES    := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/common \
    $(LOCAL_PATH)/../../thirdparty \
    $(LOCAL_PATH)/../../thirdparty/libcollector \
    $(LOCAL_PATH)/../../thirdparty/egl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/opencl-headers \
    $(LOCAL_PATH)/../../thirdparty/opengl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/snappy

LOCAL_CFLAGS        := -Wall -frtti -DRETRACE $(PA_BUILD_64BIT) -Wno-attributes -pthread
LOCAL_CPPFLAGS      += -std=c++11

LOCAL_STATIC_LIBRARIES := common graphicbuffer snappy md5 jsoncpp png collector_android
LOCAL_LDLIBS        := -nodefaultlibs -lc -lm -llog -landroid -ldl -lz -pthread

include $(BUILD_SHARED_LIBRARY)
