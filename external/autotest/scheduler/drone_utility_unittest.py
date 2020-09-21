#!/usr/bin/python

"""Tests for drone_utility."""

import os
import unittest

import common
from autotest_lib.client.common_lib import autotemp
from autotest_lib.client.common_lib.test_utils import mock
from autotest_lib.scheduler import drone_utility


class TestProcessRefresher(unittest.TestCase):
    """Tests for the drone_utility.ProcessRefresher object."""

    def setUp(self):
        self._tempdir = autotemp.tempdir(unique_id='test_process_refresher')
        self.addCleanup(self._tempdir.clean)
        self._fake_command = '!faketest!'
        self._fake_proc_info = {'pid': 3, 'pgid': 4, 'ppid': 2,
                                'comm': self._fake_command, 'args': ''}
        self.god = mock.mock_god()
        self.god.stub_function(drone_utility, '_get_process_info')
        self._mock_get_process_info = drone_utility._get_process_info
        self.god.stub_function(drone_utility, '_process_has_dark_mark')
        self._mock_process_has_dark_mark = (
                drone_utility._process_has_dark_mark)


    def tearDown(self):
        self.god.unstub_all()

    def test_no_processes(self):
        """Sanity check the case when there is nothing to do"""
        self._mock_get_process_info.expect_call().and_return([])
        process_refresher = drone_utility.ProcessRefresher(check_mark=False)
        got, warnings = process_refresher([])
        expected = {
                'pidfiles': dict(),
                'all_processes': [],
                'autoserv_processes': [],
                'parse_processes': [],
                'pidfiles_second_read': dict(),
        }
        self.god.check_playback()
        self.assertEqual(got, expected)


    def test_read_pidfiles_use_pool(self):
        """Readable subset of pidfile paths are included in the result

        Uses process pools.
        """
        self._parameterized_test_read_pidfiles(use_pool=True)


    def test_read_pidfiles_no_pool(self):
        """Readable subset of pidfile paths are included in the result

        Does not use process pools.
        """
        self._parameterized_test_read_pidfiles(use_pool=False)


    def test_read_many_pidfiles(self):
        """Read a large number of pidfiles (more than pool size)."""
        self._mock_get_process_info.expect_call().and_return([])
        expected_pidfiles = {}
        for i in range(1000):
            data = 'data number %d' % i
            path = self._write_pidfile('pidfile%d' % i, data)
            expected_pidfiles[path] = data

        process_refresher = drone_utility.ProcessRefresher(check_mark=False,
                                                           use_pool=True)
        got, _ = process_refresher(expected_pidfiles.keys())
        expected = {
                'pidfiles': expected_pidfiles,
                'all_processes': [],
                'autoserv_processes': [],
                'parse_processes': [],
                'pidfiles_second_read': expected_pidfiles,
        }
        self.god.check_playback()
        self.assertEqual(got, expected)


    def test_filter_processes(self):
        """Various filtered results correctly classify processes by name."""
        self.maxDiff = None
        process_refresher = drone_utility.ProcessRefresher(check_mark=False)
        autoserv_processes = [self._proc_info_dict(3, 'autoserv')]
        parse_processes = [self._proc_info_dict(4, 'parse'),
                           self._proc_info_dict(5, 'site_parse')]
        all_processes = ([self._proc_info_dict(6, 'who_cares')]
                         + autoserv_processes + parse_processes)

        self._mock_get_process_info.expect_call().and_return(all_processes)
        got, _warnings = process_refresher(self._tempdir.name)
        expected = {
                'pidfiles': dict(),
                'all_processes': all_processes,
                'autoserv_processes': autoserv_processes,
                'parse_processes': parse_processes,
                'pidfiles_second_read': dict(),
        }
        self.god.check_playback()
        self.assertEqual(got, expected)


    def test_respect_dark_mark(self):
        """When check_mark=True, dark mark check is performed and respected.

        Only filtered processes with dark mark should be returned. We only test
        this with use_pool=False because mocking out _process_has_dark_mark with
        multiprocessing.Pool is hard.
        """
        self.maxDiff = None
        process_refresher = drone_utility.ProcessRefresher(check_mark=True)
        marked_process = self._proc_info_dict(3, 'autoserv')
        unmarked_process = self._proc_info_dict(369, 'autoserv')
        all_processes = [marked_process, unmarked_process]
        self._mock_get_process_info.expect_call().and_return(all_processes)
        self._mock_process_has_dark_mark.expect_call(3).and_return(True)
        self._mock_process_has_dark_mark.expect_call(369).and_return(False)
        got, warnings = process_refresher(self._tempdir.name)
        expected = {
                'pidfiles': dict(),
                'all_processes': all_processes,
                'autoserv_processes': [marked_process],
                'parse_processes': [],
                'pidfiles_second_read': dict(),
        }
        self.god.check_playback()
        self.assertEqual(got, expected)
        self.assertEqual(len(warnings), 1)
        self.assertRegexpMatches(warnings[0], '.*autoserv.*369.*')


    def _parameterized_test_read_pidfiles(self, use_pool):
        """Readable subset of pidfile paths are included in the result

        @param: use_pool: Argument use_pool for ProcessRefresher
        """
        self._mock_get_process_info.expect_call().and_return([])
        path1 = self._write_pidfile('pidfile1', 'first pidfile')
        path2 = self._write_pidfile('pidfile2', 'second pidfile',
                                    subdir='somedir')
        process_refresher = drone_utility.ProcessRefresher(check_mark=False,
                                                           use_pool=use_pool)
        got, warnings = process_refresher(
                [path1, path2,
                 os.path.join(self._tempdir.name, 'non_existent')])
        expected_pidfiles = {
                path1: 'first pidfile',
                path2: 'second pidfile',
        }
        expected = {
                'pidfiles': expected_pidfiles,
                'all_processes': [],
                'autoserv_processes': [],
                'parse_processes': [],
                'pidfiles_second_read': expected_pidfiles,
        }
        self.god.check_playback()
        self.assertEqual(got, expected)


    def _write_pidfile(self, filename, content, subdir=''):
        parent_dir = self._tempdir.name
        if subdir:
            parent_dir = os.path.join(parent_dir, subdir)
            os.makedirs(parent_dir)
        path = os.path.join(parent_dir, filename)
        with open(path, 'w') as f:
            f.write(content)
        return path

    def _proc_info_dict(self, pid, comm, pgid=33, ppid=44, args=''):
        return {'pid': pid, 'comm': comm, 'pgid': pgid, 'ppid': ppid,
                'args': args}


if __name__ == '__main__':
    unittest.main()
