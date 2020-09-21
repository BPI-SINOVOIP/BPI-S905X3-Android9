# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""The pipeline_worker functions of the build and test stage of the framework.

Part of the Chrome build flags optimization.

This module defines the helper and the worker. If there are duplicate tasks, for
example t1 and t2, needs to be built/tested, one of them, for example t1, will
be built/tested and the helper waits for the result of t1 and set the results of
the other task, t2 here, to be the same as that of t1. Setting the result of t2
to be the same as t1 is referred to as resolving the result of t2.
The worker invokes the work method of the tasks that are not duplicate.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

import pipeline_process


def Helper(stage, done_dict, helper_queue, completed_queue, result_queue):
  """Helper that filters duplicate tasks.

  This method Continuously pulls duplicate tasks from the helper_queue. The
  duplicate tasks need not be compiled/tested. This method also pulls completed
  tasks from the worker queue and let the results of the duplicate tasks be the
  same as their corresponding finished task.

  Args:
    stage: The current stage of the pipeline, for example, build stage or test
      stage.
    done_dict: A dictionary of tasks that are done. The key of the dictionary is
      the identifier of the task. The value of the dictionary is the results of
      performing the corresponding task.
    helper_queue: A queue of duplicate tasks whose results need to be resolved.
      This is a communication channel between the pipeline_process and this
      helper process.
    completed_queue: A queue of tasks that have been built/tested. The results
      of these tasks are needed to resolve the results of the duplicate tasks.
      This is the communication channel between the workers and this helper
      process.
    result_queue: After the results of the duplicate tasks have been resolved,
      the duplicate tasks will be sent to the next stage via this queue.
  """

  # The list of duplicate tasks, the results of which need to be resolved.
  waiting_list = []

  while True:
    # Pull duplicate task from the helper queue.
    if not helper_queue.empty():
      task = helper_queue.get()

      if task == pipeline_process.POISONPILL:
        # Poison pill means no more duplicate task from the helper queue.
        break

      # The task has not been performed before.
      assert not task.Done(stage)

      # The identifier of this task.
      identifier = task.GetIdentifier(stage)

      # If a duplicate task comes before the corresponding resolved results from
      # the completed_queue, it will be put in the waiting list. If the result
      # arrives before the duplicate task, the duplicate task will be resolved
      # right away.
      if identifier in done_dict:
        # This task has been encountered before and the result is available. The
        # result can be resolved right away.
        task.SetResult(stage, done_dict[identifier])
        result_queue.put(task)
      else:
        waiting_list.append(task)

    # Check and get completed tasks from completed_queue.
    GetResultFromCompletedQueue(stage, completed_queue, done_dict, waiting_list,
                                result_queue)

  # Wait to resolve the results of the remaining duplicate tasks.
  while waiting_list:
    GetResultFromCompletedQueue(stage, completed_queue, done_dict, waiting_list,
                                result_queue)


def GetResultFromCompletedQueue(stage, completed_queue, done_dict, waiting_list,
                                result_queue):
  """Pull results from the completed queue and resolves duplicate tasks.

  Args:
    stage: The current stage of the pipeline, for example, build stage or test
      stage.
    completed_queue: A queue of tasks that have been performed. The results of
      these tasks are needed to resolve the results of the duplicate tasks. This
      is the communication channel between the workers and this method.
    done_dict: A dictionary of tasks that are done. The key of the dictionary is
      the optimization flags of the task. The value of the dictionary is the
      compilation results of the corresponding task.
    waiting_list: The list of duplicate tasks, the results of which need to be
      resolved.
    result_queue: After the results of the duplicate tasks have been resolved,
      the duplicate tasks will be sent to the next stage via this queue.

  This helper method tries to pull a completed task from the completed queue.
  If it gets a task from the queue, it resolves the results of all the relevant
  duplicate tasks in the waiting list. Relevant tasks are the tasks that have
  the same flags as the currently received results from the completed_queue.
  """
  # Pull completed task from the worker queue.
  if not completed_queue.empty():
    (identifier, result) = completed_queue.get()
    done_dict[identifier] = result

    tasks = [t for t in waiting_list if t.GetIdentifier(stage) == identifier]
    for duplicate_task in tasks:
      duplicate_task.SetResult(stage, result)
      result_queue.put(duplicate_task)
      waiting_list.remove(duplicate_task)


def Worker(stage, task, helper_queue, result_queue):
  """Worker that performs the task.

  This method calls the work method of the input task and distribute the result
  to the helper and the next stage.

  Args:
    stage: The current stage of the pipeline, for example, build stage or test
      stage.
    task: Input task that needs to be performed.
    helper_queue: Queue that holds the completed tasks and the results. This is
      the communication channel between the worker and the helper.
    result_queue: Queue that holds the completed tasks and the results. This is
      the communication channel between the worker and the next stage.
  """

  # The task has not been completed before.
  assert not task.Done(stage)

  task.Work(stage)
  helper_queue.put((task.GetIdentifier(stage), task.GetResult(stage)))
  result_queue.put(task)
