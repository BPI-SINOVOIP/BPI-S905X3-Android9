# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import logging
import os
import random
import time


from autotest_lib.client.common_lib import base_job, global_config, log
from autotest_lib.client.common_lib import time_utils

_DEFAULT_POLL_INTERVAL_SECONDS = 30.0

HQE_MAXIMUM_ABORT_RATE_FLOAT = global_config.global_config.get_config_value(
            'SCHEDULER', 'hqe_maximum_abort_rate_float', type=float,
            default=0.5)


def view_is_relevant(view):
    """
    Indicates whether the view of a given test is meaningful or not.

    @param view: a detailed test 'view' from the TKO DB to look at.
    @return True if this is a test result worth looking at further.
    """
    return not view['test_name'].startswith('CLIENT_JOB')


def view_is_for_suite_job(view):
    """
    Indicates whether the given test view is the view of Suite job.

    @param view: a detailed test 'view' from the TKO DB to look at.
    @return True if this is view of suite job.
    """
    return view['test_name'] == 'SERVER_JOB'


def view_is_for_infrastructure_fail(view):
    """
    Indicates whether the given test view is from an infra fail.

    @param view: a detailed test 'view' from the TKO DB to look at.
    @return True if this view indicates an infrastructure-side issue during
                 a test.
    """
    return view['test_name'].endswith('SERVER_JOB')


def is_for_infrastructure_fail(status):
    """
    Indicates whether the given Status is from an infra fail.

    @param status: the Status object to look at.
    @return True if this Status indicates an infrastructure-side issue during
                 a test.
    """
    return view_is_for_infrastructure_fail({'test_name': status.test_name})


def _abort_jobs_if_timedout(afe, jobs, start_time, timeout_mins):
    """
    Abort all of the jobs in jobs if the running time has past the timeout.

    @param afe: an instance of AFE as defined in server/frontend.py.
    @param jobs: an iterable of Running frontend.Jobs
    @param start_time: Time to compare to the current time to see if a timeout
                       has occurred.
    @param timeout_mins: Time in minutes to wait before aborting the jobs we
                         are waiting on.

    @returns True if we there was a timeout, False if not.
    """
    if datetime.datetime.utcnow() < (start_time +
                                     datetime.timedelta(minutes=timeout_mins)):
        return False
    for job in jobs:
        logging.debug('Job: %s has timed out after %s minutes. Aborting job.',
                      job.id, timeout_mins)
        afe.run('abort_host_queue_entries', job=job.id)
    return True


def _collate_aborted(current_value, entry):
    """
    reduce() over a list of HostQueueEntries for a job; True if any aborted.

    Functor that can be reduced()ed over a list of
    HostQueueEntries for a job.  If any were aborted
    (|entry.aborted| exists and is True), then the reduce() will
    return True.

    Ex:
      entries = AFE.run('get_host_queue_entries', job=job.id)
      reduce(_collate_aborted, entries, False)

    @param current_value: the current accumulator (a boolean).
    @param entry: the current entry under consideration.
    @return the value of |entry.aborted| if it exists, False if not.
    """
    return current_value or ('aborted' in entry and entry['aborted'])


def _status_for_test(status):
    """
    Indicates whether the status of a given test is meaningful or not.

    @param status: frontend.TestStatus object to look at.
    @return True if this is a test result worth looking at further.
    """
    return not (status.test_name.startswith('SERVER_JOB') or
                status.test_name.startswith('CLIENT_JOB'))


class JobResultWaiter(object):
    """Class for waiting on job results."""

    def __init__(self, afe, tko):
        """Instantiate class

        @param afe: an instance of AFE as defined in server/frontend.py.
        @param tko: an instance of TKO as defined in server/frontend.py.
        """
        self._afe = afe
        self._tko = tko
        self._job_ids = set()

    def add_job(self, job):
        """Add job to wait on.

        @param job: Job object to get results from, as defined in
                    server/frontend.py
        """
        self.add_jobs((job,))

    def add_jobs(self, jobs):
        """Add job to wait on.

        @param jobs: Iterable of Job object to get results from, as defined in
                     server/frontend.py
        """
        self._job_ids.update(job.id for job in jobs)

    def wait_for_results(self):
        """Wait for jobs to finish and return their results.

        The returned generator blocks until all jobs have finished,
        naturally.

        @yields an iterator of Statuses, one per test.
        """
        while self._job_ids:
            for job in self._get_finished_jobs():
                for result in _yield_job_results(self._afe, self._tko, job):
                    yield result
                self._job_ids.remove(job.id)
            self._sleep()

    def _get_finished_jobs(self):
        # This is an RPC call which serializes to JSON, so we can't pass
        # in sets.
        return self._afe.get_jobs(id__in=list(self._job_ids), finished=True)

    def _sleep(self):
        time.sleep(_DEFAULT_POLL_INTERVAL_SECONDS * (random.random() + 0.5))


