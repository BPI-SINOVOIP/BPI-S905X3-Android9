#!/usr/bin/python
#
# Copyright 2008 Google Inc. All Rights Reserved.

"""Test for host."""

# pylint: disable=missing-docstring

import sys
import unittest

import mock

import common
from autotest_lib.cli import cli_mock, host
from autotest_lib.client.common_lib import control_data
from autotest_lib.server import hosts
CLIENT = control_data.CONTROL_TYPE_NAMES.CLIENT
SERVER = control_data.CONTROL_TYPE_NAMES.SERVER

class host_ut(cli_mock.cli_unittest):
    def test_parse_lock_options_both_set(self):
        hh = host.host()
        class opt(object):
            lock = True
            unlock = True
        options = opt()
        self.usage = "unused"
        sys.exit.expect_call(1).and_raises(cli_mock.ExitException)
        self.god.mock_io()
        self.assertRaises(cli_mock.ExitException,
                          hh._parse_lock_options, options)
        self.god.unmock_io()


    def test_cleanup_labels_with_platform(self):
        labels = ['l0', 'l1', 'l2', 'p0', 'l3']
        hh = host.host()
        self.assertEqual(['l0', 'l1', 'l2', 'l3'],
                         hh._cleanup_labels(labels, 'p0'))


    def test_cleanup_labels_no_platform(self):
        labels = ['l0', 'l1', 'l2', 'l3']
        hh = host.host()
        self.assertEqual(['l0', 'l1', 'l2', 'l3'],
                         hh._cleanup_labels(labels))


    def test_cleanup_labels_with_non_avail_platform(self):
        labels = ['l0', 'l1', 'l2', 'l3']
        hh = host.host()
        self.assertEqual(['l0', 'l1', 'l2', 'l3'],
                         hh._cleanup_labels(labels, 'p0'))


