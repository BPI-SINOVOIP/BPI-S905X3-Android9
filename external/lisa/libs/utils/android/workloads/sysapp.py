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

import os
import re

from subprocess import Popen, PIPE
from time import sleep

from android import Screen, System, Workload

import pandas as pd


class SysApp(Workload):
    """
    Android system app jank test workload.
    """

    packages = [
        Workload.WorkloadPackage("com.android.sysapp.janktests",
                                 "data/app/SystemAppJankTests/SystemAppJankTests.apk",
                                 "vendor/google_testing/integration/tests/jank/sysapp"),
        Workload.WorkloadPackage("com.android.chrome",
                                 "system/app/Chrome/Chrome.apk",
                                 "vendor/unbundled_google/packages/Chrome"),
        Workload.WorkloadPackage("com.google.android.youtube",
                                 "system/app/Youtube/Youtube.apk",
                                 "vendor/unbundled_google/packages/YouTube")
    ]

    test_package = packages[0].package_name

    test_list = [
        "ChromeJankTests#testChromeOverflowMenuTap",
        "YouTubeJankTests#testYouTubeRecomendationWindowFling"
    ]

    def __init__(self, test_env):
        super(SysApp, self).__init__(test_env)

    def get_test_list(self):
        return SysApp.test_list

    def _get_test_package(self, test_name):
        name_start = test_name.partition('JankTests')[0]
        name_map = {
            "Chrome": "com.android.chrome",
            "YouTube": "com.google.android.youtube",
        }
        return name_map[name_start]

    def run(self, out_dir, test_name, iterations, collect=''):
        """
        Run single system app jank test workload.
        Performance statistics are stored in self.results, and can be retrieved
        after the fact by calling SystemUi.get_results()

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
        if "energy" in collect:
            raise ValueError('System app workload does not support energy data collection')

        activity = "." + test_name
        package = self._get_test_package(test_name)

        # Keep track of mandatory parameters
        self.out_dir = out_dir
        self.collect = collect

        # Filter out test overhead
        filter_prop = System.get_boolean_property(self._target, "debug.hwui.filter_test_overhead")
        if not filter_prop:
            System.set_property(self._target, "debug.hwui.filter_test_overhead", "true", restart=True)

        # Unlock device screen (assume no password required)
        Screen.unlock(self._target)

        # Close and clear application
        System.force_stop(self._target, package, clear=True)

        # Set min brightness
        Screen.set_brightness(self._target, auto=False, percent=0)

        # Force screen in PORTRAIT mode
        Screen.set_orientation(self._target, portrait=True)

        # Delete old test results
        self._target.remove("/sdcard/results.log")

        # Clear logcat
        self._target.execute("logcat -c")

        # Regexps for benchmark synchronization
        start_logline = r"TestRunner: started"
        SYSAPP_BENCHMARK_START_RE = re.compile(start_logline)
        self._log.debug("START string [%s]", start_logline)

        finish_logline = r"TestRunner: finished"
        SYSAPP_BENCHMARK_FINISH_RE = re.compile(finish_logline)
        self._log.debug("FINISH string [%s]", finish_logline)

        # Parse logcat output lines
        logcat_cmd = self._adb("logcat TestRunner:* System.out:I *:S BENCH:*")
        self._log.info("%s", logcat_cmd)

        command = "am instrument -e iterations {} -e class {}{} -w {}".format(
            iterations, self.test_package, activity, self.test_package)

        logcat = Popen(logcat_cmd, shell=True, stdout=PIPE)

        test_proc = self._target.background(command)
        while True:
            # read next logcat line (up to max 1024 chars)
            message = logcat.stdout.readline(1024)

            # Benchmark start
            match = SYSAPP_BENCHMARK_START_RE.search(message)
            if match:
                self.tracingStart()
                self._log.debug("Benchmark started!")

            # Benchmark finish
            match = SYSAPP_BENCHMARK_FINISH_RE.search(message)
            if match:
                self.tracingStop()
                self._log.debug("Benchmark finished!")
                test_proc.wait()
                break
        sleep(5)
        self._target.pull("/sdcard/results.log", os.path.join(out_dir, "results.log"))
        self.db_file = os.path.join(out_dir, "results.log")
        self.results = self.get_results(out_dir)

        # Close and clear application
        System.force_stop(self._target, package, clear=True)

        # Go back to home screen
        System.home(self._target)

        # Switch back to original settings
        Screen.set_orientation(self._target, auto=True)
        Screen.set_brightness(self._target, auto=True)


    @staticmethod
    def get_results(out_dir):
        """
        Parse SysApp test output log and return a pandas dataframe of test results.

        :param out_dir: Output directory for a run of the SysApp workload.
        :type out_dir: str
        """
        path = os.path.join(out_dir, "results.log")
        with open(path, "r") as f:
            lines = f.readlines()
        cols = []
        vals = []
        for line in lines:
            name, val = str.split(line)
            if name == "Result":
                cols.append("test-name")
                vals.append(val)
            elif name.startswith("gfx-"):
                cols.append(name[4:])
                vals.append(float(val))
            else:
                raise ValueError("Unrecognized line in results file")
        return pd.DataFrame([vals], columns=cols)
