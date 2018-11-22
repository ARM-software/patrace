#!/usr/bin/env python

import os, sys;

serial_no = ''
if len(sys.argv) > 1:
    serial_no = sys.argv[1]

# remount to make sure that the destination dir is writeable
os.system('adb%sremount' % ((' -s '+serial_no + ' ') if serial_no else ' '))

# update tracer
os.system('adb%spush ./libs/armeabi-v7a/libinterceptor_patrace.so /system/lib/egl/' % ((' -s '+serial_no + ' ') if serial_no else ' '))

os.system('adb%sshell "echo \'/system/lib/egl/libinterceptor_patrace.so\' > /system/lib/egl/interceptor.cfg"' % ((' -s '+serial_no + ' ') if serial_no else ' '))

# reboot the Android system to enable the update
#os.system('adb reboot')
