# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cStringIO
import inspect
import json
import unittest

import common
from autotest_lib.server.hosts import host_info


class HostInfoTest(unittest.TestCase):
    """Tests the non-trivial attributes of HostInfo."""

    def setUp(self):
        self.info = host_info.HostInfo()

    def test_info_comparison_to_wrong_type(self):
        """Comparing HostInfo to a different type always returns False."""
        self.assertNotEqual(host_info.HostInfo(), 42)
        self.assertNotEqual(host_info.HostInfo(), None)
        # equality and non-equality are unrelated by the data model.
        self.assertFalse(host_info.HostInfo() == 42)
        self.assertFalse(host_info.HostInfo() == None)


    def test_empty_infos_are_equal(self):
        """Tests that empty HostInfo objects are considered equal."""
        self.assertEqual(host_info.HostInfo(), host_info.HostInfo())
        # equality and non-equality are unrelated by the data model.
        self.assertFalse(host_info.HostInfo() != host_info.HostInfo())


    def test_non_trivial_infos_are_equal(self):
        """Tests that the most complicated infos are correctly stated equal."""
        info1 = host_info.HostInfo(
                labels=['label1', 'label2', 'label1'],
                attributes={'attrib1': None, 'attrib2': 'val2'})
        info2 = host_info.HostInfo(
                labels=['label1', 'label2', 'label1'],
                attributes={'attrib1': None, 'attrib2': 'val2'})
        self.assertEqual(info1, info2)
        # equality and non-equality are unrelated by the data model.
        self.assertFalse(info1 != info2)


    def test_non_equal_infos(self):
        """Tests that HostInfo objects with different information are unequal"""
        info1 = host_info.HostInfo(labels=['label'])
        info2 = host_info.HostInfo(attributes={'attrib': 'value'})
        self.assertNotEqual(info1, info2)
        # equality and non-equality are unrelated by the data model.
        self.assertFalse(info1 == info2)


    def test_build_needs_prefix(self):
        """The build prefix is of the form '<type>-version:'"""
        self.info.labels = ['cros-version', 'ab-version', 'testbed-version',
                            'fwrw-version', 'fwro-version']
        self.assertIsNone(self.info.build)


    def test_build_prefix_must_be_anchored(self):
        """Ensure that build ignores prefixes occuring mid-string."""
        self.info.labels = ['not-at-start-cros-version:cros1',
                            'not-at-start-ab-version:ab1',
                            'not-at-start-testbed-version:testbed1']
        self.assertIsNone(self.info.build)


    def test_build_ignores_firmware(self):
        """build attribute should ignore firmware versions."""
        self.info.labels = ['fwrw-version:fwrw1', 'fwro-version:fwro1']
        self.assertIsNone(self.info.build)


    def test_build_returns_first_match(self):
        """When multiple labels match, first one should be used as build."""
        self.info.labels = ['cros-version:cros1', 'cros-version:cros2']
        self.assertEqual(self.info.build, 'cros1')
        self.info.labels = ['ab-version:ab1', 'ab-version:ab2']
        self.assertEqual(self.info.build, 'ab1')
        self.info.labels = ['testbed-version:tb1', 'testbed-version:tb2']
        self.assertEqual(self.info.build, 'tb1')


    def test_build_prefer_cros_over_others(self):
        """When multiple versions are available, prefer cros."""
        self.info.labels = ['testbed-version:tb1', 'ab-version:ab1',
                            'cros-version:cros1']
        self.assertEqual(self.info.build, 'cros1')
        self.info.labels = ['cros-version:cros1', 'ab-version:ab1',
                            'testbed-version:tb1']
        self.assertEqual(self.info.build, 'cros1')


    def test_build_prefer_ab_over_testbed(self):
        """When multiple versions are available, prefer ab over testbed."""
        self.info.labels = ['testbed-version:tb1', 'ab-version:ab1']
        self.assertEqual(self.info.build, 'ab1')
        self.info.labels = ['ab-version:ab1', 'testbed-version:tb1']
        self.assertEqual(self.info.build, 'ab1')


    def test_os_no_match(self):
        """Use proper prefix to search for os information."""
        self.info.labels = ['something_else', 'cros-version:hana',
                            'os_without_colon']
        self.assertEqual(self.info.os, '')


    def test_os_returns_first_match(self):
        """Return the first matching os label."""
        self.info.labels = ['os:linux', 'os:windows', 'os_corrupted_label']
        self.assertEqual(self.info.os, 'linux')


    def test_board_no_match(self):
        """Use proper prefix to search for board information."""
        self.info.labels = ['something_else', 'cros-version:hana', 'os:blah',
                            'board_my_board_no_colon']
        self.assertEqual(self.info.board, '')


    def test_board_returns_first_match(self):
        """Return the first matching board label."""
        self.info.labels = ['board_corrupted', 'board:walk', 'board:bored']
        self.assertEqual(self.info.board, 'walk')


    def test_pools_no_match(self):
        """Use proper prefix to search for pool information."""
        self.info.labels = ['something_else', 'cros-version:hana', 'os:blah',
                            'board_my_board_no_colon', 'board:my_board']
        self.assertEqual(self.info.pools, set())


    def test_pools_returns_all_matches(self):
        """Return all matching pool labels."""
        self.info.labels = ['board_corrupted', 'board:walk', 'board:bored',
                            'pool:first_pool', 'pool:second_pool']
        self.assertEqual(self.info.pools, {'second_pool', 'first_pool'})


    def test_str(self):
        """Sanity checks the __str__ implementation."""
        info = host_info.HostInfo(labels=['a'], attributes={'b': 2})
        self.assertEqual(str(info),
                         "HostInfo[Labels: ['a'], Attributes: {'b': 2}]")


    def test_clear_version_labels_no_labels(self):
        """When no version labels exit, do nothing for clear_version_labels."""
        original_labels = ['board:something', 'os:something_else',
                           'pool:mypool', 'ab-version-corrupted:blah',
                           'cros-version']
        self.info.labels = list(original_labels)
        self.info.clear_version_labels()
        self.assertListEqual(self.info.labels, original_labels)


    def test_clear_all_version_labels(self):
        """Clear each recognized type of version label."""
        original_labels = ['extra_label', 'cros-version:cr1', 'ab-version:ab1',
                           'testbed-version:tb1']
        self.info.labels = list(original_labels)
        self.info.clear_version_labels()
        self.assertListEqual(self.info.labels, ['extra_label'])

    def test_clear_all_version_label_prefixes(self):
        """Clear each recognized type of version label with empty value."""
        original_labels = ['extra_label', 'cros-version:', 'ab-version:',
                           'testbed-version:']
        self.info.labels = list(original_labels)
        self.info.clear_version_labels()
        self.assertListEqual(self.info.labels, ['extra_label'])


    def test_set_version_labels_updates_in_place(self):
        """Update version label in place if prefix already exists."""
        self.info.labels = ['extra', 'cros-version:X', 'ab-version:Y']
        self.info.set_version_label('cros-version', 'Z')
        self.assertListEqual(self.info.labels, ['extra', 'cros-version:Z',
                                                'ab-version:Y'])

    def test_set_version_labels_appends(self):
        """Append a new version label if the prefix doesn't exist."""
        self.info.labels = ['extra', 'ab-version:Y']
        self.info.set_version_label('cros-version', 'Z')
        self.assertListEqual(self.info.labels, ['extra', 'ab-version:Y',
                                                'cros-version:Z'])


