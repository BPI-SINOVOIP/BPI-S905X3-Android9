# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Library for frontend.afe.models.JobHandoff and job cleanup."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import datetime
import logging
import socket

from lucifer import autotest

logger = logging.getLogger(__name__)


_JOB_GRACE_SECS = 10


def incomplete():
    """Return a QuerySet of incomplete JobHandoffs.

    JobHandoff created within a cutoff period are exempt to allow the
    job the chance to acquire its lease file; otherwise, incomplete jobs
    without an active lease are considered dead.

    @returns: Django QuerySet
    """
    models = autotest.load('frontend.afe.models')
    Q = autotest.deps_load('django.db.models').Q
    # Time ---*---------|---------*-------|--->
    #    incomplete   cutoff   newborn   now
    cutoff = (datetime.datetime.now()
              - datetime.timedelta(seconds=_JOB_GRACE_SECS))
    return (models.JobHandoff.objects
            .filter(completed=False, created__lt=cutoff)
            .filter(Q(drone=socket.gethostname()) | Q(drone=None)))


def clean_up(job_ids):
    """Clean up failed jobs failed in database.

    This brings the database into a clean state, which includes marking
    the job, HQEs, and hosts.
    """
    if not job_ids:
        return
    models = autotest.load('frontend.afe.models')
    logger.info('Cleaning up failed jobs: %r', job_ids)
    hqes = models.HostQueueEntry.objects.filter(job_id__in=job_ids)
    logger.debug('Cleaning up HQEs: %r', hqes.values_list('id', flat=True))
    _clean_up_hqes(hqes)
    host_ids = {id for id in hqes.values_list('host_id', flat=True)
                if id is not None}
    logger.debug('Found Hosts associated with jobs: %r', host_ids)
    _clean_up_hosts(host_ids)


def _clean_up_hqes(hqes):
    models = autotest.load('frontend.afe.models')
    logger.debug('Cleaning up HQEs: %r', hqes.values_list('id', flat=True))
    hqes.update(complete=True,
                active=False,
                status=models.HostQueueEntry.Status.FAILED)
    (hqes.exclude(started_on=None)
     .update(finished_on=datetime.datetime.now()))


def _clean_up_hosts(host_ids):
    models = autotest.load('frontend.afe.models')
    transaction = autotest.deps_load('django.db.transaction')
    with transaction.commit_on_success():
        active_hosts = {
            id for id in (models.HostQueueEntry.objects
                          .filter(active=True, complete=False)
                          .values_list('host_id', flat=True))
            if id is not None}
        logger.debug('Found active Hosts: %r', active_hosts)
        (models.Host.objects
         .filter(id__in=host_ids)
         .exclude(id__in=active_hosts)
         .update(status=None))


def mark_complete(job_ids):
    """Mark the corresponding JobHandoffs as completed."""
    if not job_ids:
        return
    models = autotest.load('frontend.afe.models')
    logger.info('Marking job handoffs complete: %r', job_ids)
    (models.JobHandoff.objects
     .filter(job_id__in=job_ids)
     .update(completed=True))
