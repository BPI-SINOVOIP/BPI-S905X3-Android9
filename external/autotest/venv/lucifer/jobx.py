# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Extra functions for frontend.afe.models.Job objects.

Most of these exist in tightly coupled forms in legacy Autotest code
(e.g., part of methods with completely unrelated names on Task objects
under multiple layers of abstract classes).  These are defined here to
sanely reuse without having to commit to a long refactor of legacy code
that is getting deleted soon.

It's not really a good idea to define these on the Job class either;
they are specialized and the Job class already suffers from method
bloat.
"""


def is_hostless(job):
    """Return True if the job is hostless.

    @param job: frontend.afe.models.Job instance
    """
    return bool(hostnames(job))


def hostnames(job):
    """Return a list of hostnames for a job.

    @param job: frontend.afe.models.Job instance
    """
    hqes = job.hostqueueentry_set.all().prefetch_related('host')
    return [hqe.host.hostname for hqe in hqes if hqe.host is not None]


def is_aborted(job):
    """Return if the job is aborted.

    (This means the job is marked for abortion; the job can still be
    running.)

    @param job: frontend.afe.models.Job instance
    """
    for hqe in job.hostqueueentry_set.all():
        if hqe.aborted:
            return True
    return False
