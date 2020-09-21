# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Generation unittest.

Part of the Chrome build flags optimization.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

import random
import unittest

from generation import Generation
from mock_task import IdentifierMockTask

# Pick an integer at random.
TEST_STAGE = -125

# The number of tasks to be put in a generation to be tested.
NUM_TASKS = 20

# The stride of permutation used to shuffle the input list of tasks. Should be
# relatively prime with NUM_TASKS.
STRIDE = 7


class GenerationTest(unittest.TestCase):
  """This class test the Generation class.

  Given a set of tasks in the generation, if there is any task that is pending,
  then the Done method will return false, and true otherwise.
  """

  def testDone(self):
    """"Test the Done method.

    Produce a generation with a set of tasks. Set the cost of the task one by
    one and verify that the Done method returns false before setting the cost
    for all the tasks. After the costs of all the tasks are set, the Done method
    should return true.
    """

    random.seed(0)

    testing_tasks = range(NUM_TASKS)

    # The tasks for the generation to be tested.
    tasks = [IdentifierMockTask(TEST_STAGE, t) for t in testing_tasks]

    gen = Generation(set(tasks), None)

    # Permute the list.
    permutation = [(t * STRIDE) % NUM_TASKS for t in range(NUM_TASKS)]
    permuted_tasks = [testing_tasks[index] for index in permutation]

    # The Done method of the Generation should return false before all the tasks
    # in the permuted list are set.
    for testing_task in permuted_tasks:
      assert not gen.Done()

      # Mark a task as done by calling the UpdateTask method of the generation.
      # Send the generation the task as well as its results.
      gen.UpdateTask(IdentifierMockTask(TEST_STAGE, testing_task))

    # The Done method should return true after all the tasks in the permuted
    # list is set.
    assert gen.Done()


if __name__ == '__main__':
  unittest.main()
