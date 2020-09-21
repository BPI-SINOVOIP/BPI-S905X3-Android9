# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""A variation of the hill climbing algorithm.

Part of the Chrome build flags optimization.

This algorithm explores all the neighbors of the current task. If at least one
neighbor gives better performance than the current task, it explores the best
neighbor.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

from flags import FlagSet
import flags_util
from generation import Generation
from task import Task


class HillClimbingBestBranch(Generation):
  """A variation of the hill climbing algorithm.

  Given a task, it explores all its neighbors. Pick the best neighbor for the
  next iteration.
  """

  def __init__(self, exe_set, parents, specs):
    """Set up the tasks set of this generation.

    Args:
      exe_set: A set of tasks to be run.
      parents: A set of tasks to be used to check whether their neighbors have
        improved upon them.
      specs: A list of specs to explore. The spec specifies the flags that can
        be changed to find neighbors of a task.
    """

    Generation.__init__(self, exe_set, parents)
    self._specs = specs

    # This variable will be used, by the Next method, to generate the tasks for
    # the next iteration. This self._next_task contains the best task in the
    # current iteration and it will be set by the IsImproved method. The tasks
    # of the next iteration are the neighbor of self._next_task.
    self._next_task = None

  def IsImproved(self):
    """True if this generation has improvement over its parent generation.

    If this generation improves upon the previous generation, this method finds
    out the best task in this generation and sets it to _next_task for the
    method Next to use.

    Returns:
      True if the best neighbor improves upon the parent task.
    """

    # Find the best neighbor.
    best_task = None
    for task in self._exe_set:
      if not best_task or task.IsImproved(best_task):
        best_task = task

    if not best_task:
      return False

    # The first generation may not have parent generation.
    parents = list(self._candidate_pool)
    if parents:
      assert len(parents) == 1
      self._next_task = best_task
      # If the best neighbor improves upon the parent task.
      return best_task.IsImproved(parents[0])

    self._next_task = best_task
    return True

  def Next(self, cache):
    """Calculate the next generation.

    The best neighbor b of the current task is the parent of the next
    generation. The neighbors of b will be the set of tasks to be evaluated
    next.

    Args:
      cache: A set of tasks that have been generated before.

    Returns:
      A set of new generations.
    """

    # The best neighbor.
    current_task = self._next_task
    flag_set = current_task.GetFlags()

    # The neighbors of the best neighbor.
    children_tasks = set([])
    for spec in self._specs:
      for next_flag in flags_util.ClimbNext(flag_set.GetFlags(), spec):
        new_task = Task(FlagSet(next_flag.values()))

        if new_task not in cache:
          children_tasks.add(new_task)

    return [HillClimbingBestBranch(children_tasks, set([current_task]),
                                   self._specs)]
