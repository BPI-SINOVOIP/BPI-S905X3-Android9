# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Pipeline process that encapsulates the actual content.

Part of the Chrome build flags optimization.

The actual stages include the builder and the executor.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

import multiprocessing

# Pick an integer at random.
POISONPILL = 975


class PipelineProcess(multiprocessing.Process):
  """A process that encapsulates the actual content pipeline stage.

  The actual pipeline stage can be the builder or the tester.  This process
  continuously pull tasks from the queue until a poison pill is received.
  Once a job is received, it will hand it to the actual stage for processing.

  Each pipeline stage contains three modules.
  The first module continuously pulls task from the input queue. It searches the
  cache to check whether the task has encountered before. If so, duplicate
  computation can be avoided.
  The second module consists of a pool of workers that do the actual work, e.g.,
  the worker will compile the source code and get the image in the builder
  pipeline stage.
  The third module is a helper that put the result cost to the cost field of the
  duplicate tasks. For example, if two tasks are equivalent, only one task, say
  t1 will be executed and the other task, say t2 will not be executed. The third
  mode gets the result from t1, when it is available and set the cost of t2 to
  be the same as that of t1.
  """

  def __init__(self, num_processes, name, cache, stage, task_queue, helper,
               worker, result_queue):
    """Set up input/output queue and the actual method to be called.

    Args:
      num_processes: Number of helpers subprocessors this stage has.
      name: The name of this stage.
      cache: The computed tasks encountered before.
      stage: An int value that specifies the stage for this pipeline stage, for
        example, build stage or test stage. This value will be used to retrieve
        the keys in different stage. I.e., the flags set is the key in build
        stage and the checksum is the key in the test stage. The key is used to
        detect duplicates.
      task_queue: The input task queue for this pipeline stage.
      helper: The method hosted by the helper module to fill up the cost of the
        duplicate tasks.
      worker: The method hosted by the worker pools to do the actual work, e.g.,
        compile the image.
      result_queue: The output task queue for this pipeline stage.
    """

    multiprocessing.Process.__init__(self)

    self._name = name
    self._task_queue = task_queue
    self._result_queue = result_queue

    self._helper = helper
    self._worker = worker

    self._cache = cache
    self._stage = stage
    self._num_processes = num_processes

    # the queues used by the modules for communication
    manager = multiprocessing.Manager()
    self._helper_queue = manager.Queue()
    self._work_queue = manager.Queue()

  def run(self):
    """Busy pulling the next task from the queue for execution.

    Once a job is pulled, this stage invokes the actual stage method and submits
    the result to the next pipeline stage.

    The process will terminate on receiving the poison pill from previous stage.
    """

    # the worker pool
    work_pool = multiprocessing.Pool(self._num_processes)

    # the helper process
    helper_process = multiprocessing.Process(
        target=self._helper,
        args=(self._stage, self._cache, self._helper_queue, self._work_queue,
              self._result_queue))
    helper_process.start()
    mycache = self._cache.keys()

    while True:
      task = self._task_queue.get()
      if task == POISONPILL:
        # Poison pill means shutdown
        self._result_queue.put(POISONPILL)
        break

      task_key = task.GetIdentifier(self._stage)
      if task_key in mycache:
        # The task has been encountered before. It will be sent to the helper
        # module for further processing.
        self._helper_queue.put(task)
      else:
        # Let the workers do the actual work.
        work_pool.apply_async(
            self._worker,
            args=(self._stage, task, self._work_queue, self._result_queue))
        mycache.append(task_key)

    # Shutdown the workers pool and the helper process.
    work_pool.close()
    work_pool.join()

    self._helper_queue.put(POISONPILL)
    helper_process.join()
