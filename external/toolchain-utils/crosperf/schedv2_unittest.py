#!/usr/bin/env python2

# Copyright 2015 Google Inc. All Rights Reserved.
"""This contains the unit tests for the new Crosperf task scheduler."""

from __future__ import print_function

import mock
import unittest
import StringIO

import benchmark_run
import test_flag
from experiment_factory import ExperimentFactory
from experiment_file import ExperimentFile
from cros_utils.command_executer import CommandExecuter
from experiment_runner_unittest import FakeLogger
from schedv2 import Schedv2

EXPERIMENT_FILE_1 = """\
board: daisy
remote: chromeos-daisy1.cros chromeos-daisy2.cros

benchmark: kraken {
  suite: telemetry_Crosperf
  iterations: 3
}

image1 {
  chromeos_image: /chromeos/src/build/images/daisy/latest/cros_image1.bin
  remote: chromeos-daisy3.cros
}

image2 {
  chromeos_image: /chromeos/src/build/imaages/daisy/latest/cros_image2.bin
  remote: chromeos-daisy4.cros chromeos-daisy5.cros
}
"""

EXPERIMENT_FILE_WITH_FORMAT = """\
board: daisy
remote: chromeos-daisy1.cros chromeos-daisy2.cros

benchmark: kraken {{
  suite: telemetry_Crosperf
  iterations: {kraken_iterations}
}}

image1 {{
  chromeos_image: /chromeos/src/build/images/daisy/latest/cros_image1.bin
  remote: chromeos-daisy3.cros
}}

image2 {{
  chromeos_image: /chromeos/src/build/imaages/daisy/latest/cros_image2.bin
  remote: chromeos-daisy4.cros chromeos-daisy5.cros
}}
"""


