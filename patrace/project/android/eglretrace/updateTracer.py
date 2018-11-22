#!/usr/bin/env python

import sys
import os

script_dir = os.path.dirname(os.path.realpath(__file__))

script = """
echo Remount file system
adb {serial} remount
echo Uninstall old patracer
adb {serial} shell pm uninstall -k com.arm.pa.paretrace com.arm.pa.paretrace
echo Install new patracer
adb {serial} install {script_dir}/bin/eglretrace-release.apk
echo Done
"""


serial_parameter = ''
if len(sys.argv) > 1:
    serial_parameter = '-s ' + sys.argv[1]

for line in script.split('\n')[1:-1]:
    command = line.format(
        serial=serial_parameter,
        script_dir=script_dir
    )

    os.system(command)
