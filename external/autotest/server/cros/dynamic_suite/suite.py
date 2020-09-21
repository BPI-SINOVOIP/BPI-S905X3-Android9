# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import abc
import datetime
import difflib
import functools
import hashlib
import logging
import operator
import os
import re
import sys
import warnings

import common

from autotest_lib.frontend.afe.json_rpc import proxy
from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import enum
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import priorities
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib import utils
from autotest_lib.frontend.afe import model_attributes
from autotest_lib.frontend.afe.json_rpc import proxy
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.dynamic_suite import control_file_getter
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server.cros.dynamic_suite import job_status
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.server.cros.dynamic_suite.job_status import Status

try:
    from chromite.lib import boolparse_lib
    from chromite.lib import cros_logging as logging
except ImportError:
    print 'Unable to import chromite.'
    print 'This script must be either:'
    print '  - Be run in the chroot.'
    print '  - (not yet supported) be run after running '
    print '    ../utils/build_externals.py'

_FILE_BUG_SUITES = ['au', 'bvt', 'bvt-cq', 'bvt-inline', 'paygen_au_beta',
                    'paygen_au_canary', 'paygen_au_dev', 'paygen_au_stable',
                    'sanity', 'push_to_prod']
_AUTOTEST_DIR = global_config.global_config.get_config_value(
        'SCHEDULER', 'drone_installation_directory')
ENABLE_CONTROLS_IN_BATCH = global_config.global_config.get_config_value(
        'CROS', 'enable_getting_controls_in_batch', type=bool, default=False)

class RetryHandler(object):
    """Maintain retry information.

    @var _retry_map: A dictionary that stores retry history.
            The key is afe job id. The value is a dictionary.
            {job_id: {'state':RetryHandler.States, 'retry_max':int}}
            - state:
                The retry state of a job.
                NOT_ATTEMPTED:
                    We haven't done anything about the job.
                ATTEMPTED:
                    We've made an attempt to schedule a retry job. The
                    scheduling may or may not be successful, e.g.
                    it might encounter an rpc error. Note failure
                    in scheduling a retry is different from a retry job failure.
                    For each job, we only attempt to schedule a retry once.
                    For example, assume we have a test with JOB_RETRIES=5 and
                    its second retry job failed. When we attempt to create
                    a third retry job to retry the second, we hit an rpc
                    error. In such case, we will give up on all following
                    retries.
                RETRIED:
                    A retry job has already been successfully
                    scheduled.
            - retry_max:
                The maximum of times the job can still
                be retried, taking into account retries
                that have occurred.
    @var _retry_level: A retry might be triggered only if the result
            is worse than the level.
    @var _max_retries: Maximum retry limit at suite level.
                     Regardless how many times each individual test
                     has been retried, the total number of retries happening in
                     the suite can't exceed _max_retries.
    """

    States = enum.Enum('NOT_ATTEMPTED', 'ATTEMPTED', 'RETRIED',
                       start_value=1, step=1)

    def __init__(self, initial_jobs_to_tests, retry_level='WARN',
                 max_retries=None):
        """Initialize RetryHandler.

        @param initial_jobs_to_tests: A dictionary that maps a job id to
                a ControlData object. This dictionary should contain
                jobs that are originally scheduled by the suite.
        @param retry_level: A retry might be triggered only if the result is
                worse than the level.
        @param max_retries: Integer, maxmium total retries allowed
                                  for the suite. Default to None, no max.
        """
        self._retry_map = {}
        self._retry_level = retry_level
        self._max_retries = (max_retries
                             if max_retries is not None else sys.maxint)
        for job_id, test in initial_jobs_to_tests.items():
            if test.job_retries > 0:
                self._add_job(new_job_id=job_id,
                              retry_max=test.job_retries)


    def _add_job(self, new_job_id, retry_max):
        """Add a newly-created job to the retry map.

        @param new_job_id: The afe_job_id of a newly created job.
        @param retry_max: The maximum of times that we could retry
                          the test if the job fails.

        @raises ValueError if new_job_id is already in retry map.

        """
        if new_job_id in self._retry_map:
            raise ValueError('add_job called when job is already in retry map.')

        self._retry_map[new_job_id] = {
                'state': self.States.NOT_ATTEMPTED,
                'retry_max': retry_max}


    def _suite_max_reached(self):
        """Return whether maximum retry limit for a suite has been reached."""
        return self._max_retries <= 0


    def add_retry(self, old_job_id, new_job_id):
        """Record a retry.

        Update retry map with the retry information.

        @param old_job_id: The afe_job_id of the job that is retried.
        @param new_job_id: The afe_job_id of the retry job.

        @raises KeyError if old_job_id isn't in the retry map.
        @raises ValueError if we have already retried or made an attempt
                to retry the old job.

        """
        old_record = self._retry_map[old_job_id]
        if old_record['state'] != self.States.NOT_ATTEMPTED:
            raise ValueError(
                    'We have already retried or attempted to retry job %d' %
                    old_job_id)
        old_record['state'] = self.States.RETRIED
        self._add_job(new_job_id=new_job_id,
                      retry_max=old_record['retry_max'] - 1)
        self._max_retries -= 1


    def set_attempted(self, job_id):
        """Set the state of the job to ATTEMPTED.

        @param job_id: afe_job_id of a job.

        @raises KeyError if job_id isn't in the retry map.
        @raises ValueError if the current state is not NOT_ATTEMPTED.

        """
        current_state = self._retry_map[job_id]['state']
        if current_state != self.States.NOT_ATTEMPTED:
            # We are supposed to retry or attempt to retry each job
            # only once. Raise an error if this is not the case.
            raise ValueError('Unexpected state transition: %s -> %s' %
                             (self.States.get_string(current_state),
                              self.States.get_string(self.States.ATTEMPTED)))
        else:
            self._retry_map[job_id]['state'] = self.States.ATTEMPTED


    def has_following_retry(self, result):
        """Check whether there will be a following retry.

        We have the following cases for a given job id (result.id),
        - no retry map entry -> retry not required, no following retry
        - has retry map entry:
            - already retried -> has following retry
            - has not retried
                (this branch can be handled by checking should_retry(result))
                - retry_max == 0 --> the last retry job, no more retry
                - retry_max > 0
                   - attempted, but has failed in scheduling a
                     following retry due to rpc error  --> no more retry
                   - has not attempped --> has following retry if test failed.

        @param result: A result, encapsulating the status of the job.

        @returns: True, if there will be a following retry.
                  False otherwise.

        """
        return (result.test_executed
                and result.id in self._retry_map
                and (self._retry_map[result.id]['state'] == self.States.RETRIED
                     or self._should_retry(result)))


    def _should_retry(self, result):
        """Check whether we should retry a job based on its result.

        We will retry the job that corresponds to the result
        when all of the following are true.
        a) The test was actually executed, meaning that if
           a job was aborted before it could ever reach the state
           of 'Running', the job will not be retried.
        b) The result is worse than |self._retry_level| which
           defaults to 'WARN'.
        c) The test requires retry, i.e. the job has an entry in the retry map.
        d) We haven't made any retry attempt yet, i.e. state == NOT_ATTEMPTED
           Note that if a test has JOB_RETRIES=5, and the second time
           it was retried it hit an rpc error, we will give up on
           all following retries.
        e) The job has not reached its retry max, i.e. retry_max > 0

        @param result: A result, encapsulating the status of the job.

        @returns: True if we should retry the job.

        """
        return (
            result.test_executed
            and result.id in self._retry_map
            and not self._suite_max_reached()
            and result.is_worse_than(
                job_status.Status(self._retry_level, '', 'reason'))
            and self._retry_map[result.id]['state'] == self.States.NOT_ATTEMPTED
            and self._retry_map[result.id]['retry_max'] > 0
        )


    def get_retry_max(self, job_id):
        """Get the maximum times the job can still be retried.

        @param job_id: afe_job_id of a job.

        @returns: An int, representing the maximum times the job can still be
                  retried.
        @raises KeyError if job_id isn't in the retry map.

        """
        return self._retry_map[job_id]['retry_max']