class Schedv2Test(unittest.TestCase):
  """Class for setting up and running the unit tests."""

  def setUp(self):
    self.exp = None

  mock_logger = FakeLogger()
  mock_cmd_exec = mock.Mock(spec=CommandExecuter)

  @mock.patch('benchmark_run.BenchmarkRun', new=benchmark_run.MockBenchmarkRun)
  def _make_fake_experiment(self, expstr):
    """Create fake experiment from string.

        Note - we mock out BenchmarkRun in this step.
    """
    experiment_file = ExperimentFile(StringIO.StringIO(expstr))
    experiment = ExperimentFactory().GetExperiment(
        experiment_file, working_directory='', log_dir='')
    return experiment

  def test_remote(self):
    """Test that remotes in labels are aggregated into experiment.remote."""

    self.exp = self._make_fake_experiment(EXPERIMENT_FILE_1)
    self.exp.log_level = 'verbose'
    my_schedv2 = Schedv2(self.exp)
    self.assertFalse(my_schedv2.is_complete())
    self.assertIn('chromeos-daisy1.cros', self.exp.remote)
    self.assertIn('chromeos-daisy2.cros', self.exp.remote)
    self.assertIn('chromeos-daisy3.cros', self.exp.remote)
    self.assertIn('chromeos-daisy4.cros', self.exp.remote)
    self.assertIn('chromeos-daisy5.cros', self.exp.remote)

  def test_unreachable_remote(self):
    """Test unreachable remotes are removed from experiment and label."""

    def MockIsReachable(cm):
      return (cm.name != 'chromeos-daisy3.cros' and
              cm.name != 'chromeos-daisy5.cros')

    with mock.patch(
        'machine_manager.MockCrosMachine.IsReachable', new=MockIsReachable):
      self.exp = self._make_fake_experiment(EXPERIMENT_FILE_1)
      self.assertIn('chromeos-daisy1.cros', self.exp.remote)
      self.assertIn('chromeos-daisy2.cros', self.exp.remote)
      self.assertNotIn('chromeos-daisy3.cros', self.exp.remote)
      self.assertIn('chromeos-daisy4.cros', self.exp.remote)
      self.assertNotIn('chromeos-daisy5.cros', self.exp.remote)

      for l in self.exp.labels:
        if l.name == 'image2':
          self.assertNotIn('chromeos-daisy5.cros', l.remote)
          self.assertIn('chromeos-daisy4.cros', l.remote)
        elif l.name == 'image1':
          self.assertNotIn('chromeos-daisy3.cros', l.remote)

  @mock.patch('schedv2.BenchmarkRunCacheReader')
  def test_BenchmarkRunCacheReader_1(self, reader):
    """Test benchmarkrun set is split into 5 segments."""

    self.exp = self._make_fake_experiment(
        EXPERIMENT_FILE_WITH_FORMAT.format(kraken_iterations=9))
    my_schedv2 = Schedv2(self.exp)
    self.assertFalse(my_schedv2.is_complete())
    # We have 9 * 2 == 18 brs, we use 5 threads, each reading 4, 4, 4,
    # 4, 2 brs respectively.
    # Assert that BenchmarkRunCacheReader() is called 5 times.
    self.assertEquals(reader.call_count, 5)
    # reader.call_args_list[n] - nth call.
    # reader.call_args_list[n][0] - positioned args in nth call.
    # reader.call_args_list[n][0][1] - the 2nd arg in nth call,
    # that is 'br_list' in 'schedv2.BenchmarkRunCacheReader'.
    self.assertEquals(len(reader.call_args_list[0][0][1]), 4)
    self.assertEquals(len(reader.call_args_list[1][0][1]), 4)
    self.assertEquals(len(reader.call_args_list[2][0][1]), 4)
    self.assertEquals(len(reader.call_args_list[3][0][1]), 4)
    self.assertEquals(len(reader.call_args_list[4][0][1]), 2)

  @mock.patch('schedv2.BenchmarkRunCacheReader')
  def test_BenchmarkRunCacheReader_2(self, reader):
    """Test benchmarkrun set is split into 4 segments."""

    self.exp = self._make_fake_experiment(
        EXPERIMENT_FILE_WITH_FORMAT.format(kraken_iterations=8))
    my_schedv2 = Schedv2(self.exp)
    self.assertFalse(my_schedv2.is_complete())
    # We have 8 * 2 == 16 brs, we use 4 threads, each reading 4 brs.
    self.assertEquals(reader.call_count, 4)
    self.assertEquals(len(reader.call_args_list[0][0][1]), 4)
    self.assertEquals(len(reader.call_args_list[1][0][1]), 4)
    self.assertEquals(len(reader.call_args_list[2][0][1]), 4)
    self.assertEquals(len(reader.call_args_list[3][0][1]), 4)

  @mock.patch('schedv2.BenchmarkRunCacheReader')
  def test_BenchmarkRunCacheReader_3(self, reader):
    """Test benchmarkrun set is split into 2 segments."""

    self.exp = self._make_fake_experiment(
        EXPERIMENT_FILE_WITH_FORMAT.format(kraken_iterations=3))
    my_schedv2 = Schedv2(self.exp)
    self.assertFalse(my_schedv2.is_complete())
    # We have 3 * 2 == 6 brs, we use 2 threads.
    self.assertEquals(reader.call_count, 2)
    self.assertEquals(len(reader.call_args_list[0][0][1]), 3)
    self.assertEquals(len(reader.call_args_list[1][0][1]), 3)

  @mock.patch('schedv2.BenchmarkRunCacheReader')
  def test_BenchmarkRunCacheReader_4(self, reader):
    """Test benchmarkrun set is not splitted."""

    self.exp = self._make_fake_experiment(
        EXPERIMENT_FILE_WITH_FORMAT.format(kraken_iterations=1))
    my_schedv2 = Schedv2(self.exp)
    self.assertFalse(my_schedv2.is_complete())
    # We have 1 * 2 == 2 br, so only 1 instance.
    self.assertEquals(reader.call_count, 1)
    self.assertEquals(len(reader.call_args_list[0][0][1]), 2)

  def test_cachehit(self):
    """Test cache-hit and none-cache-hit brs are properly organized."""

    def MockReadCache(br):
      br.cache_hit = (br.label.name == 'image2')

    with mock.patch(
        'benchmark_run.MockBenchmarkRun.ReadCache', new=MockReadCache):
      # We have 2 * 30 brs, half of which are put into _cached_br_list.
      self.exp = self._make_fake_experiment(
          EXPERIMENT_FILE_WITH_FORMAT.format(kraken_iterations=30))
      my_schedv2 = Schedv2(self.exp)
      self.assertEquals(len(my_schedv2.get_cached_run_list()), 30)
      # The non-cache-hit brs are put into Schedv2._label_brl_map.
      self.assertEquals(
          reduce(lambda a, x: a + len(x[1]),
                 my_schedv2.get_label_map().iteritems(), 0), 30)

  def test_nocachehit(self):
    """Test no cache-hit."""

    def MockReadCache(br):
      br.cache_hit = False

    with mock.patch(
        'benchmark_run.MockBenchmarkRun.ReadCache', new=MockReadCache):
      # We have 2 * 30 brs, none of which are put into _cached_br_list.
      self.exp = self._make_fake_experiment(
          EXPERIMENT_FILE_WITH_FORMAT.format(kraken_iterations=30))
      my_schedv2 = Schedv2(self.exp)
      self.assertEquals(len(my_schedv2.get_cached_run_list()), 0)
      # The non-cache-hit brs are put into Schedv2._label_brl_map.
      self.assertEquals(
          reduce(lambda a, x: a + len(x[1]),
                 my_schedv2.get_label_map().iteritems(), 0), 60)


if __name__ == '__main__':
  test_flag.SetTestMode(True)
  unittest.main()
