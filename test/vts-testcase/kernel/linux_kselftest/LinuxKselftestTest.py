#!/usr/bin/env python
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

import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.utils.python.os import path_utils

from vts.testcases.kernel.linux_kselftest import kselftest_config as config

class LinuxKselftestTest(base_test.BaseTestClass):
    """Runs Linux Kselftest test cases against Android OS kernel.

    Attributes:
        _dut: AndroidDevice, the device under test as config
        _shell: ShellMirrorObject, shell mirror
        _testcases: string list, list of testcases to run
    """
    _32BIT = 32
    _64BIT = 64

    def setUpClass(self):
        """Creates a remote shell instance, and copies data files."""
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            config.ConfigKeys.TEST_TYPE
        ]
        self.getUserParams(required_params)

        logging.info("%s: %s", keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            self.data_file_path)

        self._dut = self.android_devices[0]
        self._shell = self._dut.shell

        if self.test_type == "presubmit":
            self._testcases = config.KSFT_CASES_PRESUBMIT
        elif self.test_type == "stable":
            self._testcases = config.KSFT_CASES_STABLE
        elif self.test_type == "staging":
            self._testcases = config.KSFT_CASES_STAGING
        else:
            asserts.fail("Test config is incorrect!")

    def tearDownClass(self):
        """Deletes all copied data."""
        self._shell.Execute("rm -rf %s" % config.KSFT_DIR)

    def PushFiles(self, n_bit):
        """adb pushes related file to target.

        Args:
            n_bit: _32BIT or 32 for 32-bit tests;
                _64BIT or 64 for 64-bit tests;
        """
        self._shell.Execute("mkdir %s -p" % config.KSFT_DIR)
        test_bit = 'nativetest'
        if n_bit == self._64BIT:
            test_bit += '64'
        self._dut.adb.push("%s/DATA/%s/linux-kselftest/. %s" %
            (self.data_file_path, test_bit, config.KSFT_DIR))

    def PreTestSetup(self):
        """Sets up test before running."""
        # This sed command makes shell scripts compatible wiht android shell.
        sed_pattern = [
            's?/bin/echo?echo?',
            's?#!/bin/sh?#!/system/bin/sh?',
            's?#!/bin/bash?#!/system/bin/sh?'
        ]
        sed_cmd = 'sed %s' % ' '.join(
            ['-i -e ' + ('"%s"' % p) for p in sed_pattern])

        # This grep command is used to identify shell scripts.
        grep_pattern = [
           'bin/sh',
           'bin/bash'
        ]
        grep_cmd = 'grep -l %s' % ' '.join(
            ['-e ' + ('"%s"' % p) for p in grep_pattern])

        # This applies sed_cmd to every shell script.
        cmd = 'find %s -type f | xargs %s | xargs %s' % (
            config.KSFT_DIR, grep_cmd, sed_cmd)
        result = self._shell.Execute(cmd)

        asserts.assertFalse(
            any(result[const.EXIT_CODE]),
            "Error: pre-test setup failed.")

    def RunTestcase(self, testcase):
        """Runs the given testcase and asserts the result.

        Args:
            testcase: a LinuxKselftestTestcase object, specifies which
                test case to run.
        """
        if not testcase:
            asserts.skip("Test is not supported on this abi.")

        chmod_cmd = "chmod -R 755 %s" % path_utils.JoinTargetPath(
            config.KSFT_DIR, testcase.testsuite)
        cd_cmd = "cd %s" % path_utils.JoinTargetPath(
            config.KSFT_DIR, testcase.testsuite)

        cmd = [
            chmod_cmd,
            "%s && %s" % (cd_cmd, testcase.test_cmd)
        ]
        logging.info("Executing: %s", cmd)

        result = self._shell.Execute(cmd)
        logging.info("EXIT_CODE: %s:", result[const.EXIT_CODE])

        asserts.assertFalse(
            any(result[const.EXIT_CODE]),
            "%s failed." % testcase.testname)

    def TestNBits(self, n_bit):
        """Runs all 32-bit or all 64-bit tests.

        Args:
            n_bit: _32BIT or 32 for 32-bit tests;
                _64BIT or 64 for 64-bit tests;
        """
        self.PushFiles(n_bit)
        self.PreTestSetup()

        cpu_abi = self._dut.cpu_abi
        relevant_testcases = filter(
            lambda x: x.IsRelevant(cpu_abi, n_bit),
            self._testcases)

        self.runGeneratedTests(
            test_func=self.RunTestcase,
            settings=relevant_testcases,
            name_func=lambda testcase: "%s_%sbit" % (
                testcase.testname.replace('/','_'), n_bit))

    def generate32BitTests(self):
        """Runs all 32-bit tests."""
        self.TestNBits(self._32BIT)

    def generate64BitTests(self):
        """Runs all 64-bit tests."""
        if self._dut.is64Bit:
            self.TestNBits(self._64BIT)

if __name__ == "__main__":
    test_runner.main()
