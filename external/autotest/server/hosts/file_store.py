# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import errno

import common
from autotest_lib.server.hosts import host_info
from chromite.lib import locking
from chromite.lib import retry_util


_FILE_LOCK_TIMEOUT_SECONDS = 5


class FileStore(host_info.CachingHostInfoStore):
    """A CachingHostInfoStore backed by an on-disk file."""

    def __init__(self, store_file,
                 file_lock_timeout_seconds=_FILE_LOCK_TIMEOUT_SECONDS):
        """
        @param store_file: Absolute path to the backing file to use.
        @param info: Optional HostInfo to initialize the store.  When not None,
                any data in store_file will be overwritten.
        @param file_lock_timeout_seconds: Timeout for aborting the attempt to
                lock the backing file in seconds. Set this to <= 0 to request
                just a single attempt.
        """
        super(FileStore, self).__init__()
        self._store_file = store_file
        self._lock_path = '%s.lock' % store_file

        if file_lock_timeout_seconds <= 0:
            self._lock_max_retry = 0
            self._lock_sleep = 0
        else:
            # A total of 3 attempts at times (0 + sleep + 2*sleep).
            self._lock_max_retry = 2
            self._lock_sleep = file_lock_timeout_seconds / 3.0
        self._lock = locking.FileLock(
                self._lock_path,
                locktype=locking.FLOCK,
                description='Locking FileStore to read/write HostInfo.',
                blocking=False)


    def __str__(self):
        return '%s[%s]' % (type(self).__name__, self._store_file)


    def _refresh_impl(self):
        """See parent class docstring."""
        with self._lock_backing_file():
            return self._refresh_impl_locked()


    def _commit_impl(self, info):
        """See parent class docstring."""
        with self._lock_backing_file():
            return self._commit_impl_locked(info)


    def _refresh_impl_locked(self):
        """Same as _refresh_impl, but assumes relevant files are locked."""
        try:
            with open(self._store_file, 'r') as fp:
                return host_info.json_deserialize(fp)
        except IOError as e:
            if e.errno == errno.ENOENT:
                raise host_info.StoreError(
                        'No backing file. You must commit to the store before '
                        'trying to read a value from it.')
            raise host_info.StoreError('Failed to read backing file (%s) : %r'
                                       % (self._store_file, e))
        except host_info.DeserializationError as e:
            raise host_info.StoreError(
                    'Failed to desrialize backing file %s: %r' %
                    (self._store_file, e))


    def _commit_impl_locked(self, info):
        """Same as _commit_impl, but assumes relevant files are locked."""
        try:
            with open(self._store_file, 'w') as fp:
                host_info.json_serialize(info, fp)
        except IOError as e:
            raise host_info.StoreError('Failed to write backing file (%s) : %r'
                                       % (self._store_file, e))


    @contextlib.contextmanager
    def _lock_backing_file(self):
        """Context to lock the backing store file.

        @raises StoreError if the backing file can not be locked.
        """
        def _retry_locking_failures(exc):
            return isinstance(exc, locking.LockNotAcquiredError)

        try:
            retry_util.GenericRetry(
                    handler=_retry_locking_failures,
                    functor=self._lock.write_lock,
                    max_retry=self._lock_max_retry,
                    sleep=self._lock_sleep)
        # If self._lock fails to write the locking file, it'll leak an OSError
        except (locking.LockNotAcquiredError, OSError) as e:
            raise host_info.StoreError(e)

        with self._lock:
            yield
