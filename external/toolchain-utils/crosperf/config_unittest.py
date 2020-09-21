#!/usr/bin/env python2
#
# Copyright 2014 Google Inc. All Rights Reserved.
"""Unit tests for config.py"""

from __future__ import print_function

import config

import unittest


class ConfigTestCase(unittest.TestCase):
  """Class for the config unit tests."""

  def test_config(self):
    # Verify that config exists, that it's a dictionary, and that it's
    # empty.
    self.assertTrue(type(config.config) is dict)
    self.assertEqual(len(config.config), 0)

    # Verify that attempting to get a non-existant key out of the
    # dictionary returns None.
    self.assertIsNone(config.GetConfig('rabbit'))
    self.assertIsNone(config.GetConfig('key1'))

    config.AddConfig('key1', 16)
    config.AddConfig('key2', 32)
    config.AddConfig('key3', 'third value')

    # Verify that after 3 calls to AddConfig we have 3 values in the
    # dictionary.
    self.assertEqual(len(config.config), 3)

    # Verify that GetConfig works and gets the expected values.
    self.assertIs(config.GetConfig('key2'), 32)
    self.assertIs(config.GetConfig('key3'), 'third value')
    self.assertIs(config.GetConfig('key1'), 16)

    # Re-set config.
    config.config.clear()

    # Verify that config exists, that it's a dictionary, and that it's
    # empty.
    self.assertTrue(type(config.config) is dict)
    self.assertEqual(len(config.config), 0)


if __name__ == '__main__':
  unittest.main()
