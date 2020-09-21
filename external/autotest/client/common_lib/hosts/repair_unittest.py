# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for the `repair` module."""

# pylint: disable=missing-docstring

import functools
import logging
import unittest

import common
from autotest_lib.client.common_lib import hosts
from autotest_lib.client.common_lib.hosts import repair
from autotest_lib.server import constants
from autotest_lib.server.hosts import host_info


class _StubHost(object):
    """
    Stub class to fill in the relevant methods of `Host`.

    This class provides mocking and stub behaviors for `Host` for use by
    tests within this module.  The class implements only those methods
    that `Verifier` and `RepairAction` actually use.
    """

    def __init__(self):
        self._record_sequence = []
        fake_board_name = constants.Labels.BOARD_PREFIX + 'fubar'
        self.host_info_store = host_info.HostInfo(labels=[fake_board_name])


    def record(self, status_code, subdir, operation, status=''):
        """
        Mock method to capture records written to `status.log`.

        Each record is remembered in order to be checked for correctness
        by individual tests later.

        @param status_code  As for `Host.record()`.
        @param subdir       As for `Host.record()`.
        @param operation    As for `Host.record()`.
        @param status       As for `Host.record()`.
        """
        full_record = (status_code, subdir, operation, status)
        self._record_sequence.append(full_record)


    def get_log_records(self):
        """
        Return the records logged for this fake host.

        The returned list of records excludes records where the
        `operation` parameter is not in `tagset`.

        @param tagset   Only include log records with these tags.
        """
        return self._record_sequence


    def reset_log_records(self):
        """Clear our history of log records to allow re-testing."""
        self._record_sequence = []


class _StubVerifier(hosts.Verifier):
    """
    Stub implementation of `Verifier` for testing purposes.

    This is a full implementation of a concrete `Verifier` subclass
    designed to allow calling unit tests control over whether verify
    passes or fails.

    A `_StubVerifier()` will pass whenever the value of `_fail_count`
    is non-zero.  Calls to `try_repair()` (typically made by a
    `_StubRepairAction()`) will reduce this count, eventually
    "repairing" the verifier.

    @property verify_count  The number of calls made to the instance's
                            `verify()` method.
    @property message       If verification fails, the exception raised,
                            when converted to a string, will have this
                            value.
    @property _fail_count   The number of repair attempts required
                            before this verifier will succeed.  A
                            non-zero value means verification will fail.
    @property _description  The value of the `description` property.
    """

    def __init__(self, tag, deps, fail_count):
        super(_StubVerifier, self).__init__(tag, deps)
        self.verify_count = 0
        self.message = 'Failing "%s" by request' % tag
        self._fail_count = fail_count
        self._description = 'Testing verify() for "%s"' % tag
        self._log_record_map = {
            r[0]: r for r in [
                ('GOOD', None, self._record_tag, ''),
                ('FAIL', None, self._record_tag, self.message),
            ]
        }


    def __repr__(self):
        return '_StubVerifier(%r, %r, %r)' % (
                self.tag, self._dependency_list, self._fail_count)


    def verify(self, host):
        self.verify_count += 1
        if self._fail_count:
            raise hosts.AutoservVerifyError(self.message)


    def try_repair(self):
        """Bring ourselves one step closer to working."""
        if self._fail_count:
            self._fail_count -= 1


    def unrepair(self):
        """Make ourselves more broken."""
        self._fail_count += 1


    def get_log_record(self, status):
        """
        Return a host log record for this verifier.

        Calculates the arguments expected to be passed to
        `Host.record()` by `Verifier._verify_host()` when this verifier
        runs.  The passed in `status` corresponds to the argument of the
        same name to be passed to `Host.record()`.

        @param status   Status value of the log record.
        """
        return self._log_record_map[status]


    @property
    def description(self):
        return self._description


class _StubRepairFailure(Exception):
    """Exception to be raised by `_StubRepairAction.repair()`."""
    pass


class _StubRepairAction(hosts.RepairAction):
    """Stub implementation of `RepairAction` for testing purposes.

    This is a full implementation of a concrete `RepairAction` subclass
    designed to allow calling unit tests control over whether repair
    passes or fails.

    The behavior of `repair()` depends on the `_success` property of a
    `_StubRepairAction`.  When the property is true, repair will call
    `try_repair()` for all triggers, and then report success.  When the
    property is false, repair reports failure.

    @property repair_count  The number of calls made to the instance's
                            `repair()` method.
    @property message       If repair fails, the exception raised, when
                            converted to a string, will have this value.
    @property _success      Whether repair will follow its "success" or
                            "failure" paths.
    @property _description  The value of the `description` property.
    """

    def __init__(self, tag, deps, triggers, success):
        super(_StubRepairAction, self).__init__(tag, deps, triggers)
        self.repair_count = 0
        self.message = 'Failed repair for "%s"' % tag
        self._success = success
        self._description = 'Testing repair for "%s"' % tag
        self._log_record_map = {
            r[0]: r for r in [
                ('START', None, self._record_tag, ''),
                ('FAIL', None, self._record_tag, self.message),
                ('END FAIL', None, self._record_tag, ''),
                ('END GOOD', None, self._record_tag, ''),
            ]
        }


    def __repr__(self):
        return '_StubRepairAction(%r, %r, %r, %r)' % (
                self.tag, self._dependency_list,
                self._trigger_list, self._success)


    def repair(self, host):
        self.repair_count += 1
        if not self._success:
            raise _StubRepairFailure(self.message)
        for v in self._trigger_list:
            v.try_repair()


    def get_log_record(self, status):
        """
        Return a host log record for this repair action.

        Calculates the arguments expected to be passed to
        `Host.record()` by `RepairAction._repair_host()` when repair
        runs.  The passed in `status` corresponds to the argument of the
        same name to be passed to `Host.record()`.

        @param status   Status value of the log record.
        """
        return self._log_record_map[status]


    @property
    def description(self):
        return self._description


