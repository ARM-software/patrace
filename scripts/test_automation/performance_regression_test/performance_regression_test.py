#!/usr/bin/env python
import os
import io
import sys
import shlex
import subprocess
import time
import json

developmentPath = "patrace/android/release/"
releasePath = "/work/jenkins/patrace_releases/extracted/latest/patrace/android/"
traceListPath = "content_capturing/scripts/test_automation/performance_regression_test/performanceTraceList.cfg"
deviceID = "4d0062ef602590b3"
android_version = "J"
selection = "-s " + deviceID
threshold = 0.9
cooldown = 120  # Cooldown time in seconds

# Whether or not to 'su -s /system/bin/sh -c'-prefix all commands run with adb shell
suPrefixCmd = True

def get_apk_in_dir(dir):
    apkname = [name for name in os.listdir(dir) if name.endswith('.apk')][0]
    return os.path.join(dir, apkname)


def execute_command(command):
    ret_code = os.system(command)

    if ret_code != 0:
        raise Exception("The command '{}' returned error code: {}".format(command, ret_code))

def setTargetDevice(newDeviceID):
    global deviceID
    deviceID = newDeviceID
    global selection
    selection = "-s " + deviceID

def commandToDeviceCommand(cmd):
    if suPrefixCmd:
        # Need sh to be able to start intents (am start) when using su -c
        cmd = "su -s /system/bin/sh -c " + cmd
    return "adb " + selection + " shell " + cmd


def onDevice(cmd):
    fullcmd = commandToDeviceCommand(cmd)
    sys.stderr.write(fullcmd + "\n")
    execute_command(fullcmd)


def onDeviceWithOutput(cmd):
    deviceCmd = commandToDeviceCommand(cmd)
    args = shlex.split(deviceCmd)
    return subprocess.check_output(args)

def devicePush(local, remote, filename=None):
    """Push local file at "local" to dir "remote" on device with name "filename" """

    if filename is None:
        filename = os.path.basename(local)
    pushCmd = "adb " + selection + " push " + local + " " + "/sdcard/tmpFile"
    sys.stderr.write(pushCmd + "\n")
    execute_command(pushCmd)

    # Can't use mv across filesystems
    remoteFile = remote + "/" + filename
    onDevice("cp /sdcard/tmpFile " + remoteFile)
    onDevice("chmod 666 " + remoteFile)
    onDevice("rm /sdcard/tmpFile")

def devicePull(remote, local, filename=None):
    """Pull remote file at "remote" to dir "local" on host with name "filename" """

    # Can't use mv across filesystems
    onDevice("cp  " + remote + " /sdcard/tmpFile")

    if filename is None:
        filename = os.path.basename(remote)
    pullCmd = "adb " + selection + " pull /sdcard/tmpFile " + os.path.join(local, filename)
    sys.stderr.write(pullCmd + "\n")
    execute_command(pullCmd)
    onDevice("rm /sdcard/tmpFile")

def waitUntilRetraceFinished():
    # Check if retracer is still running on the device.
    processRunning = True
    while processRunning is True:
        processRunning = False
        output = onDeviceWithOutput("ps")
        if output.find("com.arm.pa.paretrace") != -1:
            processRunning = True
        time.sleep(1)

def cleanup():
    onDevice("rm /data/apitrace/tmp/results/*")
    os.system("rm *development.json *release.json")

def init():
    onDevice("mount -o remount,rw /system")
    onDevice("mkdir /data/apitrace/tmp")
    onDevice("chmod 777 /data/apitrace/tmp")
    onDevice("mkdir /data/apitrace/tmp/results")
    onDevice("chmod 777 /data/apitrace/tmp/results")
    devicePush("content_capturing/scripts/test_automation/performance_regression_test/tracerparams_nosnap.cfg", "/system/lib/egl", "tracerparams.cfg")
    cleanup()

def wakeupDevice():
    output = onDeviceWithOutput("dumpsys activity")

    # If device is sleeping, press the menu button to wake it up
    if output.find("mSleeping=true") != -1:
        onDevice("input keyevent 26")

def deviceSleep():
    output = onDeviceWithOutput("dumpsys activity")

    # If device is awake, press the power button to make it sleep
    if (android_version == "J" and output.find("mSleeping=false") != -1) or (android_version == "K" and output.find("mSleeping=") == -1):
        onDevice("input keyevent 26")

def makeResultPath(traceFilename, isRelease):
    retracer = "release" if isRelease else "development"
    traceName = traceFilename.rpartition(".")[0]

    return "/data/apitrace/tmp/results/" + traceName + "." + retracer + ".json"

def executePerformanceTest(traceFilename, startFrame, endFrame, isRelease):
    resultPath = makeResultPath(traceFilename, isRelease)

    wakeupDevice()
    print traceFilename
    results = []
    for i in range(3):
        print "Sleeping for " + str(cooldown) + " seconds before starting retracing to avoid thermal issues"
        time.sleep(cooldown)
        onDevice("am start -n com.arm.pa.paretrace/.Activities.RetraceActivity --ez forceOffscreen true --ez preload true --ei frame_start " + startFrame + " --ei frame_end " + endFrame + " --es fileName /data/apitrace/tmp/" + traceFilename + " --es resultFile " + resultPath)

        waitUntilRetraceFinished()

        deviceSleep()
        devicePull(resultPath, ".")

        with open(resultPath.rpartition("/")[2], "r") as jsonfile:
            result = float(json.load(jsonfile)["result"][0]["fps"])
            results.append(result)

    if len(results) == 1:
        return results[0]
    elif len(results) > 1 and len(results) % 2 > 0:
        return results[len(results)/2]
    else:
        return (results[len(results)/2-1] + results[len(results)/2]) / 2


def setupPATraceVersion(retracerPath):
            print "Updating retracer"

            os.system("adb " + selection + " uninstall com.arm.pa.paretrace")
            apk = get_apk_in_dir(os.path.join(retracerPath, 'eglretrace'))
            os.system("adb " + selection + " install " + apk)

def measurePerformance():
    performanceRatio = 999.0

    with open(traceListPath, "r") as inFile:
        for line in inFile:
            if line.startswith("#") or line.strip("") == "":
                continue

            cols = line.strip().split(",")

            if len(cols) != 3:
                print "Incorrect input file format"
                sys.exit(1)

            print "Pushing tracefile " + cols[0] + " to device"
            devicePush(cols[0], "/data/apitrace/tmp")
            filename = cols[0].rpartition("/")[2]
            setupPATraceVersion(releasePath)
            releaseFps = executePerformanceTest(filename, cols[1], cols[2], True)
            setupPATraceVersion(developmentPath)
            developmentFps = executePerformanceTest(filename, cols[1], cols[2], False)

            performanceRatio = developmentFps / releaseFps

            print ""
            print "Results for tracefile, frames " + str(cols[1]) + "-" + str(cols[2]) + ": " + filename
            print "Release version FPS: " + str(releaseFps)
            print "Trunk version FPS: " + str(developmentFps)
            print "Trunk/Release ratio: " + str(performanceRatio)

            if performanceRatio < threshold:
                print "Performance of trunk version worse than the threshold. Aborting."
                sys.exit(1)

            onDevice("rm /data/apitrace/tmp/" + filename)
    return performanceRatio

# Program starts here
init()
measurePerformance()
cleanup()
