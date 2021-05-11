# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# LOCAL_PATH refers to the jni directory
LOCAL_PATH := $(call my-dir)

##############################################################
# Target: libcollector static library
include $(CLEAR_VARS)

LOCAL_MODULE    	:= collector_android
LOCAL_SRC_FILES 	:=  \
                    ../../interface.cpp \
                    ../../collectors/collector_utility.cpp \
                    ../../collectors/cputemp.cpp \
                    ../../collectors/ferret.cpp \
                    ../../collectors/rusage.cpp \
                    ../../collectors/streamline.cpp \
                    ../../collectors/streamline_annotate.cpp \
                    ../../collectors/memory.cpp \
                    ../../collectors/cpufreq.cpp \
                    ../../collectors/gpufreq.cpp \
                    ../../collectors/perf.cpp \
                    ../../collectors/power.cpp \
                    ../../collectors/procfs_stat.cpp \
                    ../../collectors/hwcpipe.cpp \
                    ../../collectors/mali_counters.cpp \
                    ../../thirdparty/jsoncpp/json_writer.cpp \
                    ../../thirdparty/jsoncpp/json_reader.cpp \
                    ../../thirdparty/jsoncpp/json_value.cpp

LOCAL_C_INCLUDES 	:= \
                    $(LOCAL_PATH)/../../collectors \
                    $(LOCAL_PATH)/../../thirdparty \
                    $(LOCAL_PATH)/../../thirdparty/jsoncpp \
                    $(LOCAL_PATH)/../../thirdparty/jsoncpp/json \
                    $(LOCAL_PATH)/../..

LOCAL_CFLAGS 		:= -O3 -frtti -D__arm__ -D__gnu_linux__
LOCAL_CPPFLAGS          += -std=c++11

ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_CFLAGS		+= -Wno-attributes
endif

LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../thirdparty/

include $(BUILD_STATIC_LIBRARY)

LOCAL_MODULE := burrow
LOCAL_MODULE_FILENAME := burrow
LOCAL_SRC_FILES := ../../burrow.cpp
LOCAL_C_INCLUDES := \
                    $(LOCAL_PATH)/../../collectors \
                    $(LOCAL_PATH)/../../thirdparty \
                    $(LOCAL_PATH)/../../thirdparty/jsoncpp \
                    $(LOCAL_PATH)/../../thirdparty/jsoncpp/json \
                    $(LOCAL_PATH)/../..

LOCAL_LDLIBS    := -L$(SYSROOT)/usr/lib -llog -latomic

LOCAL_STATIC_LIBRARIES := android_native_app_glue collector_android

include $(BUILD_EXECUTABLE)
