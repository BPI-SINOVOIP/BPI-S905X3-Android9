# Copyright 2018, The Android Open Source Project
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
Robolectric test runner class.

This test runner will be short lived, once robolectric support v2 is in, then
robolectric tests will be invoked through AtestTFTestRunner.
"""

import logging

# pylint: disable=import-error
import atest_utils
import constants
import test_runner_base


class RobolectricTestRunner(test_runner_base.TestRunnerBase):
    """Robolectric Test Runner class."""
    NAME = 'RobolectricTestRunner'
    # We don't actually use EXECUTABLE because we're going to use
    # atest_utils.build to kick off the test but if we don't set it, the base
    # class will raise an exception.
    EXECUTABLE = 'make'

    def __init__(self, results_dir, **kwargs):
        """Init stuff for robolectric runner class."""
        super(RobolectricTestRunner, self).__init__(results_dir)

    def run_tests(self, test_infos, extra_args):
        """Run the list of test_infos.

        Args:
            test_infos: List of TestInfo.
            extra_args: Dict of extra args to add to test run.
        """
        for test_info in test_infos:
            env_vars = self.generate_env_vars(test_info, extra_args)
            atest_utils.build(set([test_info.test_name]), verbose=True,
                              env_vars=env_vars)

    @staticmethod
    def generate_env_vars(test_info, extra_args):
        """Turn the args into env vars.

        Robolectric tests specify args through env vars, so look for class
        filters and debug args to apply to the env.

        Args:
            test_info: TestInfo class that holds the class filter info.
            extra_args: Dict of extra args to apply for test run.

        Returns:
            Dict of env vars to pass into invocation.
        """
        env_var = {}
        for arg in extra_args:
            if constants.WAIT_FOR_DEBUGGER == arg:
                env_var['DEBUG_ROBOLECTRIC'] = 'true'
                continue
        filters = test_info.data.get(constants.TI_FILTER)
        if filters:
            robo_filter = next(iter(filters))
            env_var['ROBOTEST_FILTER'] = robo_filter.class_name
            if robo_filter.methods:
                logging.debug('method filtering not supported for robolectric '
                              'tests yet.')
        return env_var

    def host_env_check(self):
        """Check that host env has everything we need.

        We actually can assume the host env is fine because we have the same
        requirements that atest has. Update this to check for android env vars
        if that changes.
        """
        pass

    def get_test_runner_build_reqs(self):
        """Return the build requirements.

        Returns:
            Set of build targets.
        """
        return set()