class InMemoryHostInfoStoreTest(unittest.TestCase):
    """Basic tests for CachingHostInfoStore using InMemoryHostInfoStore."""

    def setUp(self):
        self.store = host_info.InMemoryHostInfoStore()


    def _verify_host_info_data(self, host_info, labels, attributes):
        """Verifies the data in the given host_info."""
        self.assertListEqual(host_info.labels, labels)
        self.assertDictEqual(host_info.attributes, attributes)


    def test_first_get_refreshes_cache(self):
        """Test that the first call to get gets the data from store."""
        self.store.info = host_info.HostInfo(['label1'], {'attrib1': 'val1'})
        got = self.store.get()
        self._verify_host_info_data(got, ['label1'], {'attrib1': 'val1'})


    def test_repeated_get_returns_from_cache(self):
        """Tests that repeated calls to get do not refresh cache."""
        self.store.info = host_info.HostInfo(['label1'], {'attrib1': 'val1'})
        got = self.store.get()
        self._verify_host_info_data(got, ['label1'], {'attrib1': 'val1'})

        self.store.info = host_info.HostInfo(['label1', 'label2'], {})
        got = self.store.get()
        self._verify_host_info_data(got, ['label1'], {'attrib1': 'val1'})


    def test_get_uncached_always_refreshes_cache(self):
        """Tests that calling get_uncached always refreshes the cache."""
        self.store.info = host_info.HostInfo(['label1'], {'attrib1': 'val1'})
        got = self.store.get(force_refresh=True)
        self._verify_host_info_data(got, ['label1'], {'attrib1': 'val1'})

        self.store.info = host_info.HostInfo(['label1', 'label2'], {})
        got = self.store.get(force_refresh=True)
        self._verify_host_info_data(got, ['label1', 'label2'], {})


    def test_commit(self):
        """Test that commit sends data to store."""
        info = host_info.HostInfo(['label1'], {'attrib1': 'val1'})
        self._verify_host_info_data(self.store.info, [], {})
        self.store.commit(info)
        self._verify_host_info_data(self.store.info, ['label1'],
                                    {'attrib1': 'val1'})


    def test_commit_then_get(self):
        """Test a commit-get roundtrip."""
        got = self.store.get()
        self._verify_host_info_data(got, [], {})

        info = host_info.HostInfo(['label1'], {'attrib1': 'val1'})
        self.store.commit(info)
        got = self.store.get()
        self._verify_host_info_data(got, ['label1'], {'attrib1': 'val1'})


    def test_commit_then_get_uncached(self):
        """Test a commit-get_uncached roundtrip."""
        got = self.store.get()
        self._verify_host_info_data(got, [], {})

        info = host_info.HostInfo(['label1'], {'attrib1': 'val1'})
        self.store.commit(info)
        got = self.store.get(force_refresh=True)
        self._verify_host_info_data(got, ['label1'], {'attrib1': 'val1'})


    def test_commit_deepcopies_data(self):
        """Once commited, changes to HostInfo don't corrupt the store."""
        info = host_info.HostInfo(['label1'], {'attrib1': {'key1': 'data1'}})
        self.store.commit(info)
        info.labels.append('label2')
        info.attributes['attrib1']['key1'] = 'data2'
        self._verify_host_info_data(self.store.info,
                                    ['label1'], {'attrib1': {'key1': 'data1'}})


    def test_get_returns_deepcopy(self):
        """The cached object is protected from |get| caller modifications."""
        self.store.info = host_info.HostInfo(['label1'],
                                             {'attrib1': {'key1': 'data1'}})
        got = self.store.get()
        self._verify_host_info_data(got,
                                    ['label1'], {'attrib1': {'key1': 'data1'}})
        got.labels.append('label2')
        got.attributes['attrib1']['key1'] = 'data2'
        got = self.store.get()
        self._verify_host_info_data(got,
                                    ['label1'], {'attrib1': {'key1': 'data1'}})


    def test_str(self):
        """Sanity tests __str__ implementation."""
        self.store.info = host_info.HostInfo(['label1'],
                                             {'attrib1': {'key1': 'data1'}})
        self.assertEqual(str(self.store),
                         'InMemoryHostInfoStore[%s]' % self.store.info)


