#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Tool for running suites of tests and waiting for completion.

The desired test suite will be scheduled with autotest. By default,
this tool will block until the job is complete, printing a summary
at the end.  Error conditions result in exceptions.

This is intended for use only with Chrome OS test suits that leverage the
dynamic suite infrastructure in server/cros/dynamic_suite.py.

This script exits with one of the following codes:
0 - OK: Suite finished successfully
1 - ERROR: Test(s) failed, or hits its own timeout
2 - WARNING: Test(s) raised a warning or passed on retry, none failed/timed out.
3 - INFRA_FAILURE: Infrastructure related issues, e.g.
    * Lab is down
    * Too many duts (defined as a constant) in repair failed status
    * Suite job issues, like bug in dynamic suite,
      user aborted the suite, lose a drone/all devservers/rpc server,
      0 tests ran, etc.
    * provision failed
      TODO(fdeng): crbug.com/413918, reexamine treating all provision
                   failures as INFRA failures.
4 - SUITE_TIMEOUT: Suite timed out, some tests ran,
    none failed by the time the suite job was aborted. This will cover,
    but not limited to, the following cases:
    * A devserver failure that manifests as a timeout
    * No DUTs available midway through a suite
    * Provision/Reset/Cleanup took longer time than expected for new image
    * A regression in scheduler tick time.
