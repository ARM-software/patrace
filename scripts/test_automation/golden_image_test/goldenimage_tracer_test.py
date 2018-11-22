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

    print("Using device: {}".format(device.model_name))

    app = Application(args.workspace, args.releases, args.repository)
    app.init(device)

    filepath = os.path.join(args.repository, tracedir, 'trace.pat')

    app.traceAndRetraceGoldenImages(filepath, snap_period, device)

    print("Success")

if __name__ == "__main__":
    main()
