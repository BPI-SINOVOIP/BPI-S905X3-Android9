#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
import os

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import adb
from vts.utils.python.controllers import android_device
from vts.utils.python.common import list_utils

from vts.testcases.fuzz.template.libfuzzer_test import libfuzzer_test_config as config
from vts.testcases.fuzz.template.libfuzzer_test.libfuzzer_test_case import LibFuzzerTestCase


class LibFuzzerTest(base_test.BaseTestClass):
    """Runs LLVM libfuzzer tests on target.

    Attributes:
        _dut: AndroidDevice, the device under test as config.
    """

    def setUpClass(self):
        """Creates a remote shell instance, and copies data files."""
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            keys.ConfigKeys.IKEY_BINARY_TEST_SOURCE,
        ]
        self.getUserParams(required_params)

        logging.info('%s: %s', keys.ConfigKeys.IKEY_DATA_FILE_PATH,
                     self.data_file_path)
        logging.info('%s: %s', keys.ConfigKeys.IKEY_BINARY_TEST_SOURCE,
                     self.binary_test_source)

        self._dut = self.registerController(android_device, False)[0]
        self._dut.stop()
        self._dut.adb.shell('mkdir %s -p' % config.FUZZER_TEST_DIR)

    def tearDownClass(self):
        """Deletes all copied data."""
        self._dut.adb.shell('rm -rf %s' % config.FUZZER_TEST_DIR)
        self._dut.start()

    def PushFiles(self, src):
        """adb pushes test case file to target."""
        push_src = os.path.join(self.data_file_path, src)
        push_dst = config.FUZZER_TEST_DIR
        self._dut.adb.push('%s %s' % (push_src, push_dst), no_except=True)
        logging.info('Adb pushed: %s \nto: %s', push_src, push_dst)
        return push_dst

    def CreateTestCases(self):
        """Creates LibFuzzerTestCase instances.

        Returns:
            LibFuzzerTestCase list.
        """
        test_cases = map(
            lambda x: LibFuzzerTestCase(x, config.FUZZER_DEFAULT_PARAMS, {}),
            self.binary_test_source)
        return test_cases

    # TODO: retrieve the corpus.
    def CreateCorpusDir(self, test_case):
        """Creates corpus directory on the target."""
        corpus_dir = test_case.GetCorpusName()
        self._dut.adb.shell('mkdir %s -p' % corpus_dir)

    def RunTestcase(self, test_case):
        """Runs the given test case and asserts the result.

        Args:
            test_case: LibFuzzerTestCase object
        """
        self.PushFiles(test_case.bin_host_path)
        self.CreateCorpusDir(test_case)
        fuzz_cmd = '"%s"' % test_case.GetRunCommand()
        result = self._dut.adb.shell(fuzz_cmd, no_except=True)

        # TODO: upload the corpus and, possibly, crash log.
        self.AssertTestResult(test_case, result)

    def LogCrashReport(self, test_case):
        """Logs crash-causing fuzzer input.

        Reads the crash report file and logs the contents in format:
        '\x01\x23\x45\x67\x89\xab\xcd\xef'

        Args:
            test_case: LibFuzzerTestCase object
        """
        touch_cmd = 'touch %s' % config.FUZZER_TEST_CRASH_REPORT
        self._dut.adb.shell(touch_cmd)

        # output is string of a hexdump from crash report file.
        # From the example above, output would be '0123456789abcdef'.
        xxd_cmd = 'xxd -p %s' % config.FUZZER_TEST_CRASH_REPORT
        output = self._dut.adb.shell(xxd_cmd)
        remove_chars = ['\r', '\t', '\n', ' ']
        for char in remove_chars:
            output = output.replace(char, '')

        crash_report = ''
        # output is guaranteed to be even in length since its a hexdump.
        for offset in xrange(0, len(output), 2):
            crash_report += '\\x%s' % output[offset:offset + 2]

        logging.info('FUZZER_TEST_CRASH_REPORT for %s: "%s"',
                     test_case.test_name, crash_report)

    # TODO(trong): differentiate between crashes and sanitizer rule violations.
    def AssertTestResult(self, test_case, result):
        """Asserts that test case finished as expected.

        Checks that device is in responsive state. If not, waits for boot
        then reports test as failure. If it is, asserts that all test commands
        returned exit code 0.

        Args:
            test_case: LibFuzzerTestCase object
            result: dict(str, str, int), command results from shell.
        """
        logging.info('Test case results.')
        logging.info('stdout: %s' % result[const.STDOUT])
        logging.info('stderr: %s' % result[const.STDERR])
        logging.info('exit code: %s' % result[const.EXIT_CODE])
        if not self._dut.hasBooted():
            self._dut.waitForBootCompletion()
            asserts.fail('%s left the device in unresponsive state.' %
                         test_case.test_name)

        exit_code = result[const.EXIT_CODE]
        if exit_code == config.ExitCode.FUZZER_TEST_FAIL:
            self.LogCrashReport(test_case)
            asserts.fail('%s failed normally.' % test_case.test_name)
        elif exit_code != config.ExitCode.FUZZER_TEST_PASS:
            asserts.fail('%s failed abnormally.' % test_case.test_name)

    def generateFuzzerTests(self):
        """Runs fuzzer tests."""
        self.runGeneratedTests(
            test_func=self.RunTestcase,
            settings=self.CreateTestCases(),
            name_func=lambda x: x.test_name)


if __name__ == '__main__':
    test_runner.main()