class _SuiteChildJobCreator(object):
    """Create test jobs for a suite."""

    def __init__(
            self,
            tag,
            builds,
            board,
            afe=None,
            max_runtime_mins=24*60,
            timeout_mins=24*60,
            suite_job_id=None,
            ignore_deps=False,
            extra_deps=(),
            priority=priorities.Priority.DEFAULT,
            offload_failures_only=False,
            test_source_build=None,
            job_keyvals=None,
    ):
        """
        Constructor

        @param tag: a string with which to tag jobs run in this suite.
        @param builds: the builds on which we're running this suite.
        @param board: the board on which we're running this suite.
        @param afe: an instance of AFE as defined in server/frontend.py.
        @param max_runtime_mins: Maximum suite runtime, in minutes.
        @param timeout_mins: Maximum job lifetime, in minutes.
        @param suite_job_id: Job id that will act as parent id to all sub jobs.
                             Default: None
        @param ignore_deps: True if jobs should ignore the DEPENDENCIES
                            attribute and skip applying of dependency labels.
                            (Default:False)
        @param extra_deps: A list of strings which are the extra DEPENDENCIES
                           to add to each test being scheduled.
        @param priority: Integer priority level.  Higher is more important.
        @param offload_failures_only: Only enable gs_offloading for failed
                                      jobs.
        @param test_source_build: Build that contains the server-side test code.
        @param job_keyvals: General job keyvals to be inserted into keyval file,
                            which will be used by tko/parse later.
        """
        self._tag = tag
        self._builds = builds
        self._board = board
        self._afe = afe or frontend_wrappers.RetryingAFE(timeout_min=30,
                                                         delay_sec=10,
                                                         debug=False)
        self._max_runtime_mins = max_runtime_mins
        self._timeout_mins = timeout_mins
        self._suite_job_id = suite_job_id
        self._ignore_deps = ignore_deps
        self._extra_deps = tuple(extra_deps)
        self._priority = priority
        self._offload_failures_only = offload_failures_only
        self._test_source_build = test_source_build
        self._job_keyvals = job_keyvals


    @property
    def cros_build(self):
        """Return the CrOS build or the first build in the builds dict."""
        # TODO(ayatane): Note that the builds dict isn't ordered.  I'm not
        # sure what the implications of this are, but it's probably not a
        # good thing.
        return self._builds.get(provision.CROS_VERSION_PREFIX,
                                self._builds.values()[0])


    def create_job(self, test, retry_for=None):
        """
        Thin wrapper around frontend.AFE.create_job().

        @param test: ControlData object for a test to run.
        @param retry_for: If the to-be-created job is a retry for an
                          old job, the afe_job_id of the old job will
                          be passed in as |retry_for|, which will be
                          recorded in the new job's keyvals.
        @returns: A frontend.Job object with an added test_name member.
                  test_name is used to preserve the higher level TEST_NAME
                  name of the job.
        """
        # For a system running multiple suites which share tests, the priority
        # overridden may lead to unexpected scheduling order that adds extra
        # provision jobs.
        test_priority = self._priority
        if utils.is_moblab():
            test_priority = max(self._priority, test.priority)

        reboot_before = (model_attributes.RebootBefore.NEVER if test.fast
                         else None)

        test_obj = self._afe.create_job(
            control_file=test.text,
            name=tools.create_job_name(
                    self._test_source_build or self.cros_build,
                    self._tag,
                    test.name),
            control_type=test.test_type.capitalize(),
            meta_hosts=[self._board]*test.sync_count,
            dependencies=self._create_job_deps(test),
            keyvals=self._create_keyvals_for_test_job(test, retry_for),
            max_runtime_mins=self._max_runtime_mins,
            timeout_mins=self._timeout_mins,
            parent_job_id=self._suite_job_id,
            test_retry=test.retries,
            reboot_before=reboot_before,
            run_reset=not test.fast,
            priority=test_priority,
            synch_count=test.sync_count,
            require_ssp=test.require_ssp)

        test_obj.test_name = test.name
        return test_obj


    def _create_job_deps(self, test):
        """Create job deps list for a test job.

        @returns: A list of dependency strings.
        """
        if self._ignore_deps:
            job_deps = []
        else:
            job_deps = list(test.dependencies)
        job_deps.extend(self._extra_deps)
        return job_deps


    def _create_keyvals_for_test_job(self, test, retry_for=None):
        """Create keyvals dict for creating a test job.

        @param test: ControlData object for a test to run.
        @param retry_for: If the to-be-created job is a retry for an
                          old job, the afe_job_id of the old job will
                          be passed in as |retry_for|, which will be
                          recorded in the new job's keyvals.
        @returns: A keyvals dict for creating the test job.
        """
        keyvals = {
            constants.JOB_BUILD_KEY: self.cros_build,
            constants.JOB_SUITE_KEY: self._tag,
            constants.JOB_EXPERIMENTAL_KEY: test.experimental,
            constants.JOB_BUILDS_KEY: self._builds
        }
        # test_source_build is saved to job_keyvals so scheduler can retrieve
        # the build name from database when compiling autoserv commandline.
        # This avoid a database change to add a new field in afe_jobs.
        #
        # Only add `test_source_build` to job keyvals if the build is different
        # from the CrOS build or the job uses more than one build, e.g., both
        # firmware and CrOS will be updated in the dut.
        # This is for backwards compatibility, so the update Autotest code can
        # compile an autoserv command line to run in a SSP container using
        # previous builds.
        if (self._test_source_build and
            (self.cros_build != self._test_source_build or
             len(self._builds) > 1)):
            keyvals[constants.JOB_TEST_SOURCE_BUILD_KEY] = \
                    self._test_source_build
            for prefix, build in self._builds.iteritems():
                if prefix == provision.FW_RW_VERSION_PREFIX:
                    keyvals[constants.FWRW_BUILD]= build
                elif prefix == provision.FW_RO_VERSION_PREFIX:
                    keyvals[constants.FWRO_BUILD] = build
        # Add suite job id to keyvals so tko parser can read it from keyval
        # file.
        if self._suite_job_id:
            keyvals[constants.PARENT_JOB_ID] = self._suite_job_id
        # We drop the old job's id in the new job's keyval file so that
        # later our tko parser can figure out the retry relationship and
        # invalidate the results of the old job in tko database.
        if retry_for:
            keyvals[constants.RETRY_ORIGINAL_JOB_ID] = retry_for
        if self._offload_failures_only:
            keyvals[constants.JOB_OFFLOAD_FAILURES_KEY] = True
        if self._job_keyvals:
            for key in constants.INHERITED_KEYVALS:
                if key in self._job_keyvals:
                    keyvals[key] = self._job_keyvals[key]
        return keyvals


def _get_cf_retriever(cf_getter, forgiving_parser=True, run_prod_code=False,
                      test_args=None):
    """Return the correct _ControlFileRetriever instance.

    If cf_getter is a File system ControlFileGetter, return a
    _ControlFileRetriever.  This performs a full parse of the root
    directory associated with the getter. This is the case when it's
    invoked from suite_preprocessor.

    If cf_getter is a devserver getter, return a
    _BatchControlFileRetriever.  This looks up the suite_name in a suite
    to control file map generated at build time, and parses the relevant
    control files alone. This lookup happens on the devserver, so as far
    as this method is concerned, both cases are equivalent. If
    enable_controls_in_batch is switched on, this function will call
    cf_getter.get_suite_info() to get a dict of control files and
    contents in batch.
    """
    if _should_batch_with(cf_getter):
        cls = _BatchControlFileRetriever
    else:
        cls = _ControlFileRetriever
    return cls(cf_getter, forgiving_parser, run_prod_code, test_args)


