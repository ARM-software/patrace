
LOCAL_PATH:=$(call my-dir)/../../../../src

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
PA_BUILD_64BIT := -DPLATFORM_64BIT
else
PA_BUILD_64BIT :=
endif

################################################################
# Target: EGL wrapper
include $(CLEAR_VARS)

LOCAL_MODULE    := libEGL_wrapper_$(TARGET_ARCH)

LOCAL_SRC_FILES := \
    common/library.cpp \
    common/os_posix.cpp \
    fakedriver/common.cpp \
    fakedriver/egl/auto.cpp \
    fakedriver/egl/manual.cpp \
    fakedriver/egl/fps_log.cpp \
    fakedriver/egl/proc.cpp

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/../../thirdparty/opengl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/egl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/libcollector/external/jsoncpp/include \
    $(LOCAL_PATH)/../../thirdparty/opencl-headers

LOCAL_CFLAGS    := -O3 -D__arm__ -D__gnu_linux__ $(PA_BUILD_64BIT) -fvisibility=hidden -Wno-attributes
LOCAL_LDLIBS    := -nodefaultlibs -lc -lm -lz -llog -ldl
LOCAL_CPPFLAGS  += -std=c++11
LOCAL_CFLAGS_arm += -U__ARM_ARCH_5__ -U__ARM_ARCH_5T__ \
	      -U__ARM_ARCH_5E__ -U__ARM_ARCH_5TE__ \
	      -march=armv7-a -mfpu=vfp

include $(BUILD_SHARED_LIBRARY)


################################################################
# Target: GLES 1 wrapper
include $(CLEAR_VARS)

LOCAL_MODULE    := libGLESv1_CM_wrapper_$(TARGET_ARCH)

LOCAL_SRC_FILES := \
    common/library.cpp \
    common/os_posix.cpp \
    fakedriver/common.cpp \
    fakedriver/gles1/auto.cpp \
    fakedriver/gles1/proc.cpp

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/../../thirdparty/opengl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/egl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/libcollector/external/jsoncpp/include \
    $(LOCAL_PATH)/../../thirdparty/opencl-headers

LOCAL_CFLAGS    := -O3 -D__arm__ -D__gnu_linux__ $(PA_BUILD_64BIT) -fvisibility=hidden -Wno-attributes
LOCAL_LDLIBS    := -nodefaultlibs -lc -lm -lz -llog -ldl
LOCAL_CPPFLAGS  += -std=c++11
LOCAL_CFLAGS_arm += -U__ARM_ARCH_5__ -U__ARM_ARCH_5T__ \
	      -U__ARM_ARCH_5E__ -U__ARM_ARCH_5TE__ \
	      -march=armv7-a -mfpu=vfp

include $(BUILD_SHARED_LIBRARY)


################################################################
# Target: GLES 2 wrapper
include $(CLEAR_VARS)

LOCAL_MODULE    := libGLESv2_wrapper_$(TARGET_ARCH)

LOCAL_SRC_FILES := \
    common/library.cpp \
    common/os_posix.cpp \
    fakedriver/common.cpp \
    fakedriver/gles2/auto.cpp \
    fakedriver/gles2/proc.cpp

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/../../thirdparty/opengl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/egl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/libcollector/external/jsoncpp/include \
    $(LOCAL_PATH)/../../thirdparty/opencl-headers

LOCAL_CFLAGS    := -O3 -D__arm__ -D__gnu_linux__ $(PA_BUILD_64BIT) -fvisibility=hidden -Wno-attributes
LOCAL_LDLIBS    := -nodefaultlibs -lc -lm -lz -llog -ldl
LOCAL_SHARED_LIBRARIES := libEGL_wrapper_$(TARGET_ARCH)
LOCAL_CPPFLAGS  += -std=c++11
LOCAL_CFLAGS_arm += -U__ARM_ARCH_5__ -U__ARM_ARCH_5T__ \
	      -U__ARM_ARCH_5E__ -U__ARM_ARCH_5TE__ \
	      -march=armv7-a -mfpu=vfp

include $(BUILD_SHARED_LIBRARY)


################################################################
# Target: Single wrapper
include $(CLEAR_VARS)

LOCAL_MODULE    := libGLES_wrapper_$(TARGET_ARCH)

LOCAL_SRC_FILES := \
    common/library.cpp \
    common/os_posix.cpp \
    fakedriver/common.cpp \
    fakedriver/egl/manual.cpp \
    fakedriver/egl/fps_log.cpp \
    fakedriver/single/auto.cpp \
    fakedriver/single/proc.cpp

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/../../thirdparty/opengl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/egl-registry/api \
    $(LOCAL_PATH)/../../thirdparty/opencl-headers \
    $(LOCAL_PATH)/../../thirdparty/libcollector/external/jsoncpp/include \
    $(LOCAL_PATH)/fakedriver/egl

LOCAL_CFLAGS    := -O3 -D__arm__ -D__gnu_linux__ $(PA_BUILD_64BIT) -fvisibility=hidden -Wno-attributes
LOCAL_LDLIBS    := -nodefaultlibs -lc -lm -lz -llog -ldl
LOCAL_CPPFLAGS  += -std=c++11
LOCAL_CFLAGS_arm += -U__ARM_ARCH_5__ -U__ARM_ARCH_5T__ \
	      -U__ARM_ARCH_5E__ -U__ARM_ARCH_5TE__ \
	      -march=armv7-a -mfpu=vfp

include $(BUILD_SHARED_LIBRARY)
