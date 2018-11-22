#!/usr/bin/env python
from __future__ import print_function
import os
import sys
import shutil
import subprocess
import shlex
import time
import argparse

from adb_device import AdbDevice

class GoldenImageTestError(Exception):
    pass

class ADBCommandError(Exception):
    pass

def get_apk_in_dir(dir):
    apkname = [name for name in os.listdir(dir) if name.endswith('.apk')][0]
    return os.path.join(dir, apkname)

class Application(object):
    ### Settings
    fileList = ["egltrace/libinterceptor_patrace_arm.so", "fakedriver/libGLES_wrapper.so"]

    # Start/stop after library update.
    # Note3 does this automatically, but the Nexus10 does not.
    # It seems OK to do it manually even if if's handled automatically.
    startStopAfterLibraryUpdate = True

    def __init__(self, workspace, releases, repository):
        """Golden image test script. Can be used for retracer only test, and
        tracer&retracer test

        Parameters
        ----------
        workspace:  Path to the jenkins workspace
        releases: Path to the patrace release directory
        repository: Path to the golden image repository
        """

        # Result-images are put in folder results/<result_id>
        # Incremented after each test
        self.result_id = 0

        scriptdir = os.path.dirname(os.path.realpath(__file__))

        self.paths = {
            'workspace': workspace,
            'goldenImageRepositoryPath': repository,
            'releasePath': os.path.join(releases, 'extracted/r0p6/patrace/android'),
            'developmentPath': 'patrace/android/release/',
            'eglretracePath': os.path.join(releases, 'extracted/r0p6/patrace/10.04_x11_x64/bin/eglretrace'),
            'retracePath': os.path.join(workspace, 'retrace'),
            'scriptdir': scriptdir,
            'tracerparams': os.path.join(scriptdir, 'tracerparams.cfg'),
            'tracercmd': os.path.join(scriptdir, 'tracercmd.cfg'),
        }

        # Test if all paths exists
        for name, path in self.paths.iteritems():
            print("Testing path {}: {}...".format(name, path))

            if name in ['retracePath']:
                print("Ignoring: {}".format(name))
                continue

            if not os.path.exists(path):
                raise Exception("Path does not exists: {}".format(path))
        else:
            print("All paths OK")

    def execute_command(self, command):
        sys.stderr.write(command + "\n")
        ret_code = os.system(command)

        if ret_code != 0:
            raise ADBCommandError("The command '{}' returned error code: {}".format(command, ret_code))

    def checkDeviceCanRunTestWithMask(self, device, testMask):
        """Check that the given test can run on this device.
        Currently checks if the mask is "*" (can run anywhere),
        or if the devicename has the mask as a prefix.
        """
        return testMask == "*" or device.name.startswith(testMask)

    def customPrint(self, p):
        print(p)

    def updateRetracer(self, pathPrefix, device):
        self.customPrint("Updating retracer")

        if device.is_installed("com.arm.pa.paretrace"):
            device.uninstall("com.arm.pa.paretrace")

        apk = get_apk_in_dir(os.path.join(pathPrefix, "eglretrace"))

        timeout = 30
        while timeout > 0:
            try:
                device.install(apk)
            except subprocess.CalledProcessError:
                pass

            if device.is_installed("com.arm.pa.paretrace"):
                break

            print("Retrying installation of {p}. Timeout in {s}s...".format(
                p="com.arm.pa.paretrace", s=timeout
            ))

            time.sleep(1)
            timeout -= 1
        else:
            raise Exception("Installation of the {} failed".format(apk))

    def restart(self, device):
        device.sushell("reboot")

        device.wait_until_started()

        # Remount again afterwards
        device.sushell("mount -o remount,rw /system")

    def updateLibraries(self, pathPrefix, device):
        self.customPrint("Updating libraries")
        device.sushell("mount -o remount,rw /system")

        for filepath in self.fileList:
            device.push_lib(os.path.join(pathPrefix, filepath), "/system/lib/egl")

    # example prefix: /data/apitrace/tmp/snaps/
    # example callset: frame/*/50
    def retraceWithSnapshots(self, localTraceFilePath, snapshotPrefix, snapshotCallset, device):
        self.customPrint("------- Transfering trace to device -------")
        traceTempDir = "/data/apitrace/tmp/"
        device.sushell("rm -r " + traceTempDir)
        device.sushell("mkdir " + traceTempDir)
        device.sushell("chmod 777 " + traceTempDir)
        device.sushell("mkdir " + os.path.join(traceTempDir, "snaps"))
        device.sushell("chmod 777 " + os.path.join(traceTempDir, "snaps"))

        device.push_lib(localTraceFilePath, traceTempDir)

        if not device.is_installed('com.arm.pa.paretrace'):
            raise GoldenImageTestError("Retracer is not installed!")

        self.customPrint("------- Retracing with snapshots on device -------")
        device.wakeup()
        device.shell(
            'am start '
            ' -n com.arm.pa.paretrace/.Activities.RetraceActivity'
            ' --ez forceOffscreen false'
            ' --es fileName "{fileName}"'
            ' --es snapshotPrefix "{snapshotPrefix}"'
            ' --es snapshotCallset {snapshotCallset}'
            ''.format(
                fileName=traceTempDir + os.path.basename(localTraceFilePath),
                snapshotPrefix=snapshotPrefix,
                snapshotCallset=snapshotCallset,
            )
        )

        # Give it some time to start
        time.sleep(5)

        device.wait_until_process_stopped('com.arm.pa.paretrace')
        self.customPrint("Done")
        device.sleep()

    def printTraceFileInfo(self, filepath):
        # Get display dimensions of the tracefile
        width = 0
        height = 0
        args = shlex.split(self.paths['eglretracePath'] + " -info " + filepath)
        output = subprocess.check_output(args)
        output = output.split("\n")

        for infoline in output:
            if infoline.startswith("winWidth"):
                width = infoline.split()[1]
            elif infoline.startswith("winHeight"):
                height = infoline.split()[1]

        self.customPrint("Info for: " + filepath)
        self.customPrint("Width: " + width)
        self.customPrint("Height: " + height)

    def pullFromDevice(self, device, fromDevicePath, toLocalDir):
        oldDir = os.getcwd()

        shutil.rmtree(toLocalDir, ignore_errors=True)
        os.makedirs(toLocalDir)

        os.chdir(toLocalDir)
        device.pull(fromDevicePath)

        os.chdir(oldDir)

    def init(self, device):

        if device.is_available():
            self.customPrint("Device seems to be available!")
        else:
            raise Exception("Device not available: {device}".format(device=device))

        # Setup environment
        device.sushell("mount -o remount,rw /system")

        self.setDeviceTracing(device, False)
        device.push_lib(self.paths['tracerparams'], "/system/lib/egl")

    def writeLog(self, filename, log):
        with open(filename, "w") as f:
            f.write(log)

    def pngsInDir(self, path):
        l = os.listdir(path)
        l = filter(lambda x: x.endswith(".png"), l)
        l.sort()
        l = [os.path.join(path, f) for f in l]
        return l

    def get_comparison_error(self, img1, img2, diffImg):
        """ Returns the 'Root mean squared error' (RMSE) and error in percentage.
        RMSE is calculated like this:

            MSE = SUM(S_orig - S_new)^2 / SUM(1)

            RMSE = sqrt(MSE)
        """

        output = subprocess.check_output(
            ["compare", "-metric", "RMSE", "-alpha", "Off", img1, img2, diffImg],
            stderr=subprocess.STDOUT,
        )
        rmse = float(output.split()[0])
        percent = float(output.split()[1][1:-1])
        return rmse, percent

    # Returns True if the two lists contain identical images
    def compareImages(self, list1, list2):
        if len(list1) != len(list2):
            raise GoldenImageTestError("Number of images differ: " + str(len(list1)) + " vs " + str(len(list2)))

        if len(list1) == 0:
            raise GoldenImageTestError("Tried to compare an empty list of image. Returning false as this is probably not what you wanted to do.")

        # Ignore first frame, since sometimes, when entering portrait mode the
        # first image is rendered incorrectly.
        for i in range(1, len(list1)):
            img1 = list1[i]
            img2 = list2[i]
            diffImg = img1.replace("png", "diff.png")
            self.customPrint("Comparing " + img1 + " to " + img2)

            # Returns 1 if similarity is below -dissimilarity-threshold (0.2 by default)
            rmse, percent = self.get_comparison_error(img1, img2, diffImg)

            if rmse != 0:
                #print(
                raise GoldenImageTestError(
                    "{img1} and {img2} differ! RMSE {rmse}, {percent}%. Difference-image stored in {diff}".format(
                        img1=img1,
                        img2=img2,
                        rmse=rmse,
                        percent=percent * 100,
                        diff=diffImg,
                    )
                )
            else:
                os.unlink(diffImg)

    def cleanup(self, device):
        self.customPrint("Clean up...")
        device.sushell("mount -o remount,rw /system")
        self.setDeviceTracing(device, False)

    def retraceGoldenImages(self, tracePath, referenceSnapsPath, snapNo, device, retracerVersion=None):
        self.customPrint("---------------- Starting retraceGoldenImages ---------------------")
        self.customPrint(tracePath + " " + snapNo + " " + retracerVersion)

        retracer = self.paths['developmentPath']
        if retracerVersion:
            retracer = retracerVersion

        self.customPrint("Retracing tracefile: " + tracePath)
        self.customPrint("Using retracer at: " + retracer)

        if not os.path.isfile(tracePath):
            raise GoldenImageTestError("Error: could not find tracefile: {}".format(tracePath))

        self.updateRetracer(retracer, device)

        deviceSnapsPath = "/data/apitrace/tmp/snaps/"
        self.retraceWithSnapshots(tracePath, deviceSnapsPath, "frame/*/{period}".format(period=snapNo), device)

        self.result_id += 1
        localSnapsPath = "results/" + str(self.result_id) + "/"
        self.pullFromDevice(device, deviceSnapsPath, localSnapsPath)

        self.customPrint("Starting image comparisons: " + localSnapsPath + " vs " + referenceSnapsPath)
        retraceImages = self.pngsInDir(localSnapsPath)
        referenceImages = self.pngsInDir(referenceSnapsPath)
        self.compareImages(retraceImages, referenceImages)

    def setDeviceTracing(self, device, on):
        if on:
            self.execute_command("echo \'com.arm.pa.paretrace\'  > appList.cfg")
        else:
            self.execute_command("echo \'\'  > appList.cfg")

        device.push_lib("appList.cfg", "/system/lib/egl")
        self.customPrint("Retracer tracing: {0}".format(on))

    def traceAndRetraceGoldenImages(self, pat_file, snap_period, device):
        self.customPrint("\n\n\n\n---------------- Starting traceAndRetraceGoldenImages ---------------------")

        # Prepare the retracer as target for tracing
        device.push_lib(self.paths['tracerparams'], "/system/lib/egl")
        device.push_lib(self.paths['tracercmd'], "/system/lib/egl")

        #  Update libraries and retracer with correct versions
        self.updateLibraries(self.paths['developmentPath'], device)

        if self.startStopAfterLibraryUpdate:
            self.customPrint("Calling stop + start to reload libraries")
            self.restart(device)

        self.updateRetracer(self.paths['developmentPath'], device)

        traceTempDir = "/data/apitrace/tmp/"
        device.sushell("rm -r " + traceTempDir)
        device.sushell("mkdir " + traceTempDir)
        device.sushell("chmod 777 " + traceTempDir)
        device.sushell("rm -r /data/apitrace/com.arm.pa.paretrace")
        device.sushell("mkdir /data/apitrace/com.arm.pa.paretrace")
        device.sushell("chmod 777 /data/apitrace/com.arm.pa.paretrace")
        device.sushell("rm -r /data/apitrace/snap")
        device.sushell("mkdir /data/apitrace/snap")
        device.sushell("chmod 777 /data/apitrace/snap")

        # Transfer trace to device
        pat_file_remote = os.path.join(traceTempDir, os.path.basename(pat_file))
        device.push(pat_file, pat_file_remote)
        self.customPrint("Done")

        # Capture threadId 0 as we're tracing the retracer
        device.sushell("mount -o remount,rw /system")
        self.execute_command("echo snapEvery {tid} {period} > tracercmd.cfg".format(tid=0, period=snap_period))
        device.push_lib("tracercmd.cfg", "/system/lib/egl/")

        self.printTraceFileInfo(pat_file)

        # Start tracing of retracing
        self.setDeviceTracing(device, True)
        self.customPrint("Starting to trace tracefile: {}".format(pat_file))

        device.wakeup()
        device.sushell("am start -n com.arm.pa.paretrace/.Activities.RetraceActivity --es fileName {fileName}".format(fileName=pat_file_remote))

        # Give it some time to start
        time.sleep(1)

        device.wait_until_process_stopped('com.arm.pa.paretrace')

        self.setDeviceTracing(device, False)
        self.customPrint("Done")
        device.sleep()

        # Make sure dir is empty before pulling snapshots from device
        #referenceSnapsPath = os.path.join(self.paths['retracePath'], "snap")
        #shutil.rmtree(referenceSnapsPath, ignore_errors=True)
        #os.makedirs(referenceSnapsPath)

        # Also clear folder where trace is placed
        shutil.rmtree(self.paths['retracePath'], ignore_errors=True)
        os.makedirs(self.paths['retracePath'])

        #os.chdir(self.paths['goldenImageRepositoryPath'])
        #os.chdir(self.paths['workspace'])

        script_import_android = os.path.join(self.paths['scriptdir'], 'import_from_android.sh')
        self.execute_command(script_import_android + ' ' + self.paths['retracePath'] + ' com.arm.pa.paretrace ' + device.id)

        os.chdir(self.paths['retracePath'])

        script_pat_to_golden = os.path.join(self.paths['scriptdir'], 'import_pat_to_golden.sh')
        self.execute_command(script_pat_to_golden + ' retrace.pat ' + snap_period + ' ' + self.paths['eglretracePath'])

        os.chdir(self.paths['workspace'])

        # Print some trace-file info
        traceToRetrace = os.path.join(self.paths['retracePath'], "retrace.pat")
        self.printTraceFileInfo(traceToRetrace)

        # Retrace and compare golden images
        referenceSnapsPath = os.path.join(self.paths['retracePath'], 'snap')

        self.retraceGoldenImages(traceToRetrace, referenceSnapsPath, snap_period, device, self.paths['developmentPath'])
        self.customPrint("Images collected during TRACING were identical to those collected when RETRACING that trace")
        self.customPrint("\n\n\n")

        self.customPrint("---------------- traceAndRetraceGoldenImages done ---------------------\n\n\n\n")