class host_list_unittest(cli_mock.cli_unittest):
    def test_parse_host_not_required(self):
        hl = host.host_list()
        sys.argv = ['atest']
        (options, leftover) = hl.parse()
        self.assertEqual([], hl.hosts)
        self.assertEqual([], leftover)


    def test_parse_with_hosts(self):
        hl = host.host_list()
        mfile = cli_mock.create_file('host0\nhost3\nhost4\n')
        sys.argv = ['atest', 'host1', '--mlist', mfile.name, 'host3']
        (options, leftover) = hl.parse()
        self.assertEqualNoOrder(['host0', 'host1','host3', 'host4'],
                                hl.hosts)
        self.assertEqual(leftover, [])
        mfile.clean()


    def test_parse_with_labels(self):
        hl = host.host_list()
        sys.argv = ['atest', '--label', 'label0']
        (options, leftover) = hl.parse()
        self.assertEqual(['label0'], hl.labels)
        self.assertEqual(leftover, [])


    def test_parse_with_multi_labels(self):
        hl = host.host_list()
        sys.argv = ['atest', '--label', 'label0,label2']
        (options, leftover) = hl.parse()
        self.assertEqualNoOrder(['label0', 'label2'], hl.labels)
        self.assertEqual(leftover, [])


    def test_parse_with_escaped_commas_label(self):
        hl = host.host_list()
        sys.argv = ['atest', '--label', 'label\\,0']
        (options, leftover) = hl.parse()
        self.assertEqual(['label,0'], hl.labels)
        self.assertEqual(leftover, [])


    def test_parse_with_escaped_commas_multi_labels(self):
        hl = host.host_list()
        sys.argv = ['atest', '--label', 'label\\,0,label\\,2']
        (options, leftover) = hl.parse()
        self.assertEqualNoOrder(['label,0', 'label,2'], hl.labels)
        self.assertEqual(leftover, [])


    def test_parse_with_both(self):
        hl = host.host_list()
        mfile = cli_mock.create_file('host0\nhost3\nhost4\n')
        sys.argv = ['atest', 'host1', '--mlist', mfile.name, 'host3',
                    '--label', 'label0']
        (options, leftover) = hl.parse()
        self.assertEqualNoOrder(['host0', 'host1','host3', 'host4'],
                                hl.hosts)
        self.assertEqual(['label0'], hl.labels)
        self.assertEqual(leftover, [])
        mfile.clean()


    def test_execute_list_all_no_labels(self):
        self.run_cmd(argv=['atest', 'host', 'list'],
                     rpcs=[('get_hosts', {},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host0',
                              u'locked': False,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [],
                              u'invalid': False,
                              u'platform': None,
                              u'shard': None,
                              u'id': 1},
                             {u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'plat1'],
                              u'invalid': False,
                              u'platform': u'plat1',
                              u'shard': None,
                              u'id': 2}])],
                     out_words_ok=['host0', 'host1', 'Ready',
                                   'plat1', 'False', 'True', 'None'])


    def test_execute_list_all_with_labels(self):
        self.run_cmd(argv=['atest', 'host', 'list'],
                     rpcs=[('get_hosts', {},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host0',
                              u'locked': False,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label0', u'label1'],
                              u'invalid': False,
                              u'platform': None,
                              u'shard': None,
                              u'id': 1},
                             {u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label2', u'label3', u'plat1'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat1',
                              u'id': 2}])],
                     out_words_ok=['host0', 'host1', 'Ready', 'plat1',
                                   'label0', 'label1', 'label2', 'label3',
                                   'False', 'True', 'None'])


    def test_execute_list_filter_one_host(self):
        self.run_cmd(argv=['atest', 'host', 'list', 'host1'],
                     rpcs=[('get_hosts', {'hostname__in': ['host1']},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label2', u'label3', u'plat1'],
                              u'invalid': False,
                              u'platform': u'plat1',
                              u'shard': None,
                              u'id': 2}])],
                     out_words_ok=['host1', 'Ready', 'plat1',
                                   'label2', 'label3', 'True', 'None'],
                     out_words_no=['host0', 'host2',
                                   'label1', 'False'])


    def test_execute_list_filter_two_hosts(self):
        mfile = cli_mock.create_file('host2')
        self.run_cmd(argv=['atest', 'host', 'list', 'host1',
                           '--mlist', mfile.name],
                     # This is a bit fragile as the list order may change...
                     rpcs=[('get_hosts', {'hostname__in': ['host2', 'host1']},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label2', u'label3', u'plat1'],
                              u'invalid': False,
                              u'platform': u'plat1',
                              u'shard': None,
                              u'id': 2},
                             {u'status': u'Ready',
                              u'hostname': u'host2',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label3', u'plat1'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat1',
                              u'id': 3}])],
                     out_words_ok=['host1', 'Ready', 'plat1',
                                   'label2', 'label3', 'True',
                                   'host2', 'None'],
                     out_words_no=['host0', 'label1', 'False'])
        mfile.clean()


    def test_execute_list_filter_two_hosts_one_not_found(self):
        mfile = cli_mock.create_file('host2')
        self.run_cmd(argv=['atest', 'host', 'list', 'host1',
                           '--mlist', mfile.name],
                     # This is a bit fragile as the list order may change...
                     rpcs=[('get_hosts', {'hostname__in': ['host2', 'host1']},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host2',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label3', u'plat1'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat1',
                              u'id': 3}])],
                     out_words_ok=['Ready', 'plat1',
                                   'label3', 'True', 'None'],
                     out_words_no=['host1', 'False'],
                     err_words_ok=['host1'])
        mfile.clean()


    def test_execute_list_filter_two_hosts_none_found(self):
        self.run_cmd(argv=['atest', 'host', 'list',
                           'host1', 'host2'],
                     # This is a bit fragile as the list order may change...
                     rpcs=[('get_hosts', {'hostname__in': ['host2', 'host1']},
                            True,
                            [])],
                     out_words_ok=[],
                     out_words_no=['Hostname', 'Status'],
                     err_words_ok=['Unknown', 'host1', 'host2'])


    def test_execute_list_filter_label(self):
        self.run_cmd(argv=['atest', 'host', 'list',
                           '-b', 'label3'],
                     rpcs=[('get_hosts', {'labels__name__in': ['label3']},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label2', u'label3', u'plat1'],
                              u'invalid': False,
                              u'platform': u'plat1',
                              u'shard': None,
                              u'id': 2},
                             {u'status': u'Ready',
                              u'hostname': u'host2',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label3', u'plat1'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat1',
                              u'id': 3}])],
                     out_words_ok=['host1', 'Ready', 'plat1',
                                   'label2', 'label3', 'True',
                                   'host2', 'None'],
                     out_words_no=['host0', 'label1', 'False'])


    def test_execute_list_filter_multi_labels(self):
        self.run_cmd(argv=['atest', 'host', 'list',
                           '-b', 'label3,label2'],
                     rpcs=[('get_hosts', {'multiple_labels': ['label2',
                                                              'label3']},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label2', u'label3', u'plat0'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat0',
                              u'id': 2},
                             {u'status': u'Ready',
                              u'hostname': u'host3',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label3', u'label2', u'plat2'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat2',
                              u'id': 4}])],
                     out_words_ok=['host1', 'host3', 'Ready', 'plat0',
                                   'label2', 'label3', 'plat2', 'None'],
                     out_words_no=['host2', 'False', 'plat1'])


    def test_execute_list_filter_three_labels(self):
        self.run_cmd(argv=['atest', 'host', 'list',
                           '-b', 'label3,label2'],
                     rpcs=[('get_hosts', {'multiple_labels': ['label2',
                                                              'label3']},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host2',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label3', u'label2', u'plat1'],
                              u'invalid': False,
                              u'platform': u'plat1',
                              u'shard': None,
                              u'id': 3}])],
                     out_words_ok=['host2', 'plat1',
                                   'label2', 'label3', 'None'],
                     out_words_no=['host1', 'host3'])


    def test_execute_list_filter_wild_labels(self):
        self.run_cmd(argv=['atest', 'host', 'list',
                           '-b', 'label*'],
                     rpcs=[('get_hosts',
                            {'labels__name__startswith': 'label'},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host2',
                              u'locked': 1,
                              u'shard': None,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label3', u'label2', u'plat1'],
                              u'invalid': 0,
                              u'platform': u'plat1',
                              u'id': 3}])],
                     out_words_ok=['host2', 'plat1',
                                   'label2', 'label3', 'None'],
                     out_words_no=['host1', 'host3'])


    def test_execute_list_filter_multi_labels_no_results(self):
        self.run_cmd(argv=['atest', 'host', 'list',
                           '-b', 'label3,label2, '],
                     rpcs=[('get_hosts', {'multiple_labels': ['label2',
                                                              'label3']},
                            True,
                            [])],
                     out_words_ok=[],
                     out_words_no=['host1', 'host2', 'host3',
                                   'label2', 'label3'])


    def test_execute_list_filter_label_and_hosts(self):
        self.run_cmd(argv=['atest', 'host', 'list', 'host1',
                           '-b', 'label3', 'host2'],
                     rpcs=[('get_hosts', {'labels__name__in': ['label3'],
                                          'hostname__in': ['host2', 'host1']},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'labels': [u'label2', u'label3', u'plat1'],
                              u'lock_reason': u'',
                              u'invalid': False,
                              u'platform': u'plat1',
                              u'shard': None,
                              u'id': 2},
                             {u'status': u'Ready',
                              u'hostname': u'host2',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label3', u'plat1'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat1',
                              u'id': 3}])],
                     out_words_ok=['host1', 'Ready', 'plat1',
                                   'label2', 'label3', 'True',
                                   'host2', 'None'],
                     out_words_no=['host0', 'label1', 'False'])


    def test_execute_list_filter_label_and_hosts_none(self):
        self.run_cmd(argv=['atest', 'host', 'list', 'host1',
                           '-b', 'label3', 'host2'],
                     rpcs=[('get_hosts', {'labels__name__in': ['label3'],
                                          'hostname__in': ['host2', 'host1']},
                            True,
                            [])],
                     out_words_ok=[],
                     out_words_no=['Hostname', 'Status'],
                     err_words_ok=['Unknown', 'host1', 'host2'])


    def test_execute_list_filter_status(self):
        self.run_cmd(argv=['atest', 'host', 'list',
                           '-s', 'Ready'],
                     rpcs=[('get_hosts', {'status__in': ['Ready']},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label2', u'label3', u'plat1'],
                              u'invalid': False,
                              u'platform': u'plat1',
                              u'shard': None,
                              u'id': 2},
                             {u'status': u'Ready',
                              u'hostname': u'host2',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label3', u'plat1'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat1',
                              u'id': 3}])],
                     out_words_ok=['host1', 'Ready', 'plat1',
                                   'label2', 'label3', 'True',
                                   'host2', 'None'],
                     out_words_no=['host0', 'label1', 'False'])



    def test_execute_list_filter_status_and_hosts(self):
        self.run_cmd(argv=['atest', 'host', 'list', 'host1',
                           '-s', 'Ready', 'host2'],
                     rpcs=[('get_hosts', {'status__in': ['Ready'],
                                          'hostname__in': ['host2', 'host1']},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label2', u'label3', u'plat1'],
                              u'invalid': False,
                              u'platform': u'plat1',
                              u'shard': None,
                              u'id': 2},
                             {u'status': u'Ready',
                              u'hostname': u'host2',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label3', u'plat1'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat1',
                              u'id': 3}])],
                     out_words_ok=['host1', 'Ready', 'plat1',
                                   'label2', 'label3', 'True',
                                   'host2', 'None'],
                     out_words_no=['host0', 'label1', 'False'])


    def test_execute_list_filter_status_and_hosts_none(self):
        self.run_cmd(argv=['atest', 'host', 'list', 'host1',
                           '--status', 'Repair',
                           'host2'],
                     rpcs=[('get_hosts', {'status__in': ['Repair'],
                                          'hostname__in': ['host2', 'host1']},
                            True,
                            [])],
                     out_words_ok=[],
                     out_words_no=['Hostname', 'Status'],
                     err_words_ok=['Unknown', 'host2'])


    def test_execute_list_filter_statuses_and_hosts_none(self):
        self.run_cmd(argv=['atest', 'host', 'list', 'host1',
                           '--status', 'Repair',
                           'host2'],
                     rpcs=[('get_hosts', {'status__in': ['Repair'],
                                          'hostname__in': ['host2', 'host1']},
                            True,
                            [])],
                     out_words_ok=[],
                     out_words_no=['Hostname', 'Status'],
                     err_words_ok=['Unknown', 'host2'])


    def test_execute_list_filter_locked(self):
        self.run_cmd(argv=['atest', 'host', 'list', 'host1',
                           '--locked', 'host2'],
                     rpcs=[('get_hosts', {'locked': True,
                                          'hostname__in': ['host2', 'host1']},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_reason': u'',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'labels': [u'label2', u'label3', u'plat1'],
                              u'invalid': False,
                              u'platform': u'plat1',
                              u'shard': None,
                              u'id': 2},
                             {u'status': u'Ready',
                              u'hostname': u'host2',
                              u'locked': True,
                              u'locked_by': 'user0',
                              u'lock_reason': u'',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'labels': [u'label3', u'plat1'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat1',
                              u'id': 3}])],
                     out_words_ok=['host1', 'Ready', 'plat1',
                                   'label2', 'label3', 'True',
                                   'host2', 'None'],
                     out_words_no=['host0', 'label1', 'False'])


    def test_execute_list_filter_unlocked(self):
        self.run_cmd(argv=['atest', 'host', 'list',
                           '--unlocked'],
                     rpcs=[('get_hosts', {'locked': False},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': False,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label2', u'label3', u'plat1'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat1',
                              u'id': 2},
                             {u'status': u'Ready',
                              u'hostname': u'host2',
                              u'locked': False,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'labels': [u'label3', u'plat1'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat1',
                              u'id': 3}])],
                     out_words_ok=['host1', 'Ready', 'plat1',
                                   'label2', 'label3', 'False',
                                   'host2', 'None'],
                     out_words_no=['host0', 'label1', 'True'])


