#!/usr/bin/env python
from __future__ import print_function
import argparse
import sys
import datetime
import os
from adb_device import AdbDevice
from patracetools import headerparser
from patracetools import remap_egl
from retrace import retrace


def app_with_highest_cpu_usage(adb_device):
    output = adb_device.shell('top -n 1 -m 1')
    line = output.split('\n')[-2]

    # Example line:
    # 24426  0   9% S    19 1318516K 574116K  fg u0_a95   com.glbenchmark.glbenchmark27.corporate

    return line.split()[-1]


def find_app_line(ps_output, app_name):
    for line in ps_output.split('\n'):
        if not len(line.strip()):
            continue

        if app_name == line.split()[-1]:
            return line


def is_running(adb_device, app_name):
    return find_app_line(adb_device.shell('ps'), app_name) is not None


def get_pid(adb_device, app_name):
    # Example line:
    # root      25100 25085 1132   452   00000000 b6efcffc R ps
    app_line = find_app_line(adb_device.shell('ps'), app_name)

    return app_line.split()[1]


def kill_app(adb_device, app_name):
    pid = get_pid(adb_device, app_name)
    adb_device.sushell('kill {pid}'.format(pid=pid))


def find_closed_trace(adb_device):
    closed_tracefiles = [line.split()[-1]
                         for line in adb_device.logcat().split('\n')
                         if "Close trace file " in line]
    if closed_tracefiles:
        return closed_tracefiles[-1]


def trace(args, dc):
    if not args.appname:
        args.appname = app_with_highest_cpu_usage(dc)

    print("Do you want to set up tracing for for '{appame}' [y/N]?".format(appame=args.appname))

    if raw_input().lower() != 'y':
        return 1

    if is_running(dc, args.appname):
        kill_app(dc, args.appname)

    # Modify appList
    dc.shell('echo {appname} > /sdcard/appList.cfg'.format(appname=args.appname))
    dc.sushell('cp /sdcard/appList.cfg /system/lib/egl/')
    dc.shell('rm /sdcard/appList.cfg')
    dc.sushell('chmod 644 /system/lib/egl/appList.cfg')

    # Create appdir
    dirname = '/data/apitrace/{appname}'.format(appname=args.appname)
    dc.sushell('if [ ! -d {dirname} ]; then mkdir {dirname}; fi'.format(dirname=dirname))
    dc.sushell('chmod 777 {dirname}'.format(dirname=dirname))

    if args.snap:
        print("Enabling snapshot creation every {n}:th frame on thread {t}".format(
            n=args.snapperiod,
            t=args.snapthread,
        ))
        dc.sushell('echo snapEvery {t} {n} > /system/lib/egl/tracercmd.cfg'.format(
            n=args.snapperiod,
            t=args.snapthread,
        ))
        dc.sushell('rm /data/apitrace/snap/*')
    else:
        dc.sushell('echo '' > /system/lib/egl/tracercmd.cfg')

    print("Now you need to manually start {appname} in order to trace it.".format(appname=args.appname))
    print("To finish tracing, hit the ENTER key")

    dc.logcat_clear()

    raw_input()

    if is_running(dc, args.appname):
        kill_app(dc, args.appname)

    remote_trace_file = find_closed_trace(dc)

    if not remote_trace_file:
        print("It seems that no trace file was produced")
        return 1

    print("Pulling {}...".format(remote_trace_file))
    dc.pull(remote_trace_file)

    if args.snap:
        pass
        # Pull snapshots
        # adb pull snaps

    local_trace_file = os.path.basename(remote_trace_file)
    return local_trace_file


def update_header(dc, trace_file, appname):
    log = {
        'tracefile': trace_file,
        'package_name': appname,
    }

    log.update(dc.get_app_info(appname))

    parsed = headerparser.read_json_header(trace_file)

    parsed['androidMeta'] = log
    parsed['dateTraced'] = datetime.datetime.now().isoformat()

    headerparser.write_json_header(trace_file, parsed)


def main():
    parser = argparse.ArgumentParser(description='Set up application for tracing on device.'
                                     'If appname is not specified, the application that currently '
                                     'use most CPU will be used for tracing.')
    parser.add_argument('-a', '--appname', help='Application name. Example: com.example.foo')
    parser.add_argument('-d', '--device', help='The id of the device')
    parser.add_argument('-v', '--verbose', help='Print all adb commands', action='store_true')
    parser.add_argument('-s', '--snap', help='Enables snapshot creation', action='store_true')
    parser.add_argument('-t', '--snapthread', help='Use this thread when creating snapshots', type=int, default=0)
    parser.add_argument('-f', '--snapperiod', help='Use this frame period when creating snapshots. 1 means each frame', type=int, default=1)

    args = parser.parse_args()

    #trace_file = '/work/test/com.wb.goog.batman.brawler2013.3fixed.pat'
    dc = AdbDevice(args.device, system_write=True, verbose=args.verbose)
    trace_file = trace(args, dc)

    update_header(dc, trace_file, args.appname)

    trace_file_fixed = '.'.join(trace_file.split('.')[:-1]) + '.fixed.pat'

    print("Remap relevant calls to default thread id...")
    remap_egl.remap(trace_file, trace_file_fixed)

    if not retrace(trace_file_fixed):
        return 1

    print("Done")

if __name__ == "__main__":
    sys.exit(main())
