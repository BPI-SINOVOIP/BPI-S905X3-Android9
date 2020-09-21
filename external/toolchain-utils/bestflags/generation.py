# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""A generation of a set of tasks.

Part of the Chrome build flags optimization.

This module contains the base generation class. This class contains the tasks of
this current generation. The generation will not evolve to the next generation
until all the tasks in this generation are done executing. It also contains a
set of tasks that could potentially be used to generate the next generation,
e.g., in the genetic algorithm, a set of good species will be kept to evolve to
the successive generations. For the hill climbing algorithm example, the
candidate_pool will contain a current task t being evaluated and the exe_set
will contains all the task t's neighbor.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'


class NoneOverridingError(Exception):
  """Define an Exception class for subclasses not overriding certain methods."""
  pass


class Generation(object):
  """A generation of a framework run.

  The base class of generation. Concrete subclasses, e.g., GAGeneration should
  override the Next and IsImproved method to implement algorithm specific
  applications.
  """

  def __init__(self, exe_set, candidate_pool):
    """Set up the tasks set of this generation.

    Args:
      exe_set: A set of tasks to be run.
      candidate_pool: A set of tasks to be considered to be used to generate the
        next generation.
    """

    self._exe_set = exe_set
    self._candidate_pool = candidate_pool

    # Keeping the record of how many tasks are pending. Pending tasks are the
    # ones that have been sent out to the next stage for execution but have not
    # finished. A generation is not ready for the reproduction of the new
    # generations until all its pending tasks have been executed.
    self._pending = len(exe_set)

  def CandidatePool(self):
    """Return the candidate tasks of this generation."""

    return self._candidate_pool

  def Pool(self):
    """Return the task set of this generation."""

    return self._exe_set

  def Done(self):
    """All the tasks in this generation are done.

    Returns:
      True if all the tasks have been executed. That is the number of pending
      task is 0.
    """

    return self._pending == 0

  def UpdateTask(self, task):
    """Match a task t in this generation that is equal to the input task.

    This method is called when the input task has just finished execution. This
    method finds out whether there is a pending task t in the current generation
    that is the same as the input task. Two tasks are the same if their flag
    options are the same. A task is pending if it has not been performed.
    If there is a pending task t that matches the input task, task t will be
    substituted with the input task in this generation. In that case, the input
    task, as well as its build and test results encapsulated in the task, will
    be stored in the current generation. These results could be used to produce
    the next generation.
    If there is a match, the current generation will have one less pending task.
    When there is no pending task, the generation can be used to produce the
    next generation.
    The caller of this function is responsible for not calling this method on
    the same task more than once.

    Args:
      task: A task that has its results ready.

    Returns:
      Whether the input task belongs to this generation.
    """

    # If there is a match, the input task belongs to this generation.
    if task not in self._exe_set:
      return False

    # Remove the place holder task in this generation and store the new input
    # task and its result.
    self._exe_set.remove(task)
    self._exe_set.add(task)

    # The current generation will have one less task to wait on.
    self._pending -= 1

    assert self._pending >= 0

    return True

  def IsImproved(self):
    """True if this generation has improvement upon its parent generation.

    Raises:
      NoneOverridingError: The subclass should override this method.
    """
    raise NoneOverridingError('Must be implemented by child class')

  def Next(self, _):
    """Calculate the next generation.

    This is the core of the framework implementation. It must be overridden by
    the concrete subclass to implement algorithm specific generations.

    Args:
      _: A set of tasks that have been generated before. The overridden method
        in the subclasses can use this so as not to generate task that has been
        generated before.

    Returns:
      A set of new generations.

    Raises:
      NoneOverridingError: The subclass should override this method.
    """

    raise NoneOverridingError('Must be implemented by child class')