class _DependencyNodeTestCase(unittest.TestCase):
    """
    Abstract base class for `RepairAction` and `Verifier` test cases.

    This class provides `_make_verifier()` and `_make_repair_action()`
    methods to create `_StubVerifier` and `_StubRepairAction` instances,
    respectively, for testing.  Constructed verifiers and repair actions
    are remembered in `self.nodes`, a dictionary indexed by the tag
    used to construct the object.
    """

    def setUp(self):
        logging.disable(logging.CRITICAL)
        self._fake_host = _StubHost()
        self.nodes = {}


    def tearDown(self):
        logging.disable(logging.NOTSET)


    def _make_verifier(self, count, tag, deps):
        """
        Make a `_StubVerifier` and remember it in `self.nodes`.

        @param count  As for the `_StubVerifer` constructor.
        @param tag    As for the `_StubVerifer` constructor.
        @param deps   As for the `_StubVerifer` constructor.
        """
        verifier = _StubVerifier(tag, deps, count)
        self.nodes[tag] = verifier
        return verifier


    def _make_repair_action(self, success, tag, deps, triggers):
        """
        Make a `_StubRepairAction` and remember it in `self.nodes`.

        @param success    As for the `_StubRepairAction` constructor.
        @param tag        As for the `_StubRepairAction` constructor.
        @param deps       As for the `_StubRepairAction` constructor.
        @param triggers   As for the `_StubRepairAction` constructor.
        """
        repair_action = _StubRepairAction(tag, deps, triggers, success)
        self.nodes[tag] = repair_action
        return repair_action


    def _make_expected_failures(self, *verifiers):
        """
        Make a set of `_DependencyFailure` objects from `verifiers`.

        Return the set of `_DependencyFailure` objects that we would
        expect to see in the `failures` attribute of an
        `AutoservVerifyDependencyError` if all of the given verifiers
        report failure.

        @param verifiers  A list of `_StubVerifier` objects that are
                          expected to fail.

        @return A set of `_DependencyFailure` objects.
        """
        failures = [repair._DependencyFailure(v.description, v.message)
                    for v in verifiers]
        return set(failures)


    def _generate_silent(self):
        """
        Iterator to test different settings of the `silent` parameter.

        This iterator exists to standardize testing assertions that
        This iterator exists to standardize testing common
        assertions about the `silent` parameter:
          * When the parameter is true, no calls are made to the
            `record()` method on the target host.
          * When the parameter is false, certain expected calls are made
            to the `record()` method on the target host.

        The iterator is meant to be used like this:

            for silent in self._generate_silent():
                # run test case that uses the silent parameter
                self._check_log_records(silent, ... expected records ... )

        The code above will run its test case twice, once with
        `silent=True` and once with `silent=False`.  In between the
        calls, log records are cleared.

        @yields A boolean setting for `silent`.
        """
        for silent in [False, True]:
            yield silent
            self._fake_host.reset_log_records()


    def _check_log_records(self, silent, *record_data):
        """
        Assert that log records occurred as expected.

        Elements of `record_data` should be tuples of the form
        `(tag, status)`, describing one expected log record.
        The verifier or repair action for `tag` provides the expected
        log record based on the status value.

        The `silent` parameter is the value that was passed to the
        verifier or repair action that did the logging.  When true,
        it indicates that no records should have been logged.

        @param record_data  List describing the expected record events.
        @param silent       When true, ignore `record_data` and assert
                            that nothing was logged.
        """
        expected_records = []
        if not silent:
            for tag, status in record_data:
                expected_records.append(
                        self.nodes[tag].get_log_record(status))
        actual_records = self._fake_host.get_log_records()
        self.assertEqual(expected_records, actual_records)


