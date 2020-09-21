# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock
import unittest

import common
from autotest_lib.frontend.afe.json_rpc import proxy as rpc_proxy
from autotest_lib.server import frontend
from autotest_lib.server.hosts import afe_store
from autotest_lib.server.hosts import host_info

class AfeStoreTest(unittest.TestCase):
    """Test refresh/commit success cases for AfeStore."""

    def setUp(self):
        self.hostname = 'some-host'
        self.mock_afe = mock.create_autospec(frontend.AFE, instance=True)
        self.store = afe_store.AfeStore(self.hostname, afe=self.mock_afe)


    def _create_mock_host(self, labels, attributes):
        """Create a mock frontend.Host with the given labels and attributes.

        @param labels: The labels to set on the host.
        @param attributes: The attributes to set on the host.
        @returns: A mock object for frontend.Host.
        """
        mock_host = mock.create_autospec(frontend.Host, instance=True)
        mock_host.labels = labels
        mock_host.attributes = attributes
        return mock_host


    def test_refresh(self):
        """Test that refresh correctly translates host information."""
        self.mock_afe.get_hosts.return_value = [
                self._create_mock_host(['label1'], {'attrib1': 'val1'})]
        info = self.store._refresh_impl()
        self.assertListEqual(info.labels, ['label1'])
        self.assertDictEqual(info.attributes, {'attrib1': 'val1'})


    def test_refresh_no_host_raises(self):
        """Test that refresh complains if no host is found."""
        self.mock_afe.get_hosts.return_value = []
        with self.assertRaises(host_info.StoreError):
            self.store._refresh_impl()


    def test_refresh_multiple_hosts_picks_first(self):
        """Test that refresh returns the first host if multiple match."""
        self.mock_afe.get_hosts.return_value = [
                self._create_mock_host(['label1'], {'attrib1': 'val1'}),
                self._create_mock_host(['label2'], {'attrib2': 'val2'})]
        info = self.store._refresh_impl()
        self.assertListEqual(info.labels, ['label1'])
        self.assertDictEqual(info.attributes, {'attrib1': 'val1'})


    def test_commit_labels(self):
        """Tests that labels are updated correctly on commit."""
        self.mock_afe.get_hosts.return_value = [
                self._create_mock_host(['label1'], {})]
        info = host_info.HostInfo(['label2'], {})
        self.store._commit_impl(info)
        self.assertEqual(self.mock_afe.run.call_count, 2)
        expected_run_calls = [
                mock.call('host_remove_labels', id='some-host',
                          labels=['label1']),
                mock.call('host_add_labels', id='some-host',
                          labels=['label2']),
        ]
        self.mock_afe.run.assert_has_calls(expected_run_calls,
                                           any_order=True)


    def test_commit_labels_raises(self):
        """Test that exception while committing is translated properly."""
        self.mock_afe.get_hosts.return_value = [
                self._create_mock_host(['label1'], {})]
        self.mock_afe.run.side_effect = rpc_proxy.JSONRPCException('some error')
        info = host_info.HostInfo(['label2'], {})
        with self.assertRaises(host_info.StoreError):
            self.store._commit_impl(info)


    def test_commit_adds_attributes(self):
        """Tests that new attributes are added correctly on commit."""
        self.mock_afe.get_hosts.return_value = [
                self._create_mock_host([], {})]
        info = host_info.HostInfo([], {'attrib1': 'val1'})
        self.store._commit_impl(info)
        self.assertEqual(self.mock_afe.set_host_attribute.call_count, 1)
        self.mock_afe.set_host_attribute.assert_called_once_with(
                'attrib1', 'val1', hostname=self.hostname)


    def test_commit_updates_attributes(self):
        """Tests that existing attributes are updated correctly on commit."""
        self.mock_afe.get_hosts.return_value = [
                self._create_mock_host([], {'attrib1': 'val1'})]
        info = host_info.HostInfo([], {'attrib1': 'val1_updated'})
        self.store._commit_impl(info)
        self.assertEqual(self.mock_afe.set_host_attribute.call_count, 1)
        self.mock_afe.set_host_attribute.assert_called_once_with(
                'attrib1', 'val1_updated', hostname=self.hostname)


    def test_commit_deletes_attributes(self):
        """Tests that deleted attributes are updated correctly on commit."""
        self.mock_afe.get_hosts.return_value = [
                self._create_mock_host([], {'attrib1': 'val1'})]
        info = host_info.HostInfo([], {})
        self.store._commit_impl(info)
        self.assertEqual(self.mock_afe.set_host_attribute.call_count, 1)
        self.mock_afe.set_host_attribute.assert_called_once_with(
                'attrib1', None, hostname=self.hostname)


    def test_str(self):
        """Sanity tests the __str__ implementaiton"""
        self.assertEqual(str(self.store), 'AfeStore[some-host]')


class DictDiffTest(unittest.TestCase):
    """Tests the afe_store._dict_diff private method."""

    def _assert_dict_diff(self, got_tuple, expectation_tuple):
        """Verifies the result from _dict_diff

        @param got_tuple: The tuple returned by afe_store._dict_diff
        @param expectatin_tuple: tuple (left_only, right_only, differing)
                containing iterable of keys to verify against got_tuple.
        """
        for got, expect in zip(got_tuple, expectation_tuple):
            self.assertEqual(got, set(expect))


    def test_both_empty(self):
        """Tests the case when both dicts are empty."""
        self._assert_dict_diff(afe_store._dict_diff({}, {}),
                               ((), (), ()))


    def test_right_dict_only(self):
        """Tests the case when left dict is empty."""
        self._assert_dict_diff(afe_store._dict_diff({}, {1: 1}),
                               ((), (1,), ()))


    def test_left_dict_only(self):
        """Tests the case when right dict is empty."""
        self._assert_dict_diff(afe_store._dict_diff({1: 1}, {}),
                               ((1,), (), ()))


    def test_left_dict_extra(self):
        """Tests the case when left dict has extra keys."""
        self._assert_dict_diff(afe_store._dict_diff({1: 1, 2: 2}, {1: 1}),
                               ((2,), (), ()))


    def test_right_dict_extra(self):
        """Tests the case when right dict has extra keys."""
        self._assert_dict_diff(afe_store._dict_diff({1: 1}, {1: 1, 2: 2}),
                               ((), (2,), ()))


    def test_identical_keys_with_different_values(self):
        """Tests the case when the set of keys is same, but values differ."""
        self._assert_dict_diff(afe_store._dict_diff({1: 1, 2: 3}, {1: 1, 2: 2}),
                               ((), (), (2,)))


if __name__ == '__main__':
    unittest.main()
