from __future__ import print_function
import os
import sys
import time
import subprocess


class AdbDevice(object):
    def __init__(self, id=None, name=None, system_write=False, verbose=True):
        """Initialize an AdbDevice object.

        Parameters
        ----------
        id : str, optional
            The id returned by 'adb devices'. E.g. R32D1034QMN. Not
            mandatory if only single device is connected.
        name : str
            A human readable name to identify the device. E.g. Nexus10-4"""

        self.id = id
        self.name = name
        self.system_write = system_write
        self.verbose = verbose

        self.model_name = self.shell('grep "product\.model" /system/build.prop').split('=')[1].strip()

        if system_write and not self.is_system_writable():
            self.sushell("mount -o remount,rw /system")

    def print_cmd(self, cmd):
        if self.verbose:
            final_cmd = ['adb']

            if self.id:
                final_cmd += ['-s', self.id]

            #print(subprocess.check_output(final_cmd + ['logcat', '-d']))
            print(' '.join(cmd))
            #subprocess.check_output(final_cmd + ['logcat', '-c'])

    def __execute(self, cmd):
        final_cmd = ['adb']

        if self.id:
            final_cmd += ['-s', self.id]

        final_cmd += cmd

        self.print_cmd(final_cmd)
        output = subprocess.check_output(final_cmd, stderr=subprocess.STDOUT)
        #print(''.join(output.split('\n')[0:10]))
        return output

    def sushell(self, cmdstr):
        """Run shell command on device as super user.

        Parameters
        ----------
        cmdstr : str
            The command to be executed. E.g. 'ls -l'

        Returns
        -------
        str
            The output of the command
        """
        if self.system_write and not self.is_system_writable():
            cmd = 'su -c "mount -o remount,rw /system; {cmd}"'.format(cmd=cmdstr)
        else:
            cmd = 'su -c "{cmd}"'.format(cmd=cmdstr)

        return self.shell(cmd)

    def shell(self, cmdstr):
        """Run shell command on device.

        Parameters
        ----------
        cmdstr : str
            The command to be executed. E.g. 'ls -l'

        Returns
        -------
        str
            The output of the command
        """

        return self.__execute(['shell', cmdstr])

    def push(self, path_local, path_remote):
        return self.__execute(['push', path_local, path_remote])

    def pull(self, path_remote, path_local=None):
        self.sushell("chmod -R 777 /data/apitrace/tmp")
        if path_local:
            return self.__execute(['pull', path_remote, path_local])
        else:
            return self.__execute(['pull', path_remote])

    def install(self, apk_path):
        self.__execute(['install', apk_path])

    def uninstall(self, package_name):
        self.__execute(['uninstall', package_name])

    def logcat(self):
        return self.__execute(['logcat', '-d'])

    def logcat_clear(self):
        self.__execute(['logcat', '-c'])

    def is_installed(self, package_name):
        return package_name in self.shell("pm list packages {0}".format(package_name))

    def push_lib(self, path_local, path_remote):
        self.push(path_local, '/sdcard/tmpfile')

        remote_file = path_remote

        if self.is_directory(path_remote):
            remote_file = os.path.join(path_remote,
                                       os.path.basename(path_local))

        self.sushell("cp /sdcard/tmpfile {remote}".format(remote=remote_file))
        self.sushell("rm /sdcard/tmpfile")

        self.sushell("chmod 666 {remote}".format(remote=remote_file))

    def is_directory(self, path):
        out = self.shell("if [ -d {path} ]; then echo True; fi".format(path=path))
        return out.strip() == "True"

    def is_system_writable(self):
        # System line is for example:
        # /dev/block/platform/dw_mmc.0/by-name/system /system ext4 rw,seclabel,relatime,data=ordered 0 0
        def find_system_line(output):
            for line in output.split('\n'):
                if not len(line.strip()):
                    continue

                if line.split()[1] == '/system':
                    return line

        system_line = find_system_line(self.shell('mount'))
        return 'rw' in system_line.split()[3].split(',')

    def is_available(self):
        # Check device is available
        #self.customPrint("Initializing device " + device['id'])
        #self.customPrint("Trying to run 'id' on the device to confirm it's available...")
        try:
            self.shell("id")
        except subprocess.CalledProcessError:
            return False

        return True

    def __str__(self):
        return "{name} ({id})".format(
            name=self.name, id=self.id
        )

    def is_running(self, process):
        return process in self.shell("ps | grep {}".format(process))

    def wait_until_started(self):
        # Using some uggly heuristic to detect when the device is up and
        # running
        print("Waiting for device to restart")
        seconds = 0
        sleep = 1
        running = False

        while not running:
            try:
                if self.model_name in ["SM-N900", "SM-N9005"]:
                    running = self.is_running("com.sec.android.app.SPenKeeper")
                else:
                    running = self.is_running("com.android.launcher")
            except subprocess.CalledProcessError:
                pass

            print('{s} seconds '.format(s=seconds))
            time.sleep(sleep)
            seconds += sleep

        print()

    def wait_until_process_stopped(self, process_name):
        # Blocks as long as 'process_name' is running on the device

        print("Waiting for {0} to finish...".format(process_name))

        seconds = 0
        sleep = 5
        while self.is_running(process_name):
            print('{s} seconds '.format(s=seconds))
            time.sleep(sleep)
            seconds += sleep
        print()

    def wait_for_wakeup(self):
        while True:
            output = self.shell("dumpsys activity")

            if "mSleeping=true" not in output:
                return

            time.sleep(1)

    def wakeup(self):
        output = self.shell("dumpsys activity")

        # If device is sleeping, press the power button to wake it up
        if "mSleeping=true" in output:
            print("wakeup: device is sleeping, now 'pressing' powerbutton to wake it up")
            self.shell("input keyevent 26")
            self.wait_for_wakeup()

    def sleep(self):
        output = self.shell("dumpsys activity")

        # If device is awake, press the power button to make it sleep
        if "mSleeping=true" not in output:
            print("sleep: device is awake, now 'pressing' powerbutton to go into sleep")
            self.shell("input keyevent 26")

    def get_app_info(self, package_name):
        """ Returns a dict with information about an installed app"""

        output = self.shell("dumpsys package {}".format(package_name))
        # Add some new lines, so parsing will be simpler
        outputc = output.replace('gids=', '\ngids=')
        output = outputc.replace('targetSdk=', '\nargetSdk=')

        lines = output.split('\n')
        lines_filtered = [
            line.strip()
            for line in lines
            if '=' in line
        ]

        pairs = {}
        for line in lines_filtered:
            try:
                key, value = line.split('=')
            except ValueError:
                continue
            pairs[key] = value
        return pairs
