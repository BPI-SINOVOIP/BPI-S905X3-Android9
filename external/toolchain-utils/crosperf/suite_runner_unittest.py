#!/usr/bin/env python2
#
# Copyright 2014 Google Inc. All Rights Reserved.
"""Unittest for suite_runner."""

from __future__ import print_function

import os.path
import time

import mock
import unittest

import suite_runner
import label
import test_flag

from benchmark import Benchmark

from cros_utils import command_executer
from cros_utils import logger


class SuiteRunnerTest(unittest.TestCase):
  """Class of SuiteRunner test."""
  real_logger = logger.GetLogger()

  mock_cmd_exec = mock.Mock(spec=command_executer.CommandExecuter)
  mock_cmd_term = mock.Mock(spec=command_executer.CommandTerminator)
  mock_logger = mock.Mock(spec=logger.Logger)
  mock_label = label.MockLabel(
      'lumpy', 'lumpy_chromeos_image', '', '/tmp/chromeos', 'lumpy',
      ['lumpy1.cros', 'lumpy.cros2'], '', '', False, 'average', 'gcc', '')
  telemetry_crosperf_bench = Benchmark(
      'b1_test',  # name
      'octane',  # test_name
      '',  # test_args
      3,  # iterations
      False,  # rm_chroot_tmp
      'record -e cycles',  # perf_args
      'telemetry_Crosperf',  # suite
      True)  # show_all_results

  test_that_bench = Benchmark(
      'b2_test',  # name
      'octane',  # test_name
      '',  # test_args
      3,  # iterations
      False,  # rm_chroot_tmp
      'record -e cycles')  # perf_args

  telemetry_bench = Benchmark(
      'b3_test',  # name
      'octane',  # test_name
      '',  # test_args
      3,  # iterations
      False,  # rm_chroot_tmp
      'record -e cycles',  # perf_args
      'telemetry',  # suite
      False)  # show_all_results

  def __init__(self, *args, **kwargs):
    super(SuiteRunnerTest, self).__init__(*args, **kwargs)
    self.call_test_that_run = False
    self.pin_governor_args = []
    self.test_that_args = []
    self.telemetry_run_args = []
    self.telemetry_crosperf_args = []
    self.call_telemetry_crosperf_run = False
    self.call_pin_governor = False
    self.call_telemetry_run = False

  def setUp(self):
    self.runner = suite_runner.SuiteRunner(
        self.mock_logger, 'verbose', self.mock_cmd_exec, self.mock_cmd_term)

  def test_get_profiler_args(self):
    input_str = ('--profiler=custom_perf --profiler_args=\'perf_options'
                 '="record -a -e cycles,instructions"\'')
    output_str = ("profiler=custom_perf profiler_args='record -a -e "
                  "cycles,instructions'")
    res = suite_runner.GetProfilerArgs(input_str)
    self.assertEqual(res, output_str)

  def test_run(self):

    def reset():
      self.call_pin_governor = False
      self.call_test_that_run = False
      self.call_telemetry_run = False
      self.call_telemetry_crosperf_run = False
      self.pin_governor_args = []
      self.test_that_args = []
      self.telemetry_run_args = []
      self.telemetry_crosperf_args = []

    def FakePinGovernor(machine, chroot):
      self.call_pin_governor = True
      self.pin_governor_args = [machine, chroot]

    def FakeTelemetryRun(machine, test_label, benchmark, profiler_args):
      self.telemetry_run_args = [machine, test_label, benchmark, profiler_args]
      self.call_telemetry_run = True
      return 'Ran FakeTelemetryRun'

    def FakeTelemetryCrosperfRun(machine, test_label, benchmark, test_args,
                                 profiler_args):
      self.telemetry_crosperf_args = [
          machine, test_label, benchmark, test_args, profiler_args
      ]
      self.call_telemetry_crosperf_run = True
      return 'Ran FakeTelemetryCrosperfRun'

    def FakeTestThatRun(machine, test_label, benchmark, test_args,
                        profiler_args):
      self.test_that_args = [
          machine, test_label, benchmark, test_args, profiler_args
      ]
      self.call_test_that_run = True
      return 'Ran FakeTestThatRun'

    self.runner.PinGovernorExecutionFrequencies = FakePinGovernor
    self.runner.Telemetry_Run = FakeTelemetryRun
    self.runner.Telemetry_Crosperf_Run = FakeTelemetryCrosperfRun
    self.runner.Test_That_Run = FakeTestThatRun

    machine = 'fake_machine'
    test_args = ''
    profiler_args = ''
    reset()
    self.runner.Run(machine, self.mock_label, self.telemetry_bench, test_args,
                    profiler_args)
    self.assertTrue(self.call_pin_governor)
    self.assertTrue(self.call_telemetry_run)
    self.assertFalse(self.call_test_that_run)
    self.assertFalse(self.call_telemetry_crosperf_run)
    self.assertEqual(self.telemetry_run_args, [
        'fake_machine', self.mock_label, self.telemetry_bench, ''
    ])

    reset()
    self.runner.Run(machine, self.mock_label, self.test_that_bench, test_args,
                    profiler_args)
    self.assertTrue(self.call_pin_governor)
    self.assertFalse(self.call_telemetry_run)
    self.assertTrue(self.call_test_that_run)
    self.assertFalse(self.call_telemetry_crosperf_run)
    self.assertEqual(self.test_that_args, [
        'fake_machine', self.mock_label, self.test_that_bench, '', ''
    ])

    reset()
    self.runner.Run(machine, self.mock_label, self.telemetry_crosperf_bench,
                    test_args, profiler_args)
    self.assertTrue(self.call_pin_governor)
    self.assertFalse(self.call_telemetry_run)
    self.assertFalse(self.call_test_that_run)
    self.assertTrue(self.call_telemetry_crosperf_run)
    self.assertEqual(self.telemetry_crosperf_args, [
        'fake_machine', self.mock_label, self.telemetry_crosperf_bench, '', ''
    ])

  @mock.patch.object(command_executer.CommandExecuter, 'CrosRunCommand')
  def test_pin_governor_execution_frequencies(self, mock_cros_runcmd):
    self.mock_cmd_exec.CrosRunCommand = mock_cros_runcmd
    self.runner.PinGovernorExecutionFrequencies('lumpy1.cros', '/tmp/chromeos')
    self.assertEqual(mock_cros_runcmd.call_count, 1)
    cmd = mock_cros_runcmd.call_args_list[0][0]
    # pyformat: disable
    set_cpu_cmd = (
        'set -e && '
        # Disable Turbo in Intel pstate driver
        'if [[ -e /sys/devices/system/cpu/intel_pstate/no_turbo ]]; then '
        'echo -n 1 > /sys/devices/system/cpu/intel_pstate/no_turbo; fi; '
        # Set governor to performance for each cpu
        'for f in /sys/devices/system/cpu/cpu*/cpufreq; do '
        'cd $f; '
        'echo performance > scaling_governor; '
        'done'
    )
    # pyformat: enable
    self.assertEqual(cmd, (set_cpu_cmd,))

  @mock.patch.object(time, 'sleep')
  @mock.patch.object(command_executer.CommandExecuter, 'CrosRunCommand')
  def test_reboot_machine(self, mock_cros_runcmd, mock_sleep):

    def FakePinGovernor(machine_name, chromeos_root):
      if machine_name or chromeos_root:
        pass

    self.mock_cmd_exec.CrosRunCommand = mock_cros_runcmd
    self.runner.PinGovernorExecutionFrequencies = FakePinGovernor
    self.runner.RebootMachine('lumpy1.cros', '/tmp/chromeos')
    self.assertEqual(mock_cros_runcmd.call_count, 1)
    self.assertEqual(mock_cros_runcmd.call_args_list[0][0], ('reboot && exit',))
    self.assertEqual(mock_sleep.call_count, 1)
    self.assertEqual(mock_sleep.call_args_list[0][0], (60,))

  @mock.patch.object(command_executer.CommandExecuter, 'CrosRunCommand')
  @mock.patch.object(command_executer.CommandExecuter,
                     'ChrootRunCommandWOutput')
  def test_test_that_run(self, mock_chroot_runcmd, mock_cros_runcmd):

    def FakeRebootMachine(machine, chroot):
      if machine or chroot:
        pass

    def FakeLogMsg(fd, termfd, msg, flush=True):
      if fd or termfd or msg or flush:
        pass

    save_log_msg = self.real_logger.LogMsg
    self.real_logger.LogMsg = FakeLogMsg
    self.runner.logger = self.real_logger
    self.runner.RebootMachine = FakeRebootMachine

    raised_exception = False
    try:
      self.runner.Test_That_Run('lumpy1.cros', self.mock_label,
                                self.test_that_bench, '', 'record -a -e cycles')
    except SystemExit:
      raised_exception = True
    self.assertTrue(raised_exception)

    mock_chroot_runcmd.return_value = 0
    self.mock_cmd_exec.ChrootRunCommandWOutput = mock_chroot_runcmd
    self.mock_cmd_exec.CrosRunCommand = mock_cros_runcmd
    res = self.runner.Test_That_Run('lumpy1.cros', self.mock_label,
                                    self.test_that_bench, '--iterations=2', '')
    self.assertEqual(mock_cros_runcmd.call_count, 1)
    self.assertEqual(mock_chroot_runcmd.call_count, 1)
    self.assertEqual(res, 0)
    self.assertEqual(mock_cros_runcmd.call_args_list[0][0],
                     ('rm -rf /usr/local/autotest/results/*',))
    args_list = mock_chroot_runcmd.call_args_list[0][0]
    args_dict = mock_chroot_runcmd.call_args_list[0][1]
    self.assertEqual(len(args_list), 2)
    self.assertEqual(args_list[0], '/tmp/chromeos')
    self.assertEqual(args_list[1], ('/usr/bin/test_that  '
                                    '--fast  --board=lumpy '
                                    '--iterations=2 lumpy1.cros octane'))
    self.assertEqual(args_dict['command_terminator'], self.mock_cmd_term)
    self.real_logger.LogMsg = save_log_msg

  @mock.patch.object(os.path, 'isdir')
  @mock.patch.object(command_executer.CommandExecuter,
                     'ChrootRunCommandWOutput')
  def test_telemetry_crosperf_run(self, mock_chroot_runcmd, mock_isdir):

    mock_isdir.return_value = True
    mock_chroot_runcmd.return_value = 0
    self.mock_cmd_exec.ChrootRunCommandWOutput = mock_chroot_runcmd
    profiler_args = ('--profiler=custom_perf --profiler_args=\'perf_options'
                     '="record -a -e cycles,instructions"\'')
    res = self.runner.Telemetry_Crosperf_Run('lumpy1.cros', self.mock_label,
                                             self.telemetry_crosperf_bench, '',
                                             profiler_args)
    self.assertEqual(res, 0)
    self.assertEqual(mock_chroot_runcmd.call_count, 1)
    args_list = mock_chroot_runcmd.call_args_list[0][0]
    args_dict = mock_chroot_runcmd.call_args_list[0][1]
    self.assertEqual(args_list[0], '/tmp/chromeos')
    self.assertEqual(args_list[1],
                     ('/usr/bin/test_that --autotest_dir '
                      '~/trunk/src/third_party/autotest/files '
                      ' --board=lumpy --args=" run_local=False test=octane '
                      'profiler=custom_perf profiler_args=\'record -a -e '
                      'cycles,instructions\'" lumpy1.cros telemetry_Crosperf'))
    self.assertEqual(args_dict['cros_sdk_options'],
                     ('--no-ns-pid --chrome_root= '
                      '--chrome_root_mount=/tmp/chrome_root '
                      'FEATURES="-usersandbox" CHROME_ROOT=/tmp/chrome_root'))
    self.assertEqual(args_dict['command_terminator'], self.mock_cmd_term)
    self.assertEqual(len(args_dict), 2)

  @mock.patch.object(os.path, 'isdir')
  @mock.patch.object(os.path, 'exists')
  @mock.patch.object(command_executer.CommandExecuter, 'RunCommandWOutput')
  def test_telemetry_run(self, mock_runcmd, mock_exists, mock_isdir):

    def FakeLogMsg(fd, termfd, msg, flush=True):
      if fd or termfd or msg or flush:
        pass

    save_log_msg = self.real_logger.LogMsg
    self.real_logger.LogMsg = FakeLogMsg
    mock_runcmd.return_value = 0

    self.mock_cmd_exec.RunCommandWOutput = mock_runcmd
    self.runner.logger = self.real_logger

    profiler_args = ('--profiler=custom_perf --profiler_args=\'perf_options'
                     '="record -a -e cycles,instructions"\'')

    raises_exception = False
    mock_isdir.return_value = False
    try:
      self.runner.Telemetry_Run('lumpy1.cros', self.mock_label,
                                self.telemetry_bench, '')
    except SystemExit:
      raises_exception = True
    self.assertTrue(raises_exception)

    raises_exception = False
    mock_isdir.return_value = True
    mock_exists.return_value = False
    try:
      self.runner.Telemetry_Run('lumpy1.cros', self.mock_label,
                                self.telemetry_bench, '')
    except SystemExit:
      raises_exception = True
    self.assertTrue(raises_exception)

    raises_exception = False
    mock_isdir.return_value = True
    mock_exists.return_value = True
    try:
      self.runner.Telemetry_Run('lumpy1.cros', self.mock_label,
                                self.telemetry_bench, profiler_args)
    except SystemExit:
      raises_exception = True
    self.assertTrue(raises_exception)

    test_flag.SetTestMode(True)
    res = self.runner.Telemetry_Run('lumpy1.cros', self.mock_label,
                                    self.telemetry_bench, '')
    self.assertEqual(res, 0)
    self.assertEqual(mock_runcmd.call_count, 1)
    self.assertEqual(
        mock_runcmd.call_args_list[0][0],
        (('cd src/tools/perf && ./run_measurement '
          '--browser=cros-chrome --output-format=csv '
          '--remote=lumpy1.cros --identity /tmp/chromeos/src/scripts'
          '/mod_for_test_scripts/ssh_keys/testing_rsa octane '),))

    self.real_logger.LogMsg = save_log_msg


if __name__ == '__main__':
  unittest.main()
