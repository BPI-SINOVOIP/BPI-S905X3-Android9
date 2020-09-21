#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging

from vts.runners.host import base_test
from vts.runners.host import records
from vts.runners.host import test_runner


class ParamTestClass(base_test.BaseTestClass):
    """Base class to run a parameterized test.

    A parameterized test is a test with a set of parameters and the test will be
    run against each parameter. This allows to test logic with different
    parameters without without writing multiple copies of the same test.

    An example use case of parameterized test is service name aware HAL testing
    which we expect to run the same test logic against all service instances
    through their corresponding service names. e.g to test graphics.composer HAL
    against two different instance: default and vr.

    Attributes:
        params: list, a list of parameters for test run.
        cur_param: the parameter used for the current run.
    """

    def __init__(self, configs):
        super(ParamTestClass, self).__init__(configs)
        self.initParams()

    def initParams(self):
        """Initialize test parameters. Expected to be overridden by a subclass."""
        self._params = []

    def getParamTag(self, param):
        """Get the test tag used to attach with test name from the parameter.

        expected to be overridden by a subclass.

        Args:
            param: the current test parameter.
        """
        return str(param)

    @property
    def params(self):
        """Get params"""
        return self._params

    @params.setter
    def params(self, params):
        """Set params"""
        self._params = params

    @property
    def cur_param(self):
        """Get cur_param"""
        return self._cur_param

    @cur_param.setter
    def cur_param(self, cur_param):
        """Set cur_param"""
        self._cur_param = cur_param

    def run(self, test_names=None):
        """Run a parameterized test.

        For each parameter initialized for the test, runs test cases within
        this test class against that parameter.

        Args:
            test_names: A list of string that are test case names requested in
                cmd line.

        Returns:
            The test results object of this class.
        """
        logging.info("==========> %s <==========", self.test_module_name)
        # Get the original tests.
        tests = self.getTests(test_names)
        # Run the set of original tests against each parameter.
        for param in self.params:
            self.cur_param = param
            for idx, (test_name, test_func) in enumerate(tests):
                param_test_name = str(test_name + self.getParamTag(param))
                tests[idx] = (param_test_name, test_func)
            if not self.run_as_vts_self_test:
                self.results.requested = [
                    records.TestResultRecord(test_name, self.test_module_name)
                    for test_name, _ in tests
                ]
            self.runTests(tests)
        return self.results