class VerifyTests(_DependencyNodeTestCase):
    """
    Unit tests for `Verifier`.

    The tests in this class test the fundamental behaviors of the
    `Verifier` class:
      * Results from the `verify()` method are cached; the method is
        only called the first time that `_verify_host()` is called.
      * The `_verify_host()` method makes the expected calls to
        `Host.record()` for every call to the `verify()` method.
      * When a dependency fails, the dependent verifier isn't called.
      * Verifier calls are made in the order required by the DAG.

    The test cases don't use `RepairStrategy` to build DAG structures,
    but instead rely on custom-built DAGs.
    """

    def _generate_verify_count(self, verifier):
        """
        Iterator to force a standard sequence with calls to `_reverify()`.

        This iterator exists to standardize testing two common
        assertions:
          * The side effects from calling `_verify_host()` only
            happen on the first call to the method, except...
          * Calling `_reverify()` resets a verifier so that the
            next call to `_verify_host()` will repeat the side
            effects.

        The iterator is meant to be used like this:

            for count in self._generate_verify_cases(verifier):
                # run a verifier._verify_host() test case
                self.assertEqual(verifier.verify_count, count)
                self._check_log_records(silent, ... expected records ... )

        The code above will run the `_verify_host()` test case twice,
        then call `_reverify()` to clear cached results, then re-run
        the test case two more times.

        @param verifier   The verifier to be tested and reverified.
        @yields Each iteration yields the number of times `_reverify()`
                has been called.
        """
        for i in range(1, 3):
            for _ in range(0, 2):
                yield i
            verifier._reverify()
            self._fake_host.reset_log_records()


    def test_success(self):
        """
        Test proper handling of a successful verification.

        Construct and call a simple, single-node verification that will
        pass.  Assert the following:
          * The `verify()` method is called once.
          * The expected 'GOOD' record is logged via `Host.record()`.
          * If `_verify_host()` is called more than once, there are no
            visible side-effects after the first call.
          * Calling `_reverify()` clears all cached results.
        """
        for silent in self._generate_silent():
            verifier = self._make_verifier(0, 'pass', [])
            for count in self._generate_verify_count(verifier):
                verifier._verify_host(self._fake_host, silent)
                self.assertEqual(verifier.verify_count, count)
                self._check_log_records(silent, ('pass', 'GOOD'))


    def test_fail(self):
        """
        Test proper handling of verification failure.

        Construct and call a simple, single-node verification that will
        fail.  Assert the following:
          * The failure is reported with the actual exception raised
            by the verifier.
          * The `verify()` method is called once.
          * The expected 'FAIL' record is logged via `Host.record()`.
          * If `_verify_host()` is called more than once, there are no
            visible side-effects after the first call.
          * Calling `_reverify()` clears all cached results.
        """
        for silent in self._generate_silent():
            verifier = self._make_verifier(1, 'fail', [])
            for count in self._generate_verify_count(verifier):
                with self.assertRaises(hosts.AutoservVerifyError) as e:
                    verifier._verify_host(self._fake_host, silent)
                self.assertEqual(verifier.verify_count, count)
                self.assertEqual(verifier.message, str(e.exception))
                self._check_log_records(silent, ('fail', 'FAIL'))


    def test_dependency_success(self):
        """
        Test proper handling of dependencies that succeed.

        Construct and call a two-node verification with one node
        dependent on the other, where both nodes will pass.  Assert the
        following:
          * The `verify()` method for both nodes is called once.
          * The expected 'GOOD' record is logged via `Host.record()`
            for both nodes.
          * If `_verify_host()` is called more than once, there are no
            visible side-effects after the first call.
          * Calling `_reverify()` clears all cached results.
        """
        for silent in self._generate_silent():
            child = self._make_verifier(0, 'pass', [])
            parent = self._make_verifier(0, 'parent', [child])
            for count in self._generate_verify_count(parent):
                parent._verify_host(self._fake_host, silent)
                self.assertEqual(parent.verify_count, count)
                self.assertEqual(child.verify_count, count)
                self._check_log_records(silent,
                                        ('pass', 'GOOD'),
                                        ('parent', 'GOOD'))


    def test_dependency_fail(self):
        """
        Test proper handling of dependencies that fail.

        Construct and call a two-node verification with one node
        dependent on the other, where the dependency will fail.  Assert
        the following:
          * The verification exception is `AutoservVerifyDependencyError`,
            and the exception argument is the description of the failed
            node.
          * The `verify()` method for the failing node is called once,
            and for the other node, not at all.
          * The expected 'FAIL' record is logged via `Host.record()`
            for the single failed node.
          * If `_verify_host()` is called more than once, there are no
            visible side-effects after the first call.
          * Calling `_reverify()` clears all cached results.
        """
        for silent in self._generate_silent():
            child = self._make_verifier(1, 'fail', [])
            parent = self._make_verifier(0, 'parent', [child])
            failures = self._make_expected_failures(child)
            for count in self._generate_verify_count(parent):
                expected_exception = hosts.AutoservVerifyDependencyError
                with self.assertRaises(expected_exception) as e:
                    parent._verify_host(self._fake_host, silent)
                self.assertEqual(e.exception.failures, failures)
                self.assertEqual(child.verify_count, count)
                self.assertEqual(parent.verify_count, 0)
                self._check_log_records(silent, ('fail', 'FAIL'))


    def test_two_dependencies_pass(self):
        """
        Test proper handling with two passing dependencies.

        Construct and call a three-node verification with one node
        dependent on the other two, where all nodes will pass.  Assert
        the following:
          * The `verify()` method for all nodes is called once.
          * The expected 'GOOD' records are logged via `Host.record()`
            for all three nodes.
          * If `_verify_host()` is called more than once, there are no
            visible side-effects after the first call.
          * Calling `_reverify()` clears all cached results.
        """
        for silent in self._generate_silent():
            left = self._make_verifier(0, 'left', [])
            right = self._make_verifier(0, 'right', [])
            top = self._make_verifier(0, 'top', [left, right])
            for count in self._generate_verify_count(top):
                top._verify_host(self._fake_host, silent)
                self.assertEqual(top.verify_count, count)
                self.assertEqual(left.verify_count, count)
                self.assertEqual(right.verify_count, count)
                self._check_log_records(silent,
                                        ('left', 'GOOD'),
                                        ('right', 'GOOD'),
                                        ('top', 'GOOD'))


    def test_two_dependencies_fail(self):
        """
        Test proper handling with two failing dependencies.

        Construct and call a three-node verification with one node
        dependent on the other two, where both dependencies will fail.
        Assert the following:
          * The verification exception is `AutoservVerifyDependencyError`,
            and the exception argument has the descriptions of both the
            failed nodes.
          * The `verify()` method for each failing node is called once,
            and for the parent node not at all.
          * The expected 'FAIL' records are logged via `Host.record()`
            for the failing nodes.
          * If `_verify_host()` is called more than once, there are no
            visible side-effects after the first call.
          * Calling `_reverify()` clears all cached results.
        """
        for silent in self._generate_silent():
            left = self._make_verifier(1, 'left', [])
            right = self._make_verifier(1, 'right', [])
            top = self._make_verifier(0, 'top', [left, right])
            failures = self._make_expected_failures(left, right)
            for count in self._generate_verify_count(top):
                expected_exception = hosts.AutoservVerifyDependencyError
                with self.assertRaises(expected_exception) as e:
                    top._verify_host(self._fake_host, silent)
                self.assertEqual(e.exception.failures, failures)
                self.assertEqual(top.verify_count, 0)
                self.assertEqual(left.verify_count, count)
                self.assertEqual(right.verify_count, count)
                self._check_log_records(silent,
                                        ('left', 'FAIL'),
                                        ('right', 'FAIL'))


    def test_two_dependencies_mixed(self):
        """
        Test proper handling with mixed dependencies.

        Construct and call a three-node verification with one node
        dependent on the other two, where one dependency will pass,
        and one will fail.  Assert the following:
          * The verification exception is `AutoservVerifyDependencyError`,
            and the exception argument has the descriptions of the
            single failed node.
          * The `verify()` method for each dependency is called once,
            and for the parent node not at all.
          * The expected 'GOOD' and 'FAIL' records are logged via
            `Host.record()` for the dependencies.
          * If `_verify_host()` is called more than once, there are no
            visible side-effects after the first call.
          * Calling `_reverify()` clears all cached results.
        """
        for silent in self._generate_silent():
            left = self._make_verifier(1, 'left', [])
            right = self._make_verifier(0, 'right', [])
            top = self._make_verifier(0, 'top', [left, right])
            failures = self._make_expected_failures(left)
            for count in self._generate_verify_count(top):
                expected_exception = hosts.AutoservVerifyDependencyError
                with self.assertRaises(expected_exception) as e:
                    top._verify_host(self._fake_host, silent)
                self.assertEqual(e.exception.failures, failures)
                self.assertEqual(top.verify_count, 0)
                self.assertEqual(left.verify_count, count)
                self.assertEqual(right.verify_count, count)
                self._check_log_records(silent,
                                        ('left', 'FAIL'),
                                        ('right', 'GOOD'))


    def test_diamond_pass(self):
        """
        Test a "diamond" structure DAG with all nodes passing.

        Construct and call a "diamond" structure DAG where all nodes
        will pass:

                TOP
               /   \
            LEFT   RIGHT
               \   /
               BOTTOM

       Assert the following:
          * The `verify()` method for all nodes is called once.
          * The expected 'GOOD' records are logged via `Host.record()`
            for all nodes.
          * If `_verify_host()` is called more than once, there are no
            visible side-effects after the first call.
          * Calling `_reverify()` clears all cached results.
        """
        for silent in self._generate_silent():
            bottom = self._make_verifier(0, 'bottom', [])
            left = self._make_verifier(0, 'left', [bottom])
            right = self._make_verifier(0, 'right', [bottom])
            top = self._make_verifier(0, 'top', [left, right])
            for count in self._generate_verify_count(top):
                top._verify_host(self._fake_host, silent)
                self.assertEqual(top.verify_count, count)
                self.assertEqual(left.verify_count, count)
                self.assertEqual(right.verify_count, count)
                self.assertEqual(bottom.verify_count, count)
                self._check_log_records(silent,
                                        ('bottom', 'GOOD'),
                                        ('left', 'GOOD'),
                                        ('right', 'GOOD'),
                                        ('top', 'GOOD'))


    def test_diamond_fail(self):
        """
        Test a "diamond" structure DAG with the bottom node failing.

        Construct and call a "diamond" structure DAG where the bottom
        node will fail:

                TOP
               /   \
            LEFT   RIGHT
               \   /
               BOTTOM

        Assert the following:
          * The verification exception is `AutoservVerifyDependencyError`,
            and the exception argument has the description of the
            "bottom" node.
          * The `verify()` method for the "bottom" node is called once,
            and for the other nodes not at all.
          * The expected 'FAIL' record is logged via `Host.record()`
            for the "bottom" node.
          * If `_verify_host()` is called more than once, there are no
            visible side-effects after the first call.
          * Calling `_reverify()` clears all cached results.
        """
        for silent in self._generate_silent():
            bottom = self._make_verifier(1, 'bottom', [])
            left = self._make_verifier(0, 'left', [bottom])
            right = self._make_verifier(0, 'right', [bottom])
            top = self._make_verifier(0, 'top', [left, right])
            failures = self._make_expected_failures(bottom)
            for count in self._generate_verify_count(top):
                expected_exception = hosts.AutoservVerifyDependencyError
                with self.assertRaises(expected_exception) as e:
                    top._verify_host(self._fake_host, silent)
                self.assertEqual(e.exception.failures, failures)
                self.assertEqual(top.verify_count, 0)
                self.assertEqual(left.verify_count, 0)
                self.assertEqual(right.verify_count, 0)
                self.assertEqual(bottom.verify_count, count)
                self._check_log_records(silent, ('bottom', 'FAIL'))