class host_stat_unittest(cli_mock.cli_unittest):
    def test_execute_stat_two_hosts(self):
        # The order of RPCs between host1 and host0 could change...
        self.run_cmd(argv=['atest', 'host', 'stat', 'host0', 'host1'],
                     rpcs=[('get_hosts', {'hostname': 'host1'},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'locked_by': 'user0',
                              u'lock_reason': u'',
                              u'protection': 'No protection',
                              u'labels': [u'label3', u'plat1'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat1',
                              u'id': 3,
                              u'attributes': {}}]),
                           ('get_hosts', {'hostname': 'host0'},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host0',
                              u'locked': False,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'protection': u'No protection',
                              u'labels': [u'label0', u'plat0'],
                              u'invalid': False,
                              u'shard': None,
                              u'platform': u'plat0',
                              u'id': 2,
                              u'attributes': {}}]),
                           ('get_acl_groups', {'hosts__hostname': 'host1'},
                            True,
                            [{u'description': u'',
                              u'hosts': [u'host0', u'host1'],
                              u'id': 1,
                              u'name': u'Everyone',
                              u'users': [u'user2', u'debug_user', u'user0']}]),
                           ('get_labels', {'host__hostname': 'host1'},
                            True,
                            [{u'id': 2,
                              u'platform': 1,
                              u'name': u'jme',
                              u'invalid': False,
                              u'kernel_config': u''}]),
                           ('get_acl_groups', {'hosts__hostname': 'host0'},
                            True,
                            [{u'description': u'',
                              u'hosts': [u'host0', u'host1'],
                              u'id': 1,
                              u'name': u'Everyone',
                              u'users': [u'user0', u'debug_user']},
                             {u'description': u'myacl0',
                              u'hosts': [u'host0'],
                              u'id': 2,
                              u'name': u'acl0',
                              u'users': [u'user0']}]),
                           ('get_labels', {'host__hostname': 'host0'},
                            True,
                            [{u'id': 4,
                              u'platform': 0,
                              u'name': u'label0',
                              u'invalid': False,
                              u'kernel_config': u''},
                             {u'id': 5,
                              u'platform': 1,
                              u'name': u'plat0',
                              u'invalid': False,
                              u'kernel_config': u''}])],
                     out_words_ok=['host0', 'host1', 'plat0', 'plat1',
                                   'Everyone', 'acl0', 'label0'])


    def test_execute_stat_one_bad_host_verbose(self):
        self.run_cmd(argv=['atest', 'host', 'stat', 'host0',
                           'host1', '-v'],
                     rpcs=[('get_hosts', {'hostname': 'host1'},
                            True,
                            []),
                           ('get_hosts', {'hostname': 'host0'},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host0',
                              u'locked': False,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'protection': u'No protection',
                              u'labels': [u'label0', u'plat0'],
                              u'invalid': False,
                              u'platform': u'plat0',
                              u'id': 2,
                              u'attributes': {}}]),
                           ('get_acl_groups', {'hosts__hostname': 'host0'},
                            True,
                            [{u'description': u'',
                              u'hosts': [u'host0', u'host1'],
                              u'id': 1,
                              u'name': u'Everyone',
                              u'users': [u'user0', u'debug_user']},
                             {u'description': u'myacl0',
                              u'hosts': [u'host0'],
                              u'id': 2,
                              u'name': u'acl0',
                              u'users': [u'user0']}]),
                           ('get_labels', {'host__hostname': 'host0'},
                            True,
                            [{u'id': 4,
                              u'platform': 0,
                              u'name': u'label0',
                              u'invalid': False,
                              u'kernel_config': u''},
                             {u'id': 5,
                              u'platform': 1,
                              u'name': u'plat0',
                              u'invalid': False,
                              u'kernel_config': u''}])],
                     out_words_ok=['host0', 'plat0',
                                   'Everyone', 'acl0', 'label0'],
                     out_words_no=['host1'],
                     err_words_ok=['host1', 'Unknown host'],
                     err_words_no=['host0'])


    def test_execute_stat_one_bad_host(self):
        self.run_cmd(argv=['atest', 'host', 'stat', 'host0', 'host1'],
                     rpcs=[('get_hosts', {'hostname': 'host1'},
                            True,
                            []),
                           ('get_hosts', {'hostname': 'host0'},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host0',
                              u'locked': False,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'protection': u'No protection',
                              u'labels': [u'label0', u'plat0'],
                              u'invalid': False,
                              u'platform': u'plat0',
                              u'id': 2,
                              u'attributes': {}}]),
                           ('get_acl_groups', {'hosts__hostname': 'host0'},
                            True,
                            [{u'description': u'',
                              u'hosts': [u'host0', u'host1'],
                              u'id': 1,
                              u'name': u'Everyone',
                              u'users': [u'user0', u'debug_user']},
                             {u'description': u'myacl0',
                              u'hosts': [u'host0'],
                              u'id': 2,
                              u'name': u'acl0',
                              u'users': [u'user0']}]),
                           ('get_labels', {'host__hostname': 'host0'},
                            True,
                            [{u'id': 4,
                              u'platform': 0,
                              u'name': u'label0',
                              u'invalid': False,
                              u'kernel_config': u''},
                             {u'id': 5,
                              u'platform': 1,
                              u'name': u'plat0',
                              u'invalid': False,
                              u'kernel_config': u''}])],
                     out_words_ok=['host0', 'plat0',
                                   'Everyone', 'acl0', 'label0'],
                     out_words_no=['host1'],
                     err_words_ok=['host1', 'Unknown host'],
                     err_words_no=['host0'])


    def test_execute_stat_wildcard(self):
        # The order of RPCs between host1 and host0 could change...
        self.run_cmd(argv=['atest', 'host', 'stat', 'ho*'],
                     rpcs=[('get_hosts', {'hostname__startswith': 'ho'},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'locked_by': 'user0',
                              u'lock_reason': u'',
                              u'protection': 'No protection',
                              u'labels': [u'label3', u'plat1'],
                              u'invalid': False,
                              u'platform': u'plat1',
                              u'id': 3,
                              u'attributes': {}},
                            {u'status': u'Ready',
                              u'hostname': u'host0',
                              u'locked': False,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'protection': u'No protection',
                              u'labels': [u'label0', u'plat0'],
                              u'invalid': False,
                              u'platform': u'plat0',
                              u'id': 2,
                              u'attributes': {}}]),
                           ('get_acl_groups', {'hosts__hostname': 'host1'},
                            True,
                            [{u'description': u'',
                              u'hosts': [u'host0', u'host1'],
                              u'id': 1,
                              u'name': u'Everyone',
                              u'users': [u'user2', u'debug_user', u'user0']}]),
                           ('get_labels', {'host__hostname': 'host1'},
                            True,
                            [{u'id': 2,
                              u'platform': 1,
                              u'name': u'jme',
                              u'invalid': False,
                              u'kernel_config': u''}]),
                           ('get_acl_groups', {'hosts__hostname': 'host0'},
                            True,
                            [{u'description': u'',
                              u'hosts': [u'host0', u'host1'],
                              u'id': 1,
                              u'name': u'Everyone',
                              u'users': [u'user0', u'debug_user']},
                             {u'description': u'myacl0',
                              u'hosts': [u'host0'],
                              u'id': 2,
                              u'name': u'acl0',
                              u'users': [u'user0']}]),
                           ('get_labels', {'host__hostname': 'host0'},
                            True,
                            [{u'id': 4,
                              u'platform': 0,
                              u'name': u'label0',
                              u'invalid': False,
                              u'kernel_config': u''},
                             {u'id': 5,
                              u'platform': 1,
                              u'name': u'plat0',
                              u'invalid': False,
                              u'kernel_config': u''}])],
                     out_words_ok=['host0', 'host1', 'plat0', 'plat1',
                                   'Everyone', 'acl0', 'label0'])


    def test_execute_stat_wildcard_and_host(self):
        # The order of RPCs between host1 and host0 could change...
        self.run_cmd(argv=['atest', 'host', 'stat', 'ho*', 'newhost0'],
                     rpcs=[('get_hosts', {'hostname': 'newhost0'},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'newhost0',
                              u'locked': False,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'lock_reason': u'',
                              u'protection': u'No protection',
                              u'labels': [u'label0', u'plat0'],
                              u'invalid': False,
                              u'platform': u'plat0',
                              u'id': 5,
                              u'attributes': {}}]),
                           ('get_hosts', {'hostname__startswith': 'ho'},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'locked_by': 'user0',
                              u'lock_reason': u'',
                              u'protection': 'No protection',
                              u'labels': [u'label3', u'plat1'],
                              u'invalid': False,
                              u'platform': u'plat1',
                              u'id': 3,
                              u'attributes': {}},
                            {u'status': u'Ready',
                              u'hostname': u'host0',
                              u'locked': False,
                              u'locked_by': 'user0',
                              u'lock_reason': u'',
                              u'protection': 'No protection',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'labels': [u'label0', u'plat0'],
                              u'invalid': False,
                              u'platform': u'plat0',
                              u'id': 2,
                              u'attributes': {}}]),
                           ('get_acl_groups', {'hosts__hostname': 'newhost0'},
                            True,
                            [{u'description': u'',
                              u'hosts': [u'newhost0', 'host1'],
                              u'id': 42,
                              u'name': u'my_acl',
                              u'users': [u'user0', u'debug_user']},
                             {u'description': u'my favorite acl',
                              u'hosts': [u'newhost0'],
                              u'id': 2,
                              u'name': u'acl10',
                              u'users': [u'user0']}]),
                           ('get_labels', {'host__hostname': 'newhost0'},
                            True,
                            [{u'id': 4,
                              u'platform': 0,
                              u'name': u'label0',
                              u'invalid': False,
                              u'kernel_config': u''},
                             {u'id': 5,
                              u'platform': 1,
                              u'name': u'plat0',
                              u'invalid': False,
                              u'kernel_config': u''}]),
                           ('get_acl_groups', {'hosts__hostname': 'host1'},
                            True,
                            [{u'description': u'',
                              u'hosts': [u'host0', u'host1'],
                              u'id': 1,
                              u'name': u'Everyone',
                              u'users': [u'user2', u'debug_user', u'user0']}]),
                           ('get_labels', {'host__hostname': 'host1'},
                            True,
                            [{u'id': 2,
                              u'platform': 1,
                              u'name': u'jme',
                              u'invalid': False,
                              u'kernel_config': u''}]),
                           ('get_acl_groups', {'hosts__hostname': 'host0'},
                            True,
                            [{u'description': u'',
                              u'hosts': [u'host0', u'host1'],
                              u'id': 1,
                              u'name': u'Everyone',
                              u'users': [u'user0', u'debug_user']},
                             {u'description': u'myacl0',
                              u'hosts': [u'host0'],
                              u'id': 2,
                              u'name': u'acl0',
                              u'users': [u'user0']}]),
                           ('get_labels', {'host__hostname': 'host0'},
                            True,
                            [{u'id': 4,
                              u'platform': 0,
                              u'name': u'label0',
                              u'invalid': False,
                              u'kernel_config': u''},
                             {u'id': 5,
                              u'platform': 1,
                              u'name': u'plat0',
                              u'invalid': False,
                              u'kernel_config': u''}])],
                     out_words_ok=['host0', 'host1', 'newhost0',
                                   'plat0', 'plat1',
                                   'Everyone', 'acl10', 'label0'])


