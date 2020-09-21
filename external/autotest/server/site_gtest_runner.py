# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A utility class used to run a gtest suite parsing individual tests."""

import logging, os
from autotest_lib.server import autotest, hosts, host_attributes
from autotest_lib.server import site_server_job_utils
from autotest_lib.client.common_lib import gtest_parser


class gtest_runner(object):
    """Run a gtest test suite and evaluate the individual tests."""

    def __init__(self):
        """Creates an instance of gtest_runner to run tests on a remote host."""
        self._results_dir = ''
        self._gtest = None
        self._host = None

    def run(self, gtest_entry, machine, work_dir='.'):
        """Run the gtest suite on a remote host, then parse the results.

        Like machine_worker, gtest_runner honors include/exclude attributes on
        the test item and will only run the test if the supplied host meets the
        test requirements.

        Note: This method takes a test and a machine as arguments, not a list
        of tests and a list of machines like the parallel and distribute
        methods do.

        Args:
            gtest_entry: Test tuple from control file.  See documentation in
                site_server_job_utils.test_item class.
            machine: Name (IP) if remote host to run tests on.
            work_dir: Local directory to run tests in.

        """
        self._gtest = site_server_job_utils.test_item(*gtest_entry)
        self._host = hosts.create_host(machine)
        self._results_dir = work_dir

        client_autotest = autotest.Autotest(self._host)
        client_attributes = host_attributes.host_attributes(machine)
        attribute_set = set(client_attributes.get_attributes())

        if self._gtest.validate(attribute_set):
            logging.info('%s %s Running %s', self._host,
                         [a for a in attribute_set], self._gtest)
            try:
                self._gtest.run_test(client_autotest, self._results_dir)
            finally:
                self.parse()
        else:
            self.record_failed_test(self._gtest.test_name,
                                    'No machines found for: ' + self._gtest)

    def parse(self):
        """Parse the gtest output recording individual test results.

        Uses gtest_parser to pull the test results out of the gtest log file.
        Then creates entries  in status.log file for each test.
        """
        # Find gtest log files from the autotest client run.
        log_path = os.path.join(
            self._results_dir, self._gtest.tagged_test_name,
            'debug', self._gtest.tagged_test_name + '.DEBUG')
        if not os.path.exists(log_path):
            logging.error('gtest log file "%s" is missing.', log_path)
            return

        parser = gtest_parser.gtest_parser()

        # Read the log file line-by-line, passing each line into the parser.
        with open(log_path, 'r') as log_file:
            for log_line in log_file:
                parser.ProcessLogLine(log_line)

        logging.info('gtest_runner found %d tests.', parser.TotalTests())

        # Record each failed test.
        for failed in parser.FailedTests():
            fail_description = parser.FailureDescription(failed)
            if fail_description:
                self.record_failed_test(failed, fail_description[0].strip(),
                                        ''.join(fail_description))
            else:
                self.record_failed_test(failed, 'NO ERROR LINES FOUND.')

        # Finally record each successful test.
        for passed in parser.PassedTests():
            self.record_passed_test(passed)

    def record_failed_test(self, failed_test, message, error_lines=None):
        """Insert a failure record into status.log for this test.

        Args:
           failed_test: Name of test that failed.
           message: Reason test failed, will be put in status.log file.
           error_lines: Additional failure info, will be put in ERROR log.
        """
        # Create a test name subdirectory to hold the test status.log file.
        test_dir = os.path.join(self._results_dir, failed_test)
        if not os.path.exists(test_dir):
            try:
                os.makedirs(test_dir)
            except OSError:
                logging.exception('Failed to created test directory: %s',
                                  test_dir)

        # Record failure into the global job and test specific status files.
        self._host.record('START', failed_test, failed_test)
        self._host.record('INFO', failed_test, 'FAILED: ' + failed_test)
        self._host.record('END FAIL', failed_test, failed_test, message)

        # If we have additional information on the failure, create an error log
        # file for this test in the location a normal autotest would have left
        # it so the frontend knows where to find it.
        if error_lines is not None:
            fail_log_dir = os.path.join(test_dir, 'debug')
            fail_log_path = os.path.join(fail_log_dir, failed_test + '.ERROR')

            if not os.path.exists(fail_log_dir):
                try:
                    os.makedirs(fail_log_dir)
                except OSError:
                    logging.exception('Failed to created log directory: %s',
                                      fail_log_dir)
                    return
            try:
                with open(fail_log_path, 'w') as fail_log:
                    fail_log.write(error_lines)
            except IOError:
                logging.exception('Failed to open log file: %s', fail_log_path)

    def record_passed_test(self, passed_test):
        """Insert a failure record into status.log for this test.

        Args:
            passed_test: Name of test that passed.
        """
        self._host.record('START', None, passed_test)
        self._host.record('INFO', None, 'PASSED: ' + passed_test)
        self._host.record('END GOOD', None, passed_test)
