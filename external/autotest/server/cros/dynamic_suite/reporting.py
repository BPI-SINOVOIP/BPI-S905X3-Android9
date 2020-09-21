# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging

import common

from autotest_lib.client.common_lib import global_config
from autotest_lib.server import site_utils
from autotest_lib.server.cros.dynamic_suite import job_status
from autotest_lib.server.cros.dynamic_suite import reporting_utils
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.site_utils  import gmail_lib

try:
    from chromite.lib import metrics
except ImportError:
    metrics = site_utils.metrics_mock


EMAIL_CREDS_FILE = global_config.global_config.get_config_value(
        'NOTIFICATIONS', 'gmail_api_credentials_test_failure', default=None)


class TestBug(object):
    """
    Wrap up all information needed to make an intelligent report about an
    issue. Each TestBug has a search marker associated with it that can be
    used to find similar reports.
    """

    def __init__(self, build, chrome_version, suite, result):
        """
        @param build: The build type, of the form <board>/<milestone>-<release>.
                      eg: x86-mario-release/R25-4321.0.0
        @param chrome_version: The chrome version associated with the build.
                               eg: 28.0.1498.1
        @param suite: The name of the suite that this test run is a part of.
        @param result: The status of the job associated with this issue.
                       This contains the status, job id, test name, hostname
                       and reason for issue.
        """
        self.build = build
        self.chrome_version = chrome_version
        self.suite = suite
        self.name = tools.get_test_name(build, suite, result.test_name)
        self.reason = result.reason
        # The result_owner is used to find results and logs.
        self.result_owner = result.owner
        self.hostname = result.hostname
        self.job_id = result.id

        # Aborts, server/client job failures or a test failure without a
        # reason field need lab attention. Lab bugs for the aborted case
        # are disabled till crbug.com/188217 is resolved.
        self.lab_error = job_status.is_for_infrastructure_fail(result)

        # The owner is who the bug is assigned to.
        self.owner = ''
        self.cc = []
        self.components = []

        if result.is_warn():
            self.labels = ['Test-Warning']
            self.status = 'Warning'
        else:
            self.labels = []
            self.status = 'Failure'


    def title(self):
        """Combines information about this bug into a title string."""
        return '[%s] %s %s on %s' % (self.suite, self.name,
                                     self.status, self.build)


    def summary(self):
        """Combines information about this bug into a summary string."""

        links = self._get_links_for_failure()
        template = ('This report is automatically generated to track the '
                    'following %(status)s:\n'
                    'Test: %(test)s.\n'
                    'Suite: %(suite)s.\n'
                    'Chrome Version: %(chrome_version)s.\n'
                    'Build: %(build)s.\n\nReason:\n%(reason)s.\n'
                    'build artifacts: %(build_artifacts)s.\n'
                    'results log: %(results_log)s.\n'
                    'status log: %(status_log)s.\n'
                    'job link: %(job)s.\n\n'
                    'You may want to check the test history: '
                    '%(test_history_url)s\n'
                    'You may also want to check the test retry dashboard in '
                    'case this is a flakey test: %(retry_url)s\n')

        specifics = {
            'status': self.status,
            'test': self.name,
            'suite': self.suite,
            'build': self.build,
            'chrome_version': self.chrome_version,
            'reason': self.reason,
            'build_artifacts': links.artifacts,
            'results_log': links.results,
            'status_log': links.status_log,
            'job': links.job,
            'test_history_url': links.test_history_url,
            'retry_url': links.retry_url,
        }

        return template % specifics


    # TO-DO(shuqianz) Fix the dedupe failing issue because reason contains
    # special characters after
    # https://bugs.chromium.org/p/monorail/issues/detail?id=806 being fixed.
    def search_marker(self):
        """Return an Anchor that we can use to dedupe this exact bug."""
        board = ''
        try:
            board = site_utils.ParseBuildName(self.build)[0]
        except site_utils.ParseBuildNameException as e:
            logging.error(str(e))

        # Substitute the board name for a placeholder. We try both build and
        # release board name variants.
        reason = self.reason
        if board:
            for b in (board, board.replace('_', '-')):
                reason = reason.replace(b, 'BOARD_PLACEHOLDER')

        return "%s{%s,%s,%s}" % ('Test%s' % self.status, self.suite,
                                 self.name, reason)


    def _get_links_for_failure(self):
        """Returns a named tuple of links related to this failure."""
        links = collections.namedtuple('links', ('results,'
                                                 'status_log,'
                                                 'artifacts,'
                                                 'job,'
                                                 'test_history_url,'
                                                 'retry_url'))
        return links(reporting_utils.link_result_logs(
                         self.job_id, self.result_owner, self.hostname),
                     reporting_utils.link_status_log(
                         self.job_id, self.result_owner, self.hostname),
                     reporting_utils.link_build_artifacts(self.build),
                     reporting_utils.link_job(self.job_id),
                     reporting_utils.link_test_history(self.name),
                     reporting_utils.link_retry_url(self.name))


ReportResult = collections.namedtuple('ReportResult', ['bug_id', 'update_count'])


def send_email(bug, bug_template):
    """Send email to the owner and cc's to notify the TestBug.

    @param bug: TestBug instance.
    @param bug_template: A template dictionary specifying the default bug
                         filing options for failures in this suite.
    """
    to_set = set(bug.cc) if bug.cc else set()
    if bug.owner:
        to_set.add(bug.owner)
    if bug_template.get('cc'):
        to_set = to_set.union(bug_template.get('cc'))
    if bug_template.get('owner'):
        to_set.add(bug_template.get('owner'))
    recipients = ', '.join(to_set)
    if not recipients:
        logging.warning('No owner/cc found. Will skip sending a mail.')
        return
    success = False
    try:
        gmail_lib.send_email(
            recipients, bug.title(), bug.summary(), retry=False,
            creds_path=site_utils.get_creds_abspath(EMAIL_CREDS_FILE))
        success = True
    finally:
        (metrics.Counter('chromeos/autotest/errors/send_bug_email')
         .increment(fields={'success': success}))
