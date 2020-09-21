# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import print_function
from __future__ import absolute_import

import mock
import unittest

import common
from autotest_lib.server.hosts import host_info
from autotest_lib.server.hosts import shadowing_store


class ShadowingStoreTestCase(unittest.TestCase):
    """Tests shadowing capabilities of ShadowingStore"""

    def test_init_commits_to_shadow(self):
        """Initialization updates the shadow store"""
        info = host_info.HostInfo(labels='blah', attributes='boo')
        primary = _FakeRaisingStore(info)
        shadow = _FakeRaisingStore()
        store = shadowing_store.ShadowingStore(primary, shadow)
        self.assertEqual(shadow.get(), info)

    def test_commit_commits_to_both_stores(self):
        """Successful commit involves committing to both stores"""
        primary = _FakeRaisingStore()
        shadow = _FakeRaisingStore()
        store = shadowing_store.ShadowingStore(primary, shadow)
        info = host_info.HostInfo(labels='blah', attributes='boo')
        store.commit(info)
        self.assertEqual(primary.get(), info)
        self.assertEqual(shadow.get(), info)

    def test_commit_ignores_failure_to_commit_to_shadow(self):
        """Failure to commit to shadow store does not affect the result"""
        init_info = host_info.HostInfo(labels='init')
        primary = _FakeRaisingStore(init_info)
        shadow = _FakeRaisingStore(init_info, raise_on_commit=True)
        store = shadowing_store.ShadowingStore(primary, shadow)
        info = host_info.HostInfo(labels='blah', attributes='boo')
        store.commit(info)
        self.assertEqual(primary.get(), info)
        self.assertEqual(shadow.get(), init_info)

    def test_refresh_validates_matching_stores(self):
        """Successful validation on refresh returns the correct info"""
        init_info = host_info.HostInfo(labels='init')
        primary = _FakeRaisingStore(init_info)
        shadow = _FakeRaisingStore(init_info)
        store = shadowing_store.ShadowingStore(primary, shadow)
        got = store.get(force_refresh=True)
        self.assertEqual(got, init_info)

    def test_refresh_ignores_failed_refresh_from_shadow_store(self):
        """Failure to refresh from shadow store does not affect the result"""
        init_info = host_info.HostInfo(labels='init')
        primary = _FakeRaisingStore(init_info)
        shadow = _FakeRaisingStore(init_info, raise_on_refresh=True)
        store = shadowing_store.ShadowingStore(primary, shadow)
        got = store.get(force_refresh=True)
        self.assertEqual(got, init_info)

    def test_refresh_complains_on_mismatching_stores(self):
        """Store complains on mismatched responses from the primary / shadow"""
        callback = mock.MagicMock()
        p_info = host_info.HostInfo('primary')
        primary = _FakeRaisingStore(p_info)
        shadow = _FakeRaisingStore()
        store = shadowing_store.ShadowingStore(primary, shadow,
                                               mismatch_callback=callback)
        # ShadowingStore will update shadow on initialization, so we modify it
        # after creating store.
        s_info = host_info.HostInfo('shadow')
        shadow.commit(s_info)

        got = store.get(force_refresh=True)
        self.assertEqual(got, p_info)
        callback.assert_called_once_with(p_info, s_info)

    def test_refresh_fixes_mismatch_in_stores(self):
        """On finding a mismatch, the difference is fixed by the store"""
        callback = mock.MagicMock()
        p_info = host_info.HostInfo('primary')
        primary = _FakeRaisingStore(p_info)
        shadow = _FakeRaisingStore()
        store = shadowing_store.ShadowingStore(primary, shadow,
                                               mismatch_callback=callback)
        # ShadowingStore will update shadow on initialization, so we modify it
        # after creating store.
        s_info = host_info.HostInfo('shadow')
        shadow.commit(s_info)

        got = store.get(force_refresh=True)
        self.assertEqual(got, p_info)
        callback.assert_called_once_with(p_info, s_info)
        self.assertEqual(got, shadow.get())

        got = store.get(force_refresh=True)
        self.assertEqual(got, p_info)
        # No extra calls, just the one we already saw above.
        callback.assert_called_once_with(p_info, s_info)


class _FakeRaisingStore(host_info.InMemoryHostInfoStore):
    """A fake store that raises an error on refresh / commit if requested"""

    def __init__(self, info=None, raise_on_refresh=False,
                 raise_on_commit=False):
        """
        @param info: A HostInfo to initialize the store with.
        @param raise_on_refresh: If True, _refresh_impl raises a StoreError.
        @param raise_on_commit: If True, _commit_impl raises a StoreError.
        """
        super(_FakeRaisingStore, self).__init__(info)
        self._raise_on_refresh = raise_on_refresh
        self._raise_on_commit = raise_on_commit

    def _refresh_impl(self):
        print('refresh_impl')
        if self._raise_on_refresh:
            raise host_info.StoreError('Test refresh error')
        return super(_FakeRaisingStore, self)._refresh_impl()

    def _commit_impl(self, info):
        print('commit_impl: %s' % info)
        if self._raise_on_commit:
            raise host_info.StoreError('Test commit error')
        super(_FakeRaisingStore, self)._commit_impl(info)


if __name__ == '__main__':
    unittest.main()
