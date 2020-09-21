# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import logging
import time
import warnings

import common

from autotest_lib.client.common_lib import base_job
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import priorities
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server.cros.dynamic_suite.suite import ProvisionSuite
from autotest_lib.server.cros.dynamic_suite.suite import Suite
from autotest_lib.tko import utils as tko_utils


"""CrOS dynamic test suite generation and execution module.

This module implements runtime-generated test suites for CrOS.
Design doc: http://goto.google.com/suitesv2

Individual tests can declare themselves as a part of one or more
suites, and the code here enables control files to be written
that can refer to these "dynamic suites" by name.  We also provide
support for reimaging devices with a given build and running a
dynamic suite across all reimaged devices.

The public API for defining a suite includes one method: reimage_and_run().
A suite control file can be written by importing this module and making
an appropriate call to this single method.  In normal usage, this control
file will be run in a 'hostless' server-side autotest job, scheduling
sub-jobs to do the needed reimaging and test running.

Example control file:

import common
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import dynamic_suite

dynamic_suite.reimage_and_run(
    builds={provision.CROS_VERSION_PREFIX: build}, board=board, name='bvt',
    job=job, pool=pool, check_hosts=check_hosts, add_experimental=True,
    devserver_url=devserver_url)

This will -- at runtime -- find all control files that contain "bvt" in their
"SUITE=" clause, schedule jobs to reimage devices in the
specified pool of the specified board with the specified build and, upon
completion of those jobs, schedule and wait for jobs that run all the tests it
discovered.

Suites can be run by using the atest command-line tool:
  atest suite create -b <board> -i <build/name> <suite>
e.g.
  atest suite create -b x86-mario -i x86-mario/R20-2203.0.0 bvt

-------------------------------------------------------------------------
Implementation details

A Suite instance represents a single test suite, defined by some predicate
run over all known control files.  The simplest example is creating a Suite
by 'name'.

create_suite_job() takes the parameters needed to define a suite run (board,
build to test, machine pool, and which suite to run), ensures important
preconditions are met, finds the appropraite suite control file, and then
schedules the hostless job that will do the rest of the work.

Note that we have more than one Dev server in our test lab architecture.
We currently load balance per-build being tested, so one and only one dev
server is used by any given run through the reimaging/testing flow.

- create_suite_job()
The primary role of create_suite_job() is to ensure that the required
artifacts for the build to be tested are staged on the dev server.  This
includes payloads required to autoupdate machines to the desired build, as
well as the autotest control files appropriate for that build.  Then, the
RPC pulls the control file for the suite to be run from the dev server and
uses it to create the suite job with the autotest frontend.

     +----------------+
     | Google Storage |                                Client
     +----------------+                                   |
               | ^                                        | create_suite_job()
 payloads/     | |                                        |
 control files | | request                                |
               V |                                        V
       +-------------+   download request    +--------------------------+
       |             |<----------------------|                          |
       | Dev Server  |                       | Autotest Frontend (AFE)  |
       |             |---------------------->|                          |
       +-------------+  suite control file   +--------------------------+
                                                          |
                                                          V
                                                      Suite Job (hostless)

- Reimage and Run
The overall process is to schedule all the tests, and then wait for the tests
to complete.

- The Reimaging Process

As an artifact of an old implementation, the number of machines to use
is called the 'sharding_factor', and the default is defined in the [CROS]
section of global_config.ini.

There used to be a 'num' parameter to control the maximum number of
machines, but it does not do anything any more.

A test control file can specify a list of DEPENDENCIES, which are really just
the set of labels a host needs to have in order for that test to be scheduled
on it.  In the case of a dynamic_suite, many tests in the suite may have
DEPENDENCIES specified.  All tests are scheduled with the DEPENDENCIES that
they specify, along with any suite dependencies that were specified, and the
scheduler will find and provision a host capable of running the test.

- Scheduling Suites
A Suite instance uses the labels specified in the suite dependencies to
schedule tests across all the hosts in the pool.  It then waits for all these
jobs.  As an optimization, the Dev server stages the payloads necessary to
run a suite in the background _after_ it has completed all the things
necessary for reimaging.  Before running a suite, reimage_and_run() calls out
to the Dev server and blocks until it's completed staging all build artifacts
needed to run test suites.

Step by step:
0) At instantiation time, find all appropriate control files for this suite
   that were included in the build to be tested.  To do this, we consult the
   Dev Server, where all these control files are staged.

          +------------+    control files?     +--------------------------+
          |            |<----------------------|                          |
          | Dev Server |                       | Autotest Frontend (AFE)  |
          |            |---------------------->|       [Suite Job]        |
          +------------+    control files!     +--------------------------+

1) Now that the Suite instance exists, it schedules jobs for every control
   file it deemed appropriate, to be run on the hosts that were labeled
   by the provisioning.  We stuff keyvals into these jobs, indicating what
   build they were testing and which suite they were for.

   +--------------------------+ Job for VersLabel       +--------+
   |                          |------------------------>| Host 1 | VersLabel
   | Autotest Frontend (AFE)  |            +--------+   +--------+
   |       [Suite Job]        |----------->| Host 2 |
   +--------------------------+ Job for    +--------+
       |                ^       VersLabel        VersLabel
       |                |
       +----------------+
        One job per test
        {'build': build/name,
         'suite': suite_name}

2) Now that all jobs are scheduled, they'll be doled out as labeled hosts
   finish their assigned work and become available again.

- Waiting on Suites
0) As we clean up each test job, we check to see if any crashes occurred.  If
   they did, we look at the 'build' keyval in the job to see which build's debug
   symbols we'll need to symbolicate the crash dump we just found.

1) Using this info, we tell a special Crash Server to stage the required debug
   symbols. Once that's done, we ask the Crash Server to use those symbols to
   symbolicate the crash dump in question.

     +----------------+
     | Google Storage |
     +----------------+
          |     ^
 symbols! |     | symbols?
          V     |
      +------------+  stage symbols for build  +--------------------------+
      |            |<--------------------------|                          |
      |   Crash    |                           |                          |
      |   Server   |   dump to symbolicate     | Autotest Frontend (AFE)  |
      |            |<--------------------------|       [Suite Job]        |
      |            |-------------------------->|                          |
      +------------+    symbolicated dump      +--------------------------+

2) As jobs finish, we record their success or failure in the status of the suite
   job.  We also record a 'job keyval' in the suite job for each test, noting
   the job ID and job owner.  This can be used to refer to test logs later.
3) Once all jobs are complete, status is recorded for the suite job, and the
   job_repo_url host attribute is removed from all hosts used by the suite.

"""


