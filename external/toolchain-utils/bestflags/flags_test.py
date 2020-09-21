# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Unit tests for the classes in module 'flags'.

Part of the Chrome build flags optimization.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

import random
import sys
import unittest

from flags import Flag
from flags import FlagSet

# The number of tests to test.
NUM_TESTS = 20


class FlagTest(unittest.TestCase):
  """This class tests the Flag class."""

  def testInit(self):
    """The value generated should fall within start and end of the spec.

    If the value is not specified, the value generated should fall within start
    and end of the spec.
    """

    for _ in range(NUM_TESTS):
      start = random.randint(1, sys.maxint - 1)
      end = random.randint(start + 1, sys.maxint)

      spec = 'flag=[%s-%s]' % (start, end)

      test_flag = Flag(spec)

      value = test_flag.GetValue()

      # If the value is not specified when the flag is constructed, a random
      # value is chosen. This value should fall within start and end of the
      # spec.
      assert start <= value and value < end

  def testEqual(self):
    """Test the equal operator (==) of the flag.

    Two flags are equal if and only if their spec and value are equal.
    """

    tests = range(NUM_TESTS)

    # Two tasks having the same spec and value should be equivalent.
    for test in tests:
      assert Flag(str(test), test) == Flag(str(test), test)

    # Two tasks having different flag set should be different.
    for test in tests:
      flag = Flag(str(test), test)
      other_flag_sets = [other for other in tests if test != other]
      for other_test in other_flag_sets:
        assert flag != Flag(str(other_test), other_test)

  def testFormattedForUse(self):
    """Test the FormattedForUse method of the flag.

    The FormattedForUse replaces the string within the [] with the actual value.
    """

    for _ in range(NUM_TESTS):
      start = random.randint(1, sys.maxint - 1)
      end = random.randint(start + 1, sys.maxint)
      value = random.randint(start, end - 1)

      spec = 'flag=[%s-%s]' % (start, end)

      test_flag = Flag(spec, value)

      # For numeric flag, the FormattedForUse replaces the string within the []
      # with the actual value.
      test_value = test_flag.FormattedForUse()
      actual_value = 'flag=%s' % value

      assert test_value == actual_value

    for _ in range(NUM_TESTS):
      value = random.randint(1, sys.maxint - 1)

      test_flag = Flag('flag', value)

      # For boolean flag, the FormattedForUse returns the spec.
      test_value = test_flag.FormattedForUse()
      actual_value = 'flag'
      assert test_value == actual_value


class FlagSetTest(unittest.TestCase):
  """This class test the FlagSet class."""

  def testEqual(self):
    """Test the equal method of the Class FlagSet.

    Two FlagSet instances are equal if all their flags are equal.
    """

    flag_names = range(NUM_TESTS)

    # Two flag sets having the same flags should be equivalent.
    for flag_name in flag_names:
      spec = '%s' % flag_name

      assert FlagSet([Flag(spec)]) == FlagSet([Flag(spec)])

    # Two flag sets having different flags should be different.
    for flag_name in flag_names:
      spec = '%s' % flag_name
      flag_set = FlagSet([Flag(spec)])
      other_flag_sets = [other for other in flag_names if flag_name != other]
      for other_name in other_flag_sets:
        other_spec = '%s' % other_name
        assert flag_set != FlagSet([Flag(other_spec)])

  def testGetItem(self):
    """Test the get item method of the Class FlagSet.

    The flag set is also indexed by the specs. The flag set should return the
    appropriate flag given the spec.
    """

    tests = range(NUM_TESTS)

    specs = [str(spec) for spec in tests]
    flag_array = [Flag(spec) for spec in specs]

    flag_set = FlagSet(flag_array)

    # Created a dictionary of spec and flag, the flag set should return the flag
    # the same as this dictionary.
    spec_flag = dict(zip(specs, flag_array))

    for spec in spec_flag:
      assert flag_set[spec] == spec_flag[spec]

  def testContain(self):
    """Test the contain method of the Class FlagSet.

    The flag set is also indexed by the specs. The flag set should return true
    for spec if it contains a flag containing spec.
    """

    true_tests = range(NUM_TESTS)
    false_tests = range(NUM_TESTS, NUM_TESTS * 2)

    specs = [str(spec) for spec in true_tests]

    flag_set = FlagSet([Flag(spec) for spec in specs])

    for spec in specs:
      assert spec in flag_set

    for spec in false_tests:
      assert spec not in flag_set

  def testFormattedForUse(self):
    """Test the FormattedForUse method of the Class FlagSet.

    The output should be a sorted list of strings.
    """

    flag_names = range(NUM_TESTS)
    flag_names.reverse()
    flags = []
    result = []

    # Construct the flag set.
    for flag_name in flag_names:
      spec = '%s' % flag_name
      flags.append(Flag(spec))
      result.append(spec)

    flag_set = FlagSet(flags)

    # The results string should be sorted.
    assert sorted(result) == flag_set.FormattedForUse()


if __name__ == '__main__':
  unittest.main()