class RepairActionTests(_DependencyNodeTestCase):
    """
    Unit tests for `RepairAction`.

    The tests in this class test the fundamental behaviors of the
    `RepairAction` class:
      * Repair doesn't run unless all dependencies pass.
      * Repair doesn't run unless at least one trigger fails.
      * Repair reports the expected value of `status` for metrics.
      * The `_repair_host()` method makes the expected calls to
        `Host.record()` for every call to the `repair()` method.

    The test cases don't use `RepairStrategy` to build repair
    graphs, but instead rely on custom-built structures.
    """

    def test_repair_not_triggered(self):
        """
        Test a repair that doesn't trigger.

        Construct and call a repair action with a verification trigger
        that passes.  Assert the following:
          * The `verify()` method for the trigger is called.
          * The `repair()` method is not called.
          * The repair action's `status` field is 'untriggered'.
          * The verifier logs the expected 'GOOD' message with
            `Host.record()`.
          * The repair action logs no messages with `Host.record()`.
        """
        for silent in self._generate_silent():
            verifier = self._make_verifier(0, 'check', [])
            repair_action = self._make_repair_action(True, 'unneeded',
                                                     [], [verifier])
            repair_action._repair_host(self._fake_host, silent)
            self.assertEqual(verifier.verify_count, 1)
            self.assertEqual(repair_action.repair_count, 0)
            self.assertEqual(repair_action.status, 'untriggered')
            self._check_log_records(silent, ('check', 'GOOD'))


    def test_repair_fails(self):
        """
        Test a repair that triggers and fails.

        Construct and call a repair action with a verification trigger
        that fails.  The repair fails by raising `_StubRepairFailure`.
        Assert the following:
          * The repair action fails with the `_StubRepairFailure` raised
            by `repair()`.
          * The `verify()` method for the trigger is called once.
          * The `repair()` method is called once.
          * The repair action's `status` field is 'failed-action'.
          * The expected 'START', 'FAIL', and 'END FAIL' messages are
            logged with `Host.record()` for the failed verifier and the
            failed repair.
        """
        for silent in self._generate_silent():
            verifier = self._make_verifier(1, 'fail', [])
            repair_action = self._make_repair_action(False, 'nofix',
                                                     [], [verifier])
            with self.assertRaises(_StubRepairFailure) as e:
                repair_action._repair_host(self._fake_host, silent)
            self.assertEqual(repair_action.message, str(e.exception))
            self.assertEqual(verifier.verify_count, 1)
            self.assertEqual(repair_action.repair_count, 1)
            self.assertEqual(repair_action.status, 'failed-action')
            self._check_log_records(silent,
                                    ('fail', 'FAIL'),
                                    ('nofix', 'START'),
                                    ('nofix', 'FAIL'),
                                    ('nofix', 'END FAIL'))


    def test_repair_success(self):
        """
        Test a repair that fixes its trigger.

        Construct and call a repair action that raises no exceptions,
        using a repair trigger that fails first, then passes after
        repair.  Assert the following:
          * The `repair()` method is called once.
          * The trigger's `verify()` method is called twice.
          * The repair action's `status` field is 'repaired'.
          * The expected 'START', 'FAIL', 'GOOD', and 'END GOOD'
            messages are logged with `Host.record()` for the verifier
            and the repair.
        """
        for silent in self._generate_silent():
            verifier = self._make_verifier(1, 'fail', [])
            repair_action = self._make_repair_action(True, 'fix',
                                                     [], [verifier])
            repair_action._repair_host(self._fake_host, silent)
            self.assertEqual(repair_action.repair_count, 1)
            self.assertEqual(verifier.verify_count, 2)
            self.assertEqual(repair_action.status, 'repaired')
            self._check_log_records(silent,
                                    ('fail', 'FAIL'),
                                    ('fix', 'START'),
                                    ('fail', 'GOOD'),
                                    ('fix', 'END GOOD'))


    def test_repair_noop(self):
        """
        Test a repair that doesn't fix a failing trigger.

        Construct and call a repair action with a trigger that fails.
        The repair action raises no exceptions, and after repair, the
        trigger still fails.  Assert the following:
          * The `_repair_host()` call fails with `AutoservRepairError`.
          * The `repair()` method is called once.
          * The trigger's `verify()` method is called twice.
          * The repair action's `status` field is 'failed-trigger'.
          * The expected 'START', 'FAIL', and 'END FAIL' messages are
            logged with `Host.record()` for the verifier and the repair.
        """
        for silent in self._generate_silent():
            verifier = self._make_verifier(2, 'fail', [])
            repair_action = self._make_repair_action(True, 'nofix',
                                                     [], [verifier])
            with self.assertRaises(hosts.AutoservRepairError) as e:
                repair_action._repair_host(self._fake_host, silent)
            self.assertEqual(repair_action.repair_count, 1)
            self.assertEqual(verifier.verify_count, 2)
            self.assertEqual(repair_action.status, 'failed-trigger')
            self._check_log_records(silent,
                                    ('fail', 'FAIL'),
                                    ('nofix', 'START'),
                                    ('fail', 'FAIL'),
                                    ('nofix', 'END FAIL'))


    def test_dependency_pass(self):
        """
        Test proper handling of repair dependencies that pass.

        Construct and call a repair action with a dependency and a
        trigger.  The dependency will pass and the trigger will fail and
        be repaired.  Assert the following:
          * Repair passes.
          * The `verify()` method for the dependency is called once.
          * The `verify()` method for the trigger is called twice.
          * The `repair()` method is called once.
          * The repair action's `status` field is 'repaired'.
          * The expected records are logged via `Host.record()`
            for the successful dependency, the failed trigger, and
            the successful repair.
        """
        for silent in self._generate_silent():
            dep = self._make_verifier(0, 'dep', [])
            trigger = self._make_verifier(1, 'trig', [])
            repair = self._make_repair_action(True, 'fixit',
                                              [dep], [trigger])
            repair._repair_host(self._fake_host, silent)
            self.assertEqual(dep.verify_count, 1)
            self.assertEqual(trigger.verify_count, 2)
            self.assertEqual(repair.repair_count, 1)
            self.assertEqual(repair.status, 'repaired')
            self._check_log_records(silent,
                                    ('dep', 'GOOD'),
                                    ('trig', 'FAIL'),
                                    ('fixit', 'START'),
                                    ('trig', 'GOOD'),
                                    ('fixit', 'END GOOD'))


    def test_dependency_fail(self):
        """
        Test proper handling of repair dependencies that fail.

        Construct and call a repair action with a dependency and a
        trigger, both of which fail.  Assert the following:
          * Repair fails with `AutoservVerifyDependencyError`,
            and the exception argument is the description of the failed
            dependency.
          * The `verify()` method for the failing dependency is called
            once.
          * The trigger and the repair action aren't invoked at all.
          * The repair action's `status` field is 'blocked'.
          * The expected 'FAIL' record is logged via `Host.record()`
            for the single failed dependency.
        """
        for silent in self._generate_silent():
            dep = self._make_verifier(1, 'dep', [])
            trigger = self._make_verifier(1, 'trig', [])
            repair = self._make_repair_action(True, 'fixit',
                                              [dep], [trigger])
            expected_exception = hosts.AutoservVerifyDependencyError
            with self.assertRaises(expected_exception) as e:
                repair._repair_host(self._fake_host, silent)
            self.assertEqual(e.exception.failures,
                             self._make_expected_failures(dep))
            self.assertEqual(dep.verify_count, 1)
            self.assertEqual(trigger.verify_count, 0)
            self.assertEqual(repair.repair_count, 0)
            self.assertEqual(repair.status, 'blocked')
            self._check_log_records(silent, ('dep', 'FAIL'))