# Relevant CrosDynamicSuiteExceptions are defined in client/common_lib/error.py.

class _SuiteSpec(object):
    """This class contains the info that defines a suite run."""

    _REQUIRED_KEYWORDS = {
            'board': str,
            'builds': dict,
            'name': str,
            'job': base_job.base_job,
            'devserver_url': str,
    }

    _VERSION_PREFIXES = frozenset((
            provision.CROS_VERSION_PREFIX,
            provision.CROS_ANDROID_VERSION_PREFIX,
            provision.ANDROID_BUILD_VERSION_PREFIX,
    ))

    def __init__(
            self,
            builds=None,
            board=None,
            name=None,
            job=None,
            devserver_url=None,
            pool=None,
            check_hosts=True,
            add_experimental=True,
            file_bugs=False,
            max_runtime_mins=24*60,
            timeout_mins=24*60,
            suite_dependencies=None,
            bug_template=None,
            priority=priorities.Priority.DEFAULT,
            predicate=None,
            wait_for_results=True,
            job_retry=False,
            max_retries=None,
            offload_failures_only=False,
            test_source_build=None,
            run_prod_code=False,
            delay_minutes=0,
            job_keyvals=None,
            test_args=None,
            child_dependencies=(),
            **dargs):
        """
        Vets arguments for reimage_and_run() and populates self with supplied
        values.

        Currently required args:
        @param builds: the builds to install e.g.
                       {'cros-version:': 'x86-alex-release/R18-1655.0.0',
                        'fwrw-version:': 'x86-alex-firmware/R36-5771.50.0'}
        @param board: which kind of devices to reimage.
        @param name: a value of the SUITE control file variable to search for.
        @param job: an instance of client.common_lib.base_job representing the
                    currently running suite job.
        @param devserver_url: url to the selected devserver.

        Currently supported optional args:
        @param pool: the pool of machines to use for scheduling purposes.
        @param check_hosts: require appropriate hosts to be available now.
        @param add_experimental: schedule experimental tests as well, or not.
        @param file_bugs: File bugs when tests in this suite fail.
        @param max_runtime_mins: Max runtime in mins for each of the sub-jobs
                                 this suite will run.
        @param timeout_mins: Max lifetime in minutes for each of the sub-jobs
                             that this suite runs.
        @param suite_dependencies: A list of strings of suite level
                                   dependencies, which act just like test
                                   dependencies and are appended to each test's
                                   set of dependencies at job creation time.
                                   A string of comma seperated labels is
                                   accepted for backwards compatibility.
        @param bug_template: A template dictionary specifying the default bug
                             filing options for failures in this suite.
        @param priority: Integer priority level.  Higher is more important.
        @param predicate: Optional argument. If present, should be a function
                          mapping ControlData objects to True if they should be
                          included in suite. If argument is absent, suite
                          behavior will default to creating a suite of based
                          on the SUITE field of control files.
        @param wait_for_results: Set to False to run the suite job without
                                 waiting for test jobs to finish.
        @param job_retry: Set to True to enable job-level retry.
        @param max_retries: Maximum retry limit at suite level if not None.
                            Regardless how many times each individual test
                            has been retried, the total number of retries
                            happening in the suite can't exceed max_retries.
        @param offload_failures_only: Only enable gs_offloading for failed
                                      jobs.
        @param test_source_build: Build that contains the server-side test code,
                e.g., it can be the value of builds['cros-version:'] or
                builds['fw-version:']. None uses the server-side test code from
                builds['cros-version:'].
        @param run_prod_code: If true, the suite will run the test code that
                              lives in prod aka the test code currently on the
                              lab servers.
        @param delay_minutes: Delay the creation of test jobs for a given number
                              of minutes.
        @param job_keyvals: General job keyvals to be inserted into keyval file
        @param test_args: A dict of args passed all the way to each individual
                          test that will be actually ran.
        @param child_dependencies: (optional) list of dependency strings
                to be added as dependencies to child jobs.
        @param **dargs: these arguments will be ignored.  This allows us to
                        deprecate and remove arguments in ToT while not
                        breaking branch builds.
        """
        self._check_init_params(
                board=board,
                builds=builds,
                name=name,
                job=job,
                devserver_url=devserver_url)

        self.board = 'board:%s' % board
        self.builds = builds
        self.name = name
        self.job = job
        self.pool = ('pool:%s' % pool) if pool else pool
        self.check_hosts = check_hosts
        self.add_experimental = add_experimental
        self.file_bugs = file_bugs
        self.dependencies = {'': []}
        self.max_runtime_mins = max_runtime_mins
        self.timeout_mins = timeout_mins
        self.bug_template = {} if bug_template is None else bug_template
        self.priority = priority
        self.wait_for_results = wait_for_results
        self.job_retry = job_retry
        self.max_retries = max_retries
        self.offload_failures_only = offload_failures_only
        self.run_prod_code = run_prod_code
        self.delay_minutes = delay_minutes
        self.job_keyvals = job_keyvals
        self.test_args = test_args
        self.child_dependencies = child_dependencies

        self._init_predicate(predicate)
        self._init_suite_dependencies(suite_dependencies)
        self._init_devserver(devserver_url)
        self._init_test_source_build(test_source_build)
        self._translate_builds()
        self._add_builds_to_suite_deps()

        for key, value in dargs.iteritems():
            warnings.warn('Ignored key %r was passed to suite with value %r'
                          % (key, value))

    def _check_init_params(self, **kwargs):
        for key, expected_type in self._REQUIRED_KEYWORDS.iteritems():
            value = kwargs.get(key)
            # TODO(ayatane): `not value` includes both the cases where value is
            # None and where value is the correct type, but empty (e.g., empty
            # dict).  It looks like this is NOT the intended behavior, but I'm
            # hesitant to remove it in case something is actually relying on
            # this behavior.
            if not value or not isinstance(value, expected_type):
                raise error.SuiteArgumentException(
                        'reimage_and_run() needs %s=<%r>'
                        % (key, expected_type))

    def _init_predicate(self, predicate):
        """Initialize predicate attribute."""
        if predicate is None:
            self.predicate = Suite.name_in_tag_predicate(self.name)
        else:
            self.predicate = predicate


    def _init_suite_dependencies(self, suite_dependencies):
        """Initialize suite dependencies attribute."""
        if suite_dependencies is None:
            self.suite_dependencies = []
        elif isinstance(suite_dependencies, str):
            self.suite_dependencies = [dep.strip(' ') for dep
                                       in suite_dependencies.split(',')]
        else:
            self.suite_dependencies = suite_dependencies

    def _init_devserver(self, devserver_url):
        """Initialize devserver attribute."""
        if provision.ANDROID_BUILD_VERSION_PREFIX in self.builds:
            self.devserver = dev_server.AndroidBuildServer(devserver_url)
        else:
            self.devserver = dev_server.ImageServer(devserver_url)

    def _init_test_source_build(self, test_source_build):
        """Initialize test_source_build attribute."""
        if test_source_build:
            test_source_build = self.devserver.translate(test_source_build)

        self.test_source_build = Suite.get_test_source_build(
                self.builds, test_source_build=test_source_build)

    def _translate_builds(self):
        """Translate build names if they are in LATEST format."""
        for prefix in self._VERSION_PREFIXES:
            if prefix in self.builds:
                translated_build = self.devserver.translate(
                        self.builds[prefix])
                self.builds[prefix] = translated_build

    def _add_builds_to_suite_deps(self):
        """Add builds to suite_dependencies.

        To support provision both CrOS and firmware, option builds are added to
        _SuiteSpec, e.g.,

        builds = {'cros-version:': 'x86-alex-release/R18-1655.0.0',
                  'fwrw-version:': 'x86-alex-firmware/R36-5771.50.0'}

        version_prefix+build should make it into each test as a DEPENDENCY.
        The easiest way to do this is to tack it onto the suite_dependencies.
        """
        self.suite_dependencies.extend(
                provision.join(version_prefix, build)
                for version_prefix, build in self.builds.iteritems()
        )