class host_jobs_unittest(cli_mock.cli_unittest):
    def test_execute_jobs_one_host(self):
        self.run_cmd(argv=['atest', 'host', 'jobs', 'host0'],
                     rpcs=[('get_host_queue_entries',
                            {'host__hostname': 'host0', 'query_limit': 20,
                             'sort_by': ['-job__id']},
                            True,
                            [{u'status': u'Failed',
                              u'complete': 1,
                              u'host': {u'status': u'Ready',
                                        u'locked': True,
                                        u'locked_by': 'user0',
                                        u'hostname': u'host0',
                                        u'invalid': False,
                                        u'id': 3232},
                              u'priority': 0,
                              u'meta_host': u'meta0',
                              u'job': {u'control_file':
                                       (u"def step_init():\n"
                                        "\tjob.next_step([step_test])\n"
                                        "def step_test():\n"
                                        "\tjob.run_test('kernbench')\n\n"),
                                       u'name': u'kernel-smp-2.6.xyz.x86_64',
                                       u'control_type': CLIENT,
                                       u'synchronizing': None,
                                       u'priority': u'Low',
                                       u'owner': u'user0',
                                       u'created_on': u'2008-01-09 10:45:12',
                                       u'synch_count': None,
                                       u'id': 216},
                                       u'active': 0,
                                       u'id': 2981},
                              {u'status': u'Aborted',
                               u'complete': 1,
                               u'host': {u'status': u'Ready',
                                         u'locked': True,
                                         u'locked_by': 'user0',
                                         u'hostname': u'host0',
                                         u'invalid': False,
                                         u'id': 3232},
                               u'priority': 0,
                               u'meta_host': None,
                               u'job': {u'control_file':
                                        u"job.run_test('sleeptest')\n\n",
                                        u'name': u'testjob',
                                        u'control_type': CLIENT,
                                        u'synchronizing': 0,
                                        u'priority': u'Low',
                                        u'owner': u'user1',
                                        u'created_on': u'2008-01-17 15:04:53',
                                        u'synch_count': None,
                                        u'id': 289},
                               u'active': 0,
                               u'id': 3167}])],
                     out_words_ok=['216', 'user0', 'Failed',
                                   'kernel-smp-2.6.xyz.x86_64', 'Aborted',
                                   '289', 'user1', 'Aborted',
                                   'testjob'])


    def test_execute_jobs_wildcard(self):
        self.run_cmd(argv=['atest', 'host', 'jobs', 'ho*'],
                     rpcs=[('get_hosts', {'hostname__startswith': 'ho'},
                            True,
                            [{u'status': u'Ready',
                              u'hostname': u'host1',
                              u'locked': True,
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'locked_by': 'user0',
                              u'labels': [u'label3', u'plat1'],
                              u'invalid': False,
                              u'platform': u'plat1',
                              u'id': 3},
                            {u'status': u'Ready',
                              u'hostname': u'host0',
                              u'locked': False,
                              u'locked_by': 'user0',
                              u'lock_time': u'2008-07-23 12:54:15',
                              u'labels': [u'label0', u'plat0'],
                              u'invalid': False,
                              u'platform': u'plat0',
                              u'id': 2}]),
                           ('get_host_queue_entries',
                            {'host__hostname': 'host1', 'query_limit': 20,
                             'sort_by': ['-job__id']},
                            True,
                            [{u'status': u'Failed',
                              u'complete': 1,
                              u'host': {u'status': u'Ready',
                                        u'locked': True,
                                        u'locked_by': 'user0',
                                        u'hostname': u'host1',
                                        u'invalid': False,
                                        u'id': 3232},
                              u'priority': 0,
                              u'meta_host': u'meta0',
                              u'job': {u'control_file':
                                       (u"def step_init():\n"
                                        "\tjob.next_step([step_test])\n"
                                        "def step_test():\n"
                                        "\tjob.run_test('kernbench')\n\n"),
                                       u'name': u'kernel-smp-2.6.xyz.x86_64',
                                       u'control_type': CLIENT,
                                       u'synchronizing': None,
                                       u'priority': u'Low',
                                       u'owner': u'user0',
                                       u'created_on': u'2008-01-09 10:45:12',
                                       u'synch_count': None,
                                       u'id': 216},
                                       u'active': 0,
                                       u'id': 2981},
                              {u'status': u'Aborted',
                               u'complete': 1,
                               u'host': {u'status': u'Ready',
                                         u'locked': True,
                                         u'locked_by': 'user0',
                                         u'hostname': u'host1',
                                         u'invalid': False,
                                         u'id': 3232},
                               u'priority': 0,
                               u'meta_host': None,
                               u'job': {u'control_file':
                                        u"job.run_test('sleeptest')\n\n",
                                        u'name': u'testjob',
                                        u'control_type': CLIENT,
                                        u'synchronizing': 0,
                                        u'priority': u'Low',
                                        u'owner': u'user1',
                                        u'created_on': u'2008-01-17 15:04:53',
                                        u'synch_count': None,
                                        u'id': 289},
                               u'active': 0,
                               u'id': 3167}]),
                           ('get_host_queue_entries',
                            {'host__hostname': 'host0', 'query_limit': 20,
                             'sort_by': ['-job__id']},
                            True,
                            [{u'status': u'Failed',
                              u'complete': 1,
                              u'host': {u'status': u'Ready',
                                        u'locked': True,
                                        u'locked_by': 'user0',
                                        u'hostname': u'host0',
                                        u'invalid': False,
                                        u'id': 3232},
                              u'priority': 0,
                              u'meta_host': u'meta0',
                              u'job': {u'control_file':
                                       (u"def step_init():\n"
                                        "\tjob.next_step([step_test])\n"
                                        "def step_test():\n"
                                        "\tjob.run_test('kernbench')\n\n"),
                                       u'name': u'kernel-smp-2.6.xyz.x86_64',
                                       u'control_type': CLIENT,
                                       u'synchronizing': None,
                                       u'priority': u'Low',
                                       u'owner': u'user0',
                                       u'created_on': u'2008-01-09 10:45:12',
                                       u'synch_count': None,
                                       u'id': 216},
                                       u'active': 0,
                                       u'id': 2981},
                              {u'status': u'Aborted',
                               u'complete': 1,
                               u'host': {u'status': u'Ready',
                                         u'locked': True,
                                         u'locked_by': 'user0',
                                         u'hostname': u'host0',
                                         u'invalid': False,
                                         u'id': 3232},
                               u'priority': 0,
                               u'meta_host': None,
                               u'job': {u'control_file':
                                        u"job.run_test('sleeptest')\n\n",
                                        u'name': u'testjob',
                                        u'control_type': CLIENT,
                                        u'synchronizing': 0,
                                        u'priority': u'Low',
                                        u'owner': u'user1',
                                        u'created_on': u'2008-01-17 15:04:53',
                                        u'synch_count': None,
                                        u'id': 289},
                               u'active': 0,
                               u'id': 3167}])],
                     out_words_ok=['216', 'user0', 'Failed',
                                   'kernel-smp-2.6.xyz.x86_64', 'Aborted',
                                   '289', 'user1', 'Aborted',
                                   'testjob'])


    def test_execute_jobs_one_host_limit(self):
        self.run_cmd(argv=['atest', 'host', 'jobs', 'host0', '-q', '10'],
                     rpcs=[('get_host_queue_entries',
                            {'host__hostname': 'host0', 'query_limit': 10,
                             'sort_by': ['-job__id']},
                            True,
                            [{u'status': u'Failed',
                              u'complete': 1,
                              u'host': {u'status': u'Ready',
                                        u'locked': True,
                                        u'locked_by': 'user0',
                                        u'hostname': u'host0',
                                        u'invalid': False,
                                        u'id': 3232},
                              u'priority': 0,
                              u'meta_host': u'meta0',
                              u'job': {u'control_file':
                                       (u"def step_init():\n"
                                        "\tjob.next_step([step_test])\n"
                                        "def step_test():\n"
                                        "\tjob.run_test('kernbench')\n\n"),
                                       u'name': u'kernel-smp-2.6.xyz.x86_64',
                                       u'control_type': CLIENT,
                                       u'synchronizing': None,
                                       u'priority': u'Low',
                                       u'owner': u'user0',
                                       u'created_on': u'2008-01-09 10:45:12',
                                       u'synch_count': None,
                                       u'id': 216},
                                       u'active': 0,
                                       u'id': 2981},
                              {u'status': u'Aborted',
                               u'complete': 1,
                               u'host': {u'status': u'Ready',
                                         u'locked': True,
                                         u'locked_by': 'user0',
                                         u'hostname': u'host0',
                                         u'invalid': False,
                                         u'id': 3232},
                               u'priority': 0,
                               u'meta_host': None,
                               u'job': {u'control_file':
                                        u"job.run_test('sleeptest')\n\n",
                                        u'name': u'testjob',
                                        u'control_type': CLIENT,
                                        u'synchronizing': 0,
                                        u'priority': u'Low',
                                        u'owner': u'user1',
                                        u'created_on': u'2008-01-17 15:04:53',
                                        u'synch_count': None,
                                        u'id': 289},
                               u'active': 0,
                               u'id': 3167}])],
                     out_words_ok=['216', 'user0', 'Failed',
                                   'kernel-smp-2.6.xyz.x86_64', 'Aborted',
                                   '289', 'user1', 'Aborted',
                                   'testjob'])


