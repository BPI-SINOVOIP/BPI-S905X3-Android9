#!/usr/bin/env python2
#
# Copyright 2014 Google Inc. All Rights Reserved.
"""Unittest for crosperf."""

from __future__ import print_function

import argparse
import StringIO

import unittest

import crosperf
import settings_factory
import experiment_file

EXPERIMENT_FILE_1 = """
  board: x86-alex
  remote: chromeos-alex3
  perf_args: record -a -e cycles
  benchmark: PageCycler {
    iterations: 3
  }

  image1 {
    chromeos_image: /usr/local/google/cros_image1.bin
  }

  image2 {
    remote: chromeos-lumpy1
    chromeos_image: /usr/local/google/cros_image2.bin
  }
  """


class CrosperfTest(unittest.TestCase):
  """Crosperf test class."""

  def setUp(self):
    input_file = StringIO.StringIO(EXPERIMENT_FILE_1)
    self.exp_file = experiment_file.ExperimentFile(input_file)

  def test_convert_options_to_settings(self):
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-l',
        '--log_dir',
        dest='log_dir',
        default='',
        help='The log_dir, default is under '
        '<crosperf_logs>/logs')
    crosperf.SetupParserOptions(parser)
    argv = ['crosperf/crosperf.py', 'temp.exp', '--rerun=True']
    options, _ = parser.parse_known_args(argv)
    settings = crosperf.ConvertOptionsToSettings(options)
    self.assertIsNotNone(settings)
    self.assertIsInstance(settings, settings_factory.GlobalSettings)
    self.assertEqual(len(settings.fields), 25)
    self.assertTrue(settings.GetField('rerun'))
    argv = ['crosperf/crosperf.py', 'temp.exp']
    options, _ = parser.parse_known_args(argv)
    settings = crosperf.ConvertOptionsToSettings(options)
    self.assertFalse(settings.GetField('rerun'))


if __name__ == '__main__':
  unittest.main()
