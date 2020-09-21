# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Iterative flags elimination.

Part of the Chrome build flags optimization.

This module implements the flag iterative elimination algorithm (IE) adopted
from the paper
Z. Pan et al. Fast and Effective Orchestration of Compiler Optimizations for
Automatic Performance Tuning.

IE begins with the base line that turns on all the optimizations flags and
setting the numeric flags to their highest values. IE turns off the one boolean
flag or lower the value of a numeric flag with the most negative effect from the
baseline. This process repeats with all remaining flags, until none of them
causes performance degradation. The complexity of IE is O(n^2).

For example, -fstrict-aliasing and -ftree-vectorize. The base line is
b=[-fstrict-aliasing, -ftree-vectorize]. The two tasks in the first iteration
are t0=[-fstrict-aliasing] and t1=[-ftree-vectorize]. The algorithm compares b
with t0 and t1, respectively, and see whether setting the numeric flag with a
lower value or removing the boolean flag -fstrict-aliasing produce a better
fitness value.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

import flags
from generation import Generation
import task


def _DecreaseFlag(flags_dict, spec):
  """Decrease the value of the flag that has the specification spec.

  If the flag that contains the spec is a boolean flag, it is eliminated.
  Otherwise the flag is a numeric flag, its value will be reduced by one.

  Args:
    flags_dict: The dictionary containing the original flags whose neighbors are
      to be explored.
    spec: The spec in the flags_dict is to be changed.

  Returns:
    Dictionary of neighbor flag that is only different from the original
    dictionary by the spec.
  """

  # The specification must be held by one of the flags.
  assert spec in flags_dict

  # The results this method returns.
  results = flags_dict.copy()

  # This method searches for a pattern [start-end] in the spec. If the spec
  # contains this pattern, it is a numeric flag. Otherwise it is a boolean flag.
  # For example, -finline-limit=[1-1000] is a numeric flag and -falign-jumps is
  # a boolean flag.
  numeric_flag_match = flags.Search(spec)

  if numeric_flag_match:
    # numeric flag
    val = results[spec].GetValue()

    # If the value of the flag is the lower boundary of the specification, this
    # flag will be turned off. Because it already contains the lowest value and
    # can not be decreased any more.
    if val == int(numeric_flag_match.group('start')):
      # Turn off the flag. A flag is turned off if it is not presented in the
      # flags_dict.
      del results[spec]
    else:
      results[spec] = flags.Flag(spec, val - 1)
  else:
    # Turn off the flag. A flag is turned off if it is not presented in the
    # flags_dict.
    del results[spec]

  return results


class IterativeEliminationGeneration(Generation):
  """The negative flag iterative elimination algorithm."""

  def __init__(self, exe_set, parent_task):
    """Set up the base line parent task.

    The parent task is the base line against which the new tasks are compared.
    The new tasks are only different from the base line from one flag f by
    either turning this flag f off, or lower the flag value by 1.
    If a new task is better than the base line, one flag is identified that
    gives degradation. The flag that give the worst degradation will be removed
    or lower the value by 1 in the base in each iteration.

    Args:
      exe_set: A set of tasks to be run. Each one only differs from the
        parent_task by one flag.
      parent_task: The base line task, against which the new tasks in exe_set
        are compared.
    """

    Generation.__init__(self, exe_set, None)
    self._parent_task = parent_task

  def IsImproved(self):
    """Whether any new task has improvement upon the parent task."""

    parent = self._parent_task
    # Whether there is any new task that has improvement over the parent base
    # line task.
    for curr in [curr for curr in self.Pool() if curr != parent]:
      if curr.IsImproved(parent):
        return True

    return False

  def Next(self, cache):
    """Find out the flag that gives the worst degradation.

    Found out the flag that gives the worst degradation. Turn that flag off from
    the base line and use the new base line for the new generation.

    Args:
      cache: A set of tasks that have been generated before.

    Returns:
      A set of new generations.
    """
    parent_task = self._parent_task

    # Find out the task that gives the worst degradation.
    worst_task = parent_task

    for curr in [curr for curr in self.Pool() if curr != parent_task]:
      # The method IsImproved, which is supposed to be called before, ensures
      # that there is at least a task that improves upon the parent_task.
      if curr.IsImproved(worst_task):
        worst_task = curr

    assert worst_task != parent_task

    # The flags_set of the worst task.
    work_flags_set = worst_task.GetFlags().GetFlags()

    results = set([])

    # If the flags_set contains no flag, i.e., all the flags have been
    # eliminated, the algorithm stops.
    if not work_flags_set:
      return []

    # Turn of the remaining flags one by one for the next generation.
    for spec in work_flags_set:
      flag_set = flags.FlagSet(_DecreaseFlag(work_flags_set, spec).values())
      new_task = task.Task(flag_set)
      if new_task not in cache:
        results.add(new_task)

    return [IterativeEliminationGeneration(results, worst_task)]


class IterativeEliminationFirstGeneration(IterativeEliminationGeneration):
  """The first iteration of the iterative elimination algorithm.

  The first iteration also evaluates the base line task. The base line tasks in
  the subsequent iterations have been evaluated. Therefore,
  IterativeEliminationGeneration does not include the base line task in the
  execution set.
  """

  def IsImproved(self):
    # Find out the base line task in the execution set.
    parent = next(task for task in self.Pool() if task == self._parent_task)
    self._parent_task = parent

    return IterativeEliminationGeneration.IsImproved(self)
