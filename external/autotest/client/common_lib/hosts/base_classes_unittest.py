#!/usr/bin/python

import common
import os
import unittest

from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.common_lib.test_utils import mock
from autotest_lib.client.common_lib.hosts import base_classes


class test_host_class(unittest.TestCase):
    def setUp(self):
        self.god = mock.mock_god()


    def tearDown(self):
        self.god.unstub_all()


    def test_run_output_notimplemented(self):
        host = base_classes.Host()
        self.assertRaises(NotImplementedError, host.run_output, "fake command")


    def _setup_test_check_diskspace(self, command, command_result,
                                    command_exit_status, directory,
                                    directory_exists):
        self.god.stub_function(os.path, 'isdir')
        self.god.stub_function(base_classes.Host, 'run')
        host = base_classes.Host()
        host.hostname = 'unittest-host'
        host.run.expect_call(
                'test -e "%s"' % directory,
                ignore_status=True).and_return(
                        utils.CmdResult(
                                exit_status = 0 if directory_exists else 1))
        if directory_exists:
            fake_cmd_status = utils.CmdResult(
                exit_status=command_exit_status, stdout=command_result)
            host.run.expect_call(command).and_return(fake_cmd_status)
        return host


    def test_check_diskspace(self):
        df_cmd = 'df -PB 1000000 /foo | tail -1'
        test_df_tail = ('/dev/sda1                    1061       939'
                        '       123      89% /')
        host = self._setup_test_check_diskspace(
            df_cmd, test_df_tail, 0, '/foo', True)
        host.check_diskspace('/foo', 0.1)
        self.god.check_playback()


    def test_check_diskspace_disk_full(self):
        df_cmd = 'df -PB 1000000 /foo | tail -1'
        test_df_tail = ('/dev/sda1                    1061       939'
                        '       123      89% /')
        host = self._setup_test_check_diskspace(
            df_cmd, test_df_tail, 0, '/foo', True)
        self.assertRaises(error.AutoservDiskFullHostError,
                          host.check_diskspace, '/foo', 0.2)
        self.god.check_playback()


    def test_check_diskspace_directory_not_found(self):
        df_cmd = 'df -PB 1000000 /foo | tail -1'
        test_df_tail = ('/dev/sda1                    1061       939'
                        '       123      89% /')
        host = self._setup_test_check_diskspace(
            df_cmd, test_df_tail, 0, '/foo', False)
        self.assertRaises(error.AutoservDirectoryNotFoundError,
                          host.check_diskspace, '/foo', 0.2)
        self.god.check_playback()


    def test_check_diskspace_directory_du_failed(self):
        df_cmd = 'df -PB 1000000 /foo | tail -1'
        test_df_tail = ('df: /foo: No such file or directory')
        host = self._setup_test_check_diskspace(
            df_cmd, test_df_tail, 1, '/foo', True)
        self.assertRaises(error.AutoservDiskSizeUnknownError,
                          host.check_diskspace, '/foo', 0.1)
        self.god.check_playback()


if __name__ == "__main__":
    unittest.main()
