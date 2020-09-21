#!/usr/bin/env python2
#
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Tests for the experiment runner module."""

from __future__ import print_function

import StringIO
import getpass
import os

import mock
import unittest

import experiment_runner
import experiment_status
import machine_manager
import config
import test_flag

from experiment_factory import ExperimentFactory
from experiment_file import ExperimentFile
from results_cache import Result
from results_report import HTMLResultsReport
from results_report import TextResultsReport

from cros_utils import command_executer
from cros_utils.email_sender import EmailSender
from cros_utils.file_utils import FileUtils

EXPERIMENT_FILE_1 = """
  board: parrot
  remote: chromeos-parrot1.cros chromreos-parrot2.cros

  benchmark: kraken {
    suite: telemetry_Crosperf
    iterations: 3
  }

  image1 {
    chromeos_root: /usr/local/google/chromeos
    chromeos_image: /usr/local/google/chromeos/src/build/images/parrot/latest/cros_image1.bin
  }

  image2 {
    chromeos_image: /usr/local/google/chromeos/src/build/imaages/parrot/latest/cros_image2.bin
  }
  """

# pylint: disable=protected-access


class FakeLogger(object):
  """Fake logger for tests."""

  def __init__(self):
    self.LogOutputCount = 0
    self.LogErrorCount = 0
    self.output_msgs = []
    self.error_msgs = []
    self.dot_count = 0
    self.LogStartDotsCount = 0
    self.LogEndDotsCount = 0
    self.LogAppendDotCount = 0

  def LogOutput(self, msg):
    self.LogOutputCount += 1
    self.output_msgs.append(msg)

  def LogError(self, msg):
    self.LogErrorCount += 1
    self.error_msgs.append(msg)

  def LogStartDots(self):
    self.LogStartDotsCount += 1
    self.dot_count += 1

  def LogAppendDot(self):
    self.LogAppendDotCount += 1
    self.dot_count += 1

  def LogEndDots(self):
    self.LogEndDotsCount += 1

  def Reset(self):
    self.LogOutputCount = 0
    self.LogErrorCount = 0
    self.output_msgs = []
    self.error_msgs = []
    self.dot_count = 0
    self.LogStartDotsCount = 0
    self.LogEndDotsCount = 0
    self.LogAppendDotCount = 0


class ExperimentRunnerTest(unittest.TestCase):
  """Test for experiment runner class."""

  run_count = 0
  is_complete_count = 0
  mock_logger = FakeLogger()
  mock_cmd_exec = mock.Mock(spec=command_executer.CommandExecuter)

  def make_fake_experiment(self):
    test_flag.SetTestMode(True)
    experiment_file = ExperimentFile(StringIO.StringIO(EXPERIMENT_FILE_1))
    experiment = ExperimentFactory().GetExperiment(
        experiment_file, working_directory='', log_dir='')
    return experiment

  @mock.patch.object(machine_manager.MachineManager, 'AddMachine')
  @mock.patch.object(os.path, 'isfile')

  # pylint: disable=arguments-differ
  def setUp(self, mock_isfile, _mock_addmachine):
    mock_isfile.return_value = True
    self.exp = self.make_fake_experiment()

  def test_init(self):
    er = experiment_runner.ExperimentRunner(
        self.exp,
        json_report=False,
        using_schedv2=False,
        log=self.mock_logger,
        cmd_exec=self.mock_cmd_exec)
    self.assertFalse(er._terminated)
    self.assertEqual(er.STATUS_TIME_DELAY, 10)

    self.exp.log_level = 'verbose'
    er = experiment_runner.ExperimentRunner(
        self.exp,
        json_report=False,
        using_schedv2=False,
        log=self.mock_logger,
        cmd_exec=self.mock_cmd_exec)
    self.assertEqual(er.STATUS_TIME_DELAY, 30)

  @mock.patch.object(experiment_status.ExperimentStatus, 'GetStatusString')
  @mock.patch.object(experiment_status.ExperimentStatus, 'GetProgressString')
  def test_run(self, mock_progress_string, mock_status_string):

    self.run_count = 0
    self.is_complete_count = 0

    def reset():
      self.run_count = 0
      self.is_complete_count = 0

    def FakeRun():
      self.run_count += 1
      return 0

    def FakeIsComplete():
      self.is_complete_count += 1
      if self.is_complete_count < 3:
        return False
      else:
        return True

    self.mock_logger.Reset()
    self.exp.Run = FakeRun
    self.exp.IsComplete = FakeIsComplete

    # Test 1: log_level == "quiet"
    self.exp.log_level = 'quiet'
    er = experiment_runner.ExperimentRunner(
        self.exp,
        json_report=False,
        using_schedv2=False,
        log=self.mock_logger,
        cmd_exec=self.mock_cmd_exec)
    er.STATUS_TIME_DELAY = 2
    mock_status_string.return_value = 'Fake status string'
    er._Run(self.exp)
    self.assertEqual(self.run_count, 1)
    self.assertTrue(self.is_complete_count > 0)
    self.assertEqual(self.mock_logger.LogStartDotsCount, 1)
    self.assertEqual(self.mock_logger.LogAppendDotCount, 1)
    self.assertEqual(self.mock_logger.LogEndDotsCount, 1)
    self.assertEqual(self.mock_logger.dot_count, 2)
    self.assertEqual(mock_progress_string.call_count, 0)
    self.assertEqual(mock_status_string.call_count, 2)
    self.assertEqual(self.mock_logger.output_msgs, [
        '==============================', 'Fake status string',
        '=============================='
    ])
    self.assertEqual(len(self.mock_logger.error_msgs), 0)

    # Test 2: log_level == "average"
    self.mock_logger.Reset()
    reset()
    self.exp.log_level = 'average'
    mock_status_string.call_count = 0
    er = experiment_runner.ExperimentRunner(
        self.exp,
        json_report=False,
        using_schedv2=False,
        log=self.mock_logger,
        cmd_exec=self.mock_cmd_exec)
    er.STATUS_TIME_DELAY = 2
    mock_status_string.return_value = 'Fake status string'
    er._Run(self.exp)
    self.assertEqual(self.run_count, 1)
    self.assertTrue(self.is_complete_count > 0)
    self.assertEqual(self.mock_logger.LogStartDotsCount, 1)
    self.assertEqual(self.mock_logger.LogAppendDotCount, 1)
    self.assertEqual(self.mock_logger.LogEndDotsCount, 1)
    self.assertEqual(self.mock_logger.dot_count, 2)
    self.assertEqual(mock_progress_string.call_count, 0)
    self.assertEqual(mock_status_string.call_count, 2)
    self.assertEqual(self.mock_logger.output_msgs, [
        '==============================', 'Fake status string',
        '=============================='
    ])
    self.assertEqual(len(self.mock_logger.error_msgs), 0)

    # Test 3: log_level == "verbose"
    self.mock_logger.Reset()
    reset()
    self.exp.log_level = 'verbose'
    mock_status_string.call_count = 0
    er = experiment_runner.ExperimentRunner(
        self.exp,
        json_report=False,
        using_schedv2=False,
        log=self.mock_logger,
        cmd_exec=self.mock_cmd_exec)
    er.STATUS_TIME_DELAY = 2
    mock_status_string.return_value = 'Fake status string'
    mock_progress_string.return_value = 'Fake progress string'
    er._Run(self.exp)
    self.assertEqual(self.run_count, 1)
    self.assertTrue(self.is_complete_count > 0)
    self.assertEqual(self.mock_logger.LogStartDotsCount, 0)
    self.assertEqual(self.mock_logger.LogAppendDotCount, 0)
    self.assertEqual(self.mock_logger.LogEndDotsCount, 0)
    self.assertEqual(self.mock_logger.dot_count, 0)
    self.assertEqual(mock_progress_string.call_count, 2)
    self.assertEqual(mock_status_string.call_count, 2)
    self.assertEqual(self.mock_logger.output_msgs, [
        '==============================', 'Fake progress string',
        'Fake status string', '==============================',
        '==============================', 'Fake progress string',
        'Fake status string', '=============================='
    ])
    self.assertEqual(len(self.mock_logger.error_msgs), 0)

  @mock.patch.object(TextResultsReport, 'GetReport')
  def test_print_table(self, mock_report):
    self.mock_logger.Reset()
    mock_report.return_value = 'This is a fake experiment report.'
    er = experiment_runner.ExperimentRunner(
        self.exp,
        json_report=False,
        using_schedv2=False,
        log=self.mock_logger,
        cmd_exec=self.mock_cmd_exec)
    er._PrintTable(self.exp)
    self.assertEqual(mock_report.call_count, 1)
    self.assertEqual(self.mock_logger.output_msgs,
                     ['This is a fake experiment report.'])

  @mock.patch.object(HTMLResultsReport, 'GetReport')
  @mock.patch.object(TextResultsReport, 'GetReport')
  @mock.patch.object(EmailSender, 'Attachment')
  @mock.patch.object(EmailSender, 'SendEmail')
  @mock.patch.object(getpass, 'getuser')
  def test_email(self, mock_getuser, mock_emailer, mock_attachment,
                 mock_text_report, mock_html_report):

    mock_getuser.return_value = 'john.smith@google.com'
    mock_text_report.return_value = 'This is a fake text report.'
    mock_html_report.return_value = 'This is a fake html report.'

    self.mock_logger.Reset()
    config.AddConfig('no_email', True)
    self.exp.email_to = ['jane.doe@google.com']
    er = experiment_runner.ExperimentRunner(
        self.exp,
        json_report=False,
        using_schedv2=False,
        log=self.mock_logger,
        cmd_exec=self.mock_cmd_exec)
    # Test 1. Config:no_email; exp.email_to set ==> no email sent
    er._Email(self.exp)
    self.assertEqual(mock_getuser.call_count, 0)
    self.assertEqual(mock_emailer.call_count, 0)
    self.assertEqual(mock_attachment.call_count, 0)
    self.assertEqual(mock_text_report.call_count, 0)
    self.assertEqual(mock_html_report.call_count, 0)

    # Test 2. Config: email. exp.email_to set; cache hit.  => send email
    self.mock_logger.Reset()
    config.AddConfig('no_email', False)
    for r in self.exp.benchmark_runs:
      r.cache_hit = True
    er._Email(self.exp)
    self.assertEqual(mock_getuser.call_count, 1)
    self.assertEqual(mock_emailer.call_count, 1)
    self.assertEqual(mock_attachment.call_count, 1)
    self.assertEqual(mock_text_report.call_count, 1)
    self.assertEqual(mock_html_report.call_count, 1)
    self.assertEqual(len(mock_emailer.call_args), 2)
    self.assertEqual(mock_emailer.call_args[0],
                     (['jane.doe@google.com',
                       'john.smith@google.com'], ': image1 vs. image2',
                      "<pre style='font-size: 13px'>This is a fake text "
                      'report.\nResults are stored in _results.\n</pre>'))
    self.assertTrue(type(mock_emailer.call_args[1]) is dict)
    self.assertEqual(len(mock_emailer.call_args[1]), 2)
    self.assertTrue('attachments' in mock_emailer.call_args[1].keys())
    self.assertEqual(mock_emailer.call_args[1]['msg_type'], 'html')

    mock_attachment.assert_called_with('report.html',
                                       'This is a fake html report.')

    # Test 3. Config: email; exp.mail_to set; no cache hit.  => send email
    self.mock_logger.Reset()
    mock_getuser.reset_mock()
    mock_emailer.reset_mock()
    mock_attachment.reset_mock()
    mock_text_report.reset_mock()
    mock_html_report.reset_mock()
    config.AddConfig('no_email', False)
    for r in self.exp.benchmark_runs:
      r.cache_hit = False
    er._Email(self.exp)
    self.assertEqual(mock_getuser.call_count, 1)
    self.assertEqual(mock_emailer.call_count, 1)
    self.assertEqual(mock_attachment.call_count, 1)
    self.assertEqual(mock_text_report.call_count, 1)
    self.assertEqual(mock_html_report.call_count, 1)
    self.assertEqual(len(mock_emailer.call_args), 2)
    self.assertEqual(mock_emailer.call_args[0],
                     ([
                         'jane.doe@google.com', 'john.smith@google.com',
                         'john.smith@google.com'
                     ], ': image1 vs. image2',
                      "<pre style='font-size: 13px'>This is a fake text "
                      'report.\nResults are stored in _results.\n</pre>'))
    self.assertTrue(type(mock_emailer.call_args[1]) is dict)
    self.assertEqual(len(mock_emailer.call_args[1]), 2)
    self.assertTrue('attachments' in mock_emailer.call_args[1].keys())
    self.assertEqual(mock_emailer.call_args[1]['msg_type'], 'html')

    mock_attachment.assert_called_with('report.html',
                                       'This is a fake html report.')

    # Test 4. Config: email; exp.mail_to = None; no cache hit. => send email
    self.mock_logger.Reset()
    mock_getuser.reset_mock()
    mock_emailer.reset_mock()
    mock_attachment.reset_mock()
    mock_text_report.reset_mock()
    mock_html_report.reset_mock()
    self.exp.email_to = []
    er._Email(self.exp)
    self.assertEqual(mock_getuser.call_count, 1)
    self.assertEqual(mock_emailer.call_count, 1)
    self.assertEqual(mock_attachment.call_count, 1)
    self.assertEqual(mock_text_report.call_count, 1)
    self.assertEqual(mock_html_report.call_count, 1)
    self.assertEqual(len(mock_emailer.call_args), 2)
    self.assertEqual(mock_emailer.call_args[0],
                     (['john.smith@google.com'], ': image1 vs. image2',
                      "<pre style='font-size: 13px'>This is a fake text "
                      'report.\nResults are stored in _results.\n</pre>'))
    self.assertTrue(type(mock_emailer.call_args[1]) is dict)
    self.assertEqual(len(mock_emailer.call_args[1]), 2)
    self.assertTrue('attachments' in mock_emailer.call_args[1].keys())
    self.assertEqual(mock_emailer.call_args[1]['msg_type'], 'html')

    mock_attachment.assert_called_with('report.html',
                                       'This is a fake html report.')

    # Test 5. Config: email; exp.mail_to = None; cache hit => no email sent
    self.mock_logger.Reset()
    mock_getuser.reset_mock()
    mock_emailer.reset_mock()
    mock_attachment.reset_mock()
    mock_text_report.reset_mock()
    mock_html_report.reset_mock()
    for r in self.exp.benchmark_runs:
      r.cache_hit = True
    er._Email(self.exp)
    self.assertEqual(mock_getuser.call_count, 0)
    self.assertEqual(mock_emailer.call_count, 0)
    self.assertEqual(mock_attachment.call_count, 0)
    self.assertEqual(mock_text_report.call_count, 0)
    self.assertEqual(mock_html_report.call_count, 0)

  @mock.patch.object(FileUtils, 'RmDir')
  @mock.patch.object(FileUtils, 'MkDirP')
  @mock.patch.object(FileUtils, 'WriteFile')
  @mock.patch.object(HTMLResultsReport, 'FromExperiment')
  @mock.patch.object(TextResultsReport, 'FromExperiment')
  @mock.patch.object(Result, 'CopyResultsTo')
  @mock.patch.object(Result, 'CleanUp')
  def test_store_results(self, mock_cleanup, mock_copy, _mock_text_report,
                         mock_report, mock_writefile, mock_mkdir, mock_rmdir):

    self.mock_logger.Reset()
    self.exp.results_directory = '/usr/local/crosperf-results'
    bench_run = self.exp.benchmark_runs[5]
    bench_path = '/usr/local/crosperf-results/' + filter(
        str.isalnum, bench_run.name)
    self.assertEqual(len(self.exp.benchmark_runs), 6)

    er = experiment_runner.ExperimentRunner(
        self.exp,
        json_report=False,
        using_schedv2=False,
        log=self.mock_logger,
        cmd_exec=self.mock_cmd_exec)

    # Test 1. Make sure nothing is done if _terminated is true.
    er._terminated = True
    er._StoreResults(self.exp)
    self.assertEqual(mock_cleanup.call_count, 0)
    self.assertEqual(mock_copy.call_count, 0)
    self.assertEqual(mock_report.call_count, 0)
    self.assertEqual(mock_writefile.call_count, 0)
    self.assertEqual(mock_mkdir.call_count, 0)
    self.assertEqual(mock_rmdir.call_count, 0)
    self.assertEqual(self.mock_logger.LogOutputCount, 0)

    # Test 2. _terminated is false; everything works properly.
    fake_result = Result(self.mock_logger, self.exp.labels[0], 'average',
                         'daisy1')
    for r in self.exp.benchmark_runs:
      r.result = fake_result
    er._terminated = False
    er._StoreResults(self.exp)
    self.assertEqual(mock_cleanup.call_count, 6)
    mock_cleanup.called_with(bench_run.benchmark.rm_chroot_tmp)
    self.assertEqual(mock_copy.call_count, 6)
    mock_copy.called_with(bench_path)
    self.assertEqual(mock_writefile.call_count, 3)
    self.assertEqual(len(mock_writefile.call_args_list), 3)
    first_args = mock_writefile.call_args_list[0]
    second_args = mock_writefile.call_args_list[1]
    self.assertEqual(first_args[0][0],
                     '/usr/local/crosperf-results/experiment.exp')
    self.assertEqual(second_args[0][0],
                     '/usr/local/crosperf-results/results.html')
    self.assertEqual(mock_mkdir.call_count, 1)
    mock_mkdir.called_with('/usr/local/crosperf-results')
    self.assertEqual(mock_rmdir.call_count, 1)
    mock_rmdir.called_with('/usr/local/crosperf-results')
    self.assertEqual(self.mock_logger.LogOutputCount, 4)
    self.assertEqual(self.mock_logger.output_msgs, [
        'Storing experiment file in /usr/local/crosperf-results.',
        'Storing results report in /usr/local/crosperf-results.',
        'Storing email message body in /usr/local/crosperf-results.',
        'Storing results of each benchmark run.'
    ])


if __name__ == '__main__':
  unittest.main()
