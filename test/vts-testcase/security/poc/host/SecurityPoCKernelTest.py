#!/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
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

import json
import logging
import os

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import adb
from vts.utils.python.controllers import android_device
from vts.utils.python.os import path_utils

from vts.testcases.security.poc.host import poc_test_config as config

class SecurityPoCKernelTest(base_test.BaseTestClass):
    """Runs security PoC kernel test cases.

    Attributes:
        _dut: AndroidDevice, the device under test as config
        _testcases: string list, list of testcases to run
        _model: string, device model e.g. "Nexus 5X"
    """
    def setUpClass(self):
        """Creates device under test instance, and copies data files."""
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            keys.ConfigKeys.IKEY_ABI_BITNESS,
            config.ConfigKeys.RUN_STAGING
        ]
        self.getUserParams(required_params)

        logging.info("%s: %s", keys.ConfigKeys.IKEY_DATA_FILE_PATH,
                self.data_file_path)

        self._dut = self.registerController(android_device, False)[0]
        self._testcases = config.POC_TEST_CASES_STABLE
        if self.run_staging:
            self._testcases += config.POC_TEST_CASES_STAGING

    def tearDownClass(self):
        """Deletes all copied data."""
        self._dut.adb.shell("rm -rf %s" % config.POC_TEST_DIR)

    def PushFiles(self):
        """adb pushes related file to target."""
        self._dut.adb.shell("mkdir %s -p" % config.POC_TEST_DIR)

        bitness = getattr(self, keys.ConfigKeys.IKEY_ABI_BITNESS)
        bitness_suffix = "64" if bitness == "64" else ""
        native_test_dir = "nativetest{0}".format(bitness_suffix)
        push_src = os.path.join(self.data_file_path, "DATA", native_test_dir,
                                "security", "poc", ".")
        self._dut.adb.push("%s %s" % (push_src, config.POC_TEST_DIR))

    def CreateHostInput(self, testcase):
        """Gathers information that will be passed to target-side code.

        Args:
            testcase: string, format testsuite/testname, specifies which
                test case to examine.

        Returns:
            dict, information passed to native PoC test, contains info collected
                from device and config. If None, poc should be skipped.
        """
        out = self._dut.adb.shell("getprop ro.product.model")
        device_model = out.strip()
        testcase_path = os.path.join(*testcase.split("/"))
        test_config_path = os.path.join(
            self.data_file_path, "vts", "testcases", "security", "poc",
            "target", testcase_path, "poc.runner_conf")

        with open(test_config_path) as test_config_file:
            poc_config = json.load(test_config_file)["target_models"]

            # If dut model is not in the test config, test should be skipped.
            if not device_model in poc_config.keys():
                return None

            params = poc_config.get("default", {})
            params.update(poc_config[device_model])

        host_input = {
            "device_model": device_model,
            "params": params
        }

        return host_input

    def CreateTestFlags(self, host_input):
        """Packs host input info into command line flags.

        Args:
            host_input: dict, information passed to native PoC test.

        Returns:
            string, host_input packed into command-line flags.
        """
        device_model_flag = "--device_model=\"%s\"" % host_input["device_model"]

        params = ["%s=%s" % (k, v) for k, v in host_input["params"].items()]
        params = ",".join(params)
        params_flag = "--params=\"%s\"" % params

        test_flags = [device_model_flag, params_flag]
        return " ".join(test_flags)

    def RunTestcase(self, testcase):
        """Runs the given testcase and asserts the result.

        Args:
            testcase: string, format testsuite/testname, specifies which
                test case to run.
        """
        host_input = self.CreateHostInput(testcase)
        asserts.skipIf(not host_input,
                "%s not configured to run against this target model." % testcase)

        items = testcase.split("/", 1)
        testsuite = items[0]

        chmod_cmd = "chmod -R 755 %s" % path_utils.JoinTargetPath(
            config.POC_TEST_DIR, testsuite)
        logging.info("Executing: %s", chmod_cmd)
        self._dut.adb.shell(chmod_cmd)

        test_flags = self.CreateTestFlags(host_input)
        test_cmd = "%s %s" % (
            path_utils.JoinTargetPath(config.POC_TEST_DIR, testcase),
            test_flags)
        logging.info("Executing: %s", test_cmd)

        try:
            stdout = self._dut.adb.shell(test_cmd)
            result = {
                const.STDOUT: stdout,
                const.STDERR: "",
                const.EXIT_CODE: 0
            }
        except adb.AdbError as e:
            result = {
                const.STDOUT: e.stdout,
                const.STDERR: e.stderr,
                const.EXIT_CODE: e.ret_code
            }
        logging.info("Test results:\n%s", result)

        self.AssertTestResult(result)

    def AssertTestResult(self, result):
        """Asserts that testcase finished as expected.

        Checks that device is in responsive state. If not, waits for boot
        then reports test as failure. If it is, asserts that all test commands
        returned exit code 0.

        Args:
            result: dict(str, str, int), stdout, stderr and return code
                from test run.
        """
        if self._dut.hasBooted():
            exit_code = result[const.EXIT_CODE]
            asserts.skipIf(exit_code == config.ExitCode.POC_TEST_SKIP,
                    "Test case was skipped.")
            asserts.assertFalse(exit_code == config.ExitCode.POC_TEST_FAIL,
                    "Test case failed.")
        else:
            self._dut.waitForBootCompletion()
            self._dut.rootAdb()
            self.PushFiles()
            asserts.fail("Test case left the device in unresponsive state.")

    def generateSecurityPoCTests(self):
        """Runs security PoC tests."""
        self.PushFiles()
        self.runGeneratedTests(
            test_func=self.RunTestcase,
            settings=self._testcases,
            name_func=lambda x: x.replace('/','_'))

if __name__ == "__main__":
    test_runner.main()
