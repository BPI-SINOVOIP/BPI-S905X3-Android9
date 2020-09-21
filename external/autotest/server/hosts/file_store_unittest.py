# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock
import os
import stat
import unittest

import common
from autotest_lib.client.common_lib import autotemp
from autotest_lib.server.hosts import file_store
from autotest_lib.server.hosts import host_info
from chromite.lib import locking

class FileStoreTestCase(unittest.TestCase):
    """Test file_store.FileStore functionality."""

    def setUp(self):
        self._tempdir = autotemp.tempdir(unique_id='file_store_test')
        self.addCleanup(self._tempdir.clean)
        self._store_file = os.path.join(self._tempdir.name, 'store_42')


    def test_commit_refresh_round_trip(self):
        """Refresh-commit cycle from a single store restores HostInfo."""
        info = host_info.HostInfo(labels=['labels'],
                                  attributes={'attrib': 'value'})
        store = file_store.FileStore(self._store_file)
        store.commit(info)
        got = store.get(force_refresh=True)
        self.assertEqual(info, got)


    def test_commit_refresh_separate_stores(self):
        """Refresh-commit cycle from separate stores restores HostInfo."""
        info = host_info.HostInfo(labels=['labels'],
                                  attributes={'attrib': 'value'})
        store = file_store.FileStore(self._store_file)
        store.commit(info)

        read_store = file_store.FileStore(self._store_file)
        got = read_store.get()
        self.assertEqual(info, got)


    def test_empty_store_raises_on_get(self):
        """Refresh from store before commit raises StoreError"""
        store = file_store.FileStore(self._store_file)
        with self.assertRaises(host_info.StoreError):
            store.get()


    def test_commit_blocks_for_locked_file(self):
        """Commit blocks when the backing file is locked.

        This is a greybox test. We artificially lock the backing file.
        This test intentionally uses a real locking.FileLock to ensure that
        locking API is used correctly.
        """
        # file_lock_timeout of 0 forces no retries (speeds up the test)
        store = file_store.FileStore(self._store_file,
                                     file_lock_timeout_seconds=0)
        file_lock = locking.FileLock(store._lock_path,
                                     locktype=locking.FLOCK)
        with file_lock.lock(), self.assertRaises(host_info.StoreError):
                store.commit(host_info.HostInfo())


    def test_refresh_blocks_for_locked_file(self):
        """Refresh blocks when the backing file is locked.

        This is a greybox test. We artificially lock the backing file.
        This test intentionally uses a real locking.FileLock to ensure that
        locking API is used correctly.
        """
        # file_lock_timeout of 0 forces no retries (speeds up the test)
        store = file_store.FileStore(self._store_file,
                                     file_lock_timeout_seconds=0)
        store.commit(host_info.HostInfo())
        store.get(force_refresh=True)
        file_lock = locking.FileLock(store._lock_path,
                                     locktype=locking.FLOCK)
        with file_lock.lock(), self.assertRaises(host_info.StoreError):
                store.get(force_refresh=True)


    def test_commit_to_bad_path_raises(self):
        """Commit to a non-writable path raises StoreError."""
        # file_lock_timeout of 0 forces no retries (speeds up the test)
        store = file_store.FileStore('/rooty/non-writable/path/mostly',
                                     file_lock_timeout_seconds=0)
        with self.assertRaises(host_info.StoreError):
            store.commit(host_info.HostInfo())


    def test_refresh_from_non_existent_path_raises(self):
        """Refresh from a non-existent backing file raises StoreError."""
        # file_lock_timeout of 0 forces no retries (speeds up the test)
        store = file_store.FileStore(self._store_file,
                                     file_lock_timeout_seconds=0)
        store.commit(host_info.HostInfo())
        os.unlink(self._store_file)
        with self.assertRaises(host_info.StoreError):
            store.get(force_refresh=True)


    def test_refresh_from_unreadable_path_raises(self):
        """Refresh from an unreadable backing file raises StoreError."""
        # file_lock_timeout of 0 forces no retries (speeds up the test)
        store = file_store.FileStore(self._store_file,
                                     file_lock_timeout_seconds=0)
        store.commit(host_info.HostInfo())
        old_mode = os.stat(self._store_file).st_mode
        os.chmod(self._store_file, old_mode & ~stat.S_IRUSR)
        self.addCleanup(os.chmod, self._store_file, old_mode)

        with self.assertRaises(host_info.StoreError):
            store.get(force_refresh=True)


    @mock.patch('chromite.lib.locking.FileLock', autospec=True)
    def test_commit_succeeds_after_lock_retry(self, mock_file_lock_class):
        """Tests that commit succeeds when locking requires retries.

        @param mock_file_lock_class: A patched version of the locking.FileLock
                class.
        """
        mock_file_lock = mock_file_lock_class.return_value
        mock_file_lock.__enter__.return_value = mock_file_lock
        mock_file_lock.write_lock.side_effect = [
                locking.LockNotAcquiredError('Testing error'),
                True,
        ]

        store = file_store.FileStore(self._store_file,
                                     file_lock_timeout_seconds=0.1)
        store.commit(host_info.HostInfo())
        self.assertEqual(2, mock_file_lock.write_lock.call_count)


    @mock.patch('chromite.lib.locking.FileLock', autospec=True)
    def test_refresh_succeeds_after_lock_retry(self, mock_file_lock_class):
        """Tests that refresh succeeds when locking requires retries.

        @param mock_file_lock_class: A patched version of the locking.FileLock
                class.
        """
        mock_file_lock = mock_file_lock_class.return_value
        mock_file_lock.__enter__.return_value = mock_file_lock
        mock_file_lock.write_lock.side_effect = [
                # For first commit
                True,
                # For refresh
                locking.LockNotAcquiredError('Testing error'),
                locking.LockNotAcquiredError('Testing error'),
                True,
        ]

        store = file_store.FileStore(self._store_file,
                                     file_lock_timeout_seconds=0.1)
        store.commit(host_info.HostInfo())
        store.get(force_refresh=True)
        self.assertEqual(4, mock_file_lock.write_lock.call_count)


    @mock.patch('chromite.lib.locking.FileLock', autospec=True)
    def test_commit_with_negative_timeout_clips(self, mock_file_lock_class):
        """Commit request with negative timeout is same as 0 timeout.

        @param mock_file_lock_class: A patched version of the locking.FileLock
                class.
        """
        mock_file_lock = mock_file_lock_class.return_value
        mock_file_lock.__enter__.return_value = mock_file_lock
        mock_file_lock.write_lock.side_effect = (
                locking.LockNotAcquiredError('Testing error'))

        store = file_store.FileStore(self._store_file,
                                     file_lock_timeout_seconds=-1)
        with self.assertRaises(host_info.StoreError):
            store.commit(host_info.HostInfo())
        self.assertEqual(1, mock_file_lock.write_lock.call_count)


    def test_str(self):
        """Sanity tests the __str__ implementaiton"""
        store = file_store.FileStore('/foo/path')
        self.assertEqual(str(store), 'FileStore[/foo/path]')


if __name__ == '__main__':
    unittest.main()
