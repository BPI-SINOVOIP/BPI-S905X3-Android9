#!/usr/bin/env python2

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Unit test for experiment_factory.py"""

from __future__ import print_function

import StringIO
import socket
import mock
import unittest

from cros_utils.file_utils import FileUtils

from experiment_factory import ExperimentFactory
from experiment_file import ExperimentFile
import test_flag
import benchmark
import experiment_factory
import settings_factory

EXPERIMENT_FILE_1 = """
  board: x86-alex
  remote: chromeos-alex3

  benchmark: PageCycler {
    iterations: 3
  }

  image1 {
    chromeos_image: /usr/local/google/cros_image1.bin
  }

  image2 {
    chromeos_image: /usr/local/google/cros_image2.bin
  }
  """

# pylint: disable=too-many-function-args


class ExperimentFactoryTest(unittest.TestCase):
  """Class for running experiment factory unittests."""

  def setUp(self):
    self.append_benchmark_call_args = []

  def testLoadExperimentFile1(self):
    experiment_file = ExperimentFile(StringIO.StringIO(EXPERIMENT_FILE_1))
    exp = ExperimentFactory().GetExperiment(
        experiment_file, working_directory='', log_dir='')
    self.assertEqual(exp.remote, ['chromeos-alex3'])

    self.assertEqual(len(exp.benchmarks), 1)
    self.assertEqual(exp.benchmarks[0].name, 'PageCycler')
    self.assertEqual(exp.benchmarks[0].test_name, 'PageCycler')
    self.assertEqual(exp.benchmarks[0].iterations, 3)

    self.assertEqual(len(exp.labels), 2)
    self.assertEqual(exp.labels[0].chromeos_image,
                     '/usr/local/google/cros_image1.bin')
    self.assertEqual(exp.labels[0].board, 'x86-alex')

  def test_append_benchmark_set(self):
    ef = ExperimentFactory()

    bench_list = []
    ef.AppendBenchmarkSet(bench_list, experiment_factory.telemetry_perfv2_tests,
                          '', 1, False, '', 'telemetry_Crosperf', False, 0,
                          False)
    self.assertEqual(
        len(bench_list), len(experiment_factory.telemetry_perfv2_tests))
    self.assertTrue(type(bench_list[0]) is benchmark.Benchmark)

    bench_list = []
    ef.AppendBenchmarkSet(bench_list,
                          experiment_factory.telemetry_pagecycler_tests, '', 1,
                          False, '', 'telemetry_Crosperf', False, 0, False)
    self.assertEqual(
        len(bench_list), len(experiment_factory.telemetry_pagecycler_tests))
    self.assertTrue(type(bench_list[0]) is benchmark.Benchmark)

    bench_list = []
    ef.AppendBenchmarkSet(bench_list,
                          experiment_factory.telemetry_toolchain_perf_tests, '',
                          1, False, '', 'telemetry_Crosperf', False, 0, False)
    self.assertEqual(
        len(bench_list), len(experiment_factory.telemetry_toolchain_perf_tests))
    self.assertTrue(type(bench_list[0]) is benchmark.Benchmark)

  @mock.patch.object(socket, 'gethostname')
  def test_get_experiment(self, mock_socket):

    test_flag.SetTestMode(False)
    self.append_benchmark_call_args = []

    def FakeAppendBenchmarkSet(bench_list, set_list, args, iters, rm_ch,
                               perf_args, suite, show_all):
      'Helper function for test_get_experiment'
      arg_list = [
          bench_list, set_list, args, iters, rm_ch, perf_args, suite, show_all
      ]
      self.append_benchmark_call_args.append(arg_list)

    def FakeGetDefaultRemotes(board):
      if not board:
        return []
      return ['fake_chromeos_machine1.cros', 'fake_chromeos_machine2.cros']

    def FakeGetXbuddyPath(build, autotest_dir, board, chroot, log_level):
      autotest_path = autotest_dir
      if not autotest_path:
        autotest_path = 'fake_autotest_path'
      if not build or not board or not chroot or not log_level:
        return '', autotest_path
      return 'fake_image_path', autotest_path

    ef = ExperimentFactory()
    ef.AppendBenchmarkSet = FakeAppendBenchmarkSet
    ef.GetDefaultRemotes = FakeGetDefaultRemotes

    label_settings = settings_factory.LabelSettings('image_label')
    benchmark_settings = settings_factory.BenchmarkSettings('bench_test')
    global_settings = settings_factory.GlobalSettings('test_name')

    label_settings.GetXbuddyPath = FakeGetXbuddyPath

    mock_experiment_file = ExperimentFile(StringIO.StringIO(''))
    mock_experiment_file.all_settings = []

    test_flag.SetTestMode(True)
    # Basic test.
    global_settings.SetField('name', 'unittest_test')
    global_settings.SetField('board', 'lumpy')
    global_settings.SetField('remote', '123.45.67.89 123.45.76.80')
    benchmark_settings.SetField('test_name', 'kraken')
    benchmark_settings.SetField('suite', 'telemetry_Crosperf')
    benchmark_settings.SetField('iterations', 1)
    label_settings.SetField(
        'chromeos_image',
        'chromeos/src/build/images/lumpy/latest/chromiumos_test_image.bin')
    label_settings.SetField('chrome_src', '/usr/local/google/home/chrome-top')
    label_settings.SetField('autotest_path', '/tmp/autotest')

    mock_experiment_file.global_settings = global_settings
    mock_experiment_file.all_settings.append(label_settings)
    mock_experiment_file.all_settings.append(benchmark_settings)
    mock_experiment_file.all_settings.append(global_settings)

    mock_socket.return_value = ''

    # First test. General test.
    exp = ef.GetExperiment(mock_experiment_file, '', '')
    self.assertEqual(exp.remote, ['123.45.67.89', '123.45.76.80'])
    self.assertEqual(exp.cache_conditions, [0, 2, 1])
    self.assertEqual(exp.log_level, 'average')

    self.assertEqual(len(exp.benchmarks), 1)
    self.assertEqual(exp.benchmarks[0].name, 'kraken')
    self.assertEqual(exp.benchmarks[0].test_name, 'kraken')
    self.assertEqual(exp.benchmarks[0].iterations, 1)
    self.assertEqual(exp.benchmarks[0].suite, 'telemetry_Crosperf')
    self.assertFalse(exp.benchmarks[0].show_all_results)

    self.assertEqual(len(exp.labels), 1)
    self.assertEqual(exp.labels[0].chromeos_image,
                     'chromeos/src/build/images/lumpy/latest/'
                     'chromiumos_test_image.bin')
    self.assertEqual(exp.labels[0].autotest_path, '/tmp/autotest')
    self.assertEqual(exp.labels[0].board, 'lumpy')

    # Second test: Remotes listed in labels.
    test_flag.SetTestMode(True)
    label_settings.SetField('remote', 'chromeos1.cros chromeos2.cros')
    exp = ef.GetExperiment(mock_experiment_file, '', '')
    self.assertEqual(exp.remote, [
        'chromeos1.cros', 'chromeos2.cros', '123.45.67.89', '123.45.76.80'
    ])

    # Third test: Automatic fixing of bad  logging_level param:
    global_settings.SetField('logging_level', 'really loud!')
    exp = ef.GetExperiment(mock_experiment_file, '', '')
    self.assertEqual(exp.log_level, 'verbose')

    # Fourth test: Setting cache conditions; only 1 remote with "same_machine"
    global_settings.SetField('rerun_if_failed', 'true')
    global_settings.SetField('rerun', 'true')
    global_settings.SetField('same_machine', 'true')
    global_settings.SetField('same_specs', 'true')

    self.assertRaises(Exception, ef.GetExperiment, mock_experiment_file, '', '')
    label_settings.SetField('remote', '')
    global_settings.SetField('remote', '123.45.67.89')
    exp = ef.GetExperiment(mock_experiment_file, '', '')
    self.assertEqual(exp.cache_conditions, [0, 2, 3, 4, 6, 1])

    # Fifth Test: Adding a second label; calling GetXbuddyPath; omitting all
    # remotes (Call GetDefaultRemotes).
    mock_socket.return_value = 'test.corp.google.com'
    global_settings.SetField('remote', '')
    global_settings.SetField('same_machine', 'false')

    label_settings_2 = settings_factory.LabelSettings('official_image_label')
    label_settings_2.SetField('chromeos_root', 'chromeos')
    label_settings_2.SetField('build', 'official-dev')
    label_settings_2.SetField('autotest_path', '')
    label_settings_2.GetXbuddyPath = FakeGetXbuddyPath

    mock_experiment_file.all_settings.append(label_settings_2)
    exp = ef.GetExperiment(mock_experiment_file, '', '')
    self.assertEqual(len(exp.labels), 2)
    self.assertEqual(exp.labels[1].chromeos_image, 'fake_image_path')
    self.assertEqual(exp.labels[1].autotest_path, 'fake_autotest_path')
    self.assertEqual(exp.remote, [
        'fake_chromeos_machine1.cros', 'fake_chromeos_machine2.cros'
    ])

  def test_get_default_remotes(self):
    board_list = [
        'x86-alex', 'lumpy', 'elm', 'parrot', 'daisy', 'peach_pit', 'peppy',
        'squawks'
    ]

    ef = ExperimentFactory()
    self.assertRaises(Exception, ef.GetDefaultRemotes, 'bad-board')

    # Verify that we have entries for every board, and that we get three
    # machines back for each board.
    for b in board_list:
      remotes = ef.GetDefaultRemotes(b)
      if b == 'elm':
        self.assertEqual(len(remotes), 2)
      else:
        self.assertEqual(len(remotes), 3)


if __name__ == '__main__':
  FileUtils.Configure(True)
  test_flag.SetTestMode(True)
  unittest.main()
