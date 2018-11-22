#!/usr/bin/env python
from __future__ import print_function
import os
import argparse
import traceback
from adb_device import AdbDevice
from golden_image_comparison_test import Application

def main():
    parser = argparse.ArgumentParser(description='Golden image test script')
    parser.add_argument('traceperiod', help=(
        'Trace dir and snapshots period divided by #.'
        ' The trace dir, is the directory name in the golden repository'
        ' Example: angrybirds_nexus10_1080p_t604#50'
    ))
    parser.add_argument('devices', help='Device to id map. Example: "nexus10:R32D1034QMN,note3:RCV32342"')
    parser.add_argument('workspace', help='Path to the jenkins workspace')
    parser.add_argument('releases', help='Path to the patrace release directory')
    parser.add_argument('repository', help='Path to the golden image repository')
    args = parser.parse_args()

    tracedir, snap_period = args.traceperiod.split('#')
    device_name = tracedir.split('_')[1]
    devices = dict([pair.split(':') for pair in args.devices.split(',')])

    device = AdbDevice(devices[device_name], device_name, verbose=True)

    app = Application(args.workspace, args.releases, args.repository)
    app.init(device)

    filepath = os.path.join(args.repository, tracedir, 'trace.pat')

    for branchName, branchPath in [
        ("Development", app.paths['developmentPath']),
    ]:
        device.shell('logcat -c')
        reference_snaps_path = os.path.join(os.path.dirname(filepath), 'snap')

        if not os.path.exists(reference_snaps_path):
            raise Exception("Path {0} does not exists".format(reference_snaps_path))

        try:
            app.retraceGoldenImages(filepath, reference_snaps_path, snap_period, device, branchPath)
        except Exception as e:
            print(device.shell('logcat -d'))
            traceback.print_exc()
            raise e

        app.customPrint(branchName + "-branch succeeded for " + filepath)

    app.cleanup(device)

    print("Success")

if __name__ == "__main__":
    main()
