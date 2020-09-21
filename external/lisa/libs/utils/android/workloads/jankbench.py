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
import sqlite3
import pandas as pd

from subprocess import Popen, PIPE
from time import sleep

from android import Screen, System, Workload

# Available test workloads
_jankbench = {
    'list_view'         : 0,
    'image_list_view'   : 1,
    'shadow_grid'       : 2,
    'low_hitrate_text'  : 3,
    'high_hitrate_text' : 4,
    'edit_text'         : 5,
    'overdraw'		: 6,
}

# Regexps for benchmark synchronization
JANKBENCH_BENCHMARK_START_RE = re.compile(
    r'ActivityManager: START.*'
    '(cmp=com.android.benchmark/.app.RunLocalBenchmarksActivity)'
)
JANKBENCH_ITERATION_COUNT_RE = re.compile(
    r'System.out: iteration: (?P<iteration>[0-9]+)'
)

# Meaning of different jankbench metrics output in the logs:
#
# BAD FRAME STATS:
# mean: Mean frame duration time of all frames that are > 12ms completion time
# std_dev: Standard deviation of all frame times > 12ms completion time
# count_bad: Total number of frames
#
# JANK FRAME STATS:
# JankP: Percent of all total frames that missed their deadline (2*16ms for
#        tripple buffering, 16ms for double buffering).
# count_jank: Total frames that missed their deadline (as described above).
JANKBENCH_ITERATION_METRICS_RE = re.compile(
    r'System.out: Mean: (?P<mean>[0-9\.]+)\s+JankP: (?P<jank_p>[0-9\.]+)\s+'
    'StdDev: (?P<std_dev>[0-9\.]+)\s+Count Bad: (?P<count_bad>[0-9]+)\s+'
    'Count Jank: (?P<count_jank>[0-9]+)'
)
JANKBENCH_BENCHMARK_DONE_RE = re.compile(
    r'I BENCH\s+:\s+BenchmarkDone!'
)

JANKBENCH_DB_PATH = '/data/data/com.android.benchmark/databases/'
JANKBENCH_DB_NAME = 'BenchmarkResults'

# The amounts of time to collect energy for each test. Used when USB communication with the device
# is disabled during energy collection to prevent it from interfering with energy sampling. Tests
# may run longer or shorter than the amount of time energy is collected.
JANKBENCH_ENERGY_DURATIONS = {
    'list_view'         : 30,
    'image_list_view'   : 30,
    'shadow_grid'       : 30,
    'low_hitrate_text'  : 30,
    'high_hitrate_text' : 30,
    'edit_text'         : 9,
}