class _ProvisionSuiteSpec(_SuiteSpec):

    def __init__(self, num_required, **kwargs):
        self.num_required = num_required
        super(_ProvisionSuiteSpec, self).__init__(**kwargs)


def run_provision_suite(**dargs):
    """
    Run a provision suite.

    Will re-image a number of devices (of the specified board) with the
    provided builds by scheduling dummy_Pass.

    @param job: an instance of client.common_lib.base_job representing the
                currently running suite job.

    @raises AsynchronousBuildFailure: if there was an issue finishing staging
                                      from the devserver.
    @raises MalformedDependenciesException: if the dependency_info file for
                                            the required build fails to parse.
    """
    spec = _ProvisionSuiteSpec(**dargs)

    afe = frontend_wrappers.RetryingAFE(timeout_min=30, delay_sec=10,
                                        user=spec.job.user, debug=False)
    tko = frontend_wrappers.RetryingTKO(timeout_min=30, delay_sec=10,
                                        user=spec.job.user, debug=False)

    try:
        my_job_id = int(tko_utils.get_afe_job_id(spec.job.tag))
        logging.debug('Determined own job id: %d', my_job_id)
    except ValueError:
        my_job_id = None
        logging.warning('Could not determine own job id.')

    suite = ProvisionSuite(
            tag=spec.name,
            builds=spec.builds,
            board=spec.board,
            devserver=spec.devserver,
            num_required=spec.num_required,
            afe=afe,
            tko=tko,
            pool=spec.pool,
            results_dir=spec.job.resultdir,
            max_runtime_mins=spec.max_runtime_mins,
            timeout_mins=spec.timeout_mins,
            file_bugs=spec.file_bugs,
            suite_job_id=my_job_id,
            extra_deps=spec.suite_dependencies,
            priority=spec.priority,
            wait_for_results=spec.wait_for_results,
            job_retry=spec.job_retry,
            max_retries=spec.max_retries,
            offload_failures_only=spec.offload_failures_only,
            test_source_build=spec.test_source_build,
            run_prod_code=spec.run_prod_code,
            job_keyvals=spec.job_keyvals,
            test_args=spec.test_args,
            child_dependencies=spec.child_dependencies,
    )

    _run_suite_with_spec(suite, spec)

    logging.debug('Returning from dynamic_suite.run_provision_suite')


