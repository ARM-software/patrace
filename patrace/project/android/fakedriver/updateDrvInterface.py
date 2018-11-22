#!/usr/bin/env python
import os;

# remount to make sure that the destination dir is writeable
os.system('adb remount')

# update wrapper
os.system('adb push ./libs/armeabi-v7a/libEGL_wrapper.so /system/lib/egl')
os.system('adb push ./libs/armeabi-v7a/libGLESv1_CM_wrapper.so /system/lib/egl')
os.system('adb push ./libs/armeabi-v7a/libGLESv2_wrapper.so /system/lib/egl')

# Please note that, the following line is
# *specific* for Vithar or JB
#os.system('adb push ./libs/armeabi-v7a/libGLES_wrapper.so /system/lib/egl')

# update egl.cfg
os.system('adb push egl.cfg /system/lib/egl')

# reboot the Android system to enable the update
#os.system('adb reboot')
