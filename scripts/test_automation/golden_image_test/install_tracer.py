#!/usr/bin/env python
from __future__ import print_function
import os
import argparse
from adb_device import AdbDevice


def update_fakedriver(adb_device, path):
    file_name = 'libGLES_wrapper.so'
    adb_device.push_lib(
        os.path.join(path, 'fakedriver', file_name),
        os.path.join('/system/lib/egl', file_name),
    )


def update_interceptor(adb_device, path):
    file_name = 'libinterceptor_patrace_arm.so'
    adb_device.push_lib(
        os.path.join(path, 'egltrace', file_name),
        os.path.join('/system/lib/egl', file_name),
    )


def get_android_sw_path(base_path, version):
    if version == 'trunk':
        return os.path.join(base_path, 'install/patrace/android/release')

    if version in ['r0p2', 'r0p3', 'r0p4']:
        release_dir = 'release'
    else:
        release_dir = ''

    return os.path.join(base_path, 'extracted', version, 'patrace/android', release_dir)


def main():
    parser = argparse.ArgumentParser(description='Install tracer and fakedriver on device. Use with care.')
    parser.add_argument('version', help='Release version. For example: r0p6')
    parser.add_argument('-d', '--device', help='The id of the device')
    parser.add_argument('-v', '--verbose', help='Print all adb commands', action='store_true')
    parser.add_argument('-p', '--releasepath',
                        help='The path to where the patrace releases are located',
                        default='/projects/pr368/patrace_releases')
    args = parser.parse_args()

    path = get_android_sw_path(args.releasepath, args.version)

    print("Using path: {}".format(path))

    dc = AdbDevice(args.device, system_write=True, verbose=args.verbose)
    update_fakedriver(dc, path)
    update_interceptor(dc, path)

if __name__ == "__main__":
    main()
