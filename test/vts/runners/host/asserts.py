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

import re
import unittest

from vts.runners.host import signals


# Have an instance of unittest.TestCase so we could reuse some logic from
# python's own unittest.
# _ProxyTest is required because py2 does not allow instantiating
# unittest.TestCase directly.
class _ProxyTest(unittest.TestCase):
    def runTest(self):
        pass


_pyunit_proxy = _ProxyTest()


def assertEqual(first, second, msg=None, extras=None):
    """Assert an expression evaluates to true, otherwise fail the test.

    Error message is "first != second" by default. Additional explanation can
    be supplied in the message.

    Args:
        first, second: The arguments that will be tested for equality.
        msg: A string that adds additional info about the failure.
        extras: An optional field for extra information to be included in
                test result.
    """
    try:
        _pyunit_proxy.assertEqual(first, second)
    except AssertionError as e:
        my_msg = str(e)
        if msg:
            my_msg = "%s %s" % (my_msg, msg)
        fail(my_msg, extras=extras)


def assertNotEqual(first, second, msg=None, extras=None):
    """Assert that the first and second args are not equal, otherwise fail
    the test.

    Error message is "first == second" by default. Additional explanation can
    be supplied in the message.

    Args:
        first, second: The arguments that will be tested for inequality.
        msg: A string that adds additional info about the failure.
        extras: An optional field for extra information to be included in
                test result.
    """
    try:
        _pyunit_proxy.assertNotEqual(first, second)
    except AssertionError as e:
        my_msg = str(e)
        if msg:
            my_msg = "%s %s" % (my_msg, msg)
        fail(my_msg, extras=extras)


def assertRaises(expected_exception, extras=None, *args, **kwargs):
    """Assert that an exception is raised when a function is called.

    If no exception is raised, test fail. If an exception is raised but not
    of the expected type, the exception is let through.

    This should only be used as a context manager:
        with assertRaises(Exception):
            func()

    Args:
        expected_exception: An exception class that is expected to be
                            raised.
        extras: An optional field for extra information to be included in
                test result.
    """
    context = _AssertRaisesContext(expected_exception, extras=extras)
    return context


def assertRaisesRegex(expected_exception,
                      expected_regex,
                      extras=None,
                      *args,
                      **kwargs):
    """Assert that an exception is raised when a function is called.

    If no exception is raised, test fail. If an exception is raised but not
    of the expected type, the exception is let through. If an exception of the
    expected type is raised but the error message does not match the
    expected_regex, test fail.

    This should only be used as a context manager:
        with assertRaises(Exception):
            func()

    Args:
        expected_exception: An exception class that is expected to be
                            raised.
        extras: An optional field for extra information to be included in
                test result.
    """
    context = _AssertRaisesContext(
        expected_exception, expected_regex, extras=extras)
    return context


def assertTrue(expr, msg, extras=None):
    """Assert an expression evaluates to True, otherwise fail the test.

    Args:
        expr: The expression that is evaluated.
        msg: A string explaining the details in case of failure.
        extras: An optional field for extra information to be included in
                test result.
    """
    if not expr:
        fail(msg, extras)


def assertFalse(expr, msg, extras=None):
    """Assert an expression evaluates to False, otherwise fail the test.

    Args:
        expr: The expression that is evaluated.
        msg: A string explaining the details in case of failure.
        extras: An optional field for extra information to be included in
                test result.
    """
    if expr:
        fail(msg, extras)


def assertLess(first, second, msg=None, extras=None):
    """Assert first < second, otherwise fail the test.

    Error message is "first >= second" by default. Additional explanation can
    be supplied in the message.

    Args:
        first: The actual value which is supposed to be smaller than `second`.
        second: The threshold.
        msg: A string that adds additional info about the failure.
        extras: An optional field for extra information to be included in
                test result.
    """
    try:
        _pyunit_proxy.assertLess(first, second)
    except AssertionError as e:
        my_msg = str(e)
        if msg:
            my_msg = "%s %s" % (my_msg, msg)
        fail(my_msg, extras=extras)


