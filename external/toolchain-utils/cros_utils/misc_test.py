# Copyright 2012 Google Inc. All Rights Reserved.
"""Tests for misc."""

from __future__ import print_function

__author__ = 'asharif@google.com (Ahmad Sharif)'

# System modules
import unittest

# Local modules
import misc


class UtilsTest(unittest.TestCase):
  """Tests for misc."""

  def testGetFilenameFromString(self):
    string = 'a /b=c"d^$?\\'
    filename = misc.GetFilenameFromString(string)
    self.assertEqual(filename, 'a___bcd')

  def testPrependMergeEnv(self):
    var = 'USE'
    use_flags = 'hello 123'
    added_use_flags = 'bla bla'
    env_string = '%s=%r' % (var, use_flags)
    new_env_string = misc.MergeEnvStringWithDict(env_string,
                                                 {var: added_use_flags})
    expected_new_env = '%s=%r' % (var, ' '.join([added_use_flags, use_flags]))
    self.assertEqual(new_env_string, ' '.join([env_string, expected_new_env]))

  def testGetChromeOSVersionFromLSBVersion(self):
    versions_dict = {'2630.0.0': '22', '2030.0.0': '19'}
    f = misc.GetChromeOSVersionFromLSBVersion
    for k, v in versions_dict.items():
      self.assertEqual(f(k), 'R%s-%s' % (v, k))

  def testPostpendMergeEnv(self):
    var = 'USE'
    use_flags = 'hello 123'
    added_use_flags = 'bla bla'
    env_string = '%s=%r' % (var, use_flags)
    new_env_string = misc.MergeEnvStringWithDict(env_string,
                                                 {var: added_use_flags}, False)
    expected_new_env = '%s=%r' % (var, ' '.join([use_flags, added_use_flags]))
    self.assertEqual(new_env_string, ' '.join([env_string, expected_new_env]))


if __name__ == '__main__':
  unittest.main()