def _should_batch_with(cf_getter):
    """Return whether control files should be fetched in batch.

    This depends on the control file getter and configuration options.

    @param cf_getter: a control_file_getter.ControlFileGetter used to list
           and fetch the content of control files
    """
    return (ENABLE_CONTROLS_IN_BATCH
            and isinstance(cf_getter, control_file_getter.DevServerGetter))


class _ControlFileRetriever(object):
    """Retrieves control files.

    This returns control data instances, unlike control file getters
    which simply return the control file text contents.
    """

    def __init__(self, cf_getter, forgiving_parser=True, run_prod_code=False,
                 test_args=None):
        """Initialize instance.

        @param cf_getter: a control_file_getter.ControlFileGetter used to list
               and fetch the content of control files
        @param forgiving_parser: If False, will raise ControlVariableExceptions
                                 if any are encountered when parsing control
                                 files. Note that this can raise an exception
                                 for syntax errors in unrelated files, because
                                 we parse them before applying the predicate.
        @param run_prod_code: If true, the retrieved tests will run the test
                              code that lives in prod aka the test code
                              currently on the lab servers by disabling
                              SSP for the discovered tests.
        @param test_args: A dict of args to be seeded in test control file under
                          the name |args_dict|.
        """
        self._cf_getter = cf_getter
        self._forgiving_parser = forgiving_parser
        self._run_prod_code = run_prod_code
        self._test_args = test_args


    def retrieve(self, test_name):
        """Retrieve a test's control data.

        This ignores forgiving_parser because we cannot return a
        forgiving value.

        @param test_name: Name of test to retrieve.

        @raises ControlVariableException: There is a syntax error in a
                                          control file.

        @returns a ControlData object
        """
        path = self._cf_getter.get_control_file_path(test_name)
        text = self._cf_getter.get_control_file_contents(path)
        return self._parse_cf_text(path, text)


    def retrieve_for_suite(self, suite_name=''):
        """Scan through all tests and find all tests.

        @param suite_name: If specified, this method will attempt to restrain
                           the search space to just this suite's control files.

        @raises ControlVariableException: If forgiving_parser is False and there
                                          is a syntax error in a control file.

        @returns a dictionary of ControlData objects that based on given
                 parameters.
        """
        control_file_texts = self._get_cf_texts_for_suite(suite_name)
        return self._parse_cf_text_many(control_file_texts)


    def _filter_cf_paths(self, paths):
        """Remove certain control file paths

        @param paths: Iterable of paths
        @returns: generator yielding paths
        """
        matcher = re.compile(r'[^/]+/(deps|profilers)/.+')
        return (path for path in paths if not matcher.match(path))


    def _get_cf_texts_for_suite(self, suite_name):
        """Get control file content for given suite.

        @param suite_name: If specified, this method will attempt to restrain
                           the search space to just this suite's control files.
        @returns: generator yielding (path, text) tuples
        """
        files = self._cf_getter.get_control_file_list(suite_name=suite_name)
        filtered_files = self._filter_cf_paths(files)
        for path in filtered_files:
            yield path, self._cf_getter.get_control_file_contents(path)


    def _parse_cf_text_many(self, control_file_texts):
        """Parse control file texts.

        @param control_file_texts: iterable of (path, text) pairs
        @returns: a dictionary of ControlData objects
        """
        tests = {}
        for path, text in control_file_texts:
            # Seed test_args into the control file.
            if self._test_args:
                text = tools.inject_vars(self._test_args, text)
            try:
                found_test = self._parse_cf_text(path, text)
            except control_data.ControlVariableException, e:
                if not self._forgiving_parser:
                    msg = "Failed parsing %s\n%s" % (path, e)
                    raise control_data.ControlVariableException(msg)
                logging.warning("Skipping %s\n%s", path, e)
            except Exception, e:
                logging.error("Bad %s\n%s", path, e)
            else:
                tests[path] = found_test
        return tests


    def _parse_cf_text(self, path, text):
        """Parse control file text.

        This ignores forgiving_parser because we cannot return a
        forgiving value.

        @param path: path to control file
        @param text: control file text contents
        @returns: a ControlData object

        @raises ControlVariableException: There is a syntax error in a
                                          control file.
        """
        test = control_data.parse_control_string(
                text, raise_warnings=True, path=path)
        test.text = text
        if self._run_prod_code:
            test.require_ssp = False
        return test


class _BatchControlFileRetriever(_ControlFileRetriever):
    """Subclass that can retrieve suite control files in batch."""


    def _get_cf_texts_for_suite(self, suite_name):
        """Get control file content for given suite.

        @param suite_name: If specified, this method will attempt to restrain
                           the search space to just this suite's control files.
        @returns: generator yielding (path, text) tuples
        """
        suite_info = self._cf_getter.get_suite_info(suite_name=suite_name)
        files = suite_info.keys()
        filtered_files = self._filter_cf_paths(files)
        for path in filtered_files:
            yield path, suite_info[path]


def get_test_source_build(builds, **dargs):
    """Get the build of test code.

    Get the test source build from arguments. If parameter
    `test_source_build` is set and has a value, return its value. Otherwise
    returns the ChromeOS build name if it exists. If ChromeOS build is not
    specified either, raise SuiteArgumentException.

    @param builds: the builds on which we're running this suite. It's a
                   dictionary of version_prefix:build.
    @param **dargs: Any other Suite constructor parameters, as described
                    in Suite.__init__ docstring.

    @return: The build contains the test code.
    @raise: SuiteArgumentException if both test_source_build and ChromeOS
            build are not specified.

    """
    if dargs.get('test_source_build', None):
        return dargs['test_source_build']
    cros_build = builds.get(provision.CROS_VERSION_PREFIX, None)
    if cros_build.endswith(provision.CHEETS_SUFFIX):
        test_source_build = re.sub(
                provision.CHEETS_SUFFIX + '$', '', cros_build)
    else:
        test_source_build = cros_build
    if not test_source_build:
        raise error.SuiteArgumentException(
                'test_source_build must be specified if CrOS build is not '
                'specified.')
    return test_source_build


def list_all_suites(build, devserver, cf_getter=None):
    """
    Parses all ControlData objects with a SUITE tag and extracts all
    defined suite names.

    @param build: the build on which we're running this suite.
    @param devserver: the devserver which contains the build.
    @param cf_getter: control_file_getter.ControlFileGetter. Defaults to
                      using DevServerGetter.

    @return list of suites
    """
    if cf_getter is None:
        cf_getter = _create_ds_getter(build, devserver)

    suites = set()
    predicate = lambda t: True
    for test in find_and_parse_tests(cf_getter, predicate):
        suites.update(test.suite_tag_parts)
    return list(suites)


def test_file_similarity_predicate(test_file_pattern):
    """Returns predicate that gets the similarity based on a test's file
    name pattern.

    Builds a predicate that takes in a parsed control file (a ControlData)
    and returns a tuple of (file path, ratio), where ratio is the
    similarity between the test file name and the given test_file_pattern.

    @param test_file_pattern: regular expression (string) to match against
                              control file names.
    @return a callable that takes a ControlData and and returns a tuple of
            (file path, ratio), where ratio is the similarity between the
            test file name and the given test_file_pattern.
    """
    return lambda t: ((None, 0) if not hasattr(t, 'path') else
            (t.path, difflib.SequenceMatcher(a=t.path,
                                             b=test_file_pattern).ratio()))


def test_name_similarity_predicate(test_name):
    """Returns predicate that matched based on a test's name.

    Builds a predicate that takes in a parsed control file (a ControlData)
    and returns a tuple of (test name, ratio), where ratio is the similarity
    between the test name and the given test_name.

    @param test_name: the test name to base the predicate on.
    @return a callable that takes a ControlData and returns a tuple of
            (test name, ratio), where ratio is the similarity between the
            test name and the given test_name.
    """
    return lambda t: ((None, 0) if not hasattr(t, 'name') else
            (t.name,
             difflib.SequenceMatcher(a=t.name, b=test_name).ratio()))