def reimage_and_run(**dargs):
    """
    Backward-compatible API for dynamic_suite.

    Will re-image a number of devices (of the specified board) with the
    provided builds, and then run the indicated test suite on them.
    Guaranteed to be compatible with any build from stable to dev.

    @param dargs: Dictionary containing the arguments passed to _SuiteSpec().
    @raises AsynchronousBuildFailure: if there was an issue finishing staging
                                      from the devserver.
    @raises MalformedDependenciesException: if the dependency_info file for
                                            the required build fails to parse.
    """
    suite_spec = _SuiteSpec(**dargs)

    afe = frontend_wrappers.RetryingAFE(timeout_min=30, delay_sec=10,
                                        user=suite_spec.job.user, debug=False)
    tko = frontend_wrappers.RetryingTKO(timeout_min=30, delay_sec=10,
                                        user=suite_spec.job.user, debug=False)

    try:
        my_job_id = int(tko_utils.get_afe_job_id(dargs['job'].tag))
        logging.debug('Determined own job id: %d', my_job_id)
    except ValueError:
        my_job_id = None
        logging.warning('Could not determine own job id.')

    _perform_reimage_and_run(suite_spec, afe, tko, suite_job_id=my_job_id)

    logging.debug('Returning from dynamic_suite.reimage_and_run.')