class host_mod_create_tests(object):

    def _gen_attributes_rpcs(self, host, attributes):
        """Generate RPCs expected to add attributes to host.

        @param host: hostname
        @param attributes: dict of attributes

        @return: list of rpcs to expect
        """
        rpcs = []
        for attr, val in attributes.iteritems():
            rpcs.append(('set_host_attribute',
                         {
                             'hostname': host,
                             'attribute': attr,
                             'value': val,
                         },
                         True, None))
        return rpcs


    def _gen_labels_rpcs(self, labels, platform=False, host_id=None):
        """Generate RPCS expected to add labels.

        @param labels: list of label names
        @param platform: labels are platform labels
        @param host_id: Host id old labels will be deleted from (if host exists)
        """
        rpcs = []
        if host_id:
            rpcs.append(('get_labels', {'host': host_id}, True, []))
        for label in labels:
            rpcs += [
                ('get_labels', {'name': label}, True, []),
                ('add_label', {'name': label}, True, None)
            ]
            if platform:
                rpcs[-1][1]['platform'] = True
        return rpcs


    def _gen_acls_rpcs(self, hosts, acls, host_ids=[]):
        """Generate RPCs expected to add acls.

        @param hosts: list of hostnames
        @param acls: list of acl names
        @param host_ids: List of host_ids if hosts already exist
        """
        rpcs = []
        for host_id in host_ids:
            rpcs.append(('get_acl_groups', {'hosts': host_id}, True, []))
        for acl in acls:
            rpcs.append(('get_acl_groups', {'name': acl}, True, []))
            rpcs.append(('add_acl_group', {'name': acl}, True, None))
        for acl in acls:
            rpcs.append((
                'acl_group_add_hosts',
                {
                    'hosts': hosts,
                    'id': acl,
                },
                True,
                None,
            ))
        return rpcs


    def test_lock_one_host(self):
        """Test locking host / creating host locked."""
        lock_reason = 'Because'
        rpcs, out = self._gen_expectations(locked=True, lock_reason=lock_reason)
        self.run_cmd(argv=self._command_single + ['--lock', '--lock_reason',
                                                  lock_reason],
                     rpcs=rpcs, out_words_ok=out)


    def test_unlock_multiple_hosts(self):
        """Test unlocking host / creating host unlocked."""
        rpcs, out = self._gen_expectations(hosts=self._hosts, locked=False)
        self.run_cmd(argv=self._command_multiple + ['--unlock'], rpcs=rpcs,
                     out_words_ok=out)


    def test_machine_list(self):
        """Test action an machines from machine list file."""
        mfile = cli_mock.create_file(','.join(self._hosts))
        rpcs, out = self._gen_expectations(hosts=self._hosts, locked=False)
        try:
            self.run_cmd(argv=self._command_multiple + ['--unlock'], rpcs=rpcs,
                         out_words_ok=out)
        finally:
            mfile.clean()


    def test_single_attributes(self):
        """Test applying one attribute to one host."""
        attrs = {'foo': 'bar'}
        rpcs, out = self._gen_expectations(attributes=attrs)
        self.run_cmd(self._command_single + ['--attribute', 'foo=bar'],
                     rpcs=rpcs, out_words_ok=out)


    def test_attributes_comma(self):
        """Test setting an attribute with a comma in the value."""
        attrs = {'foo': 'bar,zip'}
        rpcs, out = self._gen_expectations(attributes=attrs)
        self.run_cmd(self._command_single + ['--attribute', 'foo=bar,zip'],
                     rpcs=rpcs, out_words_ok=out)


    def test_multiple_attributes_comma(self):
        """Test setting attributes when one of the values contains a comma."""
        attrs = {'foo': 'bar,zip', 'zang': 'poodle'}
        rpcs, out = self._gen_expectations(attributes=attrs)
        self.run_cmd(self._command_single + ['--attribute', 'foo=bar,zip',
                                             '--attribute', 'zang=poodle'],
                     rpcs=rpcs, out_words_ok=out)


    def test_multiple_attributes_multiple_hosts(self):
        """Test applying multiple attributes to multiple hosts."""
        attrs = {'foo': 'bar', 'baz': 'zip'}
        rpcs, out = self._gen_expectations(hosts=self._hosts, attributes=attrs)
        self.run_cmd(self._command_multiple + ['--attribute', 'foo=bar',
                                               '--attribute', 'baz=zip'],
                     rpcs=rpcs, out_words_ok=out)


    def test_platform(self):
        """Test applying platform label."""
        rpcs, out = self._gen_expectations(platform='some_platform')
        self.run_cmd(argv=self._command_single + ['--platform',
                                                  'some_platform'],
                     rpcs=rpcs, out_words_ok=out)


    def test_labels(self):
        """Test applying labels."""
        labels = ['label0', 'label1']
        rpcs, out = self._gen_expectations(labels=labels)
        self.run_cmd(argv=self._command_single + ['--labels', ','.join(labels)],
                     rpcs=rpcs, out_words_ok=out)


    def test_labels_from_file(self):
        """Test applying labels from file."""
        labels = ['label0', 'label1']
        rpcs, out = self._gen_expectations(labels=labels)
        labelsf = cli_mock.create_file(','.join(labels))
        try:
            self.run_cmd(argv=self._command_single + ['--blist', labelsf.name],
                         rpcs=rpcs, out_words_ok=out)
        finally:
            labelsf.clean()


    def test_acls(self):
        """Test applying acls."""
        acls = ['acl0', 'acl1']
        rpcs, out = self._gen_expectations(acls=acls)
        self.run_cmd(argv=self._command_single + ['--acls', ','.join(acls)],
                     rpcs=rpcs, out_words_ok=out)


    def test_acls_from_file(self):
        """Test applying acls from file."""
        acls = ['acl0', 'acl1']
        rpcs, out = self._gen_expectations(acls=acls)
        aclsf = cli_mock.create_file(','.join(acls))
        try:
            self.run_cmd(argv=self._command_single + ['-A', aclsf.name],
                         rpcs=rpcs, out_words_ok=out)
        finally:
            aclsf.clean()


    def test_protection(self):
        """Test applying host protection."""
        protection = 'Do not repair'
        rpcs, out = self._gen_expectations(protection=protection)
        self.run_cmd(argv=self._command_single + ['--protection', protection],
                     rpcs=rpcs,out_words_ok=out)


    def test_protection_invalid(self):
        """Test invalid protection causes failure."""
        protection = 'Invalid protection'
        rpcs, out = self._gen_expectations(hosts=[])
        self.run_cmd(argv=self._command_single + ['--protection', protection],
                     exit_code=2, err_words_ok=['invalid', 'choice'] +
                     protection.split())


    def test_complex(self):
        """Test applying multiple modifications / creating a complex host."""
        lock_reason = 'Because I said so.'
        platform = 'some_platform'
        labels = ['label0', 'label1']
        acls = ['acl0', 'acl1']
        protection = 'Do not verify'
        labelsf = cli_mock.create_file(labels[1])
        aclsf = cli_mock.create_file(acls[1])
        cmd_args = ['-l', '-r', lock_reason, '-t', platform, '-b', labels[0],
                    '-B', labelsf.name, '-a', acls[0], '-A', aclsf.name, '-p',
                    protection]
        rpcs, out = self._gen_expectations(locked=True, lock_reason=lock_reason,
                                           acls=acls, labels=labels,
                                           platform=platform,
                                           protection=protection)

        try:
            self.run_cmd(argv=self._command_single + cmd_args, rpcs=rpcs,
                         out_words_ok=out)
        finally:
            labelsf.clean()
            aclsf.clean()