class ExceptionRaisingStore(host_info.CachingHostInfoStore):
    """A test class that always raises on refresh / commit."""

    def __init__(self):
        super(ExceptionRaisingStore, self).__init__()
        self.refresh_raises = True
        self.commit_raises = True


    def _refresh_impl(self):
        if self.refresh_raises:
            raise host_info.StoreError('no can do')
        return host_info.HostInfo()

    def _commit_impl(self, _):
        if self.commit_raises:
            raise host_info.StoreError('wont wont wont')


class CachingHostInfoStoreErrorTest(unittest.TestCase):
    """Tests error behaviours of CachingHostInfoStore."""

    def setUp(self):
        self.store = ExceptionRaisingStore()


    def test_failed_refresh_cleans_cache(self):
        """Sanity checks return values when refresh raises."""
        with self.assertRaises(host_info.StoreError):
            self.store.get()
        # Since |get| hit an error, a subsequent get should again hit the store.
        with self.assertRaises(host_info.StoreError):
            self.store.get()


    def test_failed_commit_cleans_cache(self):
        """Check that a failed commit cleanes cache."""
        # Let's initialize the store without errors.
        self.store.refresh_raises = False
        self.store.get(force_refresh=True)
        self.store.refresh_raises = True

        with self.assertRaises(host_info.StoreError):
            self.store.commit(host_info.HostInfo())
        # Since |commit| hit an error, a subsequent get should again hit the
        # store.
        with self.assertRaises(host_info.StoreError):
            self.store.get()


class GetStoreFromMachineTest(unittest.TestCase):
    """Tests the get_store_from_machine function."""

    def test_machine_is_dict(self):
        """We extract the store when machine is a dict."""
        machine = {
                'something': 'else',
                'host_info_store': 5
        }
        self.assertEqual(host_info.get_store_from_machine(machine), 5)


    def test_machine_is_string(self):
        """We return a trivial store when machine is a string."""
        machine = 'hostname'
        self.assertTrue(isinstance(host_info.get_store_from_machine(machine),
                                   host_info.InMemoryHostInfoStore))