def matches_attribute_expression_predicate(test_attr_boolstr):
    """Returns predicate that matches based on boolean expression of
    attributes.

    Builds a predicate that takes in a parsed control file (a ControlData)
    ans returns True if the test attributes satisfy the given attribute
    boolean expression.

    @param test_attr_boolstr: boolean expression of the attributes to be
                              test, like 'system:all and interval:daily'.

    @return a callable that takes a ControlData and returns True if the test
            attributes satisfy the given boolean expression.
    """
    return lambda t: boolparse_lib.BoolstrResult(
        test_attr_boolstr, t.attributes)


def test_file_matches_pattern_predicate(test_file_pattern):
    """Returns predicate that matches based on a test's file name pattern.

    Builds a predicate that takes in a parsed control file (a ControlData)
    and returns True if the test's control file name matches the given
    regular expression.

    @param test_file_pattern: regular expression (string) to match against
                              control file names.
    @return a callable that takes a ControlData and and returns
            True if control file name matches the pattern.
    """
    return lambda t: hasattr(t, 'path') and re.match(test_file_pattern,
                                                     t.path)


def test_name_matches_pattern_predicate(test_name_pattern):
    """Returns predicate that matches based on a test's name pattern.

    Builds a predicate that takes in a parsed control file (a ControlData)
    and returns True if the test name matches the given regular expression.

    @param test_name_pattern: regular expression (string) to match against
                              test names.
    @return a callable that takes a ControlData and returns
            True if the name fields matches the pattern.
    """
    return lambda t: hasattr(t, 'name') and re.match(test_name_pattern,
                                                     t.name)


def test_name_equals_predicate(test_name):
    """Returns predicate that matched based on a test's name.

    Builds a predicate that takes in a parsed control file (a ControlData)
    and returns True if the test name is equal to |test_name|.

    @param test_name: the test name to base the predicate on.
    @return a callable that takes a ControlData and looks for |test_name|
            in that ControlData's name.
    """
    return lambda t: hasattr(t, 'name') and test_name == t.name


def name_in_tag_similarity_predicate(name):
    """Returns predicate that takes a control file and gets the similarity
    of the suites in the control file and the given name.

    Builds a predicate that takes in a parsed control file (a ControlData)
    and returns a list of tuples of (suite name, ratio), where suite name
    is each suite listed in the control file, and ratio is the similarity
    between each suite and the given name.

    @param name: the suite name to base the predicate on.
    @return a callable that takes a ControlData and returns a list of tuples
            of (suite name, ratio), where suite name is each suite listed in
            the control file, and ratio is the similarity between each suite
            and the given name.
    """
    return lambda t: [(suite,
                       difflib.SequenceMatcher(a=suite, b=name).ratio())
                      for suite in t.suite_tag_parts] or [(None, 0)]


def name_in_tag_predicate(name):
    """Returns predicate that takes a control file and looks for |name|.

    Builds a predicate that takes in a parsed control file (a ControlData)
    and returns True if the SUITE tag is present and contains |name|.

    @param name: the suite name to base the predicate on.
    @return a callable that takes a ControlData and looks for |name| in that
            ControlData object's suite member.
    """
    return lambda t: name in t.suite_tag_parts


def create_fs_getter(autotest_dir):
    """
    @param autotest_dir: the place to find autotests.
    @return a FileSystemGetter instance that looks under |autotest_dir|.
    """
    # currently hard-coded places to look for tests.
    subpaths = ['server/site_tests', 'client/site_tests',
                'server/tests', 'client/tests']
    directories = [os.path.join(autotest_dir, p) for p in subpaths]
    return control_file_getter.FileSystemGetter(directories)


def _create_ds_getter(build, devserver):
    """
    @param build: the build on which we're running this suite.
    @param devserver: the devserver which contains the build.
    @return a FileSystemGetter instance that looks under |autotest_dir|.
    """
    return control_file_getter.DevServerGetter(build, devserver)


def _non_experimental_tests_predicate(test_data):
    """Test predicate for non-experimental tests."""
    return not test_data.experimental


def find_and_parse_tests(cf_getter, predicate, suite_name='',
                         add_experimental=False, forgiving_parser=True,
                         run_prod_code=False, test_args=None):
    """
    Function to scan through all tests and find eligible tests.

    Search through all tests based on given cf_getter, suite_name,
    add_experimental and forgiving_parser, return the tests that match
    given predicate.

    @param cf_getter: a control_file_getter.ControlFileGetter used to list
           and fetch the content of control files
    @param predicate: a function that should return True when run over a
           ControlData representation of a control file that should be in
           this Suite.
    @param suite_name: If specified, this method will attempt to restrain
                       the search space to just this suite's control files.
    @param add_experimental: add tests with experimental attribute set.
    @param forgiving_parser: If False, will raise ControlVariableExceptions
                             if any are encountered when parsing control
                             files. Note that this can raise an exception
                             for syntax errors in unrelated files, because
                             we parse them before applying the predicate.
    @param run_prod_code: If true, the suite will run the test code that
                          lives in prod aka the test code currently on the
                          lab servers by disabling SSP for the discovered
                          tests.
    @param test_args: A dict of args to be seeded in test control file.

    @raises ControlVariableException: If forgiving_parser is False and there
                                      is a syntax error in a control file.

    @return list of ControlData objects that should be run, with control
            file text added in |text| attribute. Results are sorted based
            on the TIME setting in control file, slowest test comes first.
    """
    logging.debug('Getting control file list for suite: %s', suite_name)
    retriever = _get_cf_retriever(cf_getter,
                                  forgiving_parser=forgiving_parser,
                                  run_prod_code=run_prod_code,
                                  test_args=test_args)
    tests = retriever.retrieve_for_suite(suite_name)
    logging.debug('Parsed %s control files.', len(tests))
    if not add_experimental:
        predicate = _ComposedPredicate([predicate,
                                        _non_experimental_tests_predicate])
    tests = [test for test in tests.itervalues() if predicate(test)]
    tests.sort(key=lambda t:
               control_data.ControlData.get_test_time_index(t.time),
               reverse=True)
    return tests


def find_possible_tests(cf_getter, predicate, suite_name='', count=10):
    """
    Function to scan through all tests and find possible tests.

    Search through all tests based on given cf_getter, suite_name,
    add_experimental and forgiving_parser. Use the given predicate to
    calculate the similarity and return the top 10 matches.

    @param cf_getter: a control_file_getter.ControlFileGetter used to list
           and fetch the content of control files
    @param predicate: a function that should return a tuple of (name, ratio)
           when run over a ControlData representation of a control file that
           should be in this Suite. `name` is the key to be compared, e.g.,
           a suite name or test name. `ratio` is a value between [0,1]
           indicating the similarity of `name` and the value to be compared.
    @param suite_name: If specified, this method will attempt to restrain
                       the search space to just this suite's control files.
    @param count: Number of suggestions to return, default to 10.

    @return list of top names that similar to the given test, sorted by
            match ratio.
    """
    logging.debug('Getting control file list for suite: %s', suite_name)
    tests = _get_cf_retriever(cf_getter).retrieve_for_suite(suite_name)
    logging.debug('Parsed %s control files.', len(tests))
    similarities = {}
    for test in tests.itervalues():
        ratios = predicate(test)
        # Some predicates may return a list of tuples, e.g.,
        # name_in_tag_similarity_predicate. Convert all returns to a list.
        if not isinstance(ratios, list):
            ratios = [ratios]
        for name, ratio in ratios:
            similarities[name] = ratio
    return [s[0] for s in
            sorted(similarities.items(), key=operator.itemgetter(1),
                   reverse=True)][:count]


