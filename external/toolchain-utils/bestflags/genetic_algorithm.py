# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""The hill genetic algorithm.

Part of the Chrome build flags optimization.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

import random

import flags
from flags import Flag
from flags import FlagSet
from generation import Generation
from task import Task


def CrossoverWith(first_flag, second_flag):
  """Get a crossed over gene.

  At present, this just picks either/or of these values.  However, it could be
  implemented as an integer maskover effort, if required.

  Args:
    first_flag: The first gene (Flag) to cross over with.
    second_flag: The second gene (Flag) to cross over with.

  Returns:
    A Flag that can be considered appropriately randomly blended between the
    first and second input flag.
  """

  return first_flag if random.randint(0, 1) else second_flag


def RandomMutate(specs, flag_set, mutation_rate):
  """Randomly mutate the content of a task.

  Args:
    specs: A list of spec from which the flag set is created.
    flag_set: The current flag set being mutated
    mutation_rate: What fraction of genes to mutate.

  Returns:
    A Genetic Task constructed by randomly mutating the input flag set.
  """

  results_flags = []

  for spec in specs:
    # Randomly choose whether this flag should be mutated.
    if random.randint(0, int(1 / mutation_rate)):
      continue

    # If the flag is not already in the flag set, it is added.
    if spec not in flag_set:
      results_flags.append(Flag(spec))
      continue

    # If the flag is already in the flag set, it is mutated.
    numeric_flag_match = flags.Search(spec)

    # The value of a numeric flag will be changed, and a boolean flag will be
    # dropped.
    if not numeric_flag_match:
      continue

    value = flag_set[spec].GetValue()

    # Randomly select a nearby value of the current value of the flag.
    rand_arr = [value]
    if value + 1 < int(numeric_flag_match.group('end')):
      rand_arr.append(value + 1)

    rand_arr.append(value - 1)
    value = random.sample(rand_arr, 1)[0]

    # If the value is smaller than the start of the spec, this flag will be
    # dropped.
    if value != int(numeric_flag_match.group('start')) - 1:
      results_flags.append(Flag(spec, value))

  return GATask(FlagSet(results_flags))


class GATask(Task):

  def __init__(self, flag_set):
    Task.__init__(self, flag_set)

  def ReproduceWith(self, other, specs, mutation_rate):
    """Reproduce with other FlagSet.

    Args:
      other: A FlagSet to reproduce with.
      specs: A list of spec from which the flag set is created.
      mutation_rate: one in mutation_rate flags will be mutated (replaced by a
        random version of the same flag, instead of one from either of the
        parents).  Set to 0 to disable mutation.

    Returns:
      A GA task made by mixing self with other.
    """

    # Get the flag dictionary.
    father_flags = self.GetFlags().GetFlags()
    mother_flags = other.GetFlags().GetFlags()

    # Flags that are common in both parents and flags that belong to only one
    # parent.
    self_flags = []
    other_flags = []
    common_flags = []

    # Find out flags that are common to both parent and flags that belong soly
    # to one parent.
    for self_flag in father_flags:
      if self_flag in mother_flags:
        common_flags.append(self_flag)
      else:
        self_flags.append(self_flag)

    for other_flag in mother_flags:
      if other_flag not in father_flags:
        other_flags.append(other_flag)

    # Randomly select flags that belong to only one parent.
    output_flags = [father_flags[f] for f in self_flags if random.randint(0, 1)]
    others = [mother_flags[f] for f in other_flags if random.randint(0, 1)]
    output_flags.extend(others)
    # Turn on flags that belong to both parent. Randomly choose the value of the
    # flag from either parent.
    for flag in common_flags:
      output_flags.append(CrossoverWith(father_flags[flag], mother_flags[flag]))

    # Mutate flags
    if mutation_rate:
      return RandomMutate(specs, FlagSet(output_flags), mutation_rate)

    return GATask(FlagSet(output_flags))


