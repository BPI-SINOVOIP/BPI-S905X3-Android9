#
# Copyright (C) 2017 The Android Open Source Project
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
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device


class CtsTest(base_test.BaseTestClass):
    """Template class for running CTS test case with VTS.

    This is the template class for running a CTS test apk which is packaged
    as part of VTS.
    """

    CTS_TESTS = [
        {
            "name": "CtsAccelerationTestCases",
            "package": "android.acceleration.cts",
            "runner": "android.support.test.runner.AndroidJUnitRunner",
            "apk": "CtsAccelerationTestCases.apk"
        },
        # TODO(zhuoyao): Add all known CTS modules.
    ]

    def setUpClass(self):
        self.dut = self.registerController(android_device)[0]
        self.dut.shell.InvokeTerminal("one")
        self.dut.shell.one.Execute("setenforce 0")  # SELinux permissive mode
        self.testcases = []
        self.CreateTestCases()

    def tearDownClass(self):
        pass

    def GetTestName(self, test_config):
        '''Get test name form a test config.'''
        return test_config["name"]

    def CreateTestCases(self):
        '''Create test configs.'''
        for testcase in self.CTS_TESTS:
            logging.info('Creating test case %s.', testcase["name"])
            self.testcases.append(testcase)

    def RunTestCase(self, test_case):
        '''Runs a test_case.

        Args:
            test_case: a cts test config.
        '''
        pass

    def generateAllTests(self):
        '''Runs all binary tests.'''
        self.runGeneratedTests(
            test_func=self.RunTestCase,
            settings=self.testcases,
            name_func=self.GetTestName)


if __name__ == "__main__":
    test_runner.main()
