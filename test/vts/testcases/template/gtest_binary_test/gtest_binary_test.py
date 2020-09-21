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
import os
import xml.etree.ElementTree

from vts.runners.host import asserts
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner

from vts.testcases.template.binary_test import binary_test
from vts.testcases.template.binary_test import binary_test_case
from vts.testcases.template.gtest_binary_test import gtest_test_case


class GtestBinaryTest(binary_test.BinaryTest):
    '''Base class to run gtests binary on target.

    Attributes:
        DEVICE_TEST_DIR: string, temp location for storing binary
        TAG_PATH_SEPARATOR: string, separator used to separate tag and path
        shell: ShellMirrorObject, shell mirror
        tags: all the tags that appeared in binary list
        testcases: list of GtestTestCase objects, list of test cases to run
        _dut: AndroidDevice, the device under test as config
        _gtest_results: list of GtestResult objects, used during batch mode
                        for result storage and parsing
    '''

    # @Override
    def setUpClass(self):
        '''Prepare class, push binaries, set permission, create test cases.'''
        self.collect_tests_only = self.getUserParam(
            keys.ConfigKeys.IKEY_COLLECT_TESTS_ONLY, default_value=False)
        self.batch_mode = self.getUserParam(
            keys.ConfigKeys.IKEY_GTEST_BATCH_MODE, default_value=False)

        if self.batch_mode:
            if self.collect_tests_only:
                self.batch_mode = False
                logging.info("Disable batch mode when collecting tests.")
            else:
                self._gtest_results = []

        super(GtestBinaryTest, self).setUpClass()

    # @Override
    def CreateTestCase(self, path, tag=''):
        '''Create a list of GtestTestCase objects from a binary path.

        Args:
            path: string, absolute path of a gtest binary on device
            tag: string, a tag that will be appended to the end of test name

        Returns:
            A list of GtestTestCase objects on success; an empty list otherwise.
            In non-batch mode, each object respresents a test case in the
            gtest binary located at the provided path. Usually there are more
            than one object returned.
            In batch mode, each object represents a gtest binary located at
            the provided path; the returned list will always be a one object
            list in batch mode. Test case names are stored in full_name
            property in the object, delimited by ':' according to gtest
            documentation, after being filtered and processed according to
            host configuration.
        '''
        working_directory = self.working_directory[
            tag] if tag in self.working_directory else None
        envp = self.envp[tag] if tag in self.envp else ''
        args = self.args[tag] if tag in self.args else ''
        ld_library_path = self.ld_library_path[
            tag] if tag in self.ld_library_path else None
        profiling_library_path = self.profiling_library_path[
            tag] if tag in self.profiling_library_path else None

        gtest_list_args = args + " --gtest_list_tests"
        list_test_case = binary_test_case.BinaryTestCase(
            'gtest_list_tests',
            path,
            path,
            tag,
            self.PutTag,
            working_directory,
            ld_library_path,
            profiling_library_path,
            envp=envp,
            args=gtest_list_args)
        cmd = ['chmod 755 %s' % path, list_test_case.GetRunCommand()]
        cmd_results = self.shell.Execute(cmd)
        test_cases = []
        if any(cmd_results[const.EXIT_CODE]
               ):  # gtest binary doesn't exist or is corrupted
            logging.error(
                'Failed to list test cases from %s. Command: %s, Result: %s.' %
                (path, cmd, cmd_results))
            return test_cases

        test_suite = ''
        for line in cmd_results[const.STDOUT][1].split('\n'):
            line = str(line)
            if not len(line.strip()):
                continue
            elif line.startswith(' '):  # Test case name
                test_name = line.split('#')[0].strip()
                test_case = gtest_test_case.GtestTestCase(
                    test_suite, test_name, path, tag, self.PutTag,
                    working_directory, ld_library_path, profiling_library_path,
                    envp=envp, args=args)
                logging.info('Gtest test case: %s' % test_case)
                test_cases.append(test_case)
            else:  # Test suite name
                test_suite = line.strip()
                if test_suite.endswith('.'):
                    test_suite = test_suite[:-1]

        if not self.batch_mode:
            return test_cases

        # Gtest batch mode
        test_names = map(lambda test: test.full_name, test_cases)

        gtest_batch = gtest_test_case.GtestTestCase(
            path, '', path, tag, self.PutTag, working_directory,
            ld_library_path, profiling_library_path, envp=envp)
        gtest_batch.full_name = ':'.join(test_names)
        return [gtest_batch]

    # @Override
    def VerifyTestResult(self, test_case, command_results):
        '''Parse Gtest xml result output.

        Sample
        <testsuites tests="1" failures="1" disabled="0" errors="0"
         timestamp="2017-05-24T18:32:10" time="0.012" name="AllTests">
          <testsuite name="ConsumerIrHidlTest"
           tests="1" failures="1" disabled="0" errors="0" time="0.01">
            <testcase name="TransmitTest" status="run" time="0.01"
             classname="ConsumerIrHidlTest">
              <failure message="hardware/interfaces..." type="">
                <![CDATA[hardware/interfaces...]]>
              </failure>
            </testcase>
          </testsuite>
        </testsuites>

        Args:
            test_case: GtestTestCase object, the test being run. This param
                       is not currently used in this method.
            command_results: dict of lists, shell command result
        '''
        asserts.assertTrue(command_results, 'Empty command response.')
        asserts.assertEqual(
            len(command_results), 3, 'Abnormal command response.')
        for item in command_results.values():
            asserts.assertEqual(
                len(item), 2,
                'Abnormal command result length: %s' % command_results)

        for stderr in command_results[const.STDERR]:
            if stderr and stderr.strip():
                for line in stderr.split('\n'):
                    logging.error(line)

        xml_str = command_results[const.STDOUT][1].strip()

        if self.batch_mode:
            self._ParseBatchResults(test_case, xml_str)
            return

        for stdout in command_results[const.STDOUT]:
            if stdout and stdout.strip():
                for line in stdout.split('\n'):
                    logging.info(line)

        asserts.assertFalse(
            command_results[const.EXIT_CODE][1],
            'Failed to show Gtest XML output: %s' % command_results)

        root = xml.etree.ElementTree.fromstring(xml_str)
        asserts.assertEqual(root.get('tests'), '1', 'No tests available')
        if root.get('errors') != '0' or root.get('failures') != '0':
            messages = [x.get('message') for x in root.findall('.//failure')]
            asserts.fail('\n'.join([x for x in messages if x]))
        asserts.skipIf(root.get('disabled') == '1', 'Gtest test case disabled')

    def _ParseBatchResults(self, test_case_original, xml_str):
        '''Parse batch mode gtest results

        Args:
            test_case_original: GtestTestCase object, original batch test case object
            xml_str: string, result xml output content
        '''
        root = xml.etree.ElementTree.fromstring(xml_str)

        for test_suite in root:
            print test_suite.tag, test_suite.attrib
            for test_case in test_suite:
                result = gtest_test_case.GtestTestCase(
                    test_suite.get('name'),
                    test_case.get('name'), '', test_case_original.tag,
                    self.PutTag, name_appendix=test_case_original.name_appendix)

                failure_message = None
                for sub in test_case:
                    if sub.tag == 'failure':
                        failure_message = sub.get('message')

                if len(test_case) and not failure_message:
                    failure_message = 'Error: %s\n' % test_case.attrib
                    for sub in test_case:
                        failure_message += '%s: %s\n' % (sub.tag, sub.attrib)

                result.failure_message = failure_message

                self._gtest_results.append(result)

    def _VerifyBatchResult(self, gtest_result):
        '''Check a gtest test case result in batch mode

        Args:
            gtest_result: GtestTestCase object, representing gtest result
        '''
        asserts.assertFalse(gtest_result.failure_message,
                            gtest_result.failure_message)

    # @Override
    def generateAllTests(self):
        '''Runs all binary tests.'''
        if self.batch_mode:
            for test_case in self.testcases:
                self.RunTestCase(test_case)

                self.runGeneratedTests(
                    test_func=self._VerifyBatchResult,
                    settings=self._gtest_results,
                    name_func=str)

                self._gtest_results = []
            return

        self.runGeneratedTests(
            test_func=self.RunTestCase, settings=self.testcases, name_func=str)


if __name__ == "__main__":
    test_runner.main()
