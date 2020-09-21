# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility class to parse the output of a gtest suite run."""

import re


class gtest_parser(object):
    """This class knows how to understand GTest test output.

    The code was borrowed with minor changes from chrome utility gtest_command.
        http://src.chromium.org/viewvc/chrome/trunk/tools/build/scripts/master/
        log_parser/gtest_command.py?view=markup
    """

    def __init__(self):
        # State tracking for log parsing
        self._current_test = ''
        self._failure_description = []
        self._current_suppression_hash = ''
        self._current_suppression = []

        # Line number currently being processed.
        self._line_number = 0

        # List of parsing errors, as human-readable strings.
        self.internal_error_lines = []

        # Tests are stored here as 'test.name': (status, [description]).
        # The status should be one of ('started', 'OK', 'failed', 'timeout').
        # The description is a list of lines detailing the test's error, as
        # reported in the log.
        self._test_status = {}

        # Suppressions are stored here as 'hash': [suppression].
        self._suppressions = {}

        # This may be either text or a number. It will be used in the phrase
        # '%s disabled' or '%s flaky' on the waterfall display.
        self.disabled_tests = 0
        self.flaky_tests = 0

        # Regular expressions for parsing GTest logs. Test names look like
        #   SomeTestCase.SomeTest
        #   SomeName/SomeTestCase.SomeTest/1
        # This regexp also matches SomeName.SomeTest/1, which should be
        # harmless.
        test_name_regexp = r'((\w+/)?\w+\.\w+(\.\w+)?(/\d+)?)'
        self._test_start = re.compile('\[\s+RUN\s+\] ' + test_name_regexp)
        self._test_ok = re.compile('\[\s+OK\s+\] ' + test_name_regexp)
        self._test_fail = re.compile('\[\s+FAILED\s+\] ' + test_name_regexp)
        self._test_timeout = re.compile(
            'Test timeout \([0-9]+ ms\) exceeded for ' + test_name_regexp)
        self._disabled = re.compile('  YOU HAVE (\d+) DISABLED TEST')
        self._flaky = re.compile('  YOU HAVE (\d+) FLAKY TEST')

        self._suppression_start = re.compile(
            'Suppression \(error hash=#([0-9A-F]+)#\):')
        self._suppression_end = re.compile('^}\s*$')

        self._master_name_re = re.compile('\[Running for master: "([^"]*)"')
        self.master_name = ''

        self._error_logging_start_re = re.compile('=' * 70)
        self._error_logging_test_name_re = re.compile(
            '[FAIL|ERROR]: ' + test_name_regexp)
        self._error_logging_end_re = re.compile('-' * 70)
        self._error_logging_first_dash_found = False

    def _TestsByStatus(self, status, include_fails, include_flaky):
        """Returns list of tests with the given status.

        Args:
            status: test results status to search for.
            include_fails: If False, tests containing 'FAILS_' anywhere in
                their names will be excluded from the list.
            include_flaky: If False, tests containing 'FLAKY_' anywhere in
                their names will be excluded from the list.
        Returns:
            List of tests with the status.
        """
        test_list = [x[0] for x in self._test_status.items()
                     if self._StatusOfTest(x[0]) == status]

        if not include_fails:
            test_list = [x for x in test_list if x.find('FAILS_') == -1]
        if not include_flaky:
            test_list = [x for x in test_list if x.find('FLAKY_') == -1]

        return test_list

    def _StatusOfTest(self, test):
        """Returns the status code for the given test, or 'not known'."""
        test_status = self._test_status.get(test, ('not known', []))
        return test_status[0]

    def _RecordError(self, line, reason):
        """Record a log line that produced a parsing error.

        Args:
            line: text of the line at which the error occurred.
            reason: a string describing the error.
        """
        self.internal_error_lines.append("%s: %s [%s]" % (self._line_number,
                                                          line.strip(),
                                                          reason))

    def TotalTests(self):
        """Returns the number of parsed tests."""
        return len(self._test_status)

    def PassedTests(self):
        """Returns list of tests that passed."""
        return self._TestsByStatus('OK', False, False)

    def FailedTests(self, include_fails=False, include_flaky=False):
        """Returns list of tests that failed, timed out, or didn't finish.

        This list will be incorrect until the complete log has been processed,
        because it will show currently running tests as having failed.

        Args:
            include_fails: If true, all failing tests with FAILS_ in their
                names will be included. Otherwise, they will only be included
                if they crashed.
            include_flaky: If true, all failing tests with FLAKY_ in their
                names will be included. Otherwise, they will only be included
                if they crashed.
        Returns:
            List of failed tests.
        """
        return (self._TestsByStatus('failed', include_fails, include_flaky) +
                self._TestsByStatus('timeout', include_fails, include_flaky) +
                self._TestsByStatus('started', include_fails, include_flaky))

    def FailureDescription(self, test):
        """Returns a list containing the failure description for the given test.

        If the test didn't fail or timeout, returns [].
        Args:
            test: Name to test to find failure reason.
        Returns:
            List of test name, and failure string.
        """
        test_status = self._test_status.get(test, ('', []))
        return test_status[1]

    def SuppressionHashes(self):
        """Returns list of suppression hashes found in the log."""
        return self._suppressions.keys()

    def Suppression(self, suppression_hash):
        """Returns a list containing the suppression for a given hash.

        If the suppression hash doesn't exist, returns [].

        Args:
            suppression_hash: name of hash.
        Returns:
            List of suppression for the hash.
        """
        return self._suppressions.get(suppression_hash, [])

    def ProcessLogLine(self, line):
        """This is called once with each line of the test log."""

        # Track line number for error messages.
        self._line_number += 1

        if not self.master_name:
            results = self._master_name_re.search(line)
            if results:
                self.master_name = results.group(1)

        # Is it a line reporting disabled tests?
        results = self._disabled.search(line)
        if results:
            try:
                disabled = int(results.group(1))
            except ValueError:
                disabled = 0
            if disabled > 0 and isinstance(self.disabled_tests, int):
                self.disabled_tests += disabled
            else:
                # If we can't parse the line, at least give a heads-up. This is
                # a safety net for a case that shouldn't happen but isn't a
                # fatal error.
                self.disabled_tests = 'some'
            return

        # Is it a line reporting flaky tests?
        results = self._flaky.search(line)
        if results:
            try:
                flaky = int(results.group(1))
            except ValueError:
                flaky = 0
            if flaky > 0 and isinstance(self.flaky_tests, int):
                self.flaky_tests = flaky
            else:
                # If we can't parse the line, at least give a heads-up. This is
                # a safety net for a case that shouldn't happen but isn't a
                # fatal error.
                self.flaky_tests = 'some'
            return

        # Is it the start of a test?
        results = self._test_start.search(line)
        if results:
            test_name = results.group(1)
            if test_name in self._test_status:
                self._RecordError(line, 'test started more than once')
                return
            if self._current_test:
                status = self._StatusOfTest(self._current_test)
                if status in ('OK', 'failed', 'timeout'):
                    self._RecordError(line, 'start while in status %s' % status)
                    return
                if status not in ('failed', 'timeout'):
                    self._test_status[self._current_test] = (
                        'failed', self._failure_description)
            self._test_status[test_name] = ('started', ['Did not complete.'])
            self._current_test = test_name
            self._failure_description = []
            return

        # Is it a test success line?
        results = self._test_ok.search(line)
        if results:
            test_name = results.group(1)
            status = self._StatusOfTest(test_name)
            if status != 'started':
                self._RecordError(line, 'success while in status %s' % status)
                return
            self._test_status[test_name] = ('OK', [])
            self._failure_description = []
            self._current_test = ''
            return

        # Is it a test failure line?
        results = self._test_fail.search(line)
        if results:
            test_name = results.group(1)
            status = self._StatusOfTest(test_name)
            if status not in ('started', 'failed', 'timeout'):
                self._RecordError(line, 'failure while in status %s' % status)
                return
            # Don't overwrite the failure description when a failing test is
            # listed a second time in the summary, or if it was already
            # recorded as timing out.
            if status not in ('failed', 'timeout'):
                self._test_status[test_name] = ('failed',
                                                self._failure_description)
            self._failure_description = []
            self._current_test = ''
            return

        # Is it a test timeout line?
        results = self._test_timeout.search(line)
        if results:
            test_name = results.group(1)
            status = self._StatusOfTest(test_name)
            if status not in ('started', 'failed'):
                self._RecordError(line, 'timeout while in status %s' % status)
                return
            self._test_status[test_name] = (
                'timeout', self._failure_description + ['Killed (timed out).'])
            self._failure_description = []
            self._current_test = ''
            return

        # Is it the start of a new valgrind suppression?
        results = self._suppression_start.search(line)
        if results:
            suppression_hash = results.group(1)
            if suppression_hash in self._suppressions:
                self._RecordError(line, 'suppression reported more than once')
                return
            self._suppressions[suppression_hash] = []
            self._current_suppression_hash = suppression_hash
            self._current_suppression = [line]
            return

        # Is it the end of a valgrind suppression?
        results = self._suppression_end.search(line)
        if results and self._current_suppression_hash:
            self._current_suppression.append(line)
            self._suppressions[self._current_suppression_hash] = (
                self._current_suppression)
            self._current_suppression_hash = ''
            self._current_suppression = []
            return

        # Is it the start of a test summary error message?
        results = self._error_logging_test_name_re.search(line)
        if results:
            test_name = results.group(1)
            self._test_status[test_name] = ('failed', ['Output not found.'])
            self._current_test = test_name
            self._failure_description = []
            self._error_logging_first_dash_found = False
            return

        # Is it the start of the next test summary signaling the end
        # of the previous message?
        results = self._error_logging_start_re.search(line)
        if results and self._current_test:
            self._test_status[self._current_test] = ('failed',
                                                     self._failure_description)
            self._failure_description = []
            self._current_test = ''
            return

        # Is it the end of the extra test failure summaries?
        results = self._error_logging_end_re.search(line)
        if results and self._current_test:
            if self._error_logging_first_dash_found:
                self._test_status[self._current_test] = (
                    'failed', self._failure_description)
                self._failure_description = []
                self._current_test = ''
            self._error_logging_first_dash_found = True
            return

        # Random line: if we're in a suppression, collect it. Suppressions are
        # generated after all tests are finished, so this should always belong
        # to the current suppression hash.
        if self._current_suppression_hash:
            self._current_suppression.append(line)
            return

        # Random line: if we're in a test, collect it for the failure
        # description. Tests may run simultaneously, so this might be off, but
        # it's worth a try.
        if self._current_test:
            self._failure_description.append(line)