class _RepairStrategyTestCase(_DependencyNodeTestCase):
    """Shared base class for testing `RepairStrategy` methods."""

    def _make_verify_data(self, *input_data):
        """
        Create `verify_data` for the `RepairStrategy` constructor.

        `RepairStrategy` expects `verify_data` as a list of tuples
        of the form `(constructor, tag, deps)`.  Each item in
        `input_data` is a tuple of the form `(tag, count, deps)` that
        creates one entry in the returned list of `verify_data` tuples
        as follows:
          * `count` is used to create a constructor function that calls
            `self._make_verifier()` with that value plus plus the
            arguments provided by the `RepairStrategy` constructor.
          * `tag` and `deps` will be passed as-is to the `RepairStrategy`
            constructor.

        @param input_data   A list of tuples, each representing one
                            tuple in the `verify_data` list.
        @return   A list suitable to be the `verify_data` parameter for
                  the `RepairStrategy` constructor.
        """
        strategy_data = []
        for tag, count, deps in input_data:
            construct = functools.partial(self._make_verifier, count)
            strategy_data.append((construct, tag, deps))
        return strategy_data


    def _make_repair_data(self, *input_data):
        """
        Create `repair_data` for the `RepairStrategy` constructor.

        `RepairStrategy` expects `repair_data` as a list of tuples
        of the form `(constructor, tag, deps, triggers)`.  Each item in
        `input_data` is a tuple of the form `(tag, success, deps, triggers)`
        that creates one entry in the returned list of `repair_data`
        tuples as follows:
          * `success` is used to create a constructor function that calls
            `self._make_verifier()` with that value plus plus the
            arguments provided by the `RepairStrategy` constructor.
          * `tag`, `deps`, and `triggers` will be passed as-is to the
            `RepairStrategy` constructor.

        @param input_data   A list of tuples, each representing one
                            tuple in the `repair_data` list.
        @return   A list suitable to be the `repair_data` parameter for
                  the `RepairStrategy` constructor.
        """
        strategy_data = []
        for tag, success, deps, triggers in input_data:
            construct = functools.partial(self._make_repair_action, success)
            strategy_data.append((construct, tag, deps, triggers))
        return strategy_data


    def _make_strategy(self, verify_input, repair_input):
        """
        Create a `RepairStrategy` from the given arguments.

        @param verify_input   As for `input_data` in
                              `_make_verify_data()`.
        @param repair_input   As for `input_data` in
                              `_make_repair_data()`.
        """
        verify_data = self._make_verify_data(*verify_input)
        repair_data = self._make_repair_data(*repair_input)
        return hosts.RepairStrategy(verify_data, repair_data)

    def _check_silent_records(self, silent):
        """
        Check that logging honored the `silent` parameter.

        Asserts that logging with `Host.record()` occurred (or did not
        occur) in accordance with the value of `silent`.

        This method only asserts the presence or absence of log records.
        Coverage for the contents of the log records is handled in other
        test cases.

        @param silent   When true, there should be no log records;
                        otherwise there should be records present.
        """
        log_records = self._fake_host.get_log_records()
        if silent:
            self.assertEqual(log_records, [])
        else:
            self.assertNotEqual(log_records, [])