def skip(reason, extras=None):
    """Skip a test case.

    Args:
        reason: The reason this test is skipped.
        extras: An optional field for extra information to be included in
                test result.

    Raises:
        signals.TestSkip is raised to mark a test case as skipped.
    """
    raise signals.TestSkip(reason, extras)


def skipIf(expr, reason, extras=None):
    """Skip a test case if expression evaluates to True.

    Args:
        expr: The expression that is evaluated.
        reason: The reason this test is skipped.
        extras: An optional field for extra information to be included in
                test result.
    """
    if expr:
        skip(reason, extras)


def abortClass(reason, extras=None):
    """Abort all subsequent test cases within the same test class in one
    iteration.

    If one test class is requested multiple times in a test run, this can
    only abort one of the requested executions, NOT all.

    Args:
        reason: The reason to abort.
        extras: An optional field for extra information to be included in
                test result.

    Raises:
        signals.TestAbortClass is raised to abort all subsequent tests in a
        test class.
    """
    raise signals.TestAbortClass(reason, extras)


def abortClassIf(expr, reason, extras=None):
    """Abort all subsequent test cases within the same test class in one
    iteration, if expression evaluates to True.

    If one test class is requested multiple times in a test run, this can
    only abort one of the requested executions, NOT all.

    Args:
        expr: The expression that is evaluated.
        reason: The reason to abort.
        extras: An optional field for extra information to be included in
                test result.

    Raises:
        signals.TestAbortClass is raised to abort all subsequent tests in a
        test class.
    """
    if expr:
        abortClass(reason, extras)


def abortAll(reason, extras=None):
    """Abort all subsequent test cases, including the ones not in this test
    class or iteration.

    Args:
        reason: The reason to abort.
        extras: An optional field for extra information to be included in
                test result.

    Raises:
        signals.TestAbortAll is raised to abort all subsequent tests.
    """
    raise signals.TestAbortAll(reason, extras)


def abortAllIf(expr, reason, extras=None):
    """Abort all subsequent test cases, if the expression evaluates to
    True.

    Args:
        expr: The expression that is evaluated.
        reason: The reason to abort.
        extras: An optional field for extra information to be included in
                test result.

    Raises:
        signals.TestAbortAll is raised to abort all subsequent tests.
    """
    if expr:
        abortAll(reason, extras)


def fail(msg, extras=None):
    """Explicitly fail a test case.

    Args:
        msg: A string explaining the details of the failure.
        extras: An optional field for extra information to be included in
                test result.

    Raises:
        signals.TestFailure is raised to mark a test case as failed.
    """
    raise signals.TestFailure(msg, extras)


def explicitPass(msg, extras=None):
    """Explicitly pass a test case.

    A test with not uncaught exception will pass implicitly so the usage of
    this is optional. It is intended for reporting extra information when a
    test passes.

    Args:
        msg: A string explaining the details of the passed test.
        extras: An optional field for extra information to be included in
                test result.

    Raises:
        signals.TestPass is raised to mark a test case as passed.
    """
    raise signals.TestPass(msg, extras)


class _AssertRaisesContext(object):
    """A context manager used to implement TestCase.assertRaises* methods."""

    def __init__(self, expected, expected_regexp=None, extras=None):
        self.expected = expected
        self.failureException = signals.TestFailure
        self.expected_regexp = expected_regexp
        self.extras = extras

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, tb):
        if exc_type is None:
            try:
                exc_name = self.expected.__name__
            except AttributeError:
                exc_name = str(self.expected)
            raise signals.TestFailure(
                "{} not raised".format(exc_name), extras=self.extras)
        if not issubclass(exc_type, self.expected):
            # let unexpected exceptions pass through
            return False
        self.exception = exc_value  # store for later retrieval
        if self.expected_regexp is None:
            return True

        expected_regexp = self.expected_regexp
        if isinstance(expected_regexp, str):
            expected_regexp = re.compile(expected_regexp)
        if not expected_regexp.search(str(exc_value)):
            raise signals.TestFailure(
                '"%s" does not match "%s"' %
                (expected_regexp.pattern, str(exc_value)),
                extras=self.extras)
        return True