class host_mod_unittest(host_mod_create_tests, cli_mock.cli_unittest):
    """Tests specific to the mod action and expectation generator for shared
    tests.
    """
    _hosts = ['localhost', '127.0.0.1']
    _host_ids = [1, 2]
    _command_base = ['atest', 'host', 'mod']
    _command_single = _command_base + [_hosts[0]]
    _command_multiple = _command_base + _hosts

    def _gen_expectations(self, hosts=['localhost'], locked=None,
                          lock_reason='', force_lock=False, protection=None,
                          acls=[], labels=[], platform=None, attributes={}):
        rpcs = []
        out = set()
        hosts = hosts[:]
        hosts.reverse()

        # Genarate result for get_hosts command to include all known hosts
        host_dicts = []
        for h, h_id in zip(self._hosts, self._host_ids):
            host_dicts.append({'hostname': h, 'id': h_id})
        rpcs.append(('get_hosts', {'hostname__in': hosts}, True, host_dicts))

        # Expect actions only for known hosts
        host_ids = []
        for host in hosts:
            if host not in self._hosts:
                continue
            host_id = self._host_ids[self._hosts.index(host)]
            host_ids.append(host_id)
            modify_args = {'id': host}

            if locked is not None:
                out.add('Locked' if locked else 'Unlocked')
                modify_args['locked'] = locked
                modify_args['lock_reason'] = lock_reason
            if force_lock:
                modify_args['force_modify_locking'] = True
            if protection:
                modify_args['protection'] = protection

            if len(modify_args.keys()) > 1:
                out.add(host)
                rpcs.append(('modify_host', modify_args, True, None))

            if labels:
                rpcs += self._gen_labels_rpcs(labels, host_id=host_id)
                rpcs.append(('host_add_labels', {'id': host, 'labels': labels},
                             True, None))

            if platform:
                rpcs += self._gen_labels_rpcs([platform], platform=True,
                                              host_id=host_id)
                rpcs.append(('host_add_labels', {'id': host,
                                                 'labels': [platform]},
                             True, None))

            rpcs += self._gen_attributes_rpcs(host, attributes)

        if acls:
            rpcs += self._gen_acls_rpcs(hosts, acls, host_ids=host_ids)

        return rpcs, list(out)


    def test_mod_force_lock_one_host(self):
        """Test mod with forced locking."""
        lock_reason = 'Because'
        rpcs, out = self._gen_expectations(locked=True, force_lock=True,
                                           lock_reason=lock_reason)
        self.run_cmd(argv=self._command_single + [
                            '--lock', '--force_modify_locking', '--lock_reason',
                            lock_reason],
                     rpcs=rpcs, out_words_ok=out)

    def test_mod_force_unlock_one_host(self):
        """Test mod forced unlocking."""
        rpcs, out = self._gen_expectations(locked=False, force_lock=True)
        self.run_cmd(argv=self._command_single + ['--unlock',
                                                   '--force_modify_locking'],
                      rpcs=rpcs, out_words_ok=out)

    def test_mod_fail_unknown_host(self):
        """Test mod fails with unknown host."""
        rpcs, out = self._gen_expectations(hosts=['nope'], locked=True)
        self.run_cmd(argv=self._command_base + ['nope', '--lock'],
                     rpcs=rpcs, err_words_ok=['Cannot', 'modify', 'nope'])


