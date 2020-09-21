# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Steering stage unittest.

Part of the Chrome build flags optimization.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

import multiprocessing
import unittest

from generation import Generation
from mock_task import IdentifierMockTask
import pipeline_process
import steering

# Pick an integer at random.
STEERING_TEST_STAGE = -8

# The number of generations to be used to do the testing.
NUMBER_OF_GENERATIONS = 20

# The number of tasks to be put in each generation to be tested.
NUMBER_OF_TASKS = 20

# The stride of permutation used to shuffle the input list of tasks. Should be
# relatively prime with NUMBER_OF_TASKS.
STRIDE = 7


class MockGeneration(Generation):
  """This class emulates an actual generation.

  It will output the next_generations when the method Next is called. The
  next_generations is initiated when the MockGeneration instance is constructed.
  """

  def __init__(self, tasks, next_generations):
    """Set up the next generations for this task.

    Args:
      tasks: A set of tasks to be run.
      next_generations: A list of generations as the next generation of the
        current generation.
    """
    Generation.__init__(self, tasks, None)
    self._next_generations = next_generations

  def Next(self, _):
    return self._next_generations

  def IsImproved(self):
    if self._next_generations:
      return True
    return False


class SteeringTest(unittest.TestCase):
  """This class test the steering method.

  The steering algorithm should return if there is no new task in the initial
  generation. The steering algorithm should send all the tasks to the next stage
  and should terminate once there is no pending generation. A generation is
  pending if it contains pending task. A task is pending if its (test) result
  is not ready.
  """

  def testSteering(self):
    """Test that the steering algorithm processes all the tasks properly.

    Test that the steering algorithm sends all the tasks to the next stage. Test
    that the steering algorithm terminates once all the tasks have been
    processed, i.e., the results for the tasks are all ready.
    """

    # A list of generations used to test the steering stage.
    generations = []

    task_index = 0
    previous_generations = None

    # Generate a sequence of generations to be tested. Each generation will
    # output the next generation in reverse order of the list when the "Next"
    # method is called.
    for _ in range(NUMBER_OF_GENERATIONS):
      # Use a consecutive sequence of numbers as identifiers for the set of
      # tasks put into a generation.
      test_ranges = range(task_index, task_index + NUMBER_OF_TASKS)
      tasks = [IdentifierMockTask(STEERING_TEST_STAGE, t) for t in test_ranges]
      steering_tasks = set(tasks)

      # Let the previous generation as the offspring generation of the current
      # generation.
      current_generation = MockGeneration(steering_tasks, previous_generations)
      generations.insert(0, current_generation)
      previous_generations = [current_generation]

      task_index += NUMBER_OF_TASKS

    # If there is no generation at all, the unittest returns right away.
    if not current_generation:
      return

    # Set up the input and result queue for the steering method.
    manager = multiprocessing.Manager()
    input_queue = manager.Queue()
    result_queue = manager.Queue()

    steering_process = multiprocessing.Process(
        target=steering.Steering,
        args=(set(), [current_generation], input_queue, result_queue))
    steering_process.start()

    # Test that each generation is processed properly. I.e., the generations are
    # processed in order.
    while generations:
      generation = generations.pop(0)
      tasks = [task for task in generation.Pool()]

      # Test that all the tasks are processed once and only once.
      while tasks:
        task = result_queue.get()

        assert task in tasks
        tasks.remove(task)

        input_queue.put(task)

    task = result_queue.get()

    # Test that the steering algorithm returns properly after processing all
    # the generations.
    assert task == pipeline_process.POISONPILL

    steering_process.join()

  def testCache(self):
    """The steering algorithm returns immediately if there is no new tasks.

    If all the new tasks have been cached before, the steering algorithm does
    not have to execute these tasks again and thus can terminate right away.
    """

    # Put a set of tasks in the cache and add this set to initial generation.
    test_ranges = range(NUMBER_OF_TASKS)
    tasks = [IdentifierMockTask(STEERING_TEST_STAGE, t) for t in test_ranges]
    steering_tasks = set(tasks)

    current_generation = MockGeneration(steering_tasks, None)

    # Set up the input and result queue for the steering method.
    manager = multiprocessing.Manager()
    input_queue = manager.Queue()
    result_queue = manager.Queue()

    steering_process = multiprocessing.Process(
        target=steering.Steering,
        args=(steering_tasks, [current_generation], input_queue, result_queue))

    steering_process.start()

    # Test that the steering method returns right away.
    assert result_queue.get() == pipeline_process.POISONPILL
    steering_process.join()


if __name__ == '__main__':
  unittest.main()