5- BOARD_NOT_AVAILABLE: If there is no host for the requested board/pool.
6- INVALID_OPTIONS: If options are not valid.
"""

import argparse
import ast
import collections
from collections import namedtuple
from datetime import datetime
from datetime import timedelta
import functools
import getpass
import json
import logging
import os
import re
import sys
import time
import warnings

import common
from chromite.lib import buildbot_annotations as annotations

from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config, enum
from autotest_lib.client.common_lib import priorities
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.frontend.afe import rpc_client_lib
from autotest_lib.frontend.afe.json_rpc import proxy
from autotest_lib.server import site_utils
from autotest_lib.server import utils
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server.cros.dynamic_suite import reporting_utils
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.site_utils import diagnosis_utils
from autotest_lib.site_utils import job_overhead

CONFIG = global_config.global_config

_DEFAULT_AUTOTEST_INSTANCE = CONFIG.get_config_value(
        'SERVER', 'hostname', type=str)
_URL_PATTERN = CONFIG.get_config_value('CROS', 'log_url_pattern', type=str)

# Return code that will be sent back to autotest_rpc_server.py
RETURN_CODES = enum.Enum(
        'OK', 'ERROR', 'WARNING', 'INFRA_FAILURE', 'SUITE_TIMEOUT',
        'BOARD_NOT_AVAILABLE', 'INVALID_OPTIONS')

# Minimum RPC timeout setting for calls expected to take long time, e.g.,
# create_suite_job. If default socket time (socket.getdefaulttimeout()) is
# None or greater than this value, the default will be used.
# The value here is set to be the same as the timeout for the RetryingAFE object
# so long running RPCs can wait long enough before being aborted.
_MIN_RPC_TIMEOUT = 600

# Number of days back to search for existing job.
_SEARCH_JOB_MAX_DAYS = 14

_PROVISION_SUITE = 'provision'


@functools.total_ordering
class _ReturnResult(object):
    """Represents overall result of run_suite operation.

    _ReturnResult instances sort based on priority (the order in
    _RETURN_RESULTS).

    Furthermore, _ReturnResult instances can be combined by bitwise or
    ("union"), which returns the instance with the higher priority
    between the two (the instance with higher priority is a "superset"
    of the other).

    Do not create new instances of this; use _RETURN_RESULTS instead.
    """

    def __init__(self, return_code, message):
        self.return_code = return_code
        self.message = message

    def __repr__(self):
        return '<{cls} {key}, {this.return_code}, {this.message}>'.format(
            cls=type(self).__name__,
            key=self._getkey(),
            this=self)

    def __gt__(self, other):
        if isinstance(other, type(self)):
            return self._getkey() > other._getkey()
        else:
            return NotImplemented

    def __eq__(self, other):
        if isinstance(other, type(self)):
            return (self.return_code == other.return_code
                    and self.message == other.message)
        else:
            return NotImplemented

    def __hash__(self):
        return hash(self.return_code) ^ hash(self.message)

    def __or__(self, other):
        if isinstance(other, type(self)):
            if self > other:
                return self
            else:
                return other
        else:
            return NotImplemented

    def _getkey(self):
        """Return sort key."""
        return _RETURN_RESULTS_LIST.index(self)

    def suite_result(self, output_dict=None):
        """Make a SuiteResult using this _ReturnResult.

        @param output_dict: output_dict to merge into SuiteResult.
        """
        if output_dict is None:
            output_dict = dict()
        else:
            output_dict = output_dict.copy()
        if self.message:
            output_dict['return_message'] = self.message
        return SuiteResult(self.return_code, output_dict)


_RETURN_RESULTS = collections.OrderedDict([
    ('ok', _ReturnResult(RETURN_CODES.OK, '')),

    ('test_warning', _ReturnResult(
        RETURN_CODES.WARNING, 'Test job raised warning.')),
    ('suite_warning', _ReturnResult(
        RETURN_CODES.WARNING, 'Suite job raised warning.')),
    ('test_retry', _ReturnResult(
        RETURN_CODES.WARNING, 'Tests were retried.')),

    ('test_aborted_prestart', _ReturnResult(
        RETURN_CODES.SUITE_TIMEOUT,
        'Tests were aborted before running; suite must have timed out.')),
    # This really indicates a user action or an infra failure. But, suite
    # timeouts cause similar fauilres in the individual tests, so we must
    # classify these lower than suite_timeout. In case of a suite_timeout, the
    # result from the suite job will promote the result to suite_timeout.
    ('test_aborted_mystery',
     _ReturnResult(
             RETURN_CODES.SUITE_TIMEOUT,
             'Tests were aborted after running, but before timeout; '
             'Test was manually aborted or parsing results failed: '
             'crbug.com/796348.')),
    ('suite_timeout', _ReturnResult(
        RETURN_CODES.SUITE_TIMEOUT, 'Suite job timed out.')),

    ('test_views_missing', _ReturnResult(
        RETURN_CODES.INFRA_FAILURE, 'No test views found.')),
    ('suite_failed', _ReturnResult(
        RETURN_CODES.INFRA_FAILURE, 'Suite job failed.')),
    ('provision_failed', _ReturnResult(
        RETURN_CODES.INFRA_FAILURE, 'Provisioning failed.')),

    ('test_failure', _ReturnResult(
        RETURN_CODES.ERROR, 'Tests failed.')),
])
_RETURN_RESULTS_LIST = list(_RETURN_RESULTS.values())


def bool_str(x):
    """Boolean string type for option arguments.

    @param x: string representation of boolean value.

    """
    if x == 'True':
        return True
    elif x == 'False':
        return False
    else:
        raise argparse.ArgumentTypeError(
            '%s is not one of True or False' % (x,))


def _get_priority_value(x):
    """Convert a priority representation to its int value.

    Priorities can be described either by an int value (possibly as a string)
    or a name string.  This function coerces both forms to an int value.

    This function is intended for casting command line arguments during
    parsing.

    @param x: priority value as an int, int string, or name string

    @returns: int value of priority
    """
    try:
        return int(x)
    except ValueError:
        try:
            return priorities.Priority.get_value(x)
        except AttributeError:
            raise argparse.ArgumentTypeError(
                'Unknown priority level %s.  Try one of %s.'
                % (x, ', '.join(priorities.Priority.names)))


def make_parser():
    """Make ArgumentParser instance for run_suite.py."""
    parser = argparse.ArgumentParser(
        usage="%(prog)s [options]")
    parser.add_argument("-b", "--board", dest="board")
    parser.add_argument(
            "--model",
            help="The device model to run tests against. For non-unified "
                 "builds, model and board are synonymous, but board is more "
                 "accurate in some cases. Only pass this option if your build "
                 "is a unified build.",
    )
    parser.add_argument("-i", "--build", dest="build")
    parser.add_argument(
        "-w", "--web", dest="web", default=None,
        help="Address of a webserver to receive suite requests.")
    parser.add_argument(
        '--cheets_build', dest='cheets_build', default=None,
        help='ChromeOS Android build to be installed on dut.')
    parser.add_argument(
        '--firmware_rw_build', dest='firmware_rw_build', default=None,
        help='Firmware build to be installed in dut RW firmware.')
    parser.add_argument(
        '--firmware_ro_build', dest='firmware_ro_build', default=None,
        help='Firmware build to be installed in dut RO firmware.')
    parser.add_argument(
        '--test_source_build', dest='test_source_build', default=None,
        help=('Build that contains the test code, '
              'e.g., it can be the value of `--build`, '
              '`--firmware_rw_build` or `--firmware_ro_build` '
              'arguments. Default is None, that is, use the test '
              'code from `--build` (CrOS image)'))
    #  This should just be a boolean flag, but the autotest "proxy" code
    #  can't handle flags that don't take arguments.
    parser.add_argument(
        "-n", "--no_wait", dest="no_wait", default=False, type=bool_str,
        help='Must pass "True" or "False" if used.')
    # If you really want no pool, --pool="" will do it. USE WITH CARE.
    parser.add_argument("-p", "--pool", dest="pool", default="suites")
    parser.add_argument("-s", "--suite_name", dest="name")
    parser.add_argument("-a", "--afe_timeout_mins", type=int,
                        dest="afe_timeout_mins", default=30)
    parser.add_argument("-t", "--timeout_mins", type=int,
                        dest="timeout_mins", default=1440)
    parser.add_argument("-x", "--max_runtime_mins", type=int,
                        dest="max_runtime_mins", default=1440)
    parser.add_argument("-d", "--delay_sec", type=int,
                        dest="delay_sec", default=10)
    parser.add_argument("-m", "--mock_job_id", dest="mock_job_id",
                        help="Attach to existing job id for already running "
                        "suite, and creates report.")
    # NOTE(akeshet): This looks similar to --no_wait, but behaves differently.
    # --no_wait is passed in to the suite rpc itself and affects the suite,
    # while this does not.
    parser.add_argument("-c", "--create_and_return", dest="create_and_return",
                        action="store_true",
                        help="Create the suite and print the job id, then "
                        "finish immediately.")
    parser.add_argument("-u", "--num", dest="num", type=int, default=None,
                        help="Deprecated, does nothing.")
    #  Same boolean flag issue applies here.
    parser.add_argument(
        "-f", "--file_bugs", dest="file_bugs", default=False, type=bool_str,
        help=('File bugs on test failures. Must pass "True" or '
              '"False" if used.'))
    parser.add_argument("-l", "--bypass_labstatus", dest="bypass_labstatus",
                        action="store_true", help='Bypass lab status check.')
    # We allow either a number or a string for the priority.  This way, if you
    # know what you're doing, one can specify a custom priority level between
    # other levels.
    parser.add_argument("-r", "--priority", dest="priority",
                        type=_get_priority_value,
                        default=priorities.Priority.DEFAULT,
                        action="store",
                        help="Priority of suite. Either numerical value, or "
                        "one of (" + ", ".join(priorities.Priority.names)
                        + ").")
    parser.add_argument(
        '--retry', dest='retry', default=False, type=bool_str, action='store',
        help='Enable test retry.  Must pass "True" or "False" if used.')
    parser.add_argument('--max_retries', dest='max_retries', default=None,
                        type=int, action='store', help='Maximum retries'
                        'allowed at suite level. No limit if not specified.')
    parser.add_argument('--minimum_duts', dest='minimum_duts', type=int,
                        default=0, action='store',
                        help='Check that the pool has at least such many '
                        'healthy machines, otherwise suite will not run. '
                        'Default to 0.')
    parser.add_argument('--suite_min_duts', dest='suite_min_duts', type=int,
                        default=0, action='store',
                        help='Preferred minimum number of machines. Scheduler '
                        'will prioritize on getting such many machines for '
                        'the suite when it is competing with another suite '
                        'that has a higher priority but already got minimum '
                        'machines it needs. Default to 0.')
    parser.add_argument("--suite_args", dest="suite_args",
                        type=ast.literal_eval,
                        default=None, action="store",
                        help="A dict of args passed to the suite control file.")
    parser.add_argument('--offload_failures_only',
                        dest='offload_failures_only', type=bool_str,
                        action='store', default=False,
                        help='Only enable gs_offloading for failed tests. '
                        'Successful tests will be deleted. Must pass "True"'
                        ' or "False" if used.')
    parser.add_argument('--use_suite_attr', dest='use_suite_attr',
                        action='store_true', default=False,
                        help='Advanced. Run the suite based on ATTRIBUTES of '
                        'control files, rather than SUITE.')
    parser.add_argument('--json_dump', dest='json_dump', action='store_true',
                        default=False,
                        help='Dump the output of run_suite to stdout.')
    parser.add_argument(
        '--run_prod_code', dest='run_prod_code',
        action='store_true', default=False,
        help='Run the test code that lives in prod aka the test '
        'code currently on the lab servers.')
    parser.add_argument(
        '--delay_minutes', type=int, default=0,
        help=('Delay the creation of test jobs for a given '
              'number of minutes. This argument can be used to '
              'force provision jobs being delayed, which helps '
              'to distribute loads across devservers.'))
    parser.add_argument(
        '--skip_duts_check', dest='skip_duts_check', action='store_true',
        default=False, help='If True, skip minimum available DUTs check')
    parser.add_argument(
        '--job_keyvals', dest='job_keyvals', type=ast.literal_eval,
        action='store', default=None,
        help='A dict of job keyvals to be inject to suite control file')
    parser.add_argument(
        '--test_args', dest='test_args', type=ast.literal_eval,
        action='store', default=None,
        help=('A dict of args passed all the way to each individual test that '
              'will be actually ran.'))
    parser.add_argument(
        '--require_logfile', action='store_true',
        help=('Stream logs of run_suite.py to a local file named '
              'run_suite-<build name>.log.'))

    # Used for monitoring purposes, to measure no-op swarming proxy latency.
    parser.add_argument('--do_nothing', action='store_true',
                        help=argparse.SUPPRESS)

    # Used when lab/job status checking is needed. Currently its only user is
    # suite scheduler v2.
    parser.add_argument(
        '--pre_check', action='store_true',
        help=('Check lab and job status before kicking off a suite. Used by '
              'suite scheduler v2.'))

    # TODO(crbug.com/763207): This is to support calling old moblab RPC
    # with ToT code.  This does not need to be supported after M62.
    parser.add_argument('--oldrpc', action='store_true',
                        help='Use old AFE RPC.')

    return parser


def verify_and_clean_options(options):
    """Verify the validity of options.

    @param options: The parsed options to verify.

    @returns: True if verification passes, False otherwise.

    """
    if options.mock_job_id and (
            not options.build or not options.name or not options.board):
        print ('When using -m, need to specify build, board and suite '
               'name which you have used for creating the original job')
        return False
    else:
        if not options.build:
            print 'Need to specify which build to use'
            return False
        if not options.board:
            print 'Need to specify board'
            return False
        if not options.name:
            print 'Need to specify suite name'
            return False
    if options.num is not None:
        warnings.warn('-u/--num option is deprecated; it does nothing.')
    del options.num
    if not options.retry and options.max_retries is not None:
        print 'max_retries can only be used with --retry=True'
        return False
    if options.use_suite_attr and options.suite_args is not None:
        print ('The new suite control file cannot parse the suite_args: %s.'
               'Please not specify any suite_args here.' % options.suite_args)
        return False
    if options.no_wait and options.retry:
        print 'Test retry is not available when using --no_wait=True'
    # Default to use the test code in CrOS build.
    if not options.test_source_build and options.build:
        options.test_source_build = options.build
    return True


def change_options_for_suite_attr(options):
    """Change options to be prepared to run the suite_attr_wrapper.

    If specify 'use_suite_attr' from the cmd line, it indicates to run the
    new style suite control file, suite_attr_wrapper. Then, change the
    options.name to 'suite_attr_wrapper', change the options.suite_args to
    include the arguments needed by suite_attr_wrapper.

    @param options: The verified options.

    @returns: The changed options.

    """
    # Convert the suite_name to attribute boolean expression.
    if type(options.name) is str:
        attr_filter_val = 'suite:%s' % options.name
    else:
        attr_filter_val = ' or '.join(['suite:%s' % x for x in options.name])

    # change the suite_args to be a dict of arguments for suite_attr_wrapper
    # if suite_args is not None, store the values in 'other_args' of the dict
    args_dict = {}
    args_dict['attr_filter'] = attr_filter_val
    options.suite_args = args_dict
    options.name = 'suite_attr_wrapper'

    return options


class TestResult(object):

    """Represents the result of a TestView."""

    def __init__(self, test_view, retry_count=0):
        """Initialize instance.

        @param test_view: TestView instance.
        @param retry_count: Retry count for test.  Optional.
        """
        self.name = test_view.get_testname()
        self.status = test_view['status']
        self.reason = test_view['reason']
        self.retry_count = retry_count

    _PRETTY_STATUS_MAP = {
        'GOOD':    '[ PASSED ]',
        'TEST_NA': '[  INFO  ]',
    }

    @property
    def _pretty_status(self):
        """Pretty status string."""
        return self._PRETTY_STATUS_MAP.get(self.status, '[ FAILED ]')

    def log_using(self, log_function, name_column_width):
        """Log the test result using the given log function.

        @param log_function: Log function to use.  Example: logging.info
        @param name_column_width: Width of name column for formatting.
        """
        padded_name = self.name.ljust(name_column_width)
        log_function('%s%s', padded_name, self._pretty_status)
        if self.status != 'GOOD':
            log_function('%s  %s: %s', padded_name, self.status, self.reason)
        if self.retry_count > 0:
            log_function('%s  retry_count: %s', padded_name, self.retry_count)


def get_original_suite_name(suite_name, suite_args):
    """Get the original suite name when running suite_attr_wrapper.

    @param suite_name: the name of the suite launched in afe. When it is
                       suite_attr_wrapper, the suite that actually running is
                       specified in the suite_args.
    @param suite_args: dict of suite args from argument parsing.

    @returns: the original suite name.

    """
    if suite_name == 'suite_attr_wrapper':
        attrs = suite_args.get('attr_filter', '')
        suite_list = ([x[6:] for x in re.split('[() ]', attrs)
                       if x and x.startswith('suite:')])
        return suite_list[0] if suite_list else suite_name
    return suite_name


class LogLink(object):
    """Information needed to record a link in the logs.

    Depending on context and the information provided at
    construction time, the link may point to either to log files for
    a job, or to a bug filed for a failure in the job.

    @var anchor  The link text.
    @var url     The link url.
    @var bug_id  Id of a bug to link to, or None.
    """

    # A list of tests that don't get retried so skip the dashboard.
    _SKIP_RETRY_DASHBOARD = ['provision']

    _BUG_LINK_PREFIX = 'Auto-Bug'
    _LOG_LINK_PREFIX = 'Test-Logs'


    def __init__(self, anchor, server, job_string, bug_info=None, reason=None,
                 retry_count=0, testname=None, sponge_url=None):
        """Initialize the LogLink by generating the log URL.

        @param anchor      The link text.
        @param server      The hostname of the server this suite ran on.
        @param job_string  The job whose logs we'd like to link to.
        @param bug_info    Info about the bug, if one was filed.
        @param reason      A string representing the reason of failure if any.
        @param retry_count How many times the test has been retried.
        @param testname    Optional Arg that supplies the testname.
        @param sponge_url  url to Sponge result.
        """
        self.anchor = anchor
        self.url = _URL_PATTERN % (rpc_client_lib.add_protocol(server),
                                   job_string)
        self.reason = reason
        self.retry_count = retry_count
        self.testname = testname
        self.sponge_url = sponge_url
        if bug_info:
            self.bug_id, self.bug_count = bug_info
        else:
            self.bug_id = None
            self.bug_count = None


    @property
    def bug_url(self):
        """URL of associated bug."""
        if self.bug_id:
            return reporting_utils.link_crbug(self.bug_id)
        else:
            return None


    @property
    def _bug_count_text(self):
        """Return bug count as human friendly text."""
        if self.bug_count is None:
            bug_info = 'unknown number of reports'
        elif self.bug_count == 1:
            bug_info = 'new report'
        else:
            bug_info = '%s reports' % self.bug_count
        return bug_info


    def GenerateBuildbotLinks(self):
        """Generate a link formatted to meet buildbot expectations.

        If there is a bug associated with this link, report a link to the bug
        and a link to the job logs; otherwise report a link to the job logs.

        @return A generator of links formatted for the buildbot log annotator.
        """
        if self.bug_url:
            yield self._get_link_to_bug()
        yield self._get_link_to_job_logs()


    def _get_link_to_bug(self):
        """Return buildbot link to bug.

        @return A link formatted for the buildbot log annotator.
        """
        info_strings = self._get_info_strings()
        info_strings.append(self._bug_count_text)
        anchor_text = self._format_anchor_text(self._BUG_LINK_PREFIX,
                                               info_strings)
        return annotations.StepLink(anchor_text, self.bug_url)


    def _get_link_to_job_logs(self):
        """Return buildbot link to job logs.

        @return A link formatted for the buildbot log annotator.
        """
        anchor_text = self._format_anchor_text(self._LOG_LINK_PREFIX,
                                               self._get_info_strings())
        return annotations.StepLink(anchor_text, self.url)


    def _get_info_strings(self):
        """Return a list of info strings for _format_anchor_text()."""
        info_strings = []
        if self.retry_count > 0:
            info_strings.append('retry_count: %d' % self.retry_count)
        if self.reason:
            info_strings.append(self.reason)
        return info_strings


    def _format_anchor_text(self, prefix, info_strings):
        """Format anchor text given a prefix and info strings.

        @param prefix        The prefix of the anchor text.
        @param info_strings  Iterable of strings.
        @return A anchor_text with the right prefix and info strings.
        """
        return '[{prefix}]: {anchor}: {info}'.format(
            prefix=prefix,
            anchor=self.anchor.strip(),
            info=', '.join(info_strings))

    @property
    def text_link(self):
        """Link to the job's logs, for consumption by a human.

        @return A link formatted for human readability.
        """
        return '%s %s' % (self.anchor, self.url)

    def GenerateRetryLink(self):
        """Generate a link to the retry dashboard.

        @return A link formatted for the buildbot log annotator.
        """
        if not self.testname or self.testname in self._SKIP_RETRY_DASHBOARD:
            return None
        return annotations.StepLink(
            text='[Flake-Dashboard]: %s' % self.testname,
            url=reporting_utils.link_retry_url(self.testname))

    def GenerateHistoryLink(self):
        """Generate a link to the test history dashboard.

        @return A link formatted for the buildbot log annotator.
        """
        if not self.testname or self.testname in self._SKIP_RETRY_DASHBOARD:
            return None
        return annotations.StepLink(
            text='[Test-History]: %s' % self.testname,
            url=reporting_utils.link_test_history(self.testname))


class Timings(object):
    """Timings for important events during a suite.

    All timestamps are datetime.datetime objects.

    @var suite_job_id: the afe job id of the suite job for which
                       we are recording the timing for.
    @var download_start_time: the time the devserver starts staging
                              the build artifacts. Recorded in create_suite_job.
    @var payload_end_time: the time when the artifacts only necessary to start
                           installsing images onto DUT's are staged.
                           Recorded in create_suite_job.
    @var artifact_end_time: the remaining artifacts are downloaded after we kick
                            off the reimaging job, at which point we record
                            artifact_end_time. Recorded in dynamic_suite.py.
    @var suite_start_time: the time the suite started.
    @var tests_start_time: the time the first test started running.
    @var tests_end_time: the time the last test finished running.
    """

    def __init__(self, suite_job_id):
        self.suite_job_id = suite_job_id
        # Timings related to staging artifacts on devserver.
        self.download_start_time = None
        self.payload_end_time = None
        self.artifact_end_time = None

        # The test_start_time, but taken off the view that corresponds to the
        # suite instead of an individual test.
        self.suite_start_time = None

        # Earliest and Latest tests in the set of TestViews passed to us.
        self.tests_start_time = None
        self.tests_end_time = None


    def RecordTiming(self, view):
        """Given a test report view, extract and record pertinent time info.

        get_detailed_test_views() returns a list of entries that provide
        info about the various parts of a suite run.  This method can take
        any one of these entries and look up timestamp info we might want
        and record it.

        If timestamps are unavailable, datetime.datetime.min/max will be used.

        @param view: A TestView object.
        """
        start_candidate = datetime.min
        end_candidate = datetime.max
        if view['test_started_time']:
            start_candidate = time_utils.time_string_to_datetime(
                    view['test_started_time'])
        if view['test_finished_time']:
            end_candidate = time_utils.time_string_to_datetime(
                    view['test_finished_time'])

        if view.get_testname() == TestView.SUITE_JOB:
            self.suite_start_time = start_candidate
        else:
            self._UpdateFirstTestStartTime(start_candidate)
            self._UpdateLastTestEndTime(end_candidate)
        if view['afe_job_id'] == self.suite_job_id and 'job_keyvals' in view:
            keyvals = view['job_keyvals']
            self.download_start_time = time_utils.time_string_to_datetime(
                    keyvals.get(constants.DOWNLOAD_STARTED_TIME),
                    handle_type_error=True)

            self.payload_end_time = time_utils.time_string_to_datetime(
                    keyvals.get(constants.PAYLOAD_FINISHED_TIME),
                    handle_type_error=True)

            self.artifact_end_time = time_utils.time_string_to_datetime(
                    keyvals.get(constants.ARTIFACT_FINISHED_TIME),
                    handle_type_error=True)


    def _UpdateFirstTestStartTime(self, candidate):
        """Update self.tests_start_time, iff candidate is an earlier time.

        @param candidate: a datetime.datetime object.
        """
        if not self.tests_start_time or candidate < self.tests_start_time:
            self.tests_start_time = candidate


    def _UpdateLastTestEndTime(self, candidate):
        """Update self.tests_end_time, iff candidate is a later time.

        @param candidate: a datetime.datetime object.
        """
        if not self.tests_end_time or candidate > self.tests_end_time:
            self.tests_end_time = candidate


    def __str__(self):
        return ('\n'
                'Suite timings:\n'
                'Downloads started at %s\n'
                'Payload downloads ended at %s\n'
                'Suite started at %s\n'
                'Artifact downloads ended (at latest) at %s\n'
                'Testing started at %s\n'
                'Testing ended at %s\n' % (self.download_start_time,
                                           self.payload_end_time,
                                           self.suite_start_time,
                                           self.artifact_end_time,
                                           self.tests_start_time,
                                           self.tests_end_time))


def instance_for_pool(pool_name):
    """
    Return the hostname of the server that should be used to service a suite
    for the specified pool.

    @param pool_name: The pool (without 'pool:' to schedule the suite against.
    @return: The correct host that should be used to service this suite run.
    """
    return CONFIG.get_config_value(
            'POOL_INSTANCE_SHARDING', pool_name,
            default=_DEFAULT_AUTOTEST_INSTANCE)


class TestView(object):
    """Represents a test view and provides a set of helper functions."""


    SUITE_JOB = 'Suite job'


    def __init__(self, view, afe_job, suite_name, build, user,
                 solo_test_run=False):
        """Init a TestView object representing a tko test view.

        @param view: A dictionary representing a tko test view.
        @param afe_job: An instance of frontend.afe.models.Job
                        representing the job that kicked off the test.
        @param suite_name: The name of the suite
                           that the test belongs to.
        @param build: The build for which the test is run.
        @param user: The user for which the test is run.
        @param solo_test_run: This is a solo test run not part of a suite.
        """
        self.view = view
        self.afe_job = afe_job
        self.suite_name = suite_name
        self.build = build
        self.is_suite_view = afe_job.parent_job is None and not solo_test_run
        # This is the test name that will be shown in the output.
        self.testname = None
        self.user = user

        # The case that a job was aborted before it got a chance to run
        # usually indicates suite has timed out (unless aborted by user).
        # In this case, the abort reason will be None.
        # Update the reason with proper information.
        if (self.is_relevant_suite_view() and
                not self.get_testname() == self.SUITE_JOB and
                self.view['status'] == 'ABORT' and
                not self.view['reason']):
            self.view['reason'] = 'Timed out, did not run.'


    def __getitem__(self, key):
        """Overload __getitem__ so that we can still use []

        @param key: A key of the tko test view.

        @returns: The value of an attribute in the view.

        """
        return self.view[key]


    def __iter__(self):
        """Overload __iter__ so that it supports 'in' operator."""
        return iter(self.view)


    def get_testname(self):
        """Get test name that should be shown in the output.

        Formalize the test_name we got from the test view.

        Remove 'build/suite' prefix if any.

        If one runs a test in control file via the following code,
           job.runtest('my_Test', tag='tag')
        for most of the cases, view['test_name'] would look like 'my_Test.tag'.
        If this is the case, this method will just return the original
        test name, i.e. 'my_Test.tag'.

        There are four special cases.
        1) A test view is for the suite job's SERVER_JOB.
           In this case, this method will return 'Suite job'.

        2) A test view is of a child job or a solo test run not part of a
           suite, and for a SERVER_JOB or CLIENT_JOB.
           In this case, we will take the job name, remove the build/suite
           prefix from the job name, and append the rest to 'SERVER_JOB'
           or 'CLIENT_JOB' as a prefix. So the names returned by this
           method will look like:
             'dummy_Pass_SERVER_JOB'
             'dummy_Fail_SERVER_JOB'

        3) A test view is of a suite job and its status is ABORT.
           In this case, the view['test_name'] is the child job's name.
           For instance,
             'lumpy-release/R35-5712.0.0/dummy/dummy_Pass'
             'lumpy-release/R35-5712.0.0/dummy/dummy_Fail'
           The above names will be converted to the following:
             'dummy_Pass'
             'dummy_Fail'

        4) A test view's status is of a suite job and its status is TEST_NA.
           In this case, the view['test_name'] is the NAME field of the control
           file. For instance,
             'dummy_Pass'
             'dummy_Fail'
           This method will not modify these names.

        @returns: Test name after normalization.

        """
        if self.testname is not None:
            return self.testname

        if (self.is_suite_view and
                self.view['test_name'].startswith('SERVER_JOB')):
            # Rename suite job's SERVER_JOB to 'Suite job'.
            self.testname = self.SUITE_JOB
            return self.testname

        if (self.view['test_name'].startswith('SERVER_JOB') or
                self.view['test_name'].startswith('CLIENT_JOB')):
            # Append job name as a prefix for SERVER_JOB and CLIENT_JOB
            testname= '%s_%s' % (self.view['job_name'], self.view['test_name'])
        else:
            testname = self.view['test_name']
        # Remove the build and suite name from testname if any.
        self.testname = tools.get_test_name(
                self.build, self.suite_name, testname)
        return self.testname


    def is_relevant_suite_view(self):
        """Checks whether this is a suite view we should care about.

        @returns: True if it is relevant. False otherwise.
        """
        return (self.get_testname() == self.SUITE_JOB or
                (self.is_suite_view and
                    not self.view['test_name'].startswith('CLIENT_JOB') and
                    not self.view['subdir']))


    def is_test(self):
        """Return whether the view is for an actual test.

        @returns True if the view is for an actual test.
                 False if the view is for SERVER_JOB or CLIENT_JOB.

        """
        return not (self.view['test_name'].startswith('SERVER_JOB') or
                self.view['test_name'].startswith('CLIENT_JOB'))


    def is_retry(self):
        """Check whether the view is for a retry.

        @returns: True, if the view is for a retry; False otherwise.

        """
        return self.view['job_keyvals'].get('retry_original_job_id') is not None


    def hit_timeout(self):
        """Check whether the corresponding job has hit its own timeout.

        Note this method should not be called for those test views
        that belongs to a suite job and are determined as irrelevant
        by is_relevant_suite_view.  This is because they are associated
        to the suite job, whose job start/finished time make no sense
        to an irrelevant test view.

        @returns: True if the corresponding afe job has hit timeout.
                  False otherwise.
        """
        if (self.is_relevant_suite_view() and
                self.get_testname() != self.SUITE_JOB):
            # Any relevant suite test view except SUITE_JOB
            # did not hit its own timeout because it was not ever run.
            return False
        start = (datetime.strptime(
                self.view['job_started_time'], time_utils.TIME_FMT)
                if self.view['job_started_time'] else None)
        end = (datetime.strptime(
                self.view['job_finished_time'], time_utils.TIME_FMT)
                if self.view['job_finished_time'] else None)
        if not start or not end:
            return False
        else:
            return ((end - start).total_seconds()/60.0
                        > self.afe_job.max_runtime_mins)


    def is_aborted(self):
        """Check if the view was aborted.

        For suite job and child job test views, we check job keyval
        'aborted_by' and test status.

        For relevant suite job test views, we only check test status
        because the suite job keyval won't make sense to individual
        test views.

        @returns: True if the test was as aborted, False otherwise.

        """

        if (self.is_relevant_suite_view() and
                self.get_testname() != self.SUITE_JOB):
            return self.view['status'] == 'ABORT'
        else:
            return (bool(self.view['job_keyvals'].get('aborted_by')) and
                    self.view['status'] in ['ABORT', 'RUNNING'])


    def is_in_fail_status(self):
        """Check if the given test's status corresponds to a failure.

        @returns: True if the test's status is FAIL or ERROR. False otherwise.

        """
        # All the statuses tests can have when they fail.
        return self.view['status'] in ['FAIL', 'ERROR', 'ABORT']


    def is_provision(self):
        """Check whether this is a provision test."""
        return self.get_testname() == 'provision'


    def get_buildbot_link_reason(self):
        """Generate the buildbot link reason for the test.

        @returns: A string representing the reason.

        """
        return ('%s: %s' % (self.view['status'], self.view['reason'])
                if self.view['reason'] else self.view['status'])


    def get_job_id_owner_str(self):
        """Generate the job_id_owner string for a test.

        @returns: A string which looks like 135036-username

        """
        return '%s-%s' % (self.view['afe_job_id'], self.user)


    def get_bug_info(self, suite_job_keyvals):
        """Get the bug info from suite_job_keyvals.

        If a bug has been filed for the test, its bug info (bug id and counts)
        will be stored in the suite job's keyvals. This method attempts to
        retrieve bug info of the test from |suite_job_keyvals|. It will return
        None if no bug info is found. No need to check bug info if the view is
        SUITE_JOB.

        @param suite_job_keyvals: The job keyval dictionary of the suite job.
                All the bug info about child jobs are stored in
                suite job's keyvals.

        @returns: None if there is no bug info, or a pair with the
                  id of the bug, and the count of the number of
                  times the bug has been seen.

        """
        if self.get_testname() == self.SUITE_JOB:
            return None
        if (self.view['test_name'].startswith('SERVER_JOB') or
                self.view['test_name'].startswith('CLIENT_JOB')):
            # Append job name as a prefix for SERVER_JOB and CLIENT_JOB
            testname= '%s_%s' % (self.view['job_name'], self.view['test_name'])
        else:
            testname = self.view['test_name']

        return tools.get_test_failure_bug_info(
                suite_job_keyvals, self.view['afe_job_id'],
                testname)


    def should_display_buildbot_link(self):
        """Check whether a buildbot link should show for this view.

        For suite job view, show buildbot link if it fails.
        For normal test view,
            show buildbot link if it is a retry
            show buildbot link if it hits its own timeout.
            show buildbot link if it fails. This doesn't
            include the case where it was aborted but has
            not hit its own timeout (most likely it was aborted because
            suite has timed out).

        @returns: True if we should show the buildbot link.
                  False otherwise.
        """
        is_bad_status = (self.view['status'] != 'GOOD' and
                         self.view['status'] != 'TEST_NA')
        if self.get_testname() == self.SUITE_JOB:
            return is_bad_status
        else:
            if self.is_retry():
                return True
            if is_bad_status:
                return not self.is_aborted() or self.hit_timeout()


    def get_control_file_attributes(self):
        """Get the attributes from the control file of the test.

        @returns: A list of test attribute or None.
        """
        control_file = self.afe_job.control_file
        attributes = None
        if control_file:
            cd = control_data.parse_control_string(control_file)
            attributes = list(cd.attributes)
        return attributes


    def override_afe_job_id(self, afe_job_id):
        """Overrides the AFE job id for the test.

        @param afe_job_id: The new AFE job id to use.
        """
        self.view['afe_job_id'] = afe_job_id


def log_buildbot_links(log_func, links):
    """Output buildbot links to log.

    @param log_func: Logging function to use.
    @param links: Iterable of LogLink instances.
    """
    for link in links:
        for generated_link in link.GenerateBuildbotLinks():
            log_func(generated_link)
        retry_link = link.GenerateRetryLink()
        if retry_link:
            log_func(retry_link)
        history_link = link.GenerateHistoryLink()
        if history_link:
            log_func(history_link)


class _ReturnCodeComputer(object):
    """This is responsible for returning the _ReturnResult for a suite."""

    def __call__(self, test_views):
        """Compute the exit code based on test results."""
        result = _RETURN_RESULTS['ok']

        for v in test_views:
            if v.get_testname() == TestView.SUITE_JOB:
                result |= self._get_suite_result(v)
            else:
                result |= self._get_test_result(v)
        return result

    def _get_suite_result(self, test_view):
        """Return the _ReturnResult for the given suite job."""
        # The order of checking each case is important.
        if test_view.is_aborted() and test_view.hit_timeout():
            return _RETURN_RESULTS['suite_timeout']
        elif test_view.is_in_fail_status():
            return _RETURN_RESULTS['suite_failed']
        elif test_view['status'] == 'WARN':
            return _RETURN_RESULTS['suite_warning']
        else:
            return _RETURN_RESULTS['ok']

    def _get_test_result(self, test_view):
        """Return the _ReturnResult for the given test job."""
        # The order of checking each case is important.
        if test_view.is_aborted() and test_view.is_relevant_suite_view():
            # The test was aborted before started
            # This gurantees that the suite has timed out.
            return _RETURN_RESULTS['test_aborted_prestart']
        elif test_view.is_aborted() and not test_view.hit_timeout():
            # The test was aborted, but
            # not due to a timeout. This is most likely
            # because the suite has timed out, but may
            # also because it was aborted by the user.
            # Since suite timing out is determined by checking
            # the suite job view, we simply ignore this view here.
            return _RETURN_RESULTS['test_aborted_mystery']
        elif test_view.is_in_fail_status():  # The test job failed
            if test_view.is_provision():
                return _RETURN_RESULTS['provision_failed']
            else:
                return _RETURN_RESULTS['test_failure']
        elif test_view['status'] == 'WARN':
            return _RETURN_RESULTS['test_warning']
        elif test_view.is_retry():
            # The test is a passing retry.
            return _RETURN_RESULTS['test_retry']
        else:
            return _RETURN_RESULTS['ok']


class _ProvisionReturnCodeComputer(_ReturnCodeComputer):
    """This is used for returning the _ReturnResult for provision suites."""

    def __init__(self, num_required):
        """Initialize instance.

        num_required is the number of passing provision jobs needed.
        """
        super(_ProvisionReturnCodeComputer, self).__init__()
        self._num_required = num_required
        self._num_successful = 0

    def __call__(self, test_views):
        result = super(_ProvisionReturnCodeComputer, self).__call__(test_views)
        if self._num_successful >= self._num_required:
            logging.info('Return result upgraded from %r'
                         ' due to enough ok provisions',
                         result)
            return _RETURN_RESULTS['ok']
        else:
            return result

    def _get_test_result(self, test_view):
        result = (super(_ProvisionReturnCodeComputer, self)
                  ._get_test_result(test_view))
        if result in {_RETURN_RESULTS[s] for s in ('ok', 'test_retry')}:
            self._num_successful += 1
        return result


class ResultCollector(object):
    """Collect test results of a suite or a single test run.

    Once a suite job has finished, use this class to collect test results.
    `run` is the core method that is to be called first. Then the caller
    could retrieve information like return code, return message, is_aborted,
    and timings by accessing the collector's public attributes. And output
    the test results and links by calling the 'output_*' methods.

    Here is a overview of what `run` method does.

    1) Collect the suite job's results from tko_test_view_2.
    For the suite job, we only pull test views without a 'subdir'.
    A NULL subdir indicates that the test was _not_ executed. This could be
    that no child job was scheduled for this test or the child job got
    aborted before starts running.
    (Note 'SERVER_JOB'/'CLIENT_JOB' are handled specially)

    2) Collect the child jobs' results from tko_test_view_2.
    For child jobs, we pull all the test views associated with them.
    (Note 'SERVER_JOB'/'CLIENT_JOB' are handled specially)

    3) Generate web and buildbot links.
    4) Compute timings of the suite run.
    5) Compute the return code based on test results.

    @var _instance_server: The hostname of the server that is used
                           to service the suite.
    @var _afe: The afe rpc client.
    @var _tko: The tko rpc client.
    @var _build: The build for which the suite is run,
                 e.g. 'lumpy-release/R35-5712.0.0'
    @var _board: The target board for which the suite is run,
                 e.g., 'lumpy', 'link'.
    @var _suite_name: The suite name, e.g. 'bvt', 'dummy'.
    @var _suite_job_id: The job id of the suite for which we are going to
                        collect results.
    @var _original_suite_name: The suite name we record timing would be
                               different from _suite_name when running
                               suite_attr_wrapper.
    @var _return_code_function: Called to return what the overall result of
                                the suite is.
    @var _suite_views: A list of TestView objects, representing relevant
                       test views of the suite job.
    @var _child_views: A list of TestView objects, representing test views
                       of the child jobs.
    @var _test_views: A list of TestView objects, representing all test views
                      from _suite_views and _child_views.
    @var _web_links: A list of web links pointing to the results of jobs.
    @var buildbot_links: A list of buildbot links for non-passing tests.
    @var _solo_test_run: True if this is a single test run.
    @var return_result: The _ReturnResult of the suite run.
    @var is_aborted: Whether the suite was aborted or not.
                     True, False or None (aborting status is unknown yet)
    @var timings: A Timing object that records the suite's timings.

    """


    def __init__(self, instance_server, afe, tko, build, board,
                 suite_name, suite_job_id,
                 return_code_function,
                 original_suite_name=None,
                 user=None, solo_test_run=False):
        self._instance_server = instance_server
        self._afe = afe
        self._tko = tko
        self._build = build
        self._board = board
        self._suite_name = suite_name
        self._suite_job_id = suite_job_id
        self._original_suite_name = original_suite_name or suite_name
        self._return_code_function = return_code_function
        self._suite_views = []
        self._child_views = []
        self._test_views = []
        self._retry_counts = {}
        self._missing_results = {}
        self._web_links = []
        self.buildbot_links = []
        self._num_child_jobs = 0
        self.return_result = None
        self.is_aborted = None
        self.timings = None
        self._user = user or getpass.getuser()
        self._solo_test_run = solo_test_run


    def _fetch_relevant_test_views_of_suite(self):
        """Fetch relevant test views of the suite job.

        For the suite job, there will be a test view for SERVER_JOB, and views
        for results of its child jobs. For example, assume we've created
        a suite job (afe_job_id: 40) that runs dummy_Pass, dummy_Fail,
        dummy_Pass.bluetooth. Assume dummy_Pass was aborted before running while
        dummy_Path.bluetooth got TEST_NA as no duts have bluetooth.
        So the suite job's test views would look like
        _____________________________________________________________________
        test_idx| job_idx|test_name           |subdir      |afe_job_id|status
        10      | 1000   |SERVER_JOB          |----        |40        |GOOD
        11      | 1000   |dummy_Pass          |NULL        |40        |ABORT
        12      | 1000   |dummy_Fail.Fail     |41-onwer/...|40        |FAIL
        13      | 1000   |dummy_Fail.Error    |42-owner/...|40        |ERROR
        14      | 1000   |dummy_Pass.bluetooth|NULL        |40        |TEST_NA

        For a suite job, we only care about
        a) The test view for the suite job's SERVER_JOB
        b) The test views for real tests without a subdir. A NULL subdir
           indicates that a test didn't get executed.
        So, for the above example, we only keep test views whose test_idxs
        are 10, 11, 14.

        @returns: A list of TestView objects, representing relevant
                  test views of the suite job.

        """
        suite_job = self._afe.get_jobs(id=self._suite_job_id)[0]
        views = self._tko.run(call='get_detailed_test_views',
                              afe_job_id=self._suite_job_id)
        relevant_views = []
        for v in views:
            v = TestView(v, suite_job, self._suite_name, self._build, self._user,
                         solo_test_run=self._solo_test_run)
            if v.is_relevant_suite_view():
                # If the test doesn't have results in TKO and is being
                # displayed in the suite view instead of the child view,
                # then afe_job_id is incorrect and from the suite.
                # Override it based on the AFE job id which was missing
                # results.
                # TODO: This is likely inaccurate if a test has multiple
                # tries which all fail TKO parse stage.
                if v['test_name'] in self._missing_results:
                    v.override_afe_job_id(
                            self._missing_results[v['test_name']][0])
                relevant_views.append(v)
        return relevant_views


    def _compute_retry_count(self, view):
        """Return how many times the test has been retried.

        @param view: A TestView instance.
        @returns: An int value indicating the retry count.

        """
        old_job = view['job_keyvals'].get('retry_original_job_id')
        count = 0
        while old_job:
            count += 1
            views = self._tko.run(
                call='get_detailed_test_views', afe_job_id=old_job)
            old_job = (views[0]['job_keyvals'].get('retry_original_job_id')
                       if views else None)
        return count


    def _fetch_test_views_of_child_jobs(self, jobs=None):
        """Fetch test views of child jobs.

        @returns: A tuple (child_views, retry_counts, missing_results)
                  child_views is list of TestView objects, representing
                  all valid views.
                  retry_counts is a dictionary that maps test_idx to retry
                  counts. It only stores retry counts that are greater than 0.
                  missing_results is a dictionary that maps test names to
                  lists of job ids.

        """
        child_views = []
        retry_counts = {}
        missing_results = {}
        child_jobs = jobs or self._afe.get_jobs(parent_job_id=self._suite_job_id)
        if child_jobs:
            self._num_child_jobs = len(child_jobs)
        for job in child_jobs:
            views = [TestView(v, job, self._suite_name, self._build, self._user)
                     for v in self._tko.run(
                         call='get_detailed_test_views', afe_job_id=job.id,
                         invalid=0)]
            if len(views) == 0:
                missing_results.setdefault(job.name, []).append(job.id)
            contains_test_failure = any(
                    v.is_test() and v['status'] != 'GOOD' for v in views)
            for v in views:
                if (v.is_test() or
                        v['status'] != 'GOOD' and not contains_test_failure):
                    # For normal test view, just keep it.
                    # For SERVER_JOB or CLIENT_JOB, only keep it
                    # if it fails and no other test failure.
                    child_views.append(v)
                    retry_count = self._compute_retry_count(v)
                    if retry_count > 0:
                        retry_counts[v['test_idx']] = retry_count
        return child_views, retry_counts, missing_results


    def _generate_web_and_buildbot_links(self):
        """Generate web links and buildbot links."""
        # TODO(fdeng): If a job was aborted before it reaches Running
        # state, we read the test view from the suite job
        # and thus this method generates a link pointing to the
        # suite job's page for the aborted job. Need a fix.
        self._web_links = []
        self.buildbot_links = []

        # Bug info are stored in the suite job's keyvals.
        if self._solo_test_run:
            suite_job_keyvals = {}
        elif not self._suite_views:
            suite_job_keyvals = {}
        else:
            suite_job_keyvals = self._suite_views[0]['job_keyvals']

        for v in self._test_views:
            retry_count = self._retry_counts.get(v['test_idx'], 0)
            bug_info = v.get_bug_info(suite_job_keyvals)
            job_id_owner = v.get_job_id_owner_str()
            link = LogLink(
                    anchor=v.get_testname(),
                    server=self._instance_server,
                    job_string=job_id_owner,
                    bug_info=bug_info, retry_count=retry_count,
                    testname=v.get_testname(),
                    sponge_url=suite_job_keyvals.get('sponge_url'))
            self._web_links.append(link)

            if v.should_display_buildbot_link():
                link.reason = v.get_buildbot_link_reason()
                self.buildbot_links.append(link)


    def _record_timings(self):
        """Record suite timings."""
        self.timings = Timings(self._suite_job_id)
        for v in self._test_views:
            self.timings.RecordTiming(v)


    def _compute_return_code(self):
        """Compute the exit code based on test results."""
        self.return_result = self._return_code_function(self._test_views)


    def _make_test_results(self):
        """Make TestResults for collected tests.

        @returns: List of TestResult instances.
        """
        test_results = []
        for test_view in self._test_views:
            test_result = TestResult(
                test_view=test_view,
                retry_count=self._retry_counts.get(test_view['test_idx'], 0))
            test_results.append(test_result)
        return test_results


    def output_results(self):
        """Output test results, timings and web links."""
        # Output test results
        test_results = self._make_test_results()
        if len(test_results) == 0:
            max_name_length = 0
        else:
            max_name_length = max(len(t.name) for t in test_results)
        for test_result in test_results:
            test_result.log_using(logging.info, max_name_length + 3)
        # Output suite timings
        logging.info(self.timings)
        # Output links to test logs
        logging.info('\nLinks to test logs:')
        for link in self._web_links:
            logging.info(link.text_link)
        logging.info('\n')


    def get_results_dict(self):
        """Write test results, timings and web links into a dict.

        @returns: A dict of results in the format like:
                  {
                  'tests': {
                        'test_1': {'status': 'PASSED', 'attributes': [1,2], ...}
                        'test_2': {'status': 'FAILED', 'attributes': [1],...}
                  }
                  'suite_timings': {
                        'download_start': '1998-07-17 00:00:00',
                        'payload_download_end': '1998-07-17 00:00:05',
                        ...
                  }
                  }
        """
        output_dict = {}
        tests_dict = output_dict.setdefault('tests', {})
        for v in self._test_views:
            test_name = v.get_testname()
            test_info = tests_dict.setdefault(test_name, {})
            test_info.update({
                'status': v['status'],
                'attributes': v.get_control_file_attributes() or list(),
                'reason': v['reason'],
                'retry_count': self._retry_counts.get(v['test_idx'], 0),
                })
            # For aborted test, the control file will not be parsed and thus
            # fail to get the attributes info. Therefore, the subsystems the
            # abort test testing will be missing. For this case, we will assume
            # the aborted test will test all subsystems, set subsystem:default.
            if (test_info['status'] == 'ABORT' and
                not any('subsystem:' in a for a in test_info['attributes'])):
                test_info['attributes'].append('subsystem:default')

        # Write the links to test logs into the |tests_dict| of |output_dict|.
        # For test whose status is not 'GOOD', the link is also buildbot_link.
        for link in self._web_links:
            test_name = link.anchor.strip()
            test_info = tests_dict.get(test_name)
            if test_info:
                test_info['link_to_logs'] = link.url
                test_info['sponge_url'] = link.sponge_url
                # Write the retry dashboard link into the dict.
                if link in self.buildbot_links and link.testname:
                    test_info['retry_dashboard_link'] \
                        = reporting_utils.link_retry_url(link.testname)
                    # Always write the wmatrix link for compatibility.
                    test_info['wmatrix_link'] \
                        = reporting_utils.link_wmatrix_retry_url(link.testname)
                # Write the bug url into the dict.
                if link.bug_id:
                    test_info['bug_url'] = link.bug_url

        # Write the suite timings into |output_dict|
        timings = self.timings
        if timings is not None:
            time_dict = output_dict.setdefault('suite_timings', {})
            time_dict.update({
                'download_start' : str(timings.download_start_time),
                'payload_download_end' : str(timings.payload_end_time),
                'suite_start' : str(timings.suite_start_time),
                'artifact_download_end' : str(timings.artifact_end_time),
                'tests_start' : str(timings.tests_start_time),
                'tests_end' : str(timings.tests_end_time),
                })

        output_dict['suite_job_id'] = self._suite_job_id

        return output_dict


    def run(self):
        """Collect test results.

        This method goes through the following steps:
            Fetch relevent test views of the suite job.
            Fetch test views of child jobs
            Check whether the suite was aborted.
            Generate links.
            Calculate suite timings.
            Compute return code based on the test result.

        """
        if self._solo_test_run:
            self._test_views, self._retry_counts, self._missing_results = (
                  self._fetch_test_views_of_child_jobs(
                          jobs=self._afe.get_jobs(id=self._suite_job_id)))
        else:
            self._child_views, self._retry_counts, self._missing_results = (
                    self._fetch_test_views_of_child_jobs())
            self._suite_views = self._fetch_relevant_test_views_of_suite()
            self._test_views = self._suite_views + self._child_views
        # For hostless job in Starting status, there is no test view associated.
        # This can happen when a suite job in Starting status is aborted. When
        # the scheduler hits some limit, e.g., max_hostless_jobs_per_drone,
        # max_jobs_started_per_cycle, a suite job can stays in Starting status.
        if not self._test_views:
            self.return_result = _RETURN_RESULTS['test_views_missing']
            return
        self.is_aborted = any([view['job_keyvals'].get('aborted_by')
                               for view in self._suite_views])
        self._generate_web_and_buildbot_links()
        self._record_timings()
        self._compute_return_code()


    def gather_timing_stats(self):
        """Collect timing related statistics."""
        # Record suite runtime in metadata db.
        # Some failure modes can leave times unassigned, report sentinel value
        # in that case.
        runtime_in_secs = -1
        if (self.timings.tests_end_time is not None and
            self.timings.suite_start_time is not None):
            runtime_in_secs = (self.timings.tests_end_time -
                    self.timings.suite_start_time).total_seconds()

        job_overhead.record_suite_runtime(self._suite_job_id, self._suite_name,
                self._board, self._build, self._num_child_jobs, runtime_in_secs)



def _make_builds_from_options(options):
    """Create a dict of builds for creating a suite job.

    The returned dict maps version label prefixes to build names.  Together,
    each key-value pair describes a complete label.

    @param options: SimpleNamespace from argument parsing.

    @return: dict mapping version label prefixes to build names
    """
    builds = {}
    build_prefix = None
    if options.build:
        build_prefix = provision.get_version_label_prefix(options.build)
        builds[build_prefix] = options.build
    if options.cheets_build:
        builds[provision.CROS_ANDROID_VERSION_PREFIX] = options.cheets_build
        if build_prefix == provision.CROS_VERSION_PREFIX:
            builds[build_prefix] += provision.CHEETS_SUFFIX
    if options.firmware_rw_build:
        builds[provision.FW_RW_VERSION_PREFIX] = options.firmware_rw_build
    if options.firmware_ro_build:
        builds[provision.FW_RO_VERSION_PREFIX] = options.firmware_ro_build
    return builds


def _make_child_deps_from_options(options):
    """Creates a list of extra dependencies for child jobs.

    @param options: Parsed arguments to run_suite.

    @returns: A list of label strings if any dependencies should be added. None
            otherwise.
    """
    if not options.model:
        return ()
    return ['model:%s' % options.model]


@retry.retry(error.StageControlFileFailure, timeout_min=10)
def create_suite(afe, options):
    """Create a suite with retries.

    @param afe: The afe object to insert the new suite job into.
    @param options: The options to use in creating the suite.

    @return: The afe_job_id of the new suite job.
    """
    logging.info('%s Submitted create_suite_job rpc',
                 diagnosis_utils.JobTimer.format_time(datetime.now()))

    # TODO(crbug.com/763207): This is to support calling old moblab RPC
    # with ToT code.  This does not need to be supported after M62.
    if options.oldrpc:
        suite_args = options.suite_args
        if 'tests' in suite_args:
            # This is for test_that_wrapper
            suite_args = ' '.join([':lab:'] + suite_args['tests'])
        else:
            # This is for suite_attr_wrapper
            suite_args = repr(suite_args)
        options.suite_args = suite_args

    return afe.run(
        'create_suite_job',
        name=options.name,
        board=options.board,
        builds=_make_builds_from_options(options),
        test_source_build=options.test_source_build,
        check_hosts=not options.no_wait,
        pool=options.pool,
        file_bugs=options.file_bugs,
        priority=options.priority,
        suite_args=options.suite_args,
        wait_for_results=not options.no_wait,
        timeout_mins=options.timeout_mins + options.delay_minutes,
        max_runtime_mins=options.max_runtime_mins + options.delay_minutes,
        job_retry=options.retry,
        max_retries=options.max_retries,
        suite_min_duts=options.suite_min_duts,
        offload_failures_only=options.offload_failures_only,
        run_prod_code=options.run_prod_code,
        delay_minutes=options.delay_minutes,
        job_keyvals=options.job_keyvals,
        test_args=options.test_args,
        child_dependencies=_make_child_deps_from_options(options),
    )


class SuiteResult(namedtuple('SuiteResult', ['return_code', 'output_dict'])):
    """Result of running a suite to return."""

    def __new__(cls, return_code, output_dict=None):
        if output_dict is None:
            output_dict = dict()
        else:
            output_dict = output_dict.copy()
        output_dict['return_code'] = return_code
        return super(SuiteResult, cls).__new__(cls, return_code, output_dict)


def _run_suite(options):
    """
    run_suite script without exception handling.

    @param options: The parsed options.

    @returns: A tuple contains the return_code of run_suite and the dictionary
              of the output.

    """
    # If indicate to use the new style suite control file, convert the args
    if options.use_suite_attr:
        options = change_options_for_suite_attr(options)

    log_name = _get_log_name(options)
    utils.setup_logging(logfile=log_name)

    if not options.bypass_labstatus and not options.web:
        utils.check_lab_status(options.build)

    afe = _create_afe(options)
    instance_server = afe.server

    rpc_helper = diagnosis_utils.RPCHelper(afe)
    is_real_time = True
    if options.mock_job_id:
        job_id = int(options.mock_job_id)
        existing_job = afe.get_jobs(id=job_id, finished=True)
        if existing_job:
            is_real_time = False
        else:
            existing_job = afe.get_jobs(id=job_id)
        if existing_job:
            job_created_on = time_utils.date_string_to_epoch_time(
                    existing_job[0].created_on)
        else:
            raise utils.TestLabException('Failed to retrieve job: %d' % job_id)
    else:
        try:
            rpc_helper.check_dut_availability(options.board, options.pool,
                                              options.minimum_duts,
                                              options.skip_duts_check)
            job_id = create_suite(afe, options)
            job_created_on = time.time()
        except (error.CrosDynamicSuiteException,
                error.RPCException, proxy.JSONRPCException) as e:
            logging.exception('Error Message: %s', e)
            return SuiteResult(RETURN_CODES.INFRA_FAILURE,
                               {'return_message': str(e)})
        except AttributeError as e:
            logging.exception('Error Message: %s', e)
            return SuiteResult(RETURN_CODES.INVALID_OPTIONS)

    job_timer = diagnosis_utils.JobTimer(
            job_created_on, float(options.timeout_mins))
    job_url = reporting_utils.link_job(job_id,
                                       instance_server=instance_server)
    logging.info('%s Created suite job: %s',
                 job_timer.format_time(job_timer.job_created_time),
                 job_url)
    logging.info(annotations.StepLink(
        text='Link to suite',
        url=job_url))

    if options.create_and_return:
        msg = '--create_and_return was specified, terminating now.'
        logging.info(msg)
        return SuiteResult(RETURN_CODES.OK, {'return_message': msg})

    if options.no_wait:
        return _handle_job_nowait(job_id, options, instance_server)
    else:
        return _handle_job_wait(afe, job_id, options, job_timer, is_real_time)


def _get_log_name(options):
    """Return local log file's name.

    @param options:         Parsed options.

    @return log_name, a string file name.
    """
    if options.require_logfile:
        # options.build is verified to exist in verify_options.
        # convert build name from containing / to containing only _.
        log_name = 'run_suite-%s.log' % options.build.replace('/', '_')
        log_dir = os.path.join(common.autotest_dir, 'logs')
        if os.path.exists(log_dir):
            log_name = os.path.join(log_dir, log_name)

        return log_name
    else:
        return None


def _create_afe(options):
    """Return an afe instance based on options.

    @param options          Parsed options.

    @return afe, an AFE instance.
    """
    instance_server = (options.web if options.web else
                       instance_for_pool(options.pool))
    afe = frontend_wrappers.RetryingAFE(server=instance_server,
                                        timeout_min=options.afe_timeout_mins,
                                        delay_sec=options.delay_sec)
    logging.info('Autotest instance created: %s', instance_server)
    return afe


def _handle_job_wait(afe, job_id, options, job_timer, is_real_time):
    """Handle suite job synchronously.

    @param afe              AFE instance.
    @param job_id           Suite job id.
    @param options          Parsed options.
    @param job_timer        JobTimer for suite job.
    @param is_real_time     Whether or not to handle job timeout.

    @return SuiteResult of suite job.
    """
    rpc_helper = diagnosis_utils.RPCHelper(afe)
    instance_server = afe.server
    while not afe.get_jobs(id=job_id, finished=True):
        _poke_buildbot_with_output(afe, job_id, job_timer)
        if job_timer.debug_output_timer.poll():
            logging.info('The suite job has another %s till timeout.',
                         job_timer.timeout_hours - job_timer.elapsed_time())
        time.sleep(10)
    logging.info('%s Suite job is finished.',
                 diagnosis_utils.JobTimer.format_time(datetime.now()))
    # For most cases, ResultCollector should be able to determine whether
    # a suite has timed out by checking information in the test view.
    # However, occationally tko parser may fail on parsing the
    # job_finished time from the job's keyval file. So we add another
    # layer of timeout check in run_suite. We do the check right after
    # the suite finishes to make it as accurate as possible.
    # There is a minor race condition here where we might have aborted
    # for some reason other than a timeout, and the job_timer thinks
    # it's a timeout because of the jitter in waiting for results.
    # The consequence would be that run_suite exits with code
    # SUITE_TIMEOUT while it should  have returned INFRA_FAILURE
    # instead, which should happen very rarely.
    # Note the timeout will have no sense when using -m option.
    is_suite_timeout = job_timer.is_suite_timeout()

    # Extract the original suite name to record timing.
    original_suite_name = get_original_suite_name(options.name,
                                                  options.suite_args)
    # Start collecting test results.
    logging.info('%s Start collecting test results and dump them to json.',
                 diagnosis_utils.JobTimer.format_time(datetime.now()))
    TKO = frontend_wrappers.RetryingTKO(server=instance_server,
                                        timeout_min=options.afe_timeout_mins,
                                        delay_sec=options.delay_sec)
    # TODO(crbug.com/672348): It needs to be possible for provision
    # suite to pass if only a few tests fail.  Otherwise, a single
    # failing test will be reported as failure even if the suite reports
    # success.
    if options.name == _PROVISION_SUITE:
        # TODO(crbug.com/672348): Creating the suite job requires that
        # suite_args contains num_required.
        return_code_function = _ProvisionReturnCodeComputer(
            num_required=options.suite_args['num_required'])
    else:
        return_code_function = _ReturnCodeComputer()
    collector = ResultCollector(instance_server=instance_server,
                                afe=afe, tko=TKO, build=options.build,
                                board=options.board,
                                suite_name=options.name,
                                suite_job_id=job_id,
                                return_code_function=return_code_function,
                                original_suite_name=original_suite_name)
    collector.run()
    # Dump test outputs into json.
    output_dict = collector.get_results_dict()
    output_dict['autotest_instance'] = instance_server
    if not options.json_dump:
        collector.output_results()
    result = collector.return_result
    if is_real_time:
        # Do not record stats if the suite was aborted (either by a user
        # or through the golo rpc).
        # Also do not record stats if is_aborted is None, indicating
        # aborting status is unknown yet.
        if collector.is_aborted == False:
            logging.info('%s Gathering timing stats for the suite job.',
                         diagnosis_utils.JobTimer.format_time(datetime.now()))
            collector.gather_timing_stats()

        if collector.is_aborted == True and is_suite_timeout:
            # There are two possible cases when a suite times out.
            # 1. the suite job was aborted due to timing out
            # 2. the suite job succeeded, but some child jobs
            #    were already aborted before the suite job exited.
            # The case 2 was handled by ResultCollector,
            # here we handle case 1.
            result |= _RETURN_RESULTS['suite_timeout']
        logging.info('\n %s Attempting to display pool info: %s',
                     diagnosis_utils.JobTimer.format_time(datetime.now()),
                     options.pool)
        try:
            # Add some jitter to make up for any latency in
            # aborting the suite or checking for results.
            cutoff = job_timer.timeout_hours + timedelta(hours=0.3)
            rpc_helper.diagnose_pool(
                    options.board, options.pool, cutoff)
        except proxy.JSONRPCException:
            logging.warning('Unable to display pool info.')

    # And output return message.
    if result.message:
        logging.info('Reason: %s', result.message)

    logging.info('\n %s Output below this line is for buildbot consumption:',
                 diagnosis_utils.JobTimer.format_time(datetime.now()))
    log_buildbot_links(logging.info, collector.buildbot_links)
    return result.suite_result(output_dict)


def _handle_job_nowait(job_id, options, instance_server):
    """Handle suite job asynchronously.

    @param job_id           Suite job id.
    @param options          Parsed options.
    @param instance_server  Autotest instance hostname.

    @return SuiteResult of suite job.
    """
    logging.info('Created suite job: %r', job_id)
    link = LogLink(options.name, instance_server,
                   '%s-%s' % (job_id, getpass.getuser()))
    for generate_link in link.GenerateBuildbotLinks():
        logging.info(generate_link)
    logging.info('--no_wait specified; Exiting.')
    return SuiteResult(RETURN_CODES.OK,
                       {'return_message': '--no_wait specified; Exiting.'})


def _should_run(options):
    """Check whether the suite should be run based on lab/job status checking.

    @param options          Parsed options.
    """
    try:
        site_utils.check_lab_status(options.test_source_build)
    except site_utils.TestLabException as ex:
        logging.exception('Lab is closed or build is blocked. Skipping '
                          'suite %s, board %s, build %s:  %s',
                          options.name, options.board,
                          options.test_source_build, str(ex))
        return False

    start_time = str(datetime.now() -
                     timedelta(days=_SEARCH_JOB_MAX_DAYS))
    afe = _create_afe(options)
    afe_job_id = afe.get_jobs(
            name__istartswith=options.test_source_build,
            name__iendswith='control.'+options.name,
            created_on__gte=start_time,
            min_rpc_timeout=_MIN_RPC_TIMEOUT)
    if afe_job_id:
        logging.info('Found duplicate suite %s scheduled in past.',
                     afe_job_id)
        return False

    return True


def _poke_buildbot_with_output(afe, job_id, job_timer):
    """Poke buildbot so it doesn't timeout from silence.

    @param afe              AFE instance.
    @param job_id           Suite job id.
    @param job_timer        JobTimer for suite job.
    """
    rpc_helper = diagnosis_utils.RPCHelper(afe)
    # Note that this call logs output, preventing buildbot's
    # 9000 second silent timeout from kicking in. Let there be no
    # doubt, this is a hack. The timeout is from upstream buildbot and
    # this is the easiest work around.
    if job_timer.first_past_halftime():
        rpc_helper.diagnose_job(job_id, afe.server)



def _run_task(options):
    """Perform this script's function minus setup.

    Boilerplate like argument parsing, logging, output formatting happen
    elsewhere.

    Returns a SuiteResult instance.

    TODO(ayatane): The try/except should be moved into _run_suite().
    Good luck trying to figure out which function calls are supposed to
    raise which of the exceptions.
    """
    try:
        return _run_suite(options)
    except diagnosis_utils.BoardNotAvailableError as e:
        result = SuiteResult(
            RETURN_CODES.BOARD_NOT_AVAILABLE,
            {'return_message': 'Skipping testing: %s' % e.message})
        logging.info(result.output_dict['return_message'])
        return result
    except utils.TestLabException as e:
        result = SuiteResult(
            RETURN_CODES.INFRA_FAILURE,
            {'return_message': 'TestLabException: %s' % e})
        logging.exception(result.output_dict['return_message'])
        return result


class _ExceptionHandler(object):
    """Global exception handler replacement."""

    def __init__(self, dump_json):
        """Initialize instance.

        @param dump_json: Whether to print a JSON dump of the result dict to
                          stdout.
        """
        self._should_dump_json = dump_json

    def __call__(self, exc_type, value, traceback):
        if self._should_dump_json:
            _dump_json({'return_message': ('Unhandled run_suite exception: %s'
                                           % value)})
        sys.exit(RETURN_CODES.INFRA_FAILURE)


def main():
    """Entry point."""
    utils.verify_not_root_user()

    parser = make_parser()
    options = parser.parse_args()
    if options.do_nothing:
        return 0

    sys.exceptionhandler = _ExceptionHandler(dump_json=options.json_dump)
    if options.json_dump:
        logging.disable(logging.CRITICAL)

    options_okay = verify_and_clean_options(options)
    # Set StreamHandler first to capture error messages if suite is not run.
    utils.setup_logging()
    if not options_okay:
        parser.print_help()
        result = SuiteResult(RETURN_CODES.INVALID_OPTIONS)
    elif options.pre_check and not _should_run(options):
        logging.info('Suite %s-%s is terminated: Lab is closed, OR build is '
                     'blocked, OR this suite has already been kicked off '
                     'once in past %d days.',
                     options.test_source_build, options.name,
                     _SEARCH_JOB_MAX_DAYS)
        result = SuiteResult(
            RETURN_CODES.ERROR,
            {'return_message': ("Lab is closed OR other reason"
                                " (see code, it's complicated)")})
    else:
        result = _run_task(options)

    if options.json_dump:
        _dump_json(result.output_dict)

    logging.info('Will return from run_suite with status: %s',
                  RETURN_CODES.get_string(result.return_code))
    return result.return_code


def _dump_json(obj):
    """Write obj JSON to stdout."""
    output_json = json.dumps(obj, sort_keys=True)
    sys.stdout.write('#JSON_START#%s#JSON_END#' % output_json.strip())


if __name__ == "__main__":
    sys.exit(main())
