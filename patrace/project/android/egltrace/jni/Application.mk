# This confidential and proprietary software may be used only as
# authorised by a licensing agreement from ARM Limited
# (C) COPYRIGHT 2011 ARM Limited
# ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorised
# copies and copies may only be made to the extent permitted
# by a licensing agreement from ARM Limited

APP_OPTIM := release

APP_CFLAGS   += -O3 -fvisibility=hidden -DGLES_CALLCONVENTION= -D__NDK_FPABI__=
APP_CPPFLAGS += -D__NDK_FPABI__= -fexceptions

APP_STL := c++_static
APP_ABI := armeabi-v7a arm64-v8a # x86
LOCAL_ARM_MODE := thumb
