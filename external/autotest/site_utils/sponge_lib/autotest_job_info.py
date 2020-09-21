# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import socket
import time

from autotest_lib.client.common_lib import base_utils
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import time_utils
from autotest_lib.site_utils import job_directories

CONFIG=global_config.global_config

RETRIEVE_LOGS_CGI = CONFIG.get_config_value(
        'BUG_REPORTING', 'retrieve_logs_cgi', default='')
USE_PROD_SERVER = CONFIG.get_config_value(
        'SERVER', 'use_prod_sponge_server', default=False, type=bool)


class AutotestJobInfo(object):
    """Autotest job info."""

    # Tell the uploader what type of info this object holds.
    tags=['autotest']

    # Version of the data stored.
    version = 2

    def __init__(self, job):
        self._job = job
        self._tasks = list(
                self.create_task_info(test) for test in self._job.tests)

        self.build = job.build
        self.build_version = job.build_version
        self.board = job.board

    @property
    def id(self):
        """The id of the autotest job."""
        return job_directories.get_job_id_or_task_id(self._job.dir)

    @property
    def label(self):
        """The label of the autotest job."""
        return self._job.label

    @property
    def user(self):
        """The user who launched the autotest job."""
        return self._job.user

    @property
    def start_time(self):
        """The utc start time of the autotest job."""
        return self._job.keyval_dict.get('job_started', time.time())

    @property
    def end_time(self):
        """The utc end time of the autotest job."""
        return self._job.keyval_dict.get('job_finished', time.time())

    @property
    def dut(self):
        """The dut for the job."""
        return self._job.machine

    @property
    def drone(self):
        """The drone used to run the job."""
        return self._job.keyval_dict.get('drone', socket.gethostname())

    @property
    def keyvals(self):
        """Keyval dict for this job."""
        return self._job.keyval_dict

    @property
    def tasks(self):
        """All tests that this job ran."""
        return self._tasks

    @property
    def results_dir(self):
        """The directory where job results are stored."""
        return os.path.abspath(self._job.dir)

    @property
    def results_url(self):
        """The url where results are stored."""
        return '%s/results/%s-%s/%s' % (
            RETRIEVE_LOGS_CGI, self.id, self.user, self.dut)

    @property
    def is_official(self):
        """If this is a production result."""
        return USE_PROD_SERVER

    def create_task_info(self, test):
        """Thunk for creating task info.

        @param test: The autotest test.

        @returns The task info.
        """
        logging.info('Using default autotest task info for %s.', test.testname)
        return AutotestTaskInfo(test, self)


class AutotestTaskInfo(object):
    """Info about an autotest test."""

    # Tell the uploader what type of info is kept in this task.
    tags = ['autotest']

    # A list of logs to upload for this task.
    logs = ['debug', 'status.log', 'crash', 'keyval', 'control', 'control.srv']

    # Version of the data stored.
    version = 2

    def __init__(self, test, job):
        """
        @param test: The autotest test to create this task from.
        @param job: The job info that owns this task.
        """
        self._test = test
        self._job = job

        keyvals_file = os.path.join(self.results_dir, 'keyval')
        self.keyvals = base_utils.read_keyval(keyvals_file)

    @property
    def taskname(self):
        """The name of the test."""
        return self._test.testname

    @property
    def status(self):
        """The autotest status of this test."""
        return self._test.status

    @property
    def start_time(self):
        """The utc recorded time of when this test started."""
        return time_utils.to_utc_timestamp(self._test.started_time)

    @property
    def end_time(self):
        """The utc recorded time of when this test ended."""
        return time_utils.to_utc_timestamp(self._test.finished_time)

    @property
    def subdir(self):
        """The sub directory used for this test."""
        return self._test.subdir

    @property
    def attributes(self):
        """Attributes of this task."""
        return getattr(self._test, 'attributes', {})

    @property
    def reason(self):
        """The reason for this tasks status."""
        return getattr(self._test, 'reason', None)

    @property
    def results_dir(self):
        """The full directory where results are stored for this test."""
        if self.subdir == '----' or not self.subdir:
            return self._job.results_dir
        else:
            return os.path.join(self._job.results_dir, self.subdir)

    @property
    def is_test(self):
        """True if this task is an actual test that ran."""
        return self.subdir != '----'
