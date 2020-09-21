# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Job leasing utilities

See infra/lucifer for the implementation of job leasing.

https://chromium.googlesource.com/chromiumos/infra/lucifer

Jobs are leased to processes to own and run.  A process owning a job
obtain a job lease.  Ongoing ownership of the lease is established using
an exclusive fcntl lock on the lease file.

If a lease file is older than a few seconds and is not locked, then its
owning process should be considered crashed.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import contextlib
import errno
import fcntl
import logging
import os
import socket
import time

from scandir import scandir

logger = logging.getLogger(__name__)


@contextlib.contextmanager
def obtain_lease(path):
    """Return a context manager owning a lease file.

    The process that obtains the lease will maintain an exclusive,
    unlimited fcntl lock on the lock file.
    """
    with open(path, 'w') as f:
        fcntl.lockf(f.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        try:
            yield path
        finally:
            os.unlink(path)


def leases_iter(jobdir):
    """Yield Lease instances from jobdir.

    @param jobdir: job lease file directory
    @returns: iterator of Leases
    """
    for entry in scandir(jobdir):
        if _is_lease_entry(entry):
            yield Lease(entry)


class Lease(object):
    "Represents a job lease."

    # Seconds after a lease file's mtime where its owning process is not
    # considered dead.
    _FRESH_LIMIT = 5

    def __init__(self, entry):
        """Initialize instance.

        @param entry: scandir.DirEntry instance
        """
        self._entry = entry

    @property
    def id(self):
        """Return id of leased job."""
        return int(self._entry.name)

    def expired(self):
        """Return True if the lease is expired.

        A lease is considered expired if there is no fcntl lock on it
        and the grace period for the owning process to obtain the lock
        has passed.  The lease is not considered expired if the owning
        process removed the lock file normally, as an expired lease
        indicates that some error has occurred and clean up operations
        are needed.
        """
        try:
            stat_result = self._entry.stat()
        except OSError as e:  # pragma: no cover
            if e.errno == errno.ENOENT:
                return False
            raise
        mtime = stat_result.st_mtime_ns / (10 ** 9)
        if time.time() - mtime < self._FRESH_LIMIT:
            return False
        return not _fcntl_locked(self._entry.path)

    def cleanup(self):
        """Remove the lease file.

        This does not need to be called normally, as the owning process
        should clean up its files.
        """
        try:
            os.unlink(self._entry.path)
        except OSError as e:
            logger.warning('Error removing %s: %s', self._entry.path, e)
        try:
            os.unlink(self._sock_path)
        except OSError as e:
            # This is fine; it means that job_reporter crashed, but
            # lucifer_run_job was able to run its cleanup.
            logger.debug('Error removing %s: %s', self._sock_path, e)

    def abort(self):
        """Abort the job.

        This sends a datagram to the abort socket associated with the
        lease.

        If the socket is closed, either the connect() call or the send()
        call will raise socket.error with ECONNREFUSED.
        """
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
        logger.debug('Connecting to abort socket %s', self._sock_path)
        sock.connect(self._sock_path)
        logger.debug('Sending abort to %s', self._sock_path)
        # The value sent does not matter.
        sent = sock.send('abort')
        # TODO(ayatane): I don't know if it is possible for sent to be 0
        assert sent > 0

    def maybe_abort(self):
        """Abort the job, ignoring errors."""
        try:
            self.abort()
        except socket.error as e:
            logger.debug('Error aborting socket: %s', e)

    @property
    def _sock_path(self):
        """Return the path of the abort socket corresponding to the lease."""
        return self._entry.path + ".sock"


def _is_lease_entry(entry):
    """Return True if the DirEntry is for a lease."""
    return entry.name.isdigit()


def _fcntl_locked(path):
    """Return True if a file is fcntl locked.

    @param path: path to file
    """
    fd = os.open(path, os.O_WRONLY)
    try:
        fcntl.lockf(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except IOError:
        return True
    else:
        return False
    finally:
        os.close(fd)