class GAGeneration(Generation):
  """The Genetic Algorithm."""

  # The value checks whether the algorithm has converged and arrives at a fixed
  # point. If STOP_THRESHOLD of generations have not seen any performance
  # improvement, the Genetic Algorithm stops.
  STOP_THRESHOLD = None

  # Number of tasks in each generation.
  NUM_CHROMOSOMES = None

  # The value checks whether the algorithm has converged and arrives at a fixed
  # point. If NUM_TRIALS of trials have been attempted to generate a new task
  # without a success, the Genetic Algorithm stops.
  NUM_TRIALS = None

  # The flags that can be used to generate new tasks.
  SPECS = None

  # What fraction of genes to mutate.
  MUTATION_RATE = 0

  @staticmethod
  def InitMetaData(stop_threshold, num_chromosomes, num_trials, specs,
                   mutation_rate):
    """Set up the meta data for the Genetic Algorithm.

    Args:
      stop_threshold: The number of generations, upon which no performance has
        seen, the Genetic Algorithm stops.
      num_chromosomes: Number of tasks in each generation.
      num_trials: The number of trials, upon which new task has been tried to
        generated without success, the Genetic Algorithm stops.
      specs: The flags that can be used to generate new tasks.
      mutation_rate: What fraction of genes to mutate.
    """

    GAGeneration.STOP_THRESHOLD = stop_threshold
    GAGeneration.NUM_CHROMOSOMES = num_chromosomes
    GAGeneration.NUM_TRIALS = num_trials
    GAGeneration.SPECS = specs
    GAGeneration.MUTATION_RATE = mutation_rate

  def __init__(self, tasks, parents, total_stucks):
    """Set up the meta data for the Genetic Algorithm.

    Args:
      tasks: A set of tasks to be run.
      parents: A set of tasks from which this new generation is produced. This
        set also contains the best tasks generated so far.
      total_stucks: The number of generations that have not seen improvement.
        The Genetic Algorithm will stop once the total_stucks equals to
        NUM_TRIALS defined in the GAGeneration class.
    """

    Generation.__init__(self, tasks, parents)
    self._total_stucks = total_stucks

  def IsImproved(self):
    """True if this generation has improvement upon its parent generation."""

    tasks = self.Pool()
    parents = self.CandidatePool()

    # The first generate does not have parents.
    if not parents:
      return True

    # Found out whether a task has improvement upon the best task in the
    # parent generation.
    best_parent = sorted(parents, key=lambda task: task.GetTestResult())[0]
    best_current = sorted(tasks, key=lambda task: task.GetTestResult())[0]

    # At least one task has improvement.
    if best_current.IsImproved(best_parent):
      self._total_stucks = 0
      return True

    # If STOP_THRESHOLD of generations have no improvement, the algorithm stops.
    if self._total_stucks >= GAGeneration.STOP_THRESHOLD:
      return False

    self._total_stucks += 1
    return True

  def Next(self, cache):
    """Calculate the next generation.

    Generate a new generation from the a set of tasks. This set contains the
      best set seen so far and the tasks executed in the parent generation.

    Args:
      cache: A set of tasks that have been generated before.

    Returns:
      A set of new generations.
    """

    target_len = GAGeneration.NUM_CHROMOSOMES
    specs = GAGeneration.SPECS
    mutation_rate = GAGeneration.MUTATION_RATE

    # Collect a set of size target_len of tasks. This set will be used to
    # produce a new generation of tasks.
    gen_tasks = [task for task in self.Pool()]

    parents = self.CandidatePool()
    if parents:
      gen_tasks.extend(parents)

    # A set of tasks that are the best. This set will be used as the parent
    # generation to produce the next generation.
    sort_func = lambda task: task.GetTestResult()
    retained_tasks = sorted(gen_tasks, key=sort_func)[:target_len]

    child_pool = set()
    for father in retained_tasks:
      num_trials = 0
      # Try num_trials times to produce a new child.
      while num_trials < GAGeneration.NUM_TRIALS:
        # Randomly select another parent.
        mother = random.choice(retained_tasks)
        # Cross over.
        child = mother.ReproduceWith(father, specs, mutation_rate)
        if child not in child_pool and child not in cache:
          child_pool.add(child)
          break
        else:
          num_trials += 1

    num_trials = 0

    while len(child_pool) < target_len and num_trials < GAGeneration.NUM_TRIALS:
      for keep_task in retained_tasks:
        # Mutation.
        child = RandomMutate(specs, keep_task.GetFlags(), mutation_rate)
        if child not in child_pool and child not in cache:
          child_pool.add(child)
          if len(child_pool) >= target_len:
            break
        else:
          num_trials += 1

    # If NUM_TRIALS of tries have been attempted without generating a set of new
    # tasks, the algorithm stops.
    if num_trials >= GAGeneration.NUM_TRIALS:
      return []

    assert len(child_pool) == target_len

    return [GAGeneration(child_pool, set(retained_tasks), self._total_stucks)]
