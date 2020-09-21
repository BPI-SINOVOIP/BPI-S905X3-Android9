# Copyright 2017, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Base test runner class.

Class that other test runners will instantiate for test runners.
"""

import logging
import subprocess

# pylint: disable=import-error
import atest_error


class TestRunnerBase(object):
    """Base Test Runner class."""
    NAME = ''
    EXECUTABLE = ''

    def __init__(self, results_dir, **kwargs):
        """Init stuff for base class."""
        self.results_dir = results_dir
        if not self.NAME:
            raise atest_error.NoTestRunnerName('Class var NAME is not defined.')
        if not self.EXECUTABLE:
            raise atest_error.NoTestRunnerExecutable('Class var EXECUTABLE is '
                                                     'not defined.')
        if kwargs:
            logging.info('ignoring the following args: %s', kwargs)

    @staticmethod
    def run(cmd):
        """Shell out and execute command.

        Args:
            cmd: A string of the command to execute.
        """
        logging.info('Executing command: %s', cmd)
        subprocess.check_call(cmd, shell=True, stderr=subprocess.STDOUT)

    def run_tests(self, test_infos, extra_args):
        """Run the list of test_infos.

        Args:
            test_infos: List of TestInfo.
            extra_args: Dict of extra args to add to test run.
        """
        raise NotImplementedError

    def host_env_check(self):
        """Checks that host env has met requirements."""
        raise NotImplementedError

    def get_test_runner_build_reqs(self):
        """Returns a list of build targets required by the test runner."""
        raise NotImplementedError