def _deprecated_suite_method(func):
    """Decorator for deprecated Suite static methods.

    TODO(ayatane): This is used to decorate functions that are called as
    static methods on Suite.
    """
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        """Wraps |func| for warning."""
        warnings.warn('Calling method "%s" from Suite is deprecated' %
                      func.__name__)
        return func(*args, **kwargs)
    return staticmethod(wrapper)


class _BaseSuite(object):
    """
    A suite of tests, defined by some predicate over control file variables.

    Given a place to search for control files a predicate to match the desired
    tests, can gather tests and fire off jobs to run them, and then wait for
    results.

    @var _predicate: a function that should return True when run over a
         ControlData representation of a control file that should be in
         this Suite.
    @var _tag: a string with which to tag jobs run in this suite.
    @var _builds: the builds on which we're running this suite.
    @var _afe: an instance of AFE as defined in server/frontend.py.
    @var _tko: an instance of TKO as defined in server/frontend.py.
    @var _jobs: currently scheduled jobs, if any.
    @var _jobs_to_tests: a dictionary that maps job ids to tests represented
                         ControlData objects.
    @var _retry: a bool value indicating whether jobs should be retried on
                 failure.
    @var _retry_handler: a RetryHandler object.

    """


    def __init__(
            self,
            tests,
            tag,
            builds,
            board,
            afe=None,
            tko=None,
            pool=None,
            results_dir=None,
            max_runtime_mins=24*60,
            timeout_mins=24*60,
            file_bugs=False,
            suite_job_id=None,
            ignore_deps=False,
            extra_deps=None,
            priority=priorities.Priority.DEFAULT,
            wait_for_results=True,
            job_retry=False,
            max_retries=sys.maxint,
            offload_failures_only=False,
            test_source_build=None,
            job_keyvals=None,
            child_dependencies=(),
            result_reporter=None,
    ):
        """Initialize instance.

        @param tests: Iterable of tests to run.
        @param tag: a string with which to tag jobs run in this suite.
        @param builds: the builds on which we're running this suite.
        @param board: the board on which we're running this suite.
        @param afe: an instance of AFE as defined in server/frontend.py.
        @param tko: an instance of TKO as defined in server/frontend.py.
        @param pool: Specify the pool of machines to use for scheduling
                purposes.
        @param results_dir: The directory where the job can write results to.
                            This must be set if you want job_id of sub-jobs
                            list in the job keyvals.
        @param max_runtime_mins: Maximum suite runtime, in minutes.
        @param timeout: Maximum job lifetime, in hours.
        @param suite_job_id: Job id that will act as parent id to all sub jobs.
                             Default: None
        @param ignore_deps: True if jobs should ignore the DEPENDENCIES
                            attribute and skip applying of dependency labels.
                            (Default:False)
        @param extra_deps: A list of strings which are the extra DEPENDENCIES
                           to add to each test being scheduled.
        @param priority: Integer priority level.  Higher is more important.
        @param wait_for_results: Set to False to run the suite job without
                                 waiting for test jobs to finish. Default is
                                 True.
        @param job_retry: A bool value indicating whether jobs should be retired
                          on failure. If True, the field 'JOB_RETRIES' in
                          control files will be respected. If False, do not
                          retry.
        @param max_retries: Maximum retry limit at suite level.
                            Regardless how many times each individual test
                            has been retried, the total number of retries
                            happening in the suite can't exceed _max_retries.
                            Default to sys.maxint.
        @param offload_failures_only: Only enable gs_offloading for failed
                                      jobs.
        @param test_source_build: Build that contains the server-side test code.
        @param job_keyvals: General job keyvals to be inserted into keyval file,
                            which will be used by tko/parse later.
        @param child_dependencies: (optional) list of dependency strings
                to be added as dependencies to child jobs.
        @param result_reporter: A _ResultReporter instance to report results. If
                None, an _EmailReporter will be created.
        """

        self.tests = list(tests)
        self._tag = tag
        self._builds = builds
        self._results_dir = results_dir
        self._afe = afe or frontend_wrappers.RetryingAFE(timeout_min=30,
                                                         delay_sec=10,
                                                         debug=False)
        self._tko = tko or frontend_wrappers.RetryingTKO(timeout_min=30,
                                                         delay_sec=10,
                                                         debug=False)
        self._jobs = []
        self._jobs_to_tests = {}

        self._file_bugs = file_bugs
        self._suite_job_id = suite_job_id
        self._job_retry=job_retry
        self._max_retries = max_retries
        # RetryHandler to be initialized in schedule()
        self._retry_handler = None
        self.wait_for_results = wait_for_results
        self._job_keyvals = job_keyvals
        if result_reporter is None:
            self._result_reporter = _EmailReporter(self)
        else:
            self._result_reporter = result_reporter

        if extra_deps is None:
            extra_deps = []
        extra_deps.append(board)
        if pool:
            extra_deps.append(pool)
        extra_deps.extend(child_dependencies)
        self._dependencies = tuple(extra_deps)

        self._job_creator = _SuiteChildJobCreator(
            tag=tag,
            builds=builds,
            board=board,
            afe=afe,
            max_runtime_mins=max_runtime_mins,
            timeout_mins=timeout_mins,
            suite_job_id=suite_job_id,
            ignore_deps=ignore_deps,
            extra_deps=extra_deps,
            priority=priority,
            offload_failures_only=offload_failures_only,
            test_source_build=test_source_build,
            job_keyvals=job_keyvals,
        )


    def _schedule_test(self, record, test, retry_for=None):
        """Schedule a single test and return the job.

        Schedule a single test by creating a job, and then update relevant
        data structures that are used to keep track of all running jobs.

        Emits a TEST_NA status log entry if it failed to schedule the test due
        to NoEligibleHostException or a non-existent board label.

        Returns a frontend.Job object if the test is successfully scheduled.
        If scheduling failed due to NoEligibleHostException or a non-existent
        board label, returns None.

        @param record: A callable to use for logging.
                       prototype: record(base_job.status_log_entry)
        @param test: ControlData for a test to run.
        @param retry_for: If we are scheduling a test to retry an
                          old job, the afe_job_id of the old job
                          will be passed in as |retry_for|.

        @returns: A frontend.Job object or None
        """
        msg = 'Scheduling %s' % test.name
        if retry_for:
            msg = msg + ', to retry afe job %d' % retry_for
        logging.debug(msg)
        begin_time_str = datetime.datetime.now().strftime(time_utils.TIME_FMT)
        try:
            job = self._job_creator.create_job(test, retry_for=retry_for)
        except (error.NoEligibleHostException, proxy.ValidationError) as e:
            if (isinstance(e, error.NoEligibleHostException)
                or (isinstance(e, proxy.ValidationError)
                    and _is_nonexistent_board_error(e))):
                # Treat a dependency on a non-existent board label the same as
                # a dependency on a board that exists, but for which there's no
                # hardware.
                logging.debug('%s not applicable for this board/pool. '
                              'Emitting TEST_NA.', test.name)
                Status('TEST_NA', test.name,
                       'Skipping:  test not supported on this board/pool.',
                       begin_time_str=begin_time_str).record_all(record)
                return None
            else:
                raise e
        except (error.RPCException, proxy.JSONRPCException):
            if retry_for:
                # Mark that we've attempted to retry the old job.
                self._retry_handler.set_attempted(job_id=retry_for)
            raise
        else:
            self._jobs.append(job)
            self._jobs_to_tests[job.id] = test
            if retry_for:
                # A retry job was just created, record it.
                self._retry_handler.add_retry(
                        old_job_id=retry_for, new_job_id=job.id)
                retry_count = (test.job_retries -
                               self._retry_handler.get_retry_max(job.id))
                logging.debug('Job %d created to retry job %d. '
                              'Have retried for %d time(s)',
                              job.id, retry_for, retry_count)
            self._remember_job_keyval(job)
            return job


    def schedule(self, record):
        """
        Schedule jobs using |self._afe|.

        frontend.Job objects representing each scheduled job will be put in
        |self._jobs|.

        @param record: A callable to use for logging.
                       prototype: record(base_job.status_log_entry)
        @returns: The number of tests that were scheduled.
        """
        scheduled_test_names = []
        logging.debug('Discovered %d tests.', len(self.tests))

        Status('INFO', 'Start %s' % self._tag).record_result(record)
        try:
            # Write job_keyvals into keyval file.
            if self._job_keyvals:
                utils.write_keyval(self._results_dir, self._job_keyvals)

            # TODO(crbug.com/730885): This is a hack to protect tests that are
            # not usually retried from getting hit by a provision error when run
            # as part of a suite. Remove this hack once provision is separated
            # out in its own suite.
            self._bump_up_test_retries(self.tests)
            for test in self.tests:
                scheduled_job = self._schedule_test(record, test)
                if scheduled_job is not None:
                    scheduled_test_names.append(test.name)

            # Write the num of scheduled tests and name of them to keyval file.
            logging.debug('Scheduled %d tests, writing the total to keyval.',
                          len(scheduled_test_names))
            utils.write_keyval(
                self._results_dir,
                self._make_scheduled_tests_keyvals(scheduled_test_names))
        except Exception:
            logging.exception('Exception while scheduling suite')
            Status('FAIL', self._tag,
                   'Exception while scheduling suite').record_result(record)

        if self._job_retry:
            self._retry_handler = RetryHandler(
                    initial_jobs_to_tests=self._jobs_to_tests,
                    max_retries=self._max_retries)
        return len(scheduled_test_names)


    def _bump_up_test_retries(self, tests):
        """Bump up individual test retries to match suite retry options."""
        if not self._job_retry:
            return

        for test in tests:
            # We do honor if a test insists on JOB_RETRIES = 0.
            if test.job_retries is None:
                logging.debug(
                        'Test %s did not request retries, but suite requires '
                        'retries. Bumping retries up to 1. '
                        '(See crbug.com/730885)',
                        test.name)
                test.job_retries = 1


    def _make_scheduled_tests_keyvals(self, scheduled_test_names):
        """Make a keyvals dict to write for scheduled test names.

        @param scheduled_test_names: A list of scheduled test name strings.

        @returns: A keyvals dict.
        """
        return {
            constants.SCHEDULED_TEST_COUNT_KEY: len(scheduled_test_names),
            constants.SCHEDULED_TEST_NAMES_KEY: repr(scheduled_test_names),
        }


    def _should_report(self, result):
        """
        Returns True if this failure requires to be reported.

        @param result: A result, encapsulating the status of the failed job.
        @return: True if we should report this failure.
        """
        return (self._file_bugs and result.test_executed and
                not result.is_testna() and
                result.is_worse_than(job_status.Status('GOOD', '', 'reason')))


    def _has_retry(self, result):
        """
        Return True if this result gets to retry.

        @param result: A result, encapsulating the status of the failed job.
        @return: bool
        """
        return (self._job_retry
                and self._retry_handler.has_following_retry(result))


    def wait(self, record):
        """
        Polls for the job statuses, using |record| to print status when each
        completes.

        @param record: callable that records job status.
                 prototype:
                   record(base_job.status_log_entry)
        """
        waiter = job_status.JobResultWaiter(self._afe, self._tko)
        try:
            if self._suite_job_id:
                jobs = self._afe.get_jobs(parent_job_id=self._suite_job_id)
            else:
                logging.warning('Unknown suite_job_id, falling back to less '
                                'efficient results_generator.')
                jobs = self._jobs
            waiter.add_jobs(jobs)
            for result in waiter.wait_for_results():
                self._handle_result(result=result, record=record, waiter=waiter)
                if self._finished_waiting():
                    break
        except Exception:  # pylint: disable=W0703
            logging.exception('Exception waiting for results')
            Status('FAIL', self._tag,
                   'Exception waiting for results').record_result(record)


    def _finished_waiting(self):
        """Return whether the suite is finished waiting for child jobs."""
        return False


    def _handle_result(self, result, record, waiter):
        """
        Handle a test job result.

        @param result: Status instance for job.
        @param record: callable that records job status.
                 prototype:
                   record(base_job.status_log_entry)
        @param waiter: JobResultsWaiter instance.
        @param reporter: _ResultReporter instance.
        """
        self._record_result(result, record)
        rescheduled = False
        if self._job_retry and self._retry_handler._should_retry(result):
            rescheduled = self._retry_result(result, record, waiter)
        # TODO (crbug.com/751428): If the suite times out before a retry could
        # finish, we would lose the chance to report errors from the original
        # job.
        if self._has_retry(result) and rescheduled:
             return

        if self._should_report(result):
            self._result_reporter.report(result)


    def _record_result(self, result, record):
        """
        Record a test job result.

        @param result: Status instance for job.
        @param record: callable that records job status.
                 prototype:
                   record(base_job.status_log_entry)
        """
        result.record_all(record)
        self._remember_job_keyval(result)


    def _retry_result(self, result, record, waiter):
        """
        Retry a test job result.

        @param result: Status instance for job.
        @param record: callable that records job status.
                 prototype:
                   record(base_job.status_log_entry)
        @param waiter: JobResultsWaiter instance.
        @returns: True if a job was scheduled for retry, False otherwise.
        """
        test = self._jobs_to_tests[result.id]
        try:
            # It only takes effect for CQ retriable job:
            #   1) in first try, test.fast=True.
            #   2) in second try, test will be run in normal mode, so reset
            #       test.fast=False.
            test.fast = False
            new_job = self._schedule_test(
                    record=record, test=test, retry_for=result.id)
        except (error.RPCException, proxy.JSONRPCException) as e:
            logging.error('Failed to schedule test: %s, Reason: %s',
                          test.name, e)
            return False
        else:
            waiter.add_job(new_job)
            return bool(new_job)


    @property
    def _should_file_bugs(self):
        """Return whether bugs should be filed.

        @returns: bool
        """
        # File bug when failure is one of the _FILE_BUG_SUITES,
        # otherwise send an email to the owner anc cc.
        return self._tag in _FILE_BUG_SUITES


    def abort(self):
        """
        Abort all scheduled test jobs.
        """
        if self._jobs:
            job_ids = [job.id for job in self._jobs]
            self._afe.run('abort_host_queue_entries', job__id__in=job_ids)


    def _remember_job_keyval(self, job):
        """
        Record provided job as a suite job keyval, for later referencing.

        @param job: some representation of a job that has the attributes:
                    id, test_name, and owner
        """
        if self._results_dir and job.id and job.owner and job.test_name:
            job_id_owner = '%s-%s' % (job.id, job.owner)
            logging.debug('Adding job keyval for %s=%s',
                          job.test_name, job_id_owner)
            utils.write_keyval(
                self._results_dir,
                {hashlib.md5(job.test_name).hexdigest(): job_id_owner})


