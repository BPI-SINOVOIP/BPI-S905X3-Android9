# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Monitor jobs and abort them as necessary.

This daemon does a number of upkeep tasks:

* When a process owning a job crashes, job_aborter will mark the job as
  aborted in the database and clean up its lease files.

* When a job is marked aborted in the database, job_aborter will signal
  the process owning the job to abort.

See also http://goto.google.com/monitor_db_per_job_refactor
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import argparse
import logging
import sys
import time

from lucifer import autotest
from lucifer import handoffs
from lucifer import leasing
from lucifer import loglib

logger = logging.getLogger(__name__)


def main(args):
    """Main function

    @param args: list of command line args
    """

    parser = argparse.ArgumentParser(prog='job_aborter', description=__doc__)
    parser.add_argument('--jobdir', required=True)
    loglib.add_logging_options(parser)
    args = parser.parse_args(args)
    loglib.configure_logging_with_args(parser, args)
    logger.info('Starting with args: %r', args)

    autotest.monkeypatch()
    ts_mon_config = autotest.chromite_load('ts_mon_config')
    with ts_mon_config.SetupTsMonGlobalState('job_aborter'):
        _main_loop(jobdir=args.jobdir)
    assert False  # cannot exit normally


def _main_loop(jobdir):
    transaction = autotest.deps_load('django.db.transaction')

    @transaction.commit_manually
    def flush_transaction():
        """Flush transaction https://stackoverflow.com/questions/3346124/"""
        transaction.commit()

    metrics = _Metrics()
    metrics.send_starting()
    while True:
        logger.debug('Tick')
        metrics.send_tick()
        _main_loop_body(metrics, jobdir)
        flush_transaction()
        time.sleep(20)


def _main_loop_body(metrics, jobdir):
    active_leases = {
            lease.id: lease for lease in leasing.leases_iter(jobdir)
            if not lease.expired()
    }
    _mark_expired_jobs_failed(metrics, active_leases)
    _abort_timed_out_jobs(active_leases)
    _abort_jobs_marked_aborting(active_leases)
    _abort_special_tasks_marked_aborted()
    _clean_up_expired_leases(jobdir)
    # TODO(crbug.com/748234): abort_jobs_past_max_runtime goes into
    # lucifer_run_job


def _mark_expired_jobs_failed(metrics, active_leases):
    """Mark expired jobs failed.

    Expired jobs are jobs that have an incomplete JobHandoff and that do
    not have an active lease.  These jobs have been handed off to a
    job_reporter, but that job_reporter has crashed.  These jobs are
    marked failed in the database.

    @param metrics: _Metrics instance.
    @param active_leases: dict mapping job ids to Leases.
    """
    logger.debug('Looking for expired jobs')
    job_ids = []
    for handoff in handoffs.incomplete():
        logger.debug('Found handoff: %d', handoff.job_id)
        if handoff.job_id not in active_leases:
            logger.debug('Handoff %d is missing active lease', handoff.job_id)
            job_ids.append(handoff.job_id)
    handoffs.clean_up(job_ids)
    handoffs.mark_complete(job_ids)
    metrics.send_expired_jobs(len(job_ids))


def _abort_timed_out_jobs(active_leases):
    """Send abort to timed out jobs.

    @param active_leases: dict mapping job ids to Leases.
    """
    for job in _timed_out_jobs_queryset():
        if job.id in active_leases:
            active_leases[job.id].maybe_abort()


def _abort_jobs_marked_aborting(active_leases):
    """Send abort to jobs marked aborting in Autotest database.

    @param active_leases: dict mapping job ids to Leases.
    """
    for job in _aborting_jobs_queryset():
        if job.id in active_leases:
            active_leases[job.id].maybe_abort()


def _abort_special_tasks_marked_aborted():
    # TODO(crbug.com/748234): Special tasks not implemented yet.  This
    # would abort jobs running on the behalf of special tasks and thus
    # need to check a different database table.
    pass


def _clean_up_expired_leases(jobdir):
    """Clean up files for expired leases.

    We only care about active leases, so we can remove the stale files
    for expired leases.
    """
    for lease in leasing.leases_iter(jobdir):
        if lease.expired():
            lease.cleanup()


def _timed_out_jobs_queryset():
    """Return a QuerySet of timed out Jobs.

    @returns: Django QuerySet
    """
    models = autotest.load('frontend.afe.models')
    return (
            models.Job.objects
            .filter(hostqueueentry__complete=False)
            .extra(where=['created_on + INTERVAL timeout_mins MINUTE < NOW()'])
            .distinct()
    )


def _aborting_jobs_queryset():
    """Return a QuerySet of aborting Jobs.

    @returns: Django QuerySet
    """
    models = autotest.load('frontend.afe.models')
    return (
            models.Job.objects
            .filter(hostqueueentry__aborted=True)
            .filter(hostqueueentry__complete=False)
            .distinct()
    )


class _Metrics(object):

    """Class for sending job_aborter metrics."""

    def __init__(self):
        metrics = autotest.chromite_load('metrics')
        prefix = 'chromeos/lucifer/job_aborter'
        self._starting_m = metrics.Counter(prefix + '/start')
        self._tick_m = metrics.Counter(prefix + '/tick')
        self._expired_m = metrics.Counter(prefix + '/expired_jobs')

    def send_starting(self):
        """Send starting metric."""
        self._starting_m.increment()

    def send_tick(self):
        """Send tick metric."""
        self._tick_m.increment()

    def send_expired_jobs(self, count):
        """Send expired_jobs metric."""
        self._expired_m.increment_by(count)


if __name__ == '__main__':
    main(sys.argv[1:])