class host_create_unittest(host_mod_create_tests, cli_mock.cli_unittest):
    """Test specific to create action and expectation generator for shared
    tests.
    """
    _hosts = ['localhost', '127.0.0.1']
    _command_base = ['atest', 'host', 'create']
    _command_single = _command_base + [_hosts[0]]
    _command_multiple = _command_base + _hosts


    def setUp(self):
        """Mock out the create_host method.
        """
        super(host_create_unittest, self).setUp()
        self._orig_create_host = hosts.create_host


    def tearDown(self):
        """Undo mock.
        """
        super(host_create_unittest, self).tearDown()
        hosts.create_host = self._orig_create_host


    def _mock_host(self, platform=None, labels=[]):
        """Update the return values of the mocked host object.

        @param platform: return value of Host.get_platform()
        @param labels: return value of Host.get_labels()
        """
        mock_host = mock.MagicMock()
        mock_host.get_platform.return_value = platform
        mock_host.get_labels.return_value = labels
        hosts.create_host = mock.MagicMock()
        hosts.create_host.return_value = mock_host


    def _gen_expectations(self, hosts=['localhost'], locked=False,
                          lock_reason=None, platform=None,
                          discovered_platform=None, labels=[],
                          discovered_labels=[], acls=[], protection=None,
                          attributes={}):
        """Build a list of expected RPC calls based on values to host command.

        @param hosts: list of hostname being created (default ['localhost'])
        @param locked: end state of host (bool)
        @param lock_reason: reason for host to be locked
        @param platform: platform label
        @param discovered_platform: platform discovered automatically by host
        @param labels: list of host labels (excluding platform)
        @param discovered_labels: list of labels discovered automatically
        @param acls: list of host acls
        @param protection: host protection level

        @return: list of expect rpc calls (each call is (op, args, success,
            result))
        """
        hosts = hosts[:]
        hosts.reverse() # No idea why
        lock_reason = lock_reason or 'Forced lock on device creation'
        acls = acls or []

        rpcs = []
        out = ['Added', 'host'] + hosts

        # Mock platform and label detection results
        self._mock_host(discovered_platform, discovered_labels)

        for host in hosts:
            add_args = {
                'hostname': host,
                'status': 'Ready',
                'locked': True,
                'lock_reason': lock_reason,
            }
            if protection:
                add_args['protection'] = protection
            rpcs.append(('add_host', add_args, True, None))

            rpcs += self._gen_labels_rpcs(labels)
            if labels:
                rpcs.append(('host_add_labels', {'id': host, 'labels': labels},
                             True, None))

            if platform:
                rpcs += self._gen_labels_rpcs([platform], platform=True)
                rpcs.append(('host_add_labels', {'id': host,
                                                 'labels': [platform]},
                             True, None))

            rpcs += self._gen_attributes_rpcs(host, attributes)

        rpcs += self._gen_acls_rpcs(hosts, acls)

        if not locked:
            for host in hosts:
                rpcs.append((
                    'modify_host',
                    {
                        'id': host,
                        'locked': False,
                        'lock_reason': '',
                    },
                    True,
                    None,
                ))
        return rpcs, out

    def test_create_no_args(self):
        """Test simple creation with to arguments."""
        rpcs, out = self._gen_expectations()
        self.run_cmd(argv=self._command_single, rpcs=rpcs, out_words_ok=out)


    def test_create_with_discovered_platform(self):
        """Test discovered platform is used when platform isn't specified."""
        rpcs, out = self._gen_expectations(platform='some_platform',
                                           discovered_platform='some_platform')
        self.run_cmd(argv=self._command_single, rpcs=rpcs, out_words_ok=out)


    def test_create_specified_platform_overrides_discovered_platform(self):
        """Test that the specified platform overrides the discovered platform.
        """
        rpcs, out = self._gen_expectations(platform='some_platform',
                                           discovered_platform='wrong_platform')
        self.run_cmd(argv=self._command_single + ['--platform',
                                                  'some_platform'],
                     rpcs=rpcs, out_words_ok=out)


    def test_create_discovered_labels(self):
        """Test applying automatically discovered labels."""
        labels = ['label0', 'label1']
        rpcs, out = self._gen_expectations(labels=labels,
                                           discovered_labels=labels)
        self.run_cmd(argv=self._command_single, rpcs=rpcs, out_words_ok=out)


    def test_create_specified_discovered_labels_combine(self):
        """Test applying both discovered and specified labels."""
        labels = ['label0', 'label1']
        rpcs, out = self._gen_expectations(labels=labels,
                                           discovered_labels=[labels[0]])
        self.run_cmd(argv=self._command_single + ['--labels', labels[1]],
                     rpcs=rpcs, out_words_ok=out)


if __name__ == '__main__':
    unittest.main()