class Suite(_BaseSuite):
    """
    A suite of tests, defined by some predicate over control file variables.

    Given a place to search for control files a predicate to match the desired
    tests, can gather tests and fire off jobs to run them, and then wait for
    results.

    @var _predicate: a function that should return True when run over a
         ControlData representation of a control file that should be in
         this Suite.
    @var _tag: a string with which to tag jobs run in this suite.
    @var _builds: the builds on which we're running this suite.
    @var _afe: an instance of AFE as defined in server/frontend.py.
    @var _tko: an instance of TKO as defined in server/frontend.py.
    @var _jobs: currently scheduled jobs, if any.
    @var _jobs_to_tests: a dictionary that maps job ids to tests represented
                         ControlData objects.
    @var _cf_getter: a control_file_getter.ControlFileGetter
    @var _retry: a bool value indicating whether jobs should be retried on
                 failure.
    @var _retry_handler: a RetryHandler object.

    """

    # TODO(ayatane): These methods are kept on the Suite class for
    # backward compatibility.
    find_and_parse_tests = _deprecated_suite_method(find_and_parse_tests)
    find_possible_tests = _deprecated_suite_method(find_possible_tests)
    create_fs_getter = _deprecated_suite_method(create_fs_getter)
    name_in_tag_predicate = _deprecated_suite_method(name_in_tag_predicate)
    name_in_tag_similarity_predicate = _deprecated_suite_method(
            name_in_tag_similarity_predicate)
    test_name_equals_predicate = _deprecated_suite_method(
            test_name_equals_predicate)
    test_name_matches_pattern_predicate = _deprecated_suite_method(
            test_name_matches_pattern_predicate)
    test_file_matches_pattern_predicate = _deprecated_suite_method(
            test_file_matches_pattern_predicate)
    matches_attribute_expression_predicate = _deprecated_suite_method(
            matches_attribute_expression_predicate)
    test_name_similarity_predicate = _deprecated_suite_method(
            test_name_similarity_predicate)
    test_file_similarity_predicate = _deprecated_suite_method(
            test_file_similarity_predicate)
    list_all_suites = _deprecated_suite_method(list_all_suites)
    get_test_source_build = _deprecated_suite_method(get_test_source_build)


    @classmethod
    def create_from_predicates(cls, predicates, builds, board, devserver,
                               cf_getter=None, name='ad_hoc_suite',
                               run_prod_code=False, **dargs):
        """
        Create a Suite using a given predicate test filters.

        Uses supplied predicate(s) to instantiate a Suite. Looks for tests in
        |autotest_dir| and will schedule them using |afe|.  Pulls control files
        from the default dev server. Results will be pulled from |tko| upon
        completion.

        @param predicates: A list of callables that accept ControlData
                           representations of control files. A test will be
                           included in suite if all callables in this list
                           return True on the given control file.
        @param builds: the builds on which we're running this suite. It's a
                       dictionary of version_prefix:build.
        @param board: the board on which we're running this suite.
        @param devserver: the devserver which contains the build.
        @param cf_getter: control_file_getter.ControlFileGetter. Defaults to
                          using DevServerGetter.
        @param name: name of suite. Defaults to 'ad_hoc_suite'
        @param run_prod_code: If true, the suite will run the tests that
                              lives in prod aka the test code currently on the
                              lab servers.
        @param **dargs: Any other Suite constructor parameters, as described
                        in Suite.__init__ docstring.
        @return a Suite instance.
        """
        if cf_getter is None:
            if run_prod_code:
                cf_getter = create_fs_getter(_AUTOTEST_DIR)
            else:
                build = get_test_source_build(builds, **dargs)
                cf_getter = _create_ds_getter(build, devserver)

        return cls(predicates,
                   name, builds, board, cf_getter, run_prod_code, **dargs)


    @classmethod
    def create_from_name(cls, name, builds, board, devserver, cf_getter=None,
                         **dargs):
        """
        Create a Suite using a predicate based on the SUITE control file var.

        Makes a predicate based on |name| and uses it to instantiate a Suite
        that looks for tests in |autotest_dir| and will schedule them using
        |afe|.  Pulls control files from the default dev server.
        Results will be pulled from |tko| upon completion.

        @param name: a value of the SUITE control file variable to search for.
        @param builds: the builds on which we're running this suite. It's a
                       dictionary of version_prefix:build.
        @param board: the board on which we're running this suite.
        @param devserver: the devserver which contains the build.
        @param cf_getter: control_file_getter.ControlFileGetter. Defaults to
                          using DevServerGetter.
        @param **dargs: Any other Suite constructor parameters, as described
                        in Suite.__init__ docstring.
        @return a Suite instance.
        """
        if cf_getter is None:
            build = get_test_source_build(builds, **dargs)
            cf_getter = _create_ds_getter(build, devserver)

        return cls([name_in_tag_predicate(name)],
                   name, builds, board, cf_getter, **dargs)


    def __init__(
            self,
            predicates,
            tag,
            builds,
            board,
            cf_getter,
            run_prod_code=False,
            afe=None,
            tko=None,
            pool=None,
            results_dir=None,
            max_runtime_mins=24*60,
            timeout_mins=24*60,
            file_bugs=False,
            suite_job_id=None,
            ignore_deps=False,
            extra_deps=None,
            priority=priorities.Priority.DEFAULT,
            forgiving_parser=True,
            wait_for_results=True,
            job_retry=False,
            max_retries=sys.maxint,
            offload_failures_only=False,
            test_source_build=None,
            job_keyvals=None,
            test_args=None,
            child_dependencies=(),
            result_reporter=None,
    ):
        """
        Constructor

        @param predicates: A list of callables that accept ControlData
                           representations of control files. A test will be
                           included in suite if all callables in this list
                           return True on the given control file.
        @param tag: a string with which to tag jobs run in this suite.
        @param builds: the builds on which we're running this suite.
        @param board: the board on which we're running this suite.
        @param cf_getter: a control_file_getter.ControlFileGetter
        @param afe: an instance of AFE as defined in server/frontend.py.
        @param tko: an instance of TKO as defined in server/frontend.py.
        @param pool: Specify the pool of machines to use for scheduling
                purposes.
        @param run_prod_code: If true, the suite will run the test code that
                              lives in prod aka the test code currently on the
                              lab servers.
        @param results_dir: The directory where the job can write results to.
                            This must be set if you want job_id of sub-jobs
                            list in the job keyvals.
        @param max_runtime_mins: Maximum suite runtime, in minutes.
        @param timeout: Maximum job lifetime, in hours.
        @param suite_job_id: Job id that will act as parent id to all sub jobs.
                             Default: None
        @param ignore_deps: True if jobs should ignore the DEPENDENCIES
                            attribute and skip applying of dependency labels.
                            (Default:False)
        @param extra_deps: A list of strings which are the extra DEPENDENCIES
                           to add to each test being scheduled.
        @param priority: Integer priority level.  Higher is more important.
        @param wait_for_results: Set to False to run the suite job without
                                 waiting for test jobs to finish. Default is
                                 True.
        @param job_retry: A bool value indicating whether jobs should be retired
                          on failure. If True, the field 'JOB_RETRIES' in
                          control files will be respected. If False, do not
                          retry.
        @param max_retries: Maximum retry limit at suite level.
                            Regardless how many times each individual test
                            has been retried, the total number of retries
                            happening in the suite can't exceed _max_retries.
                            Default to sys.maxint.
        @param offload_failures_only: Only enable gs_offloading for failed
                                      jobs.
        @param test_source_build: Build that contains the server-side test code.
        @param job_keyvals: General job keyvals to be inserted into keyval file,
                            which will be used by tko/parse later.
        @param test_args: A dict of args passed all the way to each individual
                          test that will be actually ran.
        @param child_dependencies: (optional) list of dependency strings
                to be added as dependencies to child jobs.
        @param result_reporter: A _ResultReporter instance to report results. If
                None, an _EmailReporter will be created.
        """
        tests = find_and_parse_tests(
                cf_getter,
                _ComposedPredicate(predicates),
                tag,
                forgiving_parser=forgiving_parser,
                run_prod_code=run_prod_code,
                test_args=test_args,
        )
        super(Suite, self).__init__(
                tests=tests,
                tag=tag,
                builds=builds,
                board=board,
                afe=afe,
                tko=tko,
                pool=pool,
                results_dir=results_dir,
                max_runtime_mins=max_runtime_mins,
                timeout_mins=timeout_mins,
                file_bugs=file_bugs,
                suite_job_id=suite_job_id,
                ignore_deps=ignore_deps,
                extra_deps=extra_deps,
                priority=priority,
                wait_for_results=wait_for_results,
                job_retry=job_retry,
                max_retries=max_retries,
                offload_failures_only=offload_failures_only,
                test_source_build=test_source_build,
                job_keyvals=job_keyvals,
                child_dependencies=child_dependencies,
                result_reporter=result_reporter,
        )