class RepairStrategyVerifyTests(_RepairStrategyTestCase):
    """
    Unit tests for `RepairStrategy.verify()`.

    These unit tests focus on verifying that the `RepairStrategy`
    constructor creates the expected DAG structure from given
    `verify_data`.  Functional testing here is mainly confined to
    asserting that `RepairStrategy.verify()` properly distinguishes
    success from failure.  Testing the behavior of specific DAG
    structures is left to tests in `VerifyTests`.
    """

    def test_single_node(self):
        """
        Test construction of a single-node verification DAG.

        Assert that the structure looks like this:

            Root Node -> Main Node
        """
        verify_data = self._make_verify_data(('main', 0, ()))
        strategy = hosts.RepairStrategy(verify_data, [])
        verifier = self.nodes['main']
        self.assertEqual(
                strategy._verify_root._dependency_list,
                [verifier])
        self.assertEqual(verifier._dependency_list, [])


    def test_single_dependency(self):
        """
        Test construction of a two-node dependency chain.

        Assert that the structure looks like this:

            Root Node -> Parent Node -> Child Node
        """
        verify_data = self._make_verify_data(
                ('child', 0, ()),
                ('parent', 0, ('child',)))
        strategy = hosts.RepairStrategy(verify_data, [])
        parent = self.nodes['parent']
        child = self.nodes['child']
        self.assertEqual(
                strategy._verify_root._dependency_list, [parent])
        self.assertEqual(
                parent._dependency_list, [child])
        self.assertEqual(
                child._dependency_list, [])


    def test_two_nodes_and_dependency(self):
        """
        Test construction of two nodes with a shared dependency.

        Assert that the structure looks like this:

            Root Node -> Left Node ---\
                      \                -> Bottom Node
                        -> Right Node /
        """
        verify_data = self._make_verify_data(
                ('bottom', 0, ()),
                ('left', 0, ('bottom',)),
                ('right', 0, ('bottom',)))
        strategy = hosts.RepairStrategy(verify_data, [])
        bottom = self.nodes['bottom']
        left = self.nodes['left']
        right = self.nodes['right']
        self.assertEqual(
                strategy._verify_root._dependency_list,
                [left, right])
        self.assertEqual(left._dependency_list, [bottom])
        self.assertEqual(right._dependency_list, [bottom])
        self.assertEqual(bottom._dependency_list, [])


    def test_three_nodes(self):
        """
        Test construction of three nodes with no dependencies.

        Assert that the structure looks like this:

                       -> Node One
                      /
            Root Node -> Node Two
                      \
                       -> Node Three

        N.B.  This test exists to enforce ordering expectations of
        root-level DAG nodes.  Three nodes are used to make it unlikely
        that randomly ordered roots will match expectations.
        """
        verify_data = self._make_verify_data(
                ('one', 0, ()),
                ('two', 0, ()),
                ('three', 0, ()))
        strategy = hosts.RepairStrategy(verify_data, [])
        one = self.nodes['one']
        two = self.nodes['two']
        three = self.nodes['three']
        self.assertEqual(
                strategy._verify_root._dependency_list,
                [one, two, three])
        self.assertEqual(one._dependency_list, [])
        self.assertEqual(two._dependency_list, [])
        self.assertEqual(three._dependency_list, [])


    def test_verify(self):
        """
        Test behavior of the `verify()` method.

        Build a `RepairStrategy` with a single verifier.  Assert the
        following:
          * If the verifier passes, `verify()` passes.
          * If the verifier fails, `verify()` fails.
          * The verifier is reinvoked with every call to `verify()`;
            cached results are not re-used.
        """
        verify_data = self._make_verify_data(('tester', 0, ()))
        strategy = hosts.RepairStrategy(verify_data, [])
        verifier = self.nodes['tester']
        count = 0
        for silent in self._generate_silent():
            for i in range(0, 2):
                for j in range(0, 2):
                    strategy.verify(self._fake_host, silent)
                    self._check_silent_records(silent)
                    count += 1
                    self.assertEqual(verifier.verify_count, count)
                verifier.unrepair()
                for j in range(0, 2):
                    with self.assertRaises(Exception) as e:
                        strategy.verify(self._fake_host, silent)
                    self._check_silent_records(silent)
                    count += 1
                    self.assertEqual(verifier.verify_count, count)
                verifier.try_repair()


