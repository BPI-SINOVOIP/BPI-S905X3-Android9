# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Framework for host verification and repair in Autotest.

The framework provides implementation code in support of `Host.verify()`
and `Host.repair()` used in Verify and Repair special tasks.

The framework consists of these classes:
  * `Verifier`: A class representing a single verification check.
  * `RepairAction`: A class representing a repair operation that can fix
    a failed verification check.
  * `RepairStrategy`:  A class for organizing a collection of `Verifier`
    and `RepairAction` instances, and invoking them in order.

Individual operations during verification and repair are handled by
instances of `Verifier` and `RepairAction`.  `Verifier` objects are
meant to test for specific conditions that may cause tests to fail.
`RepairAction` objects provide operations designed to fix one or
more failures identified by a `Verifier` object.
"""

import collections
import logging

import common
from autotest_lib.client.common_lib import error

try:
    from chromite.lib import metrics
except ImportError:
    from autotest_lib.client.bin.utils import metrics_mock as metrics


class AutoservVerifyError(error.AutoservError):
    """
    Generic Exception for failures from `Verifier` objects.

    Instances of this exception can be raised when a `verify()`
    method fails, if no more specific exception is available.
    """
    pass


_DependencyFailure = collections.namedtuple(
        '_DependencyFailure', ('dependency', 'error'))


class AutoservVerifyDependencyError(error.AutoservError):
    """
    Exception raised for failures in dependencies.

    This exception is used to distinguish an original failure from a
    failure being passed back from a verification dependency.  That is,
    if 'B' depends on 'A', and 'A' fails, 'B' will raise this exception
    to signal that the original failure is further down the dependency
    chain.

    The `failures` argument to the constructor for this class is a set
    of instances of `_DependencyFailure`, each corresponding to one
    failed dependency:
      * The `dependency` attribute of each failure is the description
        of the failed dependency.
      * The `error` attribute of each failure is the string value of
        the exception from the failed dependency.

    Multiple methods in this module recognize and handle this exception
    specially.

    @property failures  Set of failures passed to the constructor.
    @property _node     Instance of `_DependencyNode` reporting the
                        failed dependencies.
    """

    def __init__(self, node, failures):
        """
        Constructor for `AutoservVerifyDependencyError`.

        @param node       Instance of _DependencyNode reporting the
                          failed dependencies.
        @param failures   List of failure tuples as described above.
        """
        super(AutoservVerifyDependencyError, self).__init__(
                '\n'.join([f.error for f in failures]))
        self.failures = failures
        self._node = node

    def log_dependencies(self, action, deps):
        """
        Log an `AutoservVerifyDependencyError`.

        This writes a short summary of the dependency failures captured
        in this exception, using standard Python logging.

        The passed in `action` string plus `self._node.description`
        are logged at INFO level.  The `action` argument should
        introduce or describe an action relative to `self._node`.

        The passed in `deps` string and the description of each failed
        dependency in `self` are be logged at DEBUG level.  The `deps`
        argument is used to introduce the various failed dependencies.

        @param action   A string mentioning the action being logged
                        relative to `self._node`.
        @param deps     A string introducing the dependencies that
                        failed.
        """
        logging.info('%s: %s', action, self._node.description)
        logging.debug('%s:', deps)
        for failure in self.failures:
            logging.debug('    %s', failure.dependency)


class AutoservRepairError(error.AutoservError):
    """
    Generic Exception for failures from `RepairAction` objects.

    Instances of this exception can be raised when a `repair()`
    method fails, if no more specific exception is available.
    """
    pass


class _DependencyNode(object):
    """
    An object that can depend on verifiers.

    Both repair and verify operations have the notion of dependencies
    that must pass before the operation proceeds.  This class captures
    the shared behaviors required by both classes.

    @property tag               Short identifier to be used in logging.
    @property description       Text summary of this node's action, to be
                                used in debug logs.
    @property _dependency_list  Dependency pre-requisites.
    """

    def __init__(self, tag, record_type, dependencies):
        self._dependency_list = dependencies
        self._tag = tag
        self._record_tag = record_type + '.' + tag

    def _record(self, host, silent, status_code, *record_args):
        """
        Log a status record for `host`.

        Call `host.record()` using the given status_code, and
        operation tag `self._record_tag`, plus any extra arguments in
        `record_args`.  Do nothing if `silent` is a true value.

        @param host         Host which will record the status record.
        @param silent       Don't record the event if this is a true
                            value.
        @param status_code  Value for the `status_code` parameter to
                            `host.record()`.
        @param record_args  Additional arguments to pass to
                            `host.record()`.
        """
        if not silent:
            host.record(status_code, None, self._record_tag,
                        *record_args)

    def _record_good(self, host, silent):
        """Log a 'GOOD' status line.

        @param host         Host which will record the status record.
        @param silent       Don't record the event if this is a true
                            value.
        """
        self._record(host, silent, 'GOOD')

    def _record_fail(self, host, silent, exc):
        """Log a 'FAIL' status line.

        @param host         Host which will record the status record.
        @param silent       Don't record the event if this is a true
                            value.
        @param exc          Exception describing the cause of failure.
        """
        self._record(host, silent, 'FAIL', str(exc))

    def _verify_list(self, host, verifiers, silent):
        """
        Test a list of verifiers against a given host.

        This invokes `_verify_host()` on every verifier in the given
        list.  If any verifier in the transitive closure of dependencies
        in the list fails, an `AutoservVerifyDependencyError` is raised
        containing the description of each failed verifier.  Only
        original failures are reported; verifiers that don't run due
        to a failed dependency are omitted.

        By design, original failures are logged once in `_verify_host()`
        when `verify()` originally fails.  The additional data gathered
        here is for the debug logs to indicate why a subsequent
        operation never ran.

        @param host       The host to be tested against the verifiers.
        @param verifiers  List of verifiers to be checked.
        @param silent     If true, don't log host status records.

        @raises AutoservVerifyDependencyError   Raised when at least
                        one verifier in the list has failed.
        """
        failures = set()
        for v in verifiers:
            try:
                v._verify_host(host, silent)
            except AutoservVerifyDependencyError as e:
                failures.update(e.failures)
            except Exception as e:
                failures.add(_DependencyFailure(v.description, str(e)))
        if failures:
            raise AutoservVerifyDependencyError(self, failures)

    def _verify_dependencies(self, host, silent):
        """
        Verify that all of this node's dependencies pass for a host.

        @param host     The host to be verified.
        @param silent   If true, don't log host status records.
        """
        try:
            self._verify_list(host, self._dependency_list, silent)
        except AutoservVerifyDependencyError as e:
            e.log_dependencies(
                    'Skipping this operation',
                    'The following dependencies failed')
            raise

    @property
    def tag(self):
        """
        Tag for use in logging status records.

        This is a property with a short string used to identify the node
        in the 'status.log' file and during node construction.  The tag
        should contain only letters, digits, and '_' characters.  This
        tag is not used alone, but is combined with other identifiers,
        based on the operation being logged.

        @return A short identifier-like string.
        """
        return self._tag

    @property
    def description(self):
        """
        Text description of this node for log messages.

        This string will be logged with failures, and should describe
        the condition required for success.

        N.B. Subclasses are required to override this method, but we
        _don't_ raise NotImplementedError here.  Various methods fail in
        inscrutable ways if this method raises any exception, so for
        debugging purposes, it's better to return a default value.

        @return A descriptive string.
        """
        return ('Class %s fails to implement description().' %
                type(self).__name__)


class Verifier(_DependencyNode):
    """
    Abstract class embodying one verification check.

    A concrete subclass of `Verifier` provides a simple check that can
    determine a host's fitness for testing.  Failure indicates that the
    check found a problem that can cause at least one test to fail.

    `Verifier` objects are organized in a DAG identifying dependencies
    among operations.  The DAG controls ordering and prevents wasted
    effort:  If verification operation V2 requires that verification
    operation V1 pass, then a) V1 will run before V2, and b) if V1
    fails, V2 won't run at all.  The `_verify_host()` method ensures
    that all dependencies run and pass before invoking the `verify()`
    method.

    A `Verifier` object caches its result the first time it calls
    `verify()`.  Subsequent calls return the cached result, without
    re-running the check code.  The `_reverify()` method clears the
    cached result in the current node, and in all dependencies.

    Subclasses must supply these properties and methods:
      * `verify()`: This is the method to perform the actual
        verification check.
      * `description`:  A one-line summary of the verification check for
        debug log messages.

    Subclasses must override all of the above attributes; subclasses
    should not override or extend any other attributes of this class.

    The description string should be a simple sentence explaining what
    must be true for the verifier to pass.  Do not include a terminating
    period.  For example:

        Host is available via ssh

    The base class manages the following private data:
      * `_result`:  The cached result of verification.
      * `_dependency_list`:  The list of dependencies.
    Subclasses should not use these attributes.

    @property _result           Cached result of verification.
    """

    def __init__(self, tag, dependencies):
        super(Verifier, self).__init__(tag, 'verify', dependencies)
        self._result = None

    def _reverify(self):
        """
        Discard cached verification results.

        Reset the cached verification result for this node, and for the
        transitive closure of all dependencies.
        """
        if self._result is not None:
            self._result = None
            for v in self._dependency_list:
                v._reverify()

    def _verify_host(self, host, silent):
        """
        Determine the result of verification, and log results.

        If this verifier does not have a cached verification result,
        check dependencies, and if they pass, run `verify()`.  Log
        informational messages regarding failed dependencies.  If we
        call `verify()`, log the result in `status.log`.

        If we already have a cached result, return that result without
        logging any message.

        @param host     The host to be tested for a problem.
        @param silent   If true, don't log host status records.
        """
        if self._result is not None:
            if isinstance(self._result, Exception):
                raise self._result  # cached failure
            elif self._result:
                return              # cached success
        self._result = False
        self._verify_dependencies(host, silent)
        logging.info('Verifying this condition: %s', self.description)
        try:
            self.verify(host)
            self._record_good(host, silent)
        except Exception as e:
            logging.exception('Failed: %s', self.description)
            self._result = e
            self._record_fail(host, silent, e)
            raise
        self._result = True

    def verify(self, host):
        """
        Unconditionally perform a verification check.

        This method is responsible for testing for a single problem on a
        host.  Implementations should follow these guidelines:
          * The check should find a problem that will cause testing to
            fail.
          * Verification checks on a working system should run quickly
            and should be optimized for success; a check that passes
            should finish within seconds.
          * Verification checks are not expected have side effects, but
            may apply trivial fixes if they will finish within the time
            constraints above.

        A verification check should normally trigger a single set of
        repair actions.  If two different failures can require two
        different repairs, ideally they should use two different
        subclasses of `Verifier`.

        Implementations indicate failure by raising an exception.  The
        exception text should be a short, 1-line summary of the error.
        The text should be concise and diagnostic, as it will appear in
        `status.log` files.

        If this method finds no problems, it returns without raising any
        exception.

        Implementations should avoid most logging actions, but can log
        DEBUG level messages if they provide significant information for
        diagnosing failures.

        @param host   The host to be tested for a problem.
        """
        raise NotImplementedError('Class %s does not implement '
                                  'verify()' % type(self).__name__)


class RepairAction(_DependencyNode):
    """
    Abstract class embodying one repair procedure.

    A `RepairAction` is responsible for fixing one or more failed
    `Verifier` checks, in order to make those checks pass.

    Each repair action includes one or more verifier triggers that
    determine when the repair action should run.  A repair action
    will call its `repair()` method if one or more of its triggers
    fails.  A repair action is successful if all of its triggers pass
    after calling `repair()`.

    A `RepairAction` is a subclass of `_DependencyNode`; if any of a
    repair action's dependencies fail, the action does not check its
    triggers, and doesn't call `repair()`.

    Subclasses must supply these attributes:
      * `repair()`: This is the method to perform the necessary
        repair.  The method should avoid most logging actions, but
        can log DEBUG level messages if they provide significant
        information for diagnosing failures.
      * `description`:  A one-line summary of the repair action for
        debug log messages.

    Subclasses must override both of the above attributes and should
    not override any other attributes of this class.

    The description string should be a simple sentence explaining the
    operation that will be performed.  Do not include a terminating
    period.  For example:

        Re-install the stable build via AU

    @property _trigger_list   List of verification checks that will
                              trigger this repair when they fail.
    """

    def __init__(self, tag, dependencies, triggers):
        super(RepairAction, self).__init__(tag, 'repair', dependencies)
        self._trigger_list = triggers

    def _record_start(self, host, silent):
        """Log a 'START' status line.

        @param host         Host which will record the status record.
        @param silent       Don't record the event if this is a true
                            value.
        """
        self._record(host, silent, 'START')

    def _record_end_good(self, host, silent):
        """Log an 'END GOOD' status line.

        @param host         Host which will record the status record.
        @param silent       Don't record the event if this is a true
                            value.
        """
        self._record(host, silent, 'END GOOD')
        self.status = 'repaired'

    def _record_end_fail(self, host, silent, status, *args):
        """Log an 'END FAIL' status line.

        @param host         Host which will record the status record.
        @param silent       Don't record the event if this is a true
                            value.
        @param args         Extra arguments to `self._record()`
        """
        self._record(host, silent, 'END FAIL', *args)
        self.status = status

    def _repair_host(self, host, silent):
        """
        Apply this repair action if any triggers fail.

        Repair is triggered when all dependencies are successful, and at
        least one trigger fails.

        If the `repair()` method triggers, the success or failure of
        this operation is logged in `status.log` bracketed by 'START'
        and 'END' records.  Details of whether or why `repair()`
        triggered are written to the debug logs.   If repair doesn't
        trigger, nothing is logged to `status.log`.

        @param host     The host to be repaired.
        @param silent   If true, don't log host status records.
        """
        # Note:  Every exit path from the method must set `self.status`.
        # There's a lot of exit paths, so be careful.
        #
        # If we're blocked by a failed dependency, we exit with an
        # exception.  So set status to 'blocked' first.
        self.status = 'blocked'
        self._verify_dependencies(host, silent)
        # This is a defensive action.  Every path below should overwrite
        # this setting, but if it doesn't, we want our status to reflect
        # a coding error.
        self.status = 'unknown'
        try:
            self._verify_list(host, self._trigger_list, silent)
        except AutoservVerifyDependencyError as e:
            e.log_dependencies(
                    'Attempting this repair action',
                    'Repairing because these triggers failed')
            self._record_start(host, silent)
            try:
                self.repair(host)
            except Exception as e:
                logging.exception('Repair failed: %s', self.description)
                self._record_fail(host, silent, e)
                self._record_end_fail(host, silent, 'failed-action')
                raise
            try:
                for v in self._trigger_list:
                    v._reverify()
                self._verify_list(host, self._trigger_list, silent)
                self._record_end_good(host, silent)
            except AutoservVerifyDependencyError as e:
                e.log_dependencies(
                        'This repair action reported success',
                        'However, these triggers still fail')
                self._record_end_fail(host, silent, 'failed-trigger')
                raise AutoservRepairError(
                        'Some verification checks still fail')
            except Exception:
                # The specification for `self._verify_list()` says
                # that this can't happen; this is a defensive
                # precaution.
                self._record_end_fail(host, silent, 'unknown',
                                      'Internal error in repair')
                raise
        else:
            self.status = 'untriggered'
            logging.info('No failed triggers, skipping repair:  %s',
                         self.description)

    def repair(self, host):
        """
        Apply this repair action to the given host.

        This method is responsible for applying changes to fix failures
        in one or more verification checks.  The repair is considered
        successful if the DUT passes the specific checks after this
        method completes.

        Implementations indicate failure by raising an exception.  The
        exception text should be a short, 1-line summary of the error.
        The text should be concise and diagnostic, as it will appear in
        `status.log` files.

        If this method completes successfully, it returns without
        raising any exception.

        Implementations should avoid most logging actions, but can log
        DEBUG level messages if they provide significant information for
        diagnosing failures.

        @param host   The host to be repaired.
        """
        raise NotImplementedError('Class %s does not implement '
                                  'repair()' % type(self).__name__)


class _RootVerifier(Verifier):
    """
    Utility class used by `RepairStrategy`.

    A node of this class by itself does nothing; it always passes (if it
    can run).  This class exists merely to be the root of a DAG of
    dependencies in an instance of `RepairStrategy`.
    """

    def verify(self, host):
        pass

    @property
    def description(self):
        return 'All host verification checks pass'



class RepairStrategy(object):
    """
    A class for organizing `Verifier` and `RepairAction` objects.

    An instance of `RepairStrategy` is organized as a DAG of `Verifier`
    objects, plus a list of `RepairAction` objects.  The class provides
    methods for invoking those objects in the required order, when
    needed:
      * The `verify()` method walks the verifier DAG in dependency
        order.
      * The `repair()` method invokes the repair actions in list order.
        Each repair action will invoke its dependencies and triggers as
        needed.

    # The Verifier DAG
    The verifier DAG is constructed from the first argument passed to
    the passed to the `RepairStrategy` constructor.  That argument is an
    iterable consisting of three-element tuples in the form
    `(constructor, tag, deps)`:
      * The `constructor` value is a callable that creates a `Verifier`
        as for the interface of the class constructor.  For classes
        that inherit the default constructor from `Verifier`, this can
        be the class itself.
      * The `tag` value is the tag to be associated with the constructed
        verifier.
      * The `deps` value is an iterable (e.g. list or tuple) of strings.
        Each string corresponds to the `tag` member of a `Verifier`
        dependency.

    The tag names of verifiers in the constructed DAG must all be
    unique.  The tag name defined by `RepairStrategy.ROOT_TAG` is
    reserved and may not be used by any verifier.

    In the input data for the constructor, dependencies must appear
    before the nodes that depend on them.  Thus:

        ((A, 'a', ()), (B, 'b', ('a',)))     # This is valid
        ((B, 'b', ('a',)), (A, 'a', ()))     # This will fail!

    Internally, the DAG of verifiers is given unique root node.  So,
    given this input:

        ((C, 'c', ()),
         (A, 'a', ('c',)),
         (B, 'b', ('c',)))

    The following DAG is constructed:

          Root
          /  \
         A    B
          \  /
           C

    Since nothing depends on `A` or `B`, the root node guarantees that
    these two verifiers will both be called and properly logged.

    The root node is not directly accessible; however repair actions can
    trigger on it by using `RepairStrategy.ROOT_TAG`.  Additionally, the
    node will be logged in `status.log` whenever `verify()` succeeds.

    # The Repair Actions List
    The list of repair actions is constructed from the second argument
    passed to the passed to the `RepairStrategy` constructor.  That
    argument is an iterable consisting of four-element tuples in the
    form `(constructor, tag, deps, triggers)`:
      * The `constructor` value is a callable that creates a
        `RepairAction` as for the interface of the class constructor.
        For classes that inherit the default constructor from
        `RepairAction`, this can be the class itself.
      * The `tag` value is the tag to be associated with the constructed
        repair action.
      * The `deps` value is an iterable (e.g. list or tuple) of strings.
        Each string corresponds to the `tag` member of a `Verifier` that
        the repair action depends on.
      * The `triggers` value is an iterable (e.g. list or tuple) of
        strings.  Each string corresponds to the `tag` member of a
        `Verifier` that can trigger the repair action.

    `RepairStrategy` deps and triggers can only refer to verifiers,
    not to other repair actions.
    """

    # This name is reserved; clients may not use it.
    ROOT_TAG = 'PASS'

    @staticmethod
    def _add_verifier(verifiers, constructor, tag, dep_tags):
        """
        Construct and remember a verifier.

        Create a `Verifier` using `constructor` and `tag`.  Dependencies
        for construction are found by looking up `dep_tags` in the
        `verifiers` dictionary.

        After construction, the new verifier is added to `verifiers`.

        @param verifiers    Dictionary of verifiers, indexed by tag.
        @param constructor  Verifier construction function.
        @param tag          Tag parameter for the construction function.
        @param dep_tags     Tags of dependencies for the constructor, to
                            be found in `verifiers`.
        """
        assert tag not in verifiers
        deps = [verifiers[d] for d in dep_tags]
        verifiers[tag] = constructor(tag, deps)

    def __init__(self, verifier_data, repair_data):
        """
        Construct a `RepairStrategy` from simplified DAG data.

        The input `verifier_data` object describes how to construct
        verify nodes and the dependencies that relate them, as detailed
        above.

        The input `repair_data` object describes how to construct repair
        actions and their dependencies and triggers, as detailed above.

        @param verifier_data  Iterable value with constructors for the
                              elements of the verification DAG and their
                              dependencies.
        @param repair_data    Iterable value with constructors for the
                              elements of the repair action list, and
                              their dependencies and triggers.
        """
        # Metrics - we report on 'actions' for every repair action
        # we execute; we report on 'completions' for every complete
        # repair operation.
        self._completions_counter = metrics.Counter(
                'chromeos/autotest/repair/completions')
        self._actions_counter = metrics.Counter(
                'chromeos/autotest/repair/actions')
        # We use the `all_verifiers` list to guarantee that our root
        # verifier will execute its dependencies in the order provided
        # to us by our caller.
        verifier_map = {}
        all_tags = []
        dependencies = set()
        for constructor, tag, deps in verifier_data:
            self._add_verifier(verifier_map, constructor, tag, deps)
            dependencies.update(deps)
            all_tags.append(tag)
        # Capture all the verifiers that have nothing depending on them.
        root_tags = [t for t in all_tags if t not in dependencies]
        self._add_verifier(verifier_map, _RootVerifier,
                           self.ROOT_TAG, root_tags)
        self._verify_root = verifier_map[self.ROOT_TAG]
        self._repair_actions = []
        for constructor, tag, deps, triggers in repair_data:
            r = constructor(tag,
                            [verifier_map[d] for d in deps],
                            [verifier_map[t] for t in triggers])
            self._repair_actions.append(r)

    def _count_completions(self, host, success):
        try:
            board = host.host_info_store.board or ''
        except Exception:
            board = ''
        fields = {'success': success, 'board': board}
        self._completions_counter.increment(fields=fields)
        for ra in self._repair_actions:
            fields = {'tag': ra.tag,
                      'status': ra.status,
                      'success': success,
                      'board': board}
            self._actions_counter.increment(fields=fields)

    def verify(self, host, silent=False):
        """
        Run the verifier DAG on the given host.

        @param host     The target to be verified.
        @param silent   If true, don't log host status records.
        """
        self._verify_root._reverify()
        self._verify_root._verify_host(host, silent)

    def repair(self, host, silent=False):
        """
        Run the repair list on the given host.

        @param host     The target to be repaired.
        @param silent   If true, don't log host status records.
        """
        self._verify_root._reverify()
        for ra in self._repair_actions:
            try:
                ra._repair_host(host, silent)
            except Exception as e:
                # all logging and exception handling was done at
                # lower levels
                pass
        try:
            self._verify_root._verify_host(host, silent)
        except:
            self._count_completions(host, False)
            raise
        self._count_completions(host, True)
