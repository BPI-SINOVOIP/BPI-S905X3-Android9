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

import re
import os
import logging

from subprocess import Popen, PIPE
from time import sleep

from android import Screen, System, Workload
import pandas as pd


class UiBench(Workload):
    """
    Android UiBench workload
    """
    # Packages required by this workload
    packages = [
        Workload.WorkloadPackage("com.android.test.uibench",
            "data/app/UiBench/UiBench.apk",
            "frameworks/base/tests/UiBench"),
        Workload.WorkloadPackage("com.android.uibench.janktests",
            "data/app/UiBenchJankTests/UiBenchJankTests.apk",
            "platform_testing/tests/jank/uibench"),
    ]

    # Package required by this workload
    package = packages[0].package_name

    # Instrumentation required to run tests
    test_package = packages[1].package_name

    # Supported tests list
    test_list = \
    ['UiBenchJankTests#testClippedListView',
    'UiBenchJankTests#testDialogListFling',
    'UiBenchJankTests#testFadingEdgeListViewFling',
    'UiBenchJankTests#testFullscreenOverdraw',
    'UiBenchJankTests#testGLTextureView',
    'UiBenchJankTests#testInflatingListViewFling',
    'UiBenchJankTests#testInvalidate',
    'UiBenchJankTests#testInvalidateTree',
    'UiBenchJankTests#testOpenNavigationDrawer',
    'UiBenchJankTests#testOpenNotificationShade',
    'UiBenchJankTests#testResizeHWLayer',
    'UiBenchJankTests#testSaveLayerAnimation',
    'UiBenchJankTests#testSlowBindRecyclerViewFling',
    'UiBenchJankTests#testSlowNestedRecyclerViewFling',
    'UiBenchJankTests#testSlowNestedRecyclerViewInitialFling',
    'UiBenchJankTests#testTrivialAnimation',
    'UiBenchJankTests#testTrivialListViewFling',
    'UiBenchJankTests#testTrivialRecyclerListViewFling',
    'UiBenchRenderingJankTests#testBitmapUploadJank',
    'UiBenchRenderingJankTests#testShadowGridListFling',
    'UiBenchTextJankTests#testEditTextTyping',
    'UiBenchTextJankTests#testLayoutCacheHighHitrateFling',
    'UiBenchTextJankTests#testLayoutCacheLowHitrateFling',
    'UiBenchTransitionsJankTests#testActivityTransitionsAnimation',
    'UiBenchWebView#testWebViewFling']

    def __init__(self, test_env):
        super(UiBench, self).__init__(test_env)
        self._log = logging.getLogger('UiBench')
        self._log.debug('Workload created')

        # Set of output data reported by UiBench
        self.db_file = None

    def get_test_list(self):
	return UiBench.test_list

    def run(self, out_dir, test_name, iterations=10, collect=''):
        """
        Run single UiBench workload.

        :param out_dir: Path to experiment directory where to store results.
        :type out_dir: str

        :param test_name: Name of the test to run
        :type test_name: str

        :param iterations: Run benchmak for this required number of iterations
        :type iterations: int

        :param collect: Specifies what to collect. Possible values:
            - 'systrace'
            - 'ftrace'
        :type collect: list(str)
        """

        if 'energy' in collect:
            raise ValueError('UiBench workload does not support energy data collection')

        activity = '.' + test_name

        # Keep track of mandatory parameters
        self.out_dir = out_dir
        self.collect = collect

        # Filter out test overhead
        filter_prop = System.get_boolean_property(self._target, 'debug.hwui.filter_test_overhead')
        if not filter_prop:
            System.set_property(
                self._target, 'debug.hwui.filter_test_overhead', 'true', restart=True)

        # Unlock device screen (assume no password required)
        Screen.unlock(self._target)

        # Close and clear application
        System.force_stop(self._target, self.package, clear=True)

        # Set airplane mode
        System.set_airplane_mode(self._target, on=True)

        # Set min brightness
        Screen.set_brightness(self._target, auto=False, percent=0)

        # Force screen in PORTRAIT mode
        Screen.set_orientation(self._target, portrait=True)

        # Clear logcat
        os.system(self._adb('logcat -c'));

        # Regexps for benchmark synchronization
        start_logline = r'TestRunner: started'
        UIBENCH_BENCHMARK_START_RE = re.compile(start_logline)
        self._log.debug("START string [%s]", start_logline)

        finish_logline = r'TestRunner: finished'
        UIBENCH_BENCHMARK_FINISH_RE = re.compile(finish_logline)
        self._log.debug("FINISH string [%s]", start_logline)

        # Parse logcat output lines
        logcat_cmd = self._adb(
                'logcat TestRunner:* System.out:I *:S BENCH:*'\
                .format(self._target.adb_name))
        self._log.info("%s", logcat_cmd)

        command = "am instrument -e iterations {} -e class {}{} -w {}".format(
            iterations, self.test_package, activity, self.test_package)
        test_proc = self._target.background(command)

        logcat = Popen(logcat_cmd, shell=True, stdout=PIPE)
        while True:

            # read next logcat line (up to max 1024 chars)
            message = logcat.stdout.readline(1024)

            # Benchmark start trigger
            match = UIBENCH_BENCHMARK_START_RE.search(message)
            if match:
                self.tracingStart()
                self._log.debug("Benchmark started!")

            match = UIBENCH_BENCHMARK_FINISH_RE.search(message)
            if match:
                self.tracingStop()
                self._log.debug("Benchmark finished!")
                test_proc.wait()
                break

        # Get frame stats
        self.db_file = os.path.join(out_dir, "framestats.txt")
        with open(self.db_file, 'w') as f:
            f.writelines(test_proc.stdout.readlines())
        self.results = self.get_results(out_dir)

        # Close and clear application
        System.force_stop(self._target, self.package, clear=True)

        # Go back to home screen
        System.home(self._target)

        # Switch back to original settings
        Screen.set_orientation(self._target, auto=True)
        System.set_airplane_mode(self._target, on=False)
        Screen.set_brightness(self._target, auto=True)

    @staticmethod
    def get_results(res_dir):
        path = os.path.join(res_dir, 'framestats.txt')
        with open(path, "r") as f:
            lines = f.readlines()

        values = []
        columns = []
        RESULTS_PARSE_RE = re.compile(r'gfx-([^=]+)=([0-9.]+)')
        for line in lines:
            matches = RESULTS_PARSE_RE.search(line)
            if matches:
                columns.append(matches.group(1))
                values.append(float(matches.group(2)))
        return pd.DataFrame([values], columns=columns)
# vim :set tabstop=4 shiftwidth=4 expandtab
