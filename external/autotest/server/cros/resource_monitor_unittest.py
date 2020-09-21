# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import unittest
import re
import csv
import common
import os

from itertools import imap
from autotest_lib.server.cros import resource_monitor
from autotest_lib.server.hosts import abstract_ssh
from autotest_lib.server import utils

class HostMock(abstract_ssh.AbstractSSHHost):
    """Mocks a host object."""

    TOP_PID = '1234'

    def _initialize(self, test_env):
        self.top_is_running = False

        # Keep track of whether the top raw output file exists on the system,
        # and if it does, where it is.
        self.top_output_file_path = None

        # Keep track of whether the raw top output file is currently being
        # written to by top.
        self.top_output_file_is_open = False
        self.test_env = test_env


    def get_file(self, src, dest):
        pass


    def called_unsupported_command(self, command):
        """Raises assertion error when called.

        @param command string the unsupported command called.

        """
        raise AssertionError(
                "ResourceMonitor called unsupported command %s" % command)


    def _process_top(self, cmd_args, cmd_line):
        """Process top command.

        @param cmd_args string_list of command line args.
        @param cmd_line string the command to run.

        """
        self.test_env.assertFalse(self.top_is_running,
                msg="Top must not already be running.")
        self.test_env.assertFalse(self.top_output_file_is_open,
                msg="The top output file should not be being written "
                "to before top is started")
        self.test_env.assertIsNone(self.top_output_file_path,
                msg="The top output file should not exist"
                "before top is started")
        try:
            self.redirect_index = cmd_args.index(">")
            self.top_output_file_path = cmd_args[self.redirect_index + 1]
        except ValueError, IndexError:
            self.called_unsupported_command(cmd_line)

        self.top_is_running = True
        self.top_output_file_is_open = True

        return HostMock.TOP_PID


    def _process_kill(self, cmd_args, cmd_line):
        """Process kill command.

        @param cmd_args string_list of command line args.
        @param cmd_line string the command to run.

        """
        try:
            if cmd_args[1].startswith('-'):
                pid_to_kill = cmd_args[2]
            else:
                pid_to_kill = cmd_args[1]
        except IndexError:
            self.called_unsupported_command(cmd_line)

        self.test_env.assertEqual(pid_to_kill, HostMock.TOP_PID,
                msg="Wrong pid (%r) killed . Top pid is %r." % (pid_to_kill,
                HostMock.TOP_PID))
        self.test_env.assertTrue(self.top_is_running,
                msg="Top must be running before we try to kill it")

        self.top_is_running = False
        self.top_output_file_is_open = False


    def _process_rm(self, cmd_args, cmd_line):
        """Process rm command.

        @param cmd_args string list list of command line args.
        @param cmd_line string the command to run.

        """
        try:
            if cmd_args[1].startswith('-'):
                file_to_rm = cmd_args[2]
            else:
                file_to_rm = cmd_args[1]
        except IndexError:
            self.called_unsupported_command(cmd_line)

        self.test_env.assertEqual(file_to_rm, self.top_output_file_path,
                msg="Tried to remove file that is not the top output file.")
        self.test_env.assertFalse(self.top_output_file_is_open,
                msg="Tried to remove top output file while top is still "
                "writing to it.")
        self.test_env.assertFalse(self.top_is_running,
                msg="Top was still running when we tried to remove"
                "the top output file.")
        self.test_env.assertIsNotNone(self.top_output_file_path)

        self.top_output_file_path = None


    def _run_single_cmd(self, cmd_line, *args, **kwargs):
        """Run a single command on host.

        @param cmd_line command to run on the host.

        """
        # Make the input a little nicer.
        cmd_line = cmd_line.strip()
        cmd_line = re.sub(">", " > ", cmd_line)

        cmd_args = re.split("\s+", cmd_line)
        self.test_env.assertTrue(len(cmd_args) >= 1)
        command = cmd_args[0]
        if (command == "top"):
            return self._process_top(cmd_args, cmd_line)
        elif (command == "kill"):
            return self._process_kill(cmd_args, cmd_line)
        elif(command == "rm"):
            return self._process_rm(cmd_args, cmd_line)
        else:
            logging.warning("Called unemulated command %r", cmd_line)
            return None


    def run(self, cmd_line, *args, **kwargs):
        """Run command(s) on host.

        @param cmd_line command to run on the host.
        @return CmdResult object.

        """
        cmds = re.split("&&", cmd_line)
        for cmd in cmds:
            self._run_single_cmd(cmd)
        return utils.CmdResult(exit_status=0)


    def run_background(self, cmd_line, *args, **kwargs):
        """Run command in background on host.

        @param cmd_line command to run on the host.

        """
        return self._run_single_cmd(cmd_line, args, kwargs)


    def is_monitoring(self):
        """Return true iff host is currently running top and writing output
            to a file.
        """
        return self.top_is_running and self.top_output_file_is_open and (
            self.top_output_file_path is not None)


    def monitoring_stopped(self):
        """Return true iff host is not running top and all top output files are
            closed.
        """
        return not self.is_monitoring()