class HostInfoJsonSerializationTestCase(unittest.TestCase):
    """Tests the json_serialize and json_deserialize functions."""

    CURRENT_SERIALIZATION_VERSION = host_info._CURRENT_SERIALIZATION_VERSION

    def test_serialize_empty(self):
        """Serializing empty HostInfo results in the expected json."""
        info = host_info.HostInfo()
        file_obj = cStringIO.StringIO()
        host_info.json_serialize(info, file_obj)
        file_obj.seek(0)
        expected_dict = {
                'serializer_version': self.CURRENT_SERIALIZATION_VERSION,
                'attributes' : {},
                'labels': [],
        }
        self.assertEqual(json.load(file_obj), expected_dict)


    def test_serialize_non_empty(self):
        """Serializing a populated HostInfo results in expected json."""
        info = host_info.HostInfo(labels=['label1'],
                                  attributes={'attrib': 'val'})
        file_obj = cStringIO.StringIO()
        host_info.json_serialize(info, file_obj)
        file_obj.seek(0)
        expected_dict = {
                'serializer_version': self.CURRENT_SERIALIZATION_VERSION,
                'attributes' : {'attrib': 'val'},
                'labels': ['label1'],
        }
        self.assertEqual(json.load(file_obj), expected_dict)


    def test_round_trip_empty(self):
        """Serializing - deserializing empty HostInfo keeps it unchanged."""
        info = host_info.HostInfo()
        serialized_fp = cStringIO.StringIO()
        host_info.json_serialize(info, serialized_fp)
        serialized_fp.seek(0)
        got = host_info.json_deserialize(serialized_fp)
        self.assertEqual(got, info)


    def test_round_trip_non_empty(self):
        """Serializing - deserializing non-empty HostInfo keeps it unchanged."""
        info = host_info.HostInfo(
                labels=['label1'],
                attributes = {'attrib': 'val'})
        serialized_fp = cStringIO.StringIO()
        host_info.json_serialize(info, serialized_fp)
        serialized_fp.seek(0)
        got = host_info.json_deserialize(serialized_fp)
        self.assertEqual(got, info)


    def test_deserialize_malformed_json_raises(self):
        """Deserializing a malformed string raises."""
        with self.assertRaises(host_info.DeserializationError):
            host_info.json_deserialize(cStringIO.StringIO('{labels:['))


    def test_deserialize_no_version_raises(self):
        """Deserializing a string with no serializer version raises."""
        info = host_info.HostInfo()
        serialized_fp = cStringIO.StringIO()
        host_info.json_serialize(info, serialized_fp)
        serialized_fp.seek(0)

        serialized_dict = json.load(serialized_fp)
        del serialized_dict['serializer_version']
        serialized_no_version_str = json.dumps(serialized_dict)

        with self.assertRaises(host_info.DeserializationError):
            host_info.json_deserialize(
                    cStringIO.StringIO(serialized_no_version_str))


    def test_deserialize_malformed_host_info_raises(self):
        """Deserializing a malformed host_info raises."""
        info = host_info.HostInfo()
        serialized_fp = cStringIO.StringIO()
        host_info.json_serialize(info, serialized_fp)
        serialized_fp.seek(0)

        serialized_dict = json.load(serialized_fp)
        del serialized_dict['labels']
        serialized_no_version_str = json.dumps(serialized_dict)

        with self.assertRaises(host_info.DeserializationError):
            host_info.json_deserialize(
                    cStringIO.StringIO(serialized_no_version_str))


    def test_enforce_compatibility_version_1(self):
        """Tests that required fields are never dropped.

        Never change this test. If you must break compatibility, uprev the
        serializer version and add a new test for the newer version.

        Adding a field to compat_info_str means we're making the new field
        mandatory. This breaks backwards compatibility.
        Removing a field from compat_info_str means we're no longer requiring a
        field to be mandatory. This breaks forwards compatibility.
        """
        compat_dict = {
                'serializer_version': 1,
                'attributes': {},
                'labels': []
        }
        serialized_str = json.dumps(compat_dict)
        serialized_fp = cStringIO.StringIO(serialized_str)
        host_info.json_deserialize(serialized_fp)


    def test_serialize_pretty_print(self):
        """Serializing a host_info dumps the json in human-friendly format"""
        info = host_info.HostInfo(labels=['label1'],
                                  attributes={'attrib': 'val'})
        serialized_fp = cStringIO.StringIO()
        host_info.json_serialize(info, serialized_fp)
        expected = """{
            "attributes": {
                "attrib": "val"
            },
            "labels": [
                "label1"
            ],
            "serializer_version": %d
        }""" % self.CURRENT_SERIALIZATION_VERSION
        self.assertEqual(serialized_fp.getvalue(), inspect.cleandoc(expected))


if __name__ == '__main__':
    unittest.main()