def _perform_reimage_and_run(spec, afe, tko, suite_job_id=None):
    """
    Do the work of reimaging hosts and running tests.

    @param spec: a populated _SuiteSpec object.
    @param afe: an instance of AFE as defined in server/frontend.py.
    @param tko: an instance of TKO as defined in server/frontend.py.
    @param suite_job_id: Job id that will act as parent id to all sub jobs.
                         Default: None
    """
    # We can't create the suite until the devserver has finished downloading
    # control_files and test_suites packages so that we can get the control
    # files to schedule.
    if not spec.run_prod_code:
        _stage_artifacts_for_build(spec.devserver, spec.test_source_build)
    suite = Suite.create_from_predicates(
            predicates=[spec.predicate],
            name=spec.name,
            builds=spec.builds,
            board=spec.board,
            devserver=spec.devserver,
            afe=afe,
            tko=tko,
            pool=spec.pool,
            results_dir=spec.job.resultdir,
            max_runtime_mins=spec.max_runtime_mins,
            timeout_mins=spec.timeout_mins,
            file_bugs=spec.file_bugs,
            suite_job_id=suite_job_id,
            extra_deps=spec.suite_dependencies,
            priority=spec.priority,
            wait_for_results=spec.wait_for_results,
            job_retry=spec.job_retry,
            max_retries=spec.max_retries,
            offload_failures_only=spec.offload_failures_only,
            test_source_build=spec.test_source_build,
            run_prod_code=spec.run_prod_code,
            job_keyvals=spec.job_keyvals,
            test_args=spec.test_args,
            child_dependencies=spec.child_dependencies,
    )
    _run_suite_with_spec(suite, spec)


def _run_suite_with_spec(suite, spec):
    """
    Do the work of reimaging hosts and running tests.

    @param suite: _BaseSuite instance to run.
    @param spec: a populated _SuiteSpec object.
    """
    _run_suite(
        suite=suite,
        job=spec.job,
        delay_minutes=spec.delay_minutes,
        bug_template=spec.bug_template)


def _run_suite(
        suite,
        job,
        delay_minutes,
        bug_template):
    """
    Run a suite.

    @param suite: _BaseSuite instance.
    @param job: an instance of client.common_lib.base_job representing the
                currently running suite job.
    @param delay_minutes: Delay the creation of test jobs for a given number
                          of minutes.
    @param bug_template: A template dictionary specifying the default bug
                         filing options for failures in this suite.
    """
    timestamp = datetime.datetime.now().strftime(time_utils.TIME_FMT)
    utils.write_keyval(
        job.resultdir,
        {constants.ARTIFACT_FINISHED_TIME: timestamp})

    if delay_minutes:
        logging.debug('delay_minutes is set. Sleeping %d minutes before '
                      'creating test jobs.', delay_minutes)
        time.sleep(delay_minutes*60)
        logging.debug('Finished waiting for %d minutes before creating test '
                      'jobs.', delay_minutes)

    # Now we get to asychronously schedule tests.
    suite.schedule(job.record_entry)

    if suite.wait_for_results:
        logging.debug('Waiting on suite.')
        suite.wait(job.record_entry)
        logging.debug('Finished waiting on suite. '
                      'Returning from _perform_reimage_and_run.')
    else:
        logging.info('wait_for_results is set to False, suite job will exit '
                     'without waiting for test jobs to finish.')


def _stage_artifacts_for_build(devserver, build):
    """Stage artifacts for a suite job.

    @param devserver: devserver to stage artifacts with.
    @param build: image to stage artifacts for.
    """
    try:
        devserver.stage_artifacts(
                image=build,
                artifacts=['control_files', 'test_suites'])
    except dev_server.DevServerException as e:
        # If we can't get the control files, there's nothing to run.
        raise error.AsynchronousBuildFailure(e)