class ProvisionSuite(_BaseSuite):
    """
    A suite for provisioning DUTs.

    This is done by creating dummy_Pass tests.
    """


    def __init__(
            self,
            tag,
            builds,
            board,
            devserver,
            num_required,
            num_max=float('inf'),
            cf_getter=None,
            run_prod_code=False,
            test_args=None,
            test_source_build=None,
            **kwargs):
        """
        Constructor

        @param tag: a string with which to tag jobs run in this suite.
        @param builds: the builds on which we're running this suite.
        @param board: the board on which we're running this suite.
        @param devserver: the devserver which contains the build.
        @param num_required: number of tests that must pass.  This is
                             capped by the number of tests that are run.
        @param num_max: max number of tests to make.  By default there
                        is no cap, a test is created for each eligible host.
        @param cf_getter: a control_file_getter.ControlFileGetter.
        @param test_args: A dict of args passed all the way to each individual
                          test that will be actually ran.
        @param test_source_build: Build that contains the server-side test code.
        @param kwargs: Various keyword arguments passed to
                       _BaseSuite constructor.
        """
        super(ProvisionSuite, self).__init__(
                tests=[],
                tag=tag,
                builds=builds,
                board=board,
                **kwargs)
        self._num_successful = 0
        self._num_required = 0
        self.tests = []

        static_deps = [dep for dep in self._dependencies
                       if not provision.Provision.acts_on(dep)]
        if 'pool:suites' in static_deps:
            logging.info('Provision suite is disabled on suites pool')
            return
        logging.debug('Looking for hosts matching %r', static_deps)
        hosts = self._afe.get_hosts(
                invalid=False, multiple_labels=static_deps)
        logging.debug('Found %d matching hosts for ProvisionSuite', len(hosts))
        available_hosts = [h for h in hosts if h.is_available()]
        logging.debug('Found %d available hosts for ProvisionSuite',
                      len(available_hosts))
        dummy_test = _load_dummy_test(
                builds, devserver, cf_getter,
                run_prod_code, test_args, test_source_build)
        self.tests = [dummy_test] * min(len(available_hosts), num_max)
        logging.debug('Made %d tests for ProvisionSuite', len(self.tests))
        self._num_required = min(num_required, len(self.tests))
        logging.debug('Expecting %d tests to pass for ProvisionSuite',
                      self._num_required)

    def _handle_result(self, result, record, waiter):
        super(ProvisionSuite, self)._handle_result(result, record, waiter)
        if result.is_good():
            self._num_successful += 1

    def _finished_waiting(self):
        return self._num_successful >= self._num_required