class ResourceMonitorTest(unittest.TestCase):
    """Tests the non-trivial functionality of ResourceMonitor."""

    def setUp(self):
        self.topoutfile = '/tmp/resourcemonitorunittest-1234'
        self.monitor_period = 1
        self.rm_conf = resource_monitor.ResourceMonitorConfig(
                monitor_period=self.monitor_period,
                rawresult_output_filename=self.topoutfile)
        self.host = HostMock(self)


    def test_normal_operation(self):
        """Checks that normal (i.e. no exceptions, etc.) execution works."""
        self.assertFalse(self.host.is_monitoring())
        with resource_monitor.ResourceMonitor(self.host, self.rm_conf) as rm:
            self.assertFalse(self.host.is_monitoring())
            for i in range(3):
                rm.start()
                self.assertTrue(self.host.is_monitoring())
                rm.stop()
                self.assertTrue(self.host.monitoring_stopped())
        self.assertTrue(self.host.monitoring_stopped())


    def test_forgot_to_stop_monitor(self):
        """Checks that resource monitor is cleaned up even if user forgets to
            explicitly stop it.
        """
        self.assertFalse(self.host.is_monitoring())
        with resource_monitor.ResourceMonitor(self.host, self.rm_conf) as rm:
            self.assertFalse(self.host.is_monitoring())
            rm.start()
            self.assertTrue(self.host.is_monitoring())
        self.assertTrue(self.host.monitoring_stopped())


    def test_unexpected_interruption_while_monitoring(self):
        """Checks that monitor is cleaned up upon unexpected interrupt."""
        self.assertFalse(self.host.is_monitoring())

        with resource_monitor.ResourceMonitor(self.host, self.rm_conf) as rm:
            self.assertFalse(self.host.is_monitoring())
            rm.start()
            self.assertTrue(self.host.is_monitoring())
            raise KeyboardInterrupt

        self.assertTrue(self.host.monitoring_stopped())


class ResourceMonitorResultTest(unittest.TestCase):
    """Functional tests for ResourceMonitorParsedResult."""

    def setUp(self):
        self._res_dir = os.path.join(
                            os.path.dirname(os.path.realpath(__file__)),
                            'res_resource_monitor')


    def run_with_test_data(self, testdata_file, testans_file):
        """Parses testdata_file with the parses, and checks that results
            are the same as those in testans_file.

        @param testdata_file string filename containing top output to test.
        @param testans_file string filename containing answers to the test.

        """
        parsed_results = resource_monitor.ResourceMonitorParsedResult(
                testdata_file)
        with open(testans_file, "rb") as testans:
            csvreader = csv.reader(testans)
            columns = csvreader.next()
            self.assertEqual(list(columns),
                    resource_monitor.ResourceMonitorParsedResult._columns)
            utils_over_time = []
            for util_val in imap(
                    resource_monitor.
                            ResourceMonitorParsedResult.UtilValues._make,
                    csvreader):
                utils_over_time.append(util_val)
            self.assertEqual(utils_over_time, parsed_results._utils_over_time)


    def test_full_data(self):
        """General test with many possible changes to input."""
        self.run_with_test_data(
                os.path.join(self._res_dir, 'top_test_data.txt'),
                os.path.join(self._res_dir, 'top_test_data_ans.csv'))


    def test_whitespace_ridden(self):
        """Tests resilience to arbitrary whitespace characters between fields"""
        self.run_with_test_data(
                os.path.join(self._res_dir, 'top_whitespace_ridden.txt'),
                os.path.join(self._res_dir, 'top_whitespace_ridden_ans.csv'))


    def test_field_order_changed(self):
        """Tests resilience to changes in the order of fields
            (for e.g, if the Mem free/used fields change orders in the input).
        """
        self.run_with_test_data(
                os.path.join(self._res_dir, 'top_field_order_changed.txt'),
                os.path.join(self._res_dir, 'top_field_order_changed_ans.csv'))


if __name__ == '__main__':
    unittest.main()
