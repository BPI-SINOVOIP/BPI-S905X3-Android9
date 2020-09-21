#!/usr/bin/env python

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for client/common_lib/cros/retry.py."""

import itertools
import mox
import time
import unittest
import signal

import mock

import common
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.common_lib import error


class RetryTest(mox.MoxTestBase):
    """Unit tests for retry decorators.

    @var _FLAKY_FLAG: for use in tests that need to simulate random failures.
    """

    _FLAKY_FLAG = None

    def setUp(self):
        super(RetryTest, self).setUp()
        self._FLAKY_FLAG = False

        patcher = mock.patch('time.sleep', autospec=True)
        self._sleep_mock = patcher.start()
        self.addCleanup(patcher.stop)

        patcher = mock.patch('time.time', autospec=True)
        self._time_mock = patcher.start()
        self.addCleanup(patcher.stop)


    def testRetryDecoratorSucceeds(self):
        """Tests that a wrapped function succeeds without retrying."""
        @retry.retry(Exception)
        def succeed():
            return True
        self.assertTrue(succeed())
        self.assertFalse(self._sleep_mock.called)


    def testRetryDecoratorFlakySucceeds(self):
        """Tests that a wrapped function can retry and succeed."""
        delay_sec = 10
        self._time_mock.side_effect = itertools.count(delay_sec)
        @retry.retry(Exception, delay_sec=delay_sec)
        def flaky_succeed():
            if self._FLAKY_FLAG:
                return True
            self._FLAKY_FLAG = True
            raise Exception()
        self.assertTrue(flaky_succeed())


    def testRetryDecoratorFails(self):
        """Tests that a wrapped function retries til the timeout, then fails."""
        delay_sec = 10
        self._time_mock.side_effect = itertools.count(delay_sec)
        @retry.retry(Exception, delay_sec=delay_sec)
        def fail():
            raise Exception()
        self.assertRaises(Exception, fail)


    def testRetryDecoratorRaisesCrosDynamicSuiteException(self):
        """Tests that dynamic_suite exceptions raise immediately, no retry."""
        @retry.retry(Exception)
        def fail():
            raise error.ControlFileNotFound()
        self.assertRaises(error.ControlFileNotFound, fail)




class ActualRetryTest(unittest.TestCase):
    """Unit tests for retry decorators with real sleep."""

    def testRetryDecoratorFailsWithTimeout(self):
        """Tests that a wrapped function retries til the timeout, then fails."""
        @retry.retry(Exception, timeout_min=0.02, delay_sec=0.1)
        def fail():
            time.sleep(2)
            return True
        self.assertRaises(error.TimeoutException, fail)


    def testRetryDecoratorSucceedsBeforeTimeout(self):
        """Tests that a wrapped function succeeds before the timeout."""
        @retry.retry(Exception, timeout_min=0.02, delay_sec=0.1)
        def succeed():
            time.sleep(0.1)
            return True
        self.assertTrue(succeed())


    def testRetryDecoratorSucceedsWithExistingSignal(self):
        """Tests that a wrapped function succeeds before the timeout and
        previous signal being restored."""
        class TestTimeoutException(Exception):
            pass

        def testFunc():
            @retry.retry(Exception, timeout_min=0.05, delay_sec=0.1)
            def succeed():
                time.sleep(0.1)
                return True

            succeed()
            # Wait for 1.5 second for previous signal to be raised
            time.sleep(1.5)

        def testHandler(signum, frame):
            """
            Register a handler for the timeout.
            """
            raise TestTimeoutException('Expected timed out.')

        signal.signal(signal.SIGALRM, testHandler)
        signal.alarm(1)
        self.assertRaises(TestTimeoutException, testFunc)


    def testRetryDecoratorWithNoAlarmLeak(self):
        """Tests that a wrapped function throws exception before the timeout
        and no signal is leaked."""

        def testFunc():
            @retry.retry(Exception, timeout_min=0.06, delay_sec=0.1)
            def fail():
                time.sleep(0.1)
                raise Exception()


            def testHandler(signum, frame):
                """
                Register a handler for the timeout.
                """
                self.alarm_leaked = True


            # Set handler for signal.SIGALRM to catch any leaked alarm.
            self.alarm_leaked = False
            signal.signal(signal.SIGALRM, testHandler)
            try:
                fail()
            except Exception:
                pass
            # Wait for 2 seconds to check if any alarm is leaked
            time.sleep(2)
            return self.alarm_leaked

        self.assertFalse(testFunc())


if __name__ == '__main__':
    unittest.main()