class Jankbench(Workload):
    """
    Android Jankbench workload
    """

    # Package required by this workload
    package = 'com.android.benchmark'

    test_list = \
    ['list_view',
    'image_list_view',
    'shadow_grid',
    'low_hitrate_text',
    'high_hitrate_text',
    'edit_text',
    'overdraw']

    def __init__(self, test_env):
        super(Jankbench, self).__init__(test_env)
        self._log = logging.getLogger('Jankbench')
        self._log.debug('Workload created')

        # Set of output data reported by Jankbench
        self.db_file = None

    def get_test_list(self):
	return Jankbench.test_list

    def run(self, out_dir, test_name, iterations, collect):
        """
        Run Jankbench workload for a number of iterations.
        Returns a collection of results.

        :param out_dir: Path to experiment directory on the host
                        where to store results.
        :type out_dir: str

        :param test_name: Name of the test to run
        :type test_name: str

        :param iterations: Number of iterations for the named test
        :type iterations: int

        :param collect: Specifies what to collect. Possible values:
            - 'energy'
            - 'systrace'
            - 'ftrace'
            - any combination of the above as a single space-separated string.
        :type collect: list(str)
        """

        # Keep track of mandatory parameters
        self.out_dir = out_dir
        self.collect = collect

        # Setup test id
        try:
            test_id = _jankbench[test_name]
        except KeyError:
            raise ValueError('Jankbench test [%s] not supported', test_name)

        # Restart ADB in root mode - adb needs to run as root inorder to
        # grab the db output file
        self._log.info('Restarting ADB in root mode...')
        self._target.adb_root(force=True)

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
        self._target.clear_logcat()

        self._log.debug('Start Jank Benchmark [%d:%s]', test_id, test_name)
        test_cmd = 'am start -n "com.android.benchmark/.app.RunLocalBenchmarksActivity" '\
                    '--eia "com.android.benchmark.EXTRA_ENABLED_BENCHMARK_IDS" {0} '\
                    '--ei "com.android.benchmark.EXTRA_RUN_COUNT" {1}'\
                    .format(test_id, iterations)
        self._log.info(test_cmd)
        self._target.execute(test_cmd);

        if 'energy' in collect:
            # Start collecting energy readings
            self.tracingStart()
            self._log.debug('Benchmark started!')
            self._log.info('Running energy meter for {} seconds'
                .format(iterations * JANKBENCH_ENERGY_DURATIONS[test_name]))

            # Sleep for the approximate amount of time the test should take to run the specified
            # number of iterations.
            for i in xrange(0, iterations):
                sleep(JANKBENCH_ENERGY_DURATIONS[test_name])
                self._log.debug('Iteration: %2d', i + 1)

            # Stop collecting energy. The test may or may not be done at this point.
            self._log.debug('Benchmark done!')
            self.tracingStop()
        else:
            # Parse logcat output lines
            logcat_cmd = self._adb(
                    'logcat ActivityManager:* System.out:I *:S BENCH:*'\
                    .format(self._target.adb_name))
            self._log.info(logcat_cmd)
            logcat = Popen(logcat_cmd, shell=True, stdout=PIPE)
            self._log.debug('Iterations:')
            while True:

                # read next logcat line (up to max 1024 chars)
                message = logcat.stdout.readline(1024)

                # Benchmark start trigger
                match = JANKBENCH_BENCHMARK_START_RE.search(message)
                if match:
                    self.tracingStart()
                    self._log.debug('Benchmark started!')

                # Benchmark completed trigger
                match = JANKBENCH_BENCHMARK_DONE_RE.search(message)
                if match:
                    self._log.debug('Benchmark done!')
                    self.tracingStop()
                    break

                # Iteration completed
                match = JANKBENCH_ITERATION_COUNT_RE.search(message)
                if match:
                    self._log.debug('Iteration: %2d',
                                    int(match.group('iteration'))+1)
                # Iteration metrics
                match = JANKBENCH_ITERATION_METRICS_RE.search(message)
                if match:
                    self._log.info('   Mean: %7.3f JankP: %7.3f StdDev: %7.3f Count Bad: %4d Count Jank: %4d',
                                   float(match.group('mean')),
                                   float(match.group('jank_p')),
                                   float(match.group('std_dev')),
                                   int(match.group('count_bad')),
                                   int(match.group('count_jank')))

        # Wait until the database file is available
        db_adb = JANKBENCH_DB_PATH + JANKBENCH_DB_NAME
        while (db_adb not in self._target.execute('ls {}'.format(db_adb), check_exit_code=False)):
            sleep(1)

        # Get results
        self.db_file = os.path.join(out_dir, JANKBENCH_DB_NAME)
        self._target.pull(db_adb, self.db_file)
        self.results = self.get_results(out_dir)

        # Stop the benchmark app
        System.force_stop(self._target, self.package, clear=True)

        # Go back to home screen
        System.home(self._target)

        # Reset initial setup
        # Set orientation back to auto
        Screen.set_orientation(self._target, auto=True)

        # Turn off airplane mode
        System.set_airplane_mode(self._target, on=False)

        # Set brightness back to auto
        Screen.set_brightness(self._target, auto=True)

    @staticmethod
    def get_results(out_dir):
        """
        Extract data from results db and return as a pandas dataframe

        :param out_dir: Output directory for a run of the Jankbench workload
        :type out_dir: str
        """
        path = os.path.join(out_dir, JANKBENCH_DB_NAME)
        columns = ['_id', 'name', 'run_id', 'iteration', 'total_duration', 'jank_frame']
        data = []
        conn = sqlite3.connect(path)
        for row in conn.execute('SELECT {} FROM ui_results'.format(','.join(columns))):
            data.append(row)
        return pd.DataFrame(data, columns=columns)

# vim :set tabstop=4 shiftwidth=4 expandtab
