# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Task unittest.

Part of the Chrome build flags optimization.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

import random
import sys
import unittest

import task
from task import Task

# The number of flags be tested.
NUM_FLAGS = 20

# The random build result values used to test get set result method.
RANDOM_BUILD_RESULT = 100

# The random test result values used to test get set result method.
RANDOM_TESTRESULT = 100


class MockFlagSet(object):
  """This class emulates a set of flags.

  It returns the flags and hash value, when the FormattedForUse method and the
  __hash__ method is called, respectively. These values are initialized when the
  MockFlagSet instance is constructed.
  """

  def __init__(self, flags=0, hash_value=-1):
    self._flags = flags
    self._hash_value = hash_value

  def __eq__(self, other):
    assert isinstance(other, MockFlagSet)
    return self._flags == other.FormattedForUse()

  def FormattedForUse(self):
    return self._flags

  def __hash__(self):
    return self._hash_value

  def GetHash(self):
    return self._hash_value


class TaskTest(unittest.TestCase):
  """This class test the Task class."""

  def testEqual(self):
    """Test the equal method of the task.

    Two tasks are equal if and only if their encapsulated flag_sets are equal.
    """

    flags = range(NUM_FLAGS)

    # Two tasks having the same flag set should be equivalent.
    flag_sets = [MockFlagSet(flag) for flag in flags]
    for flag_set in flag_sets:
      assert Task(flag_set) == Task(flag_set)

    # Two tasks having different flag set should be different.
    for flag_set in flag_sets:
      test_task = Task(flag_set)
      other_flag_sets = [flags for flags in flag_sets if flags != flag_set]
      for flag_set1 in other_flag_sets:
        assert test_task != Task(flag_set1)

  def testHash(self):
    """Test the hash method of the task.

    Two tasks are equal if and only if their encapsulated flag_sets are equal.
    """

    # Random identifier that is not relevant in this test.
    identifier = random.randint(-sys.maxint - 1, -1)

    flag_sets = [MockFlagSet(identifier, value) for value in range(NUM_FLAGS)]
    for flag_set in flag_sets:
      # The hash of a task is the same as the hash of its flag set.
      hash_task = Task(flag_set)
      hash_value = hash(hash_task)
      assert hash_value == flag_set.GetHash()

      # The hash of a task does not change.
      assert hash_value == hash(hash_task)

  def testGetIdentifier(self):
    """Test the get identifier method of the task.

    The get identifier method should returns the flag set in the build stage.
    """

    flag_sets = [MockFlagSet(flag) for flag in range(NUM_FLAGS)]
    for flag_set in flag_sets:
      identifier_task = Task(flag_set)

      identifier = identifier_task.GetIdentifier(task.BUILD_STAGE)

      # The task formats the flag set into a string.
      assert identifier == str(flag_set.FormattedForUse())

  def testGetSetResult(self):
    """Test the get and set result methods of the task.

    The get result method should return the same results as were set.
    """

    flag_sets = [MockFlagSet(flag) for flag in range(NUM_FLAGS)]
    for flag_set in flag_sets:
      result_task = Task(flag_set)

      # The get result method should return the same results as were set, in
      # build stage. Currently, the build result is a 5-element tuple containing
      # the checksum of the result image, the performance cost of the build, the
      # compilation image, the length of the build, and the length of the text
      # section of the build.
      result = tuple([random.randint(0, RANDOM_BUILD_RESULT) for _ in range(5)])
      result_task.SetResult(task.BUILD_STAGE, result)
      assert result == result_task.GetResult(task.BUILD_STAGE)

      # The checksum is the identifier of the test stage.
      identifier = result_task.GetIdentifier(task.TEST_STAGE)
      # The first element of the result tuple is the checksum.
      assert identifier == result[0]

      # The get result method should return the same results as were set, in
      # test stage.
      random_test_result = random.randint(0, RANDOM_TESTRESULT)
      result_task.SetResult(task.TEST_STAGE, random_test_result)
      test_result = result_task.GetResult(task.TEST_STAGE)
      assert test_result == random_test_result

  def testDone(self):
    """Test the done methods of the task.

    The done method should return false is the task has not perform and return
    true after the task is finished.
    """

    flags = range(NUM_FLAGS)

    flag_sets = [MockFlagSet(flag) for flag in flags]
    for flag_set in flag_sets:
      work_task = Task(flag_set)

      # The task has not been compiled nor tested.
      assert not work_task.Done(task.TEST_STAGE)
      assert not work_task.Done(task.BUILD_STAGE)

      # After the task has been compiled, it should indicate finished in BUILD
      # stage.
      result = tuple([random.randint(0, RANDOM_BUILD_RESULT) for _ in range(5)])
      work_task.SetResult(task.BUILD_STAGE, result)
      assert not work_task.Done(task.TEST_STAGE)
      assert work_task.Done(task.BUILD_STAGE)

      # After the task has been tested, it should indicate finished in TEST
      # stage.
      work_task.SetResult(task.TEST_STAGE, random.randint(0, RANDOM_TESTRESULT))
      assert work_task.Done(task.TEST_STAGE)
      assert work_task.Done(task.BUILD_STAGE)


if __name__ == '__main__':
  unittest.main()
