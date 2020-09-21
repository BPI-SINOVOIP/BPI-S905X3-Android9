# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2017, ARM Limited, Google and contributors.
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

import re
import os
import logging

from subprocess import Popen, PIPE
from time import sleep

from android import Screen, System, Workload

import pandas as pd

class SystemUi(Workload):
    """
    Android SystemUi jank test workload
    """

    # Packages required by this workload
    packages = [
        Workload.WorkloadPackage("android.platform.systemui.tests.jank",
                                 "data/app/UbSystemUiJankTests/UbSystemUiJankTests.apk",
                                 "platform_testing/tests/jank/UbSystemUiJankTests")
    ]

    # Instrumentation required to run tests
    test_package = packages[0].package_name

    test_list = \
    ["LauncherJankTests#testOpenAllAppsContainer",
    "LauncherJankTests#testAllAppsContainerSwipe",
    "LauncherJankTests#testHomeScreenSwipe",
    "LauncherJankTests#testWidgetsContainerFling",
    "SettingsJankTests#testSettingsFling",
    "SystemUiJankTests#testRecentAppsFling",
    "SystemUiJankTests#testRecentAppsDismiss",
    "SystemUiJankTests#testNotificationListPull",
    "SystemUiJankTests#testNotificationListPull_manyNotifications",
    "SystemUiJankTests#testNotificationListScroll",
    "SystemUiJankTests#testQuickSettingsPull",
    "SystemUiJankTests#testUnlock",
    "SystemUiJankTests#testExpandGroup",
    "SystemUiJankTests#testClearAll",
    "SystemUiJankTests#testChangeBrightness",
    "SystemUiJankTests#testNotificationAppear",
    "SystemUiJankTests#testCameraFromLockscreen",
    "SystemUiJankTests#testAmbientWakeUp",
    "SystemUiJankTests#testGoToFullShade",
    "SystemUiJankTests#testInlineReply",
    "SystemUiJankTests#testPinAppearance",
    "SystemUiJankTests#testLaunchSettings"]

    def __init__(self, test_env):
        super(SystemUi, self).__init__(test_env)
        self._log = logging.getLogger('SystemUi')
        self._log.debug('Workload created')

        # Set of output data reported by SystemUi
        self.db_file = None

    def get_test_list(self):
        return SystemUi.test_list

    def run(self, out_dir, test_name, iterations, collect=''):
        """
        Run single SystemUi jank test workload.
        Performance statistics are stored in self.results, and can be retrieved after
        the fact by calling SystemUi.get_results()

        :param out_dir: Path to experiment directory where to store results.
        :type out_dir: str

        :param test_name: Name of the test to run
        :type test_name: str

        :param iterations: Run benchmark for this required number of iterations
        :type iterations: int

        :param collect: Specifies what to collect. Possible values:
            - 'systrace'
            - 'ftrace'
            - 'gfxinfo'
            - 'surfaceflinger'
            - any combination of the above
        :type collect: list(str)
        """
        if 'energy' in collect:
            raise ValueError('SystemUi workload does not support energy data collection')

        activity = '.' + test_name

        # Keep track of mandatory parameters
        self.out_dir = out_dir
        self.collect = collect

        # Filter out test overhead
        filter_prop = System.get_boolean_property(self._target, 'debug.hwui.filter_test_overhead')
        if not filter_prop:
            System.set_property(self._target, 'debug.hwui.filter_test_overhead', 'true', restart=True)

        # Unlock device screen (assume no password required)
        Screen.unlock(self._target)

        # Set airplane mode
        System.set_airplane_mode(self._target, on=True)

        # Set min brightness
        Screen.set_brightness(self._target, auto=False, percent=0)

        # Force screen in PORTRAIT mode
        Screen.set_orientation(self._target, portrait=True)

        # Delete old test results
        self._target.remove('/sdcard/results.log')

        # Clear logcat
        os.system(self._adb('logcat -c'));

        # Regexps for benchmark synchronization
        start_logline = r'TestRunner: started'
        SYSTEMUI_BENCHMARK_START_RE = re.compile(start_logline)
        self._log.debug("START string [%s]", start_logline)

        finish_logline = r'TestRunner: finished'
        SYSTEMUI_BENCHMARK_FINISH_RE = re.compile(finish_logline)
        self._log.debug("FINISH string [%s]", finish_logline)

        # Parse logcat output lines
        logcat_cmd = self._adb(
                'logcat TestRunner:* System.out:I *:S BENCH:*'\
                .format(self._target.adb_name))
        self._log.info("%s", logcat_cmd)

        command = "nohup am instrument -e iterations {} -e class {}{} -w {}".format(
            iterations, self.test_package, activity, self.test_package)

        print "command: {}".format(command)

        self._target.background(command)

        logcat = Popen(logcat_cmd, shell=True, stdout=PIPE)
        while True:
            # read next logcat line (up to max 1024 chars)
            message = logcat.stdout.readline(1024)

            # Benchmark start trigger
            match = SYSTEMUI_BENCHMARK_START_RE.search(message)
            if match:
                self.tracingStart()
                self._log.debug("Benchmark started!")

            match = SYSTEMUI_BENCHMARK_FINISH_RE.search(message)
            if match:
                self.tracingStop()
                self._log.debug("Benchmark finished!")
                break

        sleep(5)
        self._target.pull('/sdcard/results.log', os.path.join(out_dir, 'results.log'))
        self._target.remove('/sdcard/results.log')
        self.results = self.get_results(out_dir)

        # Go back to home screen
        System.home(self._target)

        # Switch back to original settings
        Screen.set_orientation(self._target, auto=True)
        System.set_airplane_mode(self._target, on=False)
        Screen.set_brightness(self._target, auto=True)


    @staticmethod
    def get_results(out_dir):
        """
        Parse SystemUi test output log and return a pandas dataframe of test results.

        :param out_dir: Output directory for a run of the SystemUi workload.
        :type out_dir: str
        """
        path = os.path.join(out_dir, 'results.log')
        with open(path, "r") as f:
            lines = f.readlines()
        data = {}
        for line in lines:
            key, val = str.split(line)
            if key == 'Result':
                key = 'test-name'
            elif key.startswith('gfx-'):
                key = key[4:]
                val = float(val)
            else:
                raise ValueError('Unrecognized line in results file')
            data[key] = val
        columns, values = zip(*((k,v) for k,v in data.iteritems()))
        return pd.DataFrame([values], columns=columns)
