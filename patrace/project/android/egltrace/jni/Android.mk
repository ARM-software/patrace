LOCAL_PATH :=$(call my-dir)/../../../../src

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
LOCAL_CFLAGS 		:= -frtti -D__arm__ -D__gnu_linux__ -Wno-attributes
LOCAL_CFLAGS_arm   += -U__ARM_ARCH_5__ -U__ARM_ARCH_5T__ \
	      -U__ARM_ARCH_5E__ -U__ARM_ARCH_5TE__ \
	      -march=armv7-a -mfpu=vfp

include $(BUILD_STATIC_LIBRARY)

################################################################
# Target: snappy
include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc
LOCAL_MODULE    	:= snappy

LOCAL_SRC_FILES 	:= \
    ../../thirdparty/snappy/snappy.cc \
    ../../thirdparty/snappy/snappy-sinksource.cc \
    ../../thirdparty/snappy/snappy-stubs-internal.cc \
    ../../thirdparty/snappy/snappy-c.cc

LOCAL_C_INCLUDES 	:=
LOCAL_CFLAGS 		:= -frtti -D__arm__ -D__gnu_linux__ -Wno-attributes
LOCAL_CPPFLAGS      += -std=c++11
LOCAL_CFLAGS_arm   += -U__ARM_ARCH_5__ -U__ARM_ARCH_5T__ \
	      -U__ARM_ARCH_5E__ -U__ARM_ARCH_5TE__ \
	      -march=armv7-a -mfpu=vfp

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
LOCAL_CFLAGS 		:= -frtti -D__arm__ -D__gnu_linux__ -DJSON_USE_EXCEPTION=0 -Wno-attributes
LOCAL_CFLAGS_arm   += -U__ARM_ARCH_5__ -U__ARM_ARCH_5T__ \
	      -U__ARM_ARCH_5E__ -U__ARM_ARCH_5TE__ \
	      -march=armv7-a -mfpu=vfp

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
LOCAL_CFLAGS        :=  -Wno-attributes
LOCAL_MULTILIB := both
LOCAL_CFLAGS_arm   += -U__ARM_ARCH_5__ -U__ARM_ARCH_5T__ \
	      -U__ARM_ARCH_5E__ -U__ARM_ARCH_5TE__ \
	      -march=armv7-a -mfpu=vfp

include $(BUILD_STATIC_LIBRARY)

################################################################
# Target: common
include $(CLEAR_VARS)

LOCAL_MODULE    	:= common

LOCAL_SRC_FILES     := \
    common/memory.cpp \
    common/trace_callset.cpp \
    common/os_posix.cpp \
    common/api_info_auto.cpp \
    common/api_info.cpp \
    common/in_file_mt.cpp \
    common/in_file_ra.cpp \
    common/out_file.cpp \
    common/image.cpp \
    common/image_bmp.cpp \
    common/image_png.cpp \
    common/image_pnm.cpp \
    common/gl_extension_supported.cpp \
    common/library.cpp

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
# Target: egltrace
include $(CLEAR_VARS)

LOCAL_MODULE    	:= interceptor_patrace_$(TARGET_ARCH)

LOCAL_SRC_FILES 	:= \
    tracer/egltrace.cpp \
    tracer/egltrace_auto.cpp \
    tracer/tracerparams.cpp \
    tracer/interactivecmd.cpp \
    tracer/glstate_images.cpp \
    tracer/path.cpp \
    helper/paramsize.cpp \
    dispatch/eglproc_trace.cpp \
    dispatch/eglproc_auto.cpp \
    helper/states.cpp \
    common/gl_utility.cpp \
    helper/eglsize.cpp \
    helper/shaderutility.cpp

LOCAL_C_INCLUDES 	:= \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/common \
    $(LOCAL_PATH)/../../thirdparty \
    $(LOCAL_PATH)/../../thirdparty/egl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/opencl-headers \
    $(LOCAL_PATH)/../../thirdparty/opengl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/snappy

LOCAL_CFLAGS 		:= -frtti -D__arm__ -D__gnu_linux__ -Wno-attributes
LOCAL_CPPFLAGS          += -std=c++11
LOCAL_CFLAGS_arm   += -U__ARM_ARCH_5__ -U__ARM_ARCH_5T__ \
	      -U__ARM_ARCH_5E__ -U__ARM_ARCH_5TE__ \
	      -march=armv7-a -mfpu=vfp

LOCAL_STATIC_LIBRARIES := common graphicbuffer snappy md5 jsoncpp png
LOCAL_LDLIBS        := -nodefaultlibs -lc -lm -llog -ldl -lz

include $(BUILD_SHARED_LIBRARY)