def _yield_job_results(afe, tko, job):
    """
    Yields the results of an individual job.

    Yields one Status object per test.

    @param afe: an instance of AFE as defined in server/frontend.py.
    @param tko: an instance of TKO as defined in server/frontend.py.
    @param job: Job object to get results from, as defined in
                server/frontend.py
    @yields an iterator of Statuses, one per test.
    """
    entries = afe.run('get_host_queue_entries', job=job.id)

    # This query uses the job id to search through the tko_test_view_2
    # table, for results of a test with a similar job_tag. The job_tag
    # is used to store results, and takes the form job_id-owner/host.
    # Many times when a job aborts during a test, the job_tag actually
    # exists and the results directory contains valid logs. If the job
    # was aborted prematurely i.e before it had a chance to create the
    # job_tag, this query will return no results. When statuses is not
    # empty it will contain frontend.TestStatus' with fields populated
    # using the results of the db query.
    statuses = tko.get_job_test_statuses_from_db(job.id)
    if not statuses:
        yield Status('ABORT', job.name)

    # We only care about the SERVER and CLIENT job failures when there
    # are no test failures.
    contains_test_failure = any(_status_for_test(s) and s.status != 'GOOD'
                                for s in statuses)
    for s in statuses:
        # TKO parser uniquelly identifies a test run by
        # (test_name, subdir). In dynamic suite, we need to emit
        # a subdir for each status and make sure (test_name, subdir)
        # in the suite job's status log is unique.
        # For non-test status (i.e.SERVER_JOB, CLIENT_JOB),
        # we use 'job_tag' from tko_test_view_2, which looks like
        # '1246-owner/172.22.33.44'
        # For normal test status, we use 'job_tag/subdir'
        # which looks like '1246-owner/172.22.33.44/my_DummyTest.tag.subdir_tag'
        if _status_for_test(s):
            yield Status(s.status, s.test_name, s.reason,
                         s.test_started_time, s.test_finished_time,
                         job.id, job.owner, s.hostname, job.name,
                         subdir=os.path.join(s.job_tag, s.subdir))
        else:
            if s.status != 'GOOD' and not contains_test_failure:
                yield Status(s.status,
                             '%s_%s' % (entries[0]['job']['name'],
                                        s.test_name),
                             s.reason, s.test_started_time,
                             s.test_finished_time, job.id,
                             job.owner, s.hostname, job.name,
                             subdir=s.job_tag)