def _load_dummy_test(
        builds,
        devserver,
        cf_getter=None,
        run_prod_code=False,
        test_args=None,
        test_source_build=None):
    """
    Load and return the dummy pass test.

    @param builds: the builds on which we're running this suite.
    @param devserver: the devserver which contains the build.
    @param cf_getter: a control_file_getter.ControlFileGetter.
    @param test_args: A dict of args passed all the way to each individual
                      test that will be actually ran.
    @param test_source_build: Build that contains the server-side test code.
    """
    if cf_getter is None:
        if run_prod_code:
            cf_getter = create_fs_getter(_AUTOTEST_DIR)
        else:
            build = get_test_source_build(
                    builds, test_source_build=test_source_build)
            cf_getter = _create_ds_getter(build, devserver)
    retriever = _get_cf_retriever(cf_getter,
                                  run_prod_code=run_prod_code,
                                  test_args=test_args)
    return retriever.retrieve('dummy_Pass')


class _ComposedPredicate(object):
    """Return the composition of the predicates.

    Predicates are functions that take a test control data object and
    return True of that test is to be included.  The returned
    predicate's set is the intersection of all of the input predicates'
    sets (it returns True if all predicates return True).
    """

    def __init__(self, predicates):
        """Initialize instance.

        @param predicates: Iterable of predicates.
        """
        self._predicates = list(predicates)

    def __repr__(self):
        return '{cls}({this._predicates!r})'.format(
            cls=type(self).__name__,
            this=self,
        )

    def __call__(self, control_data_):
        return all(f(control_data_) for f in self._predicates)


def _is_nonexistent_board_error(e):
    """Return True if error is caused by nonexistent board label.

    As of this writing, the particular case we want looks like this:

     1) e.problem_keys is a dictionary
     2) e.problem_keys['meta_hosts'] exists as the only key
        in the dictionary.
     3) e.problem_keys['meta_hosts'] matches this pattern:
        "Label "board:.*" not found"

    We check for conditions 1) and 2) on the
    theory that they're relatively immutable.
    We don't check condition 3) because it seems
    likely to be a maintenance burden, and for the
    times when we're wrong, being right shouldn't
    matter enough (we _hope_).

    @param e: proxy.ValidationError instance
    @returns: boolean
    """
    return (isinstance(e.problem_keys, dict)
            and len(e.problem_keys) == 1
            and 'meta_hosts' in e.problem_keys)


class _ResultReporter(object):
    """Abstract base class for reporting test results.

    Usually, this is used to report test failures.
    """

    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def report(self, result):
        """Report test result.

        @param result: Status instance for job.
        """


class _EmailReporter(_ResultReporter):
    """Class that emails based on test failures."""

    # TODO(akeshet): Document what |bug_template| is actually supposed to come
    # from, and rename it to something unrelated to "bugs" which are no longer
    # relevant now that this is purely an email sender.
    def __init__(self, suite, bug_template=None):
        self._suite = suite
        self._bug_template = bug_template or {}

    def _get_test_bug(self, result):
        """Get TestBug for the given result.

        @param result: Status instance for a test job.
        @returns: TestBug instance.
        """
        # reporting modules have dependency on external packages, e.g., httplib2
        # Such dependency can cause issue to any module tries to import suite.py
        # without building site-packages first. Since the reporting modules are
        # only used in this function, move the imports here avoid the
        # requirement of building site packages to use other functions in this
        # module.
        from autotest_lib.server.cros.dynamic_suite import reporting

        job_views = self._suite._tko.run('get_detailed_test_views',
                                         afe_job_id=result.id)
        return reporting.TestBug(self._suite._job_creator.cros_build,
                utils.get_chrome_version(job_views),
                self._suite._tag,
                result)

    def _get_bug_template(self, result):
        """Get BugTemplate for test job.

        @param result: Status instance for job.
        @param bug_template: A template dictionary specifying the default bug
                             filing options for failures in this suite.
        @returns: BugTemplate instance
        """
        # reporting modules have dependency on external packages, e.g., httplib2
        # Such dependency can cause issue to any module tries to import suite.py
        # without building site-packages first. Since the reporting modules are
        # only used in this function, move the imports here avoid the
        # requirement of building site packages to use other functions in this
        # module.
        from autotest_lib.server.cros.dynamic_suite import reporting_utils

        # Try to merge with bug template in test control file.
        template = reporting_utils.BugTemplate(self._bug_template)
        try:
            test_data = self._suite._jobs_to_tests[result.id]
            return template.finalize_bug_template(
                    test_data.bug_template)
        except AttributeError:
            # Test control file does not have bug template defined.
            return template.bug_template
        except reporting_utils.InvalidBugTemplateException as e:
            logging.error('Merging bug templates failed with '
                          'error: %s An empty bug template will '
                          'be used.', e)
            return {}

    def report(self, result):
        # reporting modules have dependency on external
        # packages, e.g., httplib2 Such dependency can cause
        # issue to any module tries to import suite.py without
        # building site-packages first. Since the reporting
        # modules are only used in this function, move the
        # imports here avoid the requirement of building site
        # packages to use other functions in this module.
        from autotest_lib.server.cros.dynamic_suite import reporting

        reporting.send_email(
                self._get_test_bug(result),
                self._get_bug_template(result))