class RepairStrategyRepairTests(_RepairStrategyTestCase):
    """
    Unit tests for `RepairStrategy.repair()`.

    These unit tests focus on verifying that the `RepairStrategy`
    constructor creates the expected repair list from given
    `repair_data`.  Functional testing here is confined to asserting
    that `RepairStrategy.repair()` properly distinguishes success from
    failure.  Testing the behavior of specific repair structures is left
    to tests in `RepairActionTests`.
    """

    def _check_common_trigger(self, strategy, repair_tags, triggers):
        self.assertEqual(strategy._repair_actions,
                         [self.nodes[tag] for tag in repair_tags])
        for tag in repair_tags:
            self.assertEqual(self.nodes[tag]._trigger_list,
                             triggers)
            self.assertEqual(self.nodes[tag]._dependency_list, [])


    def test_single_repair_with_trigger(self):
        """
        Test constructing a strategy with a single repair trigger.

        Build a `RepairStrategy` with a single repair action and a
        single trigger.  Assert that the trigger graph looks like this:

            Repair -> Trigger

        Assert that there are no repair dependencies.
        """
        verify_input = (('base', 0, ()),)
        repair_input = (('fixit', True, (), ('base',)),)
        strategy = self._make_strategy(verify_input, repair_input)
        self._check_common_trigger(strategy,
                                   ['fixit'],
                                   [self.nodes['base']])


    def test_repair_with_root_trigger(self):
        """
        Test construction of a repair triggering on the root verifier.

        Build a `RepairStrategy` with a single repair action that
        triggers on the root verifier.  Assert that the trigger graph
        looks like this:

            Repair -> Root Verifier

        Assert that there are no repair dependencies.
        """
        root_tag = hosts.RepairStrategy.ROOT_TAG
        repair_input = (('fixit', True, (), (root_tag,)),)
        strategy = self._make_strategy([], repair_input)
        self._check_common_trigger(strategy,
                                   ['fixit'],
                                   [strategy._verify_root])


    def test_three_repairs(self):
        """
        Test constructing a strategy with three repair actions.

        Build a `RepairStrategy` with a three repair actions sharing a
        single trigger.  Assert that the trigger graph looks like this:

            Repair A -> Trigger
            Repair B -> Trigger
            Repair C -> Trigger

        Assert that there are no repair dependencies.

        N.B.  This test exists to enforce ordering expectations of
        repair nodes.  Three nodes are used to make it unlikely that
        randomly ordered actions will match expectations.
        """
        verify_input = (('base', 0, ()),)
        repair_tags = ['a', 'b', 'c']
        repair_input = (
            (tag, True, (), ('base',)) for tag in repair_tags)
        strategy = self._make_strategy(verify_input, repair_input)
        self._check_common_trigger(strategy,
                                   repair_tags,
                                   [self.nodes['base']])


    def test_repair_dependency(self):
        """
        Test construction of a repair with a dependency.

        Build a `RepairStrategy` with a single repair action that
        depends on a single verifier.  Assert that the dependency graph
        looks like this:

            Repair -> Verifier

        Assert that there are no repair triggers.
        """
        verify_input = (('base', 0, ()),)
        repair_input = (('fixit', True, ('base',), ()),)
        strategy = self._make_strategy(verify_input, repair_input)
        self.assertEqual(strategy._repair_actions,
                         [self.nodes['fixit']])
        self.assertEqual(self.nodes['fixit']._trigger_list, [])
        self.assertEqual(self.nodes['fixit']._dependency_list,
                         [self.nodes['base']])


    def _check_repair_failure(self, strategy, silent):
        """
        Check the effects of a call to `repair()` that fails.

        For the given strategy object, call the `repair()` method; the
        call is expected to fail and all repair actions are expected to
        trigger.

        Assert the following:
          * The call raises an exception.
          * For each repair action in the strategy, its `repair()`
            method is called exactly once.

        @param strategy   The strategy to be tested.
        """
        action_counts = [(a, a.repair_count)
                                 for a in strategy._repair_actions]
        with self.assertRaises(Exception) as e:
            strategy.repair(self._fake_host, silent)
        self._check_silent_records(silent)
        for action, count in action_counts:
              self.assertEqual(action.repair_count, count + 1)


    def _check_repair_success(self, strategy, silent):
        """
        Check the effects of a call to `repair()` that succeeds.

        For the given strategy object, call the `repair()` method; the
        call is expected to succeed without raising an exception and all
        repair actions are expected to trigger.

        Assert that for each repair action in the strategy, its
        `repair()` method is called exactly once.

        @param strategy   The strategy to be tested.
        """
        action_counts = [(a, a.repair_count)
                                 for a in strategy._repair_actions]
        strategy.repair(self._fake_host, silent)
        self._check_silent_records(silent)
        for action, count in action_counts:
              self.assertEqual(action.repair_count, count + 1)


    def test_repair(self):
        """
        Test behavior of the `repair()` method.

        Build a `RepairStrategy` with two repair actions each depending
        on its own verifier.  Set up calls to `repair()` for each of
        the following conditions:
          * Both repair actions trigger and fail.
          * Both repair actions trigger and succeed.
          * Both repair actions trigger; the first one fails, but the
            second one succeeds.
          * Both repair actions trigger; the first one succeeds, but the
            second one fails.

        Assert the following:
          * When both repair actions succeed, `repair()` succeeds.
          * When either repair action fails, `repair()` fails.
          * After each call to the strategy's `repair()` method, each
            repair action triggered exactly once.
        """
        verify_input = (('a', 2, ()), ('b', 2, ()))
        repair_input = (('afix', True, (), ('a',)),
                        ('bfix', True, (), ('b',)))
        strategy = self._make_strategy(verify_input, repair_input)

        for silent in self._generate_silent():
            # call where both 'afix' and 'bfix' fail
            self._check_repair_failure(strategy, silent)
            # repair counts are now 1 for both verifiers

            # call where both 'afix' and 'bfix' succeed
            self._check_repair_success(strategy, silent)
            # repair counts are now 0 for both verifiers

            # call where 'afix' fails and 'bfix' succeeds
            for tag in ['a', 'a', 'b']:
                self.nodes[tag].unrepair()
            self._check_repair_failure(strategy, silent)
            # 'a' repair count is 1; 'b' count is 0

            # call where 'afix' succeeds and 'bfix' fails
            for tag in ['b', 'b']:
                self.nodes[tag].unrepair()
            self._check_repair_failure(strategy, silent)
            # 'a' repair count is 0; 'b' count is 1

            for tag in ['a', 'a', 'b']:
                self.nodes[tag].unrepair()
            # repair counts are now 2 for both verifiers


if __name__ == '__main__':
    unittest.main()