class Status(object):
    """
    A class representing a test result.

    Stores all pertinent info about a test result and, given a callable
    to use, can record start, result, and end info appropriately.

    @var _status: status code, e.g. 'INFO', 'FAIL', etc.
    @var _test_name: the name of the test whose result this is.
    @var _reason: message explaining failure, if any.
    @var _begin_timestamp: when test started (int, in seconds since the epoch).
    @var _end_timestamp: when test finished (int, in seconds since the epoch).
    @var _id: the ID of the job that generated this Status.
    @var _owner: the owner of the job that generated this Status.

    @var STATUS_MAP: a dict mapping host queue entry status strings to canonical
                     status codes; e.g. 'Aborted' -> 'ABORT'
    """
    _status = None
    _test_name = None
    _reason = None
    _begin_timestamp = None
    _end_timestamp = None

    # Queued status can occur if the try job just aborted due to not completing
    # reimaging for all machines. The Queued corresponds to an 'ABORT'.
    STATUS_MAP = {'Failed': 'FAIL', 'Aborted': 'ABORT', 'Completed': 'GOOD',
                  'Queued' : 'ABORT'}

    class sle(base_job.status_log_entry):
        """
        Thin wrapper around status_log_entry that supports stringification.
        """
        def __str__(self):
            return self.render()

        def __repr__(self):
            return self.render()


    def __init__(self, status, test_name, reason='', begin_time_str=None,
                 end_time_str=None, job_id=None, owner=None, hostname=None,
                 job_name='', subdir=None):
        """
        Constructor

        @param status: status code, e.g. 'INFO', 'FAIL', etc.
        @param test_name: the name of the test whose result this is.
        @param reason: message explaining failure, if any; Optional.
        @param begin_time_str: when test started (in time_utils.TIME_FMT);
                               now() if None or 'None'.
        @param end_time_str: when test finished (in time_utils.TIME_FMT);
                             now() if None or 'None'.
        @param job_id: the ID of the job that generated this Status.
        @param owner: the owner of the job that generated this Status.
        @param hostname: The name of the host the test that generated this
                         result ran on.
        @param job_name: The job name; Contains the test name with/without the
                         experimental prefix, the tag and the build.
        @param subdir: The result directory of the test. It will be recorded
                       as the subdir in the status.log file.
        """
        self._status = status
        self._test_name = test_name
        self._reason = reason
        self._id = job_id
        self._owner = owner
        self._hostname = hostname
        self._job_name = job_name
        self._subdir = subdir
        # Autoserv drops a keyval of the started time which eventually makes its
        # way here.  Therefore, if we have a starting time, we may assume that
        # the test reached Running and actually began execution on a drone.
        self._test_executed = begin_time_str and begin_time_str != 'None'

        if begin_time_str and begin_time_str != 'None':
            self._begin_timestamp = int(time.mktime(
                datetime.datetime.strptime(
                    begin_time_str, time_utils.TIME_FMT).timetuple()))
        else:
            self._begin_timestamp = int(time.time())

        if end_time_str and end_time_str != 'None':
            self._end_timestamp = int(time.mktime(
                datetime.datetime.strptime(
                    end_time_str, time_utils.TIME_FMT).timetuple()))
        else:
            self._end_timestamp = int(time.time())


    def is_good(self):
        """ Returns true if status is good. """
        return self._status == 'GOOD'


    def is_warn(self):
        """ Returns true if status is warn. """
        return self._status == 'WARN'


    def is_testna(self):
        """ Returns true if status is TEST_NA """
        return self._status == 'TEST_NA'


    def is_worse_than(self, candidate):
        """
        Return whether |self| represents a "worse" failure than |candidate|.

        "Worse" is defined the same as it is for log message purposes in
        common_lib/log.py.  We also consider status with a specific error
        message to represent a "worse" failure than one without.

        @param candidate: a Status instance to compare to this one.
        @return True if |self| is "worse" than |candidate|.
        """
        if self._status != candidate._status:
            return (log.job_statuses.index(self._status) <
                    log.job_statuses.index(candidate._status))
        # else, if the statuses are the same...
        if self._reason and not candidate._reason:
            return True
        return False


    def record_start(self, record_entry):
        """
        Use record_entry to log message about start of test.

        @param record_entry: a callable to use for logging.
               prototype:
                   record_entry(base_job.status_log_entry)
        """
        log_entry = Status.sle('START', self._subdir,
                                self._test_name, '',
                                None, self._begin_timestamp)
        record_entry(log_entry, log_in_subdir=False)


    def record_result(self, record_entry):
        """
        Use record_entry to log message about result of test.

        @param record_entry: a callable to use for logging.
               prototype:
                   record_entry(base_job.status_log_entry)
        """
        log_entry = Status.sle(self._status, self._subdir,
                                self._test_name, self._reason, None,
                                self._end_timestamp)
        record_entry(log_entry, log_in_subdir=False)


    def record_end(self, record_entry):
        """
        Use record_entry to log message about end of test.

        @param record_entry: a callable to use for logging.
               prototype:
                   record_entry(base_job.status_log_entry)
        """
        log_entry = Status.sle('END %s' % self._status, self._subdir,
                               self._test_name, '', None, self._end_timestamp)
        record_entry(log_entry, log_in_subdir=False)


    def record_all(self, record_entry):
        """
        Use record_entry to log all messages about test results.

        @param record_entry: a callable to use for logging.
               prototype:
                   record_entry(base_job.status_log_entry)
        """
        self.record_start(record_entry)
        self.record_result(record_entry)
        self.record_end(record_entry)


    def override_status(self, override):
        """
        Override the _status field of this Status.

        @param override: value with which to override _status.
        """
        self._status = override


    @property
    def test_name(self):
        """ Name of the test this status corresponds to. """
        return self._test_name


    @test_name.setter
    def test_name(self, value):
        """
        Test name setter.

        @param value: The test name.
        """
        self._test_name = value


    @property
    def id(self):
        """ Id of the job that corresponds to this status. """
        return self._id


    @property
    def owner(self):
        """ Owner of the job that corresponds to this status. """
        return self._owner


    @property
    def hostname(self):
        """ Host the job corresponding to this status ran on. """
        return self._hostname


    @property
    def reason(self):
        """ Reason the job corresponding to this status failed. """
        return self._reason


    @property
    def test_executed(self):
        """ If the test reached running an autoserv instance or not. """
        return self._test_executed

    @property
    def subdir(self):
        """Subdir of test this status corresponds to."""
        return self._subdir
