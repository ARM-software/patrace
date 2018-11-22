# This confidential and proprietary software may be used only as
# authorised by a licensing agreement from ARM Limited
# (C) COPYRIGHT 2011 ARM Limited
# ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorised
# copies and copies may only be made to the extent permitted
# by a licensing agreement from ARM Limited

APP_PLATFORM := android-19
APP_OPTIM := release

ifeq ($(APP_ABI),armeabi-v7a)
	APP_CFLAGS += -U__ARM_ARCH_5__ -U__ARM_ARCH_5T__ \
			  -U__ARM_ARCH_5E__ -U__ARM_ARCH_5TE__ \
			  -march=armv7-a -mfpu=vfp
endif

APP_CFLAGS += -O3 -DGLES_CALLCONVENTION= -D__NDK_FPABI__= -fvisibility=hidden
APP_CPPFLAGS      += -D__NDK_FPABI__= -fexceptions
APP_CPP_FEATURES  += exceptions

APP_STL := gnustl_static
APP_ABI := armeabi-v7a arm64-v8a x86
APP_ALLOW_MISSING_DEPS := true
LOCAL_ARM_MODE := thumb
NDK_TOOLCHAIN_VERSION := 4.9
