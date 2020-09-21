#!/usr/bin/env python2
#
# Copyright 2014 Google Inc.  All Rights Reserved
"""Test translation of xbuddy names."""

from __future__ import print_function

import argparse
import sys

import download_images

#On May 1, 2014:
#latest         : lumpy-release/R34-5500.132.0
#latest-beta    : lumpy-release/R35-5712.43.0
#latest-official: lumpy-release/R36-5814.0.0
#latest-dev     : lumpy-release/R36-5814.0.0
#latest-canary  : lumpy-release/R36-5814.0.0


class ImageDownloaderBuildIDTest(object):
  """Test translation of xbuddy names."""

  def __init__(self):
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-c',
        '--chromeos_root',
        dest='chromeos_root',
        help='Directory containing ChromeOS root.')

    options = parser.parse_known_args(sys.argv[1:])[0]
    if options.chromeos_root is None:
      self._usage(parser, '--chromeos_root must be set')
    self.chromeos_root = options.chromeos_root
    self.tests_passed = 0
    self.tests_run = 0
    self.tests_failed = 0

  def _usage(self, parser, message):
    print('ERROR: ' + message)
    parser.print_help()
    sys.exit(0)

  def print_test_status(self):
    print('----------------------------------------\n')
    print('Tests attempted: %d' % self.tests_run)
    print('Tests passed:    %d' % self.tests_passed)
    print('Tests failed:    %d' % self.tests_failed)
    print('\n----------------------------------------')

  def assert_failure(self, msg):
    print('Assert failure: %s' % msg)
    self.print_test_status()
    sys.exit(1)

  def assertIsNotNone(self, arg, arg_name):
    if arg == None:
      self.tests_failed = self.tests_failed + 1
      self.assert_failure('%s is not None' % arg_name)

  def assertNotEqual(self, arg1, arg2, arg1_name, arg2_name):
    if arg1 == arg2:
      self.tests_failed = self.tests_failed + 1
      self.assert_failure('%s is not NotEqual to %s' % (arg1_name, arg2_name))

  def assertEqual(self, arg1, arg2, arg1_name, arg2_name):
    if arg1 != arg2:
      self.tests_failed = self.tests_failed + 1
      self.assert_failure('%s is not Equal to %s' % (arg1_name, arg2_name))

  def test_one_id(self, downloader, test_id, result_string, exact_match):
    print("Translating '%s'" % test_id)
    self.tests_run = self.tests_run + 1

    result = downloader.GetBuildID(self.chromeos_root, test_id)
    # Verify that we got a build id back.
    self.assertIsNotNone(result, 'result')

    # Verify that the result either contains or exactly matches the
    # result_string, depending on the exact_match argument.
    if exact_match:
      self.assertEqual(result, result_string, 'result', result_string)
    else:
      self.assertNotEqual(result.find(result_string), -1, 'result.find', '-1')
    self.tests_passed = self.tests_passed + 1

  def test_get_build_id(self):
    """Test that the actual translating of xbuddy names is working properly."""
    downloader = download_images.ImageDownloader(log_level='quiet')

    self.test_one_id(downloader, 'remote/lumpy/latest-dev', 'lumpy-release/R',
                     False)
    self.test_one_id(downloader,
                     'remote/trybot-lumpy-release-afdo-use/R35-5672.0.0-b86',
                     'trybot-lumpy-release-afdo-use/R35-5672.0.0-b86', True)
    self.test_one_id(downloader, 'remote/lumpy-release/R35-5672.0.0',
                     'lumpy-release/R35-5672.0.0', True)
    self.test_one_id(downloader, 'remote/lumpy/latest-dev', 'lumpy-release/R',
                     False)
    self.test_one_id(downloader, 'remote/lumpy/latest-official',
                     'lumpy-release/R', False)
    self.test_one_id(downloader, 'remote/lumpy/latest-beta', 'lumpy-release/R',
                     False)

    self.print_test_status()


if __name__ == '__main__':
  tester = ImageDownloaderBuildIDTest()
  tester.test_get_build_id()
