# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2015, ARM Limited and contributors.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging

from devlib.utils.android import adb_command
from devlib import TargetError
from devlib.utils.android_build import Build
from time import sleep
import os
import re
import time
import json
import pexpect as pe
from time import sleep

GET_FRAMESTATS_CMD = 'shell dumpsys gfxinfo {} > {}'
ADB_INSTALL_CMD = 'install -g -r {}'
BOARD_CONFIG_FILE = 'board.json'

# See https://developer.android.com/reference/android/content/Intent.html#setFlags(int)
FLAG_ACTIVITY_NEW_TASK = 0x10000000
FLAG_ACTIVITY_CLEAR_TASK = 0x00008000

class System(object):
    """
    Collection of Android related services
    """

    @staticmethod
    def systrace_start(target, trace_file, time=None,
                       events=['gfx', 'view', 'sched', 'freq', 'idle'],
                       conf=None):
        buffsize = "40000"
        log = logging.getLogger('System')

        # Android needs good TGID caching support, until atrace has it,
        # just increase the cache size to avoid missing TGIDs (and also comms)
        target.target.execute("echo 8192 > /sys/kernel/debug/tracing/saved_cmdlines_size")

        # Override systrace defaults from target conf
        if conf and ('systrace' in conf):
            if 'categories' in conf['systrace']:
                events = conf['systrace']['categories']
            if 'extra_categories' in conf['systrace']:
                events += conf['systrace']['extra_categories']
            if 'buffsize' in conf['systrace']:
                buffsize = int(conf['systrace']['buffsize'])
            if 'extra_events' in conf['systrace']:
                for ev in conf['systrace']['extra_events']:
                    log.info("systrace_start: Enabling extra ftrace event {}".format(ev))
                    ev_file = target.target.execute("ls /sys/kernel/debug/tracing/events/*/{}/enable".format(ev))
                    cmd = "echo 1 > {}".format(ev_file)
                    target.target.execute(cmd, as_root=True)
            if 'event_triggers' in conf['systrace']:
                for ev in conf['systrace']['event_triggers'].keys():
                    tr_file = target.target.execute("ls /sys/kernel/debug/tracing/events/*/{}/trigger".format(ev))
                    cmd = "echo {} > {}".format(conf['systrace']['event_triggers'][ev], tr_file)
                    target.target.execute(cmd, as_root=True, check_exit_code=False)

        # Check which systrace binary is available under CATAPULT_HOME
        for systrace in ['systrace.py', 'run_systrace.py']:
                systrace_path = os.path.join(target.CATAPULT_HOME, 'systrace',
                                             'systrace', systrace)
                if os.path.isfile(systrace_path):
                        break
        else:
                log.warning("Systrace binary not available under CATAPULT_HOME: %s!",
                            target.CATAPULT_HOME)
                return None

        #  Format the command according to the specified arguments
        device = target.conf.get('device', '')
        if device:
            device = "-e {}".format(device)
        systrace_pattern = "{} {} -o {} {} -b {}"
        trace_cmd = systrace_pattern.format(systrace_path, device,
                                            trace_file, " ".join(events), buffsize)
        if time is not None:
            trace_cmd += " -t {}".format(time)

        log.info('SysTrace: %s', trace_cmd)

        # Actually spawn systrace
        return pe.spawn(trace_cmd)

    @staticmethod
    def systrace_wait(target, systrace_output):
        systrace_output.wait()

    @staticmethod
    def set_airplane_mode(target, on=True):
        """
        Set airplane mode
        """
        ap_mode = 1 if on else 0
        ap_state = 'true' if on else 'false'

        try:
            target.execute('settings put global airplane_mode_on {}'\
                           .format(ap_mode), as_root=True)
            target.execute('am broadcast '\
                           '-a android.intent.action.AIRPLANE_MODE '\
                           '--ez state {}'\
                           .format(ap_state), as_root=True)
        except TargetError:
            log = logging.getLogger('System')
            log.warning('Failed to toggle airplane mode, permission denied.')

    @staticmethod
    def _set_svc(target, cmd, on=True):
        mode = 'enable' if on else 'disable'
        try:
            target.execute('svc {} {}'.format(cmd, mode), as_root=True)
        except TargetError:
            log = logging.getLogger('System')
            log.warning('Failed to toggle {} mode, permission denied.'\
                        .format(cmd))

    @staticmethod
    def set_mobile_data(target, on=True):
        """
        Set mobile data connectivity
        """
        System._set_svc(target, 'data', on)

    @staticmethod
    def set_wifi(target, on=True):
        """
        Set mobile data connectivity
        """
        System._set_svc(target, 'wifi', on)

    @staticmethod
    def set_nfc(target, on=True):
        """
        Set mobile data connectivity
        """
        System._set_svc(target, 'nfc', on)

    @staticmethod
    def get_property(target, prop):
        """
        Get the value of a system property
        """
        try:
            value = target.execute('getprop {}'.format(prop), as_root=True)
        except TargetError:
            log = logging.getLogger('System')
            log.warning('Failed to get prop {}'.format(prop))
        return value.strip()

    @staticmethod
    def get_boolean_property(target, prop):
        """
        Get a boolean system property and return whether its value corresponds to True
        """
        return System.get_property(target, prop) in {'yes', 'true', 'on', '1', 'y'}

    @staticmethod
    def set_property(target, prop, value, restart=False):
        """
        Set a system property, then run adb shell stop && start if necessary

        When restart=True, this function clears logcat to avoid detecting old
        "Boot is finished" messages.
        """
        try:
            target.execute('setprop {} {}'.format(prop, value), as_root=True)
        except TargetError:
            log = logging.getLogger('System')
            log.warning('Failed to set {} to {}.'.format(prop, value))
        if not restart:
            return

        target.execute('logcat -c', check_exit_code=False)
        BOOT_FINISHED_RE = re.compile(r'Boot is finished')
        logcat = target.background('logcat SurfaceFlinger:* *:S')
        target.execute('stop && start', as_root=True)
        while True:
            message = logcat.stdout.readline(1024)
            match = BOOT_FINISHED_RE.search(message)
            if match:
                return

    @staticmethod
    def start_app(target, apk_name):
        """
        Start the main activity of the specified application

        :param apk_name: name of the apk
        :type apk_name: str
        """
        target.execute('monkey -p {} -c android.intent.category.LAUNCHER 1'\
                      .format(apk_name))

    @staticmethod
    def start_activity(target, apk_name, activity_name):
        """
        Start an application by specifying package and activity name.

        :param apk_name: name of the apk
        :type apk_name: str

        :param activity_name: name of the activity to launch
        :type activity_name: str
        """
        target.execute('am start -n {}/{}'.format(apk_name, activity_name))

    @staticmethod
    def start_action(target, action, action_args=''):
        """
        Start an activity by specifying an action.

        :param action: action to be executed
        :type action: str

        :param action_args: arguments for the activity
        :type action_args: str
        """
        target.execute('am start -a {} {}'.format(action, action_args))

    @staticmethod
    def screen_always_on(target, enable=True):
        """
        Keep the screen always on

        :param enable: True or false
        """
        param = 'true'
        if not enable:
            param = 'false'

        log = logging.getLogger('System')
        log.info('Setting screen always on to {}'.format(param))
        target.execute('svc power stayon {}'.format(param))

    @staticmethod
    def view_uri(target, uri, force_new=True):
        """
        Start a view activity by specifying a URI

        :param uri: URI of the item to display
        :type uri: str

        :param force_new: Force the viewing application to be
            relaunched if it is already running
        :type force_new: bool
        """
        arguments = '-d {}'.format(uri)

        if force_new:
            # Activity flags ensure the app is restarted
            arguments = '{} -f {}'.format(arguments,
                FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_CLEAR_TASK)

        System.start_action(target, 'android.intent.action.VIEW', arguments)
        # Wait for the viewing application to be completely loaded
        sleep(5)

    @staticmethod
    def force_stop(target, apk_name, clear=False):
        """
        Stop the application and clear its data if necessary.

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param apk_name: name of the apk
        :type apk_name: str

        :param clear: clear application data
        :type clear: bool
        """
        target.execute('am force-stop {}'.format(apk_name))
        if clear:
            target.execute('pm clear {}'.format(apk_name))

    @staticmethod
    def force_suspend_start(target):
        """
        Force the device to go into suspend. If a wakelock is held, the device
        will go into idle instead.

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        """
        target.execute('dumpsys deviceidle force-idle deep')

    @staticmethod
    def force_suspend_stop(target):
        """
        Stop forcing the device to suspend/idle.

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        """
        target.execute('dumpsys deviceidle unforce')

    @staticmethod
    def tap(target, x, y, absolute=False):
        """
        Tap a given point on the screen.

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param x: horizontal coordinate
        :type x: int

        :param y: vertical coordinate
        :type y: int

        :param absolute: use absolute coordinates or percentage of screen
            resolution
        :type absolute: bool
        """
        if not absolute:
            w, h = target.screen_resolution
            x = w * x / 100
            y = h * y / 100

        target.execute('input tap {} {}'.format(x, y))

    @staticmethod
    def vswipe(target, y_low_pct, y_top_pct, duration='', swipe_up=True):
        """
        Vertical swipe

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param y_low_pct: vertical lower coordinate percentage
        :type y_low_pct: int

        :param y_top_pct: vertical upper coordinate percentage
        :type y_top_pct: int

        :param duration: duration of the swipe in milliseconds
        :type duration: int

        :param swipe_up: swipe up or down
        :type swipe_up: bool
        """
        w, h = target.screen_resolution
        x = w / 2
        if swipe_up:
            y1 = h * y_top_pct / 100
            y2 = h * y_low_pct / 100
        else:
            y1 = h * y_low_pct / 100
            y2 = h * y_top_pct / 100

        target.execute('input swipe {} {} {} {} {}'\
                       .format(x, y1, x, y2, duration))

    @staticmethod
    def hswipe(target, x_left_pct, x_right_pct, duration='', swipe_right=True):
        """
        Horizontal swipe

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param x_left_pct: horizontal left coordinate percentage
        :type x_left_pct: int

        :param x_right_pct: horizontal right coordinate percentage
        :type x_right_pct: int

        :param duration: duration of the swipe in milliseconds
        :type duration: int

        :param swipe_right: swipe right or left
        :type swipe_right: bool
        """
        w, h = target.screen_resolution
        y = h / 2
        if swipe_right:
            x1 = w * x_left_pct / 100
            x2 = w * x_right_pct / 100
        else:
            x1 = w * x_right_pct / 100
            x2 = w * x_left_pct / 100
        target.execute('input swipe {} {} {} {} {}'\
                       .format(x1, y, x2, y, duration))

    @staticmethod
    def menu(target):
        """
        Press MENU button

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget
        """
        target.execute('input keyevent KEYCODE_MENU')

    @staticmethod
    def home(target):
        """
        Press HOME button

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget
        """
        target.execute('input keyevent KEYCODE_HOME')

    @staticmethod
    def back(target):
        """
        Press BACK button

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget
        """
        target.execute('input keyevent KEYCODE_BACK')

    @staticmethod
    def wakeup(target):
        """
        Wake up the system if its sleeping

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget
        """
        target.execute('input keyevent KEYCODE_WAKEUP')

    @staticmethod
    def sleep(target):
        """
        Make system sleep if its awake

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget
        """
        target.execute('input keyevent KEYCODE_SLEEP')

    @staticmethod
    def volume(target, times=1, direction='down'):
        """
        Increase or decrease volume

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param times: number of times to perform operation
        :type times: int

        :param direction: which direction to increase (up/down)
        :type direction: str
        """
        for i in range(times):
            if direction == 'up':
                target.execute('input keyevent KEYCODE_VOLUME_UP')
            elif direction == 'down':
                target.execute('input keyevent KEYCODE_VOLUME_DOWN')

    @staticmethod
    def wakelock(target, name='lisa', take=False):
        """
        Take or release wakelock

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param name: name of the wakelock
        :type name: str

        :param take: whether to take or release the wakelock
        :type take: bool
        """
        path = '/sys/power/wake_lock' if take else '/sys/power/wake_unlock'
        target.execute('echo {} > {}'.format(name, path))

    @staticmethod
    def gfxinfo_reset(target, apk_name):
        """
        Reset gfxinfo frame statistics for a given app.

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param apk_name: name of the apk
        :type apk_name: str
        """
        target.execute('dumpsys gfxinfo {} reset'.format(apk_name))
        sleep(1)

    @staticmethod
    def surfaceflinger_reset(target, apk_name):
        """
        Reset SurfaceFlinger layer statistics for a given app.

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param apk_name: name of the apk
        :type apk_name: str
        """
        target.execute('dumpsys SurfaceFlinger {} reset'.format(apk_name))

    @staticmethod
    def logcat_reset(target):
        """
        Clears the logcat buffer.

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget
        """
        target.execute('logcat -c')

    @staticmethod
    def gfxinfo_get(target, apk_name, out_file):
        """
        Collect frame statistics for the given app.

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param apk_name: name of the apk
        :type apk_name: str

        :param out_file: output file name
        :type out_file: str
        """
        adb_command(target.adb_name,
                    GET_FRAMESTATS_CMD.format(apk_name, out_file))

    @staticmethod
    def surfaceflinger_get(target, apk_name, out_file):
        """
        Collect SurfaceFlinger layer statistics for the given app.

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param apk_name: name of the apk
        :type apk_name: str

        :param out_file: output file name
        :type out_file: str
        """
        adb_command(target.adb_name,
                    'shell dumpsys SurfaceFlinger {} > {}'.format(apk_name, out_file))

    @staticmethod
    def logcat_get(target, out_file):
        """
        Collect the logs from logcat.

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param out_file: output file name
        :type out_file: str
        """
        adb_command(target.adb_name, 'logcat * -d > {}'.format(out_file))

    @staticmethod
    def monkey(target, apk_name, event_count=1):
        """
        Wrapper for adb monkey tool.

        The Monkey is a program that runs on your emulator or device and
        generates pseudo-random streams of user events such as clicks, touches,
        or gestures, as well as a number of system-level events. You can use
        the Monkey to stress-test applications that you are developing, in a
        random yet repeatable manner.

        Full documentation is available at:

        https://developer.android.com/studio/test/monkey.html

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param apk_name: name of the apk
        :type apk_name: str

        :param event_count: number of events to generate
        :type event_count: int
        """
        target.execute('monkey -p {} {}'.format(apk_name, event_count))

    @staticmethod
    def list_packages(target, apk_filter=''):
        """
        List the packages matching the specified filter

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param apk_filter: a substring which must be part of the package name
        :type apk_filter: str
        """
        packages = []

        pkgs = target.execute('cmd package list packages {}'\
                              .format(apk_filter.lower()))
        for pkg in pkgs.splitlines():
            packages.append(pkg.replace('package:', ''))
        packages.sort()

        if len(packages):
            return packages
        return None

    @staticmethod
    def packages_info(target, apk_filter=''):
        """
        Get a dictionary of installed APKs and related information

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param apk_filter: a substring which must be part of the package name
        :type apk_filter: str
        """
        packages = {}

        pkgs = target.execute('cmd package list packages {}'\
                              .format(apk_filter.lower()))
        for pkg in pkgs.splitlines():
            pkg = pkg.replace('package:', '')
            # Lookup for additional APK information
            apk = target.execute('pm path {}'.format(pkg))
            apk = apk.replace('package:', '')
            packages[pkg] = {
                'apk' : apk.strip()
            }

        if len(packages):
            return packages
        return None


    @staticmethod
    def install_apk(target, apk_path):
        """
        Get a dictionary of installed APKs and related information

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param apk_path: path to application
        :type apk_path: str
        """
        adb_command(target.adb_name, ADB_INSTALL_CMD.format(apk_path))

    @staticmethod
    def contains_package(target, package):
        """
        Returns true if the package exists on the device

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param package: the name of the package
        :type package: str
        """
        packages = System.list_packages(target)
        if not packages:
            return None

        return package in packages

    @staticmethod
    def grant_permission(target, package, permission):
        """
        Grant permission to a package

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param package: the name of the package
        :type package: str

        :param permission: the name of the permission
        :type permission: str
        """
        target.execute('pm grant {} {}'.format(package, permission))

    @staticmethod
    def reset_permissions(target, package):
        """
        Reset the permission for a package

        :param target: instance of devlib Android target
        :type target: devlib.target.AndroidTarget

        :param package: the name of the package
        :type package: str
        """
        target.execute('pm reset-permissions {}'.format(package))

    @staticmethod
    def find_config_file(test_env):
        # Try device-specific config file first
        board_cfg_file = os.path.join(test_env.DEVICE_LISA_HOME, BOARD_CONFIG_FILE)

        if not os.path.exists(board_cfg_file):
            # Try local config file $LISA_HOME/libs/utils/platforms/$TARGET_PRODUCT.json
            board_cfg_file = 'libs/utils/platforms/{}.json'.format(test_env.TARGET_PRODUCT)
            board_cfg_file = os.path.join(test_env.LISA_HOME, board_cfg_file)
            if not os.path.exists(board_cfg_file):
                return None
        return board_cfg_file

    @staticmethod
    def read_config_file(board_cfg_file):
        with open(board_cfg_file, "r") as fh:
            board_config = json.load(fh)
        return board_config

    @staticmethod
    def reimage(test_env, kernel_path='', update_cfg=''):
        """
        Get a reference to the specified Android workload

        :param test_env: target test environment
        :type test_env: TestEnv

        :param kernel_path: path to kernel sources, required if reimage option is used
        :type kernel_path: str

        :param update_cfg: update configuration name from board_cfg.json
        :type update_cfg: str

        """
        # Find board config file from device-specific or local directory
        board_cfg_file = System.find_config_file(test_env)
        if board_cfg_file == None:
            raise RuntimeError('Board config file is not found')

        # Read build config file
        board_config = System.read_config_file(board_cfg_file)
        if board_config == None:
            raise RuntimeError('Board config file {} is invalid'.format(board_cfg_file))

        # Read update-config section and execute appropriate scripts
        update_config = board_config['update-config'][update_cfg]
        if update_config == None:
            raise RuntimeError('Update config \'{}\' is not found'.format(update_cfg))

        board_cfg_dir = os.path.dirname(os.path.realpath(board_cfg_file))
        build_script = update_config['build-script']
        flash_script = update_config['flash-script']
        build_script = os.path.join(board_cfg_dir, build_script)
        flash_script = os.path.join(board_cfg_dir, flash_script)

        cmd_prefix = "LOCAL_KERNEL_HOME='{}' ".format(kernel_path)
        bld = Build(test_env)
        bld.exec_cmd(cmd_prefix + build_script)
        bld.exec_cmd(cmd_prefix + flash_script)


# vim :set tabstop=4 shiftwidth=4 expandtab
