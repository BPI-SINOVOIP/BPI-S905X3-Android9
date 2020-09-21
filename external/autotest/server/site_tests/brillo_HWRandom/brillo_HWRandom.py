# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging
import os
import tempfile
from autotest_lib.client.common_lib import error
from autotest_lib.server import test
from autotest_lib.server import utils


class brillo_HWRandom(test.test):
    """Tests that /dev/hw_random is present and passes basic tests."""
    version = 1

    # Basic info for a dieharder test.
    TestInfo = collections.namedtuple('TestInfo', 'number custom_args')

    # Basic results of a dieharder test.
    TestResult = collections.namedtuple('TestResult', 'test_name assessment')

    # Results of a test suite run.
    TestSuiteResult = collections.namedtuple('TestSuiteResult',
                                             'num_weak num_failed full_output')

    # A list of dieharder tests that can be reasonably constrained to run within
    # a sample space of <= 10MB, and the arguments to constrain them. These have
    # been applied somewhat naively and over time these can be tweaked if a test
    # has a problematic failure rate. In general, since there is only so much
    # that can be done within the constraints these tests should be viewed as a
    # sanity check and not as a measure of entropy quality. If a hardware RNG
    # repeatedly fails this test, it has a big problem and should not be used.
    _TEST_LIST = [
        TestInfo(number=0, custom_args=['-p', '50']),
        TestInfo(number=1, custom_args=['-p', '50', '-t', '50000']),
        TestInfo(number=2, custom_args=['-p', '50', '-t', '1000']),
        TestInfo(number=3, custom_args=['-p', '50', '-t', '5000']),
        TestInfo(number=8, custom_args=['-p', '40']),
        TestInfo(number=10, custom_args=[]),
        TestInfo(number=11, custom_args=[]),
        TestInfo(number=12, custom_args=[]),
        TestInfo(number=15, custom_args=['-p', '50', '-t', '50000']),
        TestInfo(number=16, custom_args=['-p', '50', '-t', '7000']),
        TestInfo(number=17, custom_args=['-p', '50', '-t', '20000']),
        TestInfo(number=100, custom_args=['-p', '50', '-t', '50000']),
        TestInfo(number=101, custom_args=['-p', '50', '-t', '50000']),
        TestInfo(number=102, custom_args=['-p', '50', '-t', '50000']),
        TestInfo(number=200, custom_args=['-p', '20', '-t', '20000',
                                          '-n', '3']),
        TestInfo(number=202, custom_args=['-p', '20', '-t', '20000']),
        TestInfo(number=203, custom_args=['-p', '50', '-t', '50000']),
        TestInfo(number=204, custom_args=['-p', '200']),
        TestInfo(number=205, custom_args=['-t', '512000']),
        TestInfo(number=206, custom_args=['-t', '40000', '-n', '64']),
        TestInfo(number=207, custom_args=['-t', '300000']),
        TestInfo(number=208, custom_args=['-t', '400000']),
        TestInfo(number=209, custom_args=['-t', '2000000']),
    ]

    def _run_dieharder_test(self, input_file, test_number, custom_args=None):
        """Runs a specific dieharder test (locally) and returns the assessment.

        @param input_file: The name of the file containing the data to be tested
        @param test_number: A dieharder test number specifying which test to run
        @param custom_args: Optional additional arguments for the test

        @returns A list of TestResult

        @raise TestError: An error occurred running the test.
        """
        command = ['dieharder',
                   '-g', '201',
                   '-D', 'test_name',
                   '-D', 'ntuple',
                   '-D', 'assessment',
                   '-D', '32768',  # no_whitespace
                   '-c', ',',
                   '-d', str(test_number),
                   '-f', input_file]
        if custom_args:
            command.extend(custom_args)
        command_result = utils.run(command)
        if command_result.stderr != '':
            raise error.TestError('Error running dieharder: %s' %
                                  command_result.stderr.rstrip())
        output = command_result.stdout.splitlines()
        results = []
        for line in output:
            fields = line.split(',')
            if len(fields) != 3:
                raise error.TestError(
                    'dieharder: unexpected output: %s' % line)
            results.append(self.TestResult(
                test_name='%s[%s]' % (fields[0], fields[1]),
                assessment=fields[2]))
        return results


    def _run_all_dieharder_tests(self, input_file):
        """Runs all the dieharder tests in _TEST_LIST, continuing on failure.

        @param input_file: The name of the file containing the data to be tested

        @returns TestSuiteResult

        @raise TestError: An error occurred running the test.
        """
        weak = 0
        failed = 0
        full_output = 'Test Results:\n'
        for test_info in self._TEST_LIST:
            results = self._run_dieharder_test(input_file,
                                               test_info.number,
                                               test_info.custom_args)
            for test_result in results:
                logging.info('%s: %s', test_result.test_name,
                             test_result.assessment)
                full_output += '  %s: %s\n' % test_result
                if test_result.assessment == 'WEAK':
                    weak += 1
                elif test_result.assessment == 'FAILED':
                    failed += 1
                elif test_result.assessment != 'PASSED':
                    raise error.TestError(
                        'Unexpected output: %s' % full_output)
        logging.info('Total: %d, Weak: %d, Failed: %d',
                     len(self._TEST_LIST), weak, failed)
        return self.TestSuiteResult(weak, failed, full_output)

    def run_once(self, host=None):
        """Runs the test.

        @param host: A host object representing the DUT.

        @raise TestError: An error occurred running the test.
        @raise TestFail: The test ran without error but failed.
        """
        # Grab 10MB of data from /dev/hw_random.
        dut_file = '/data/local/tmp/hw_random_output'
        host.run('dd count=20480 if=/dev/hw_random of=%s' % dut_file)
        with tempfile.NamedTemporaryFile() as local_file:
            host.get_file(dut_file, local_file.name)
            output_size = os.stat(local_file.name).st_size
            if output_size != 0xA00000:
                raise error.TestError(
                    'Unexpected output length: %d (expecting %d)',
                    output_size, 0xA00000)
            # Run the data through each test (even if one fails).
            result = self._run_all_dieharder_tests(local_file.name)
            if result.num_failed > 0 or result.num_weak > 5:
                raise error.TestFail(result.full_output)
