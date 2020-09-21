# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Module to optimize the scheduling of benchmark_run tasks."""

from __future__ import print_function

import sys
import test_flag
import traceback

from collections import defaultdict
from machine_image_manager import MachineImageManager
from threading import Lock
from threading import Thread
from cros_utils import command_executer
from cros_utils import logger


class DutWorker(Thread):
  """Working thread for a dut."""

  def __init__(self, dut, sched):
    super(DutWorker, self).__init__(name='DutWorker-{}'.format(dut.name))
    self._dut = dut
    self._sched = sched
    self._stat_num_br_run = 0
    self._stat_num_reimage = 0
    self._stat_annotation = ''
    self._logger = logger.GetLogger(self._sched.get_experiment().log_dir)
    self.daemon = True
    self._terminated = False
    self._active_br = None
    # Race condition accessing _active_br between _execute_benchmark_run and
    # _terminate, so lock it up.
    self._active_br_lock = Lock()

  def terminate(self):
    self._terminated = True
    with self._active_br_lock:
      if self._active_br is not None:
        # BenchmarkRun.Terminate() terminates any running testcase via
        # suite_runner.Terminate and updates timeline.
        self._active_br.Terminate()

  def run(self):
    """Do the "run-test->(optionally reimage)->run-test" chore.

        Note - 'br' below means 'benchmark_run'.
    """

    # Firstly, handle benchmarkruns that have cache hit.
    br = self._sched.get_cached_benchmark_run()
    while br:
      try:
        self._stat_annotation = 'finishing cached {}'.format(br)
        br.run()
      except RuntimeError:
        traceback.print_exc(file=sys.stdout)
      br = self._sched.get_cached_benchmark_run()

    # Secondly, handle benchmarkruns that needs to be run on dut.
    self._setup_dut_label()
    try:
      self._logger.LogOutput('{} started.'.format(self))
      while not self._terminated:
        br = self._sched.get_benchmark_run(self._dut)
        if br is None:
          # No br left for this label. Considering reimaging.
          label = self._sched.allocate_label(self._dut)
          if label is None:
            # No br even for other labels. We are done.
            self._logger.LogOutput('ImageManager found no label '
                                   'for dut, stopping working '
                                   'thread {}.'.format(self))
            break
          if self._reimage(label):
            # Reimage to run other br fails, dut is doomed, stop
            # this thread.
            self._logger.LogWarning('Re-image failed, dut '
                                    'in an unstable state, stopping '
                                    'working thread {}.'.format(self))
            break
        else:
          # Execute the br.
          self._execute_benchmark_run(br)
    finally:
      self._stat_annotation = 'finished'
      # Thread finishes. Notify scheduler that I'm done.
      self._sched.dut_worker_finished(self)

  def _reimage(self, label):
    """Reimage image to label.

    Args:
      label: the label to remimage onto dut.

    Returns:
      0 if successful, otherwise 1.
    """

    # Termination could happen anywhere, check it.
    if self._terminated:
      return 1

    self._logger.LogOutput('Reimaging {} using {}'.format(self, label))
    self._stat_num_reimage += 1
    self._stat_annotation = 'reimaging using "{}"'.format(label.name)
    try:
      # Note, only 1 reimage at any given time, this is guaranteed in
      # ImageMachine, so no sync needed below.
      retval = self._sched.get_experiment().machine_manager.ImageMachine(
          self._dut, label)

      if retval:
        return 1
    except RuntimeError:
      return 1

    self._dut.label = label
    return 0

  def _execute_benchmark_run(self, br):
    """Execute a single benchmark_run.

        Note - this function never throws exceptions.
    """

    # Termination could happen anywhere, check it.
    if self._terminated:
      return

    self._logger.LogOutput('{} started working on {}'.format(self, br))
    self._stat_num_br_run += 1
    self._stat_annotation = 'executing {}'.format(br)
    # benchmark_run.run does not throws, but just play it safe here.
    try:
      assert br.owner_thread is None
      br.owner_thread = self
      with self._active_br_lock:
        self._active_br = br
      br.run()
    finally:
      self._sched.get_experiment().BenchmarkRunFinished(br)
      with self._active_br_lock:
        self._active_br = None

  def _setup_dut_label(self):
    """Try to match dut image with a certain experiment label.

        If such match is found, we just skip doing reimage and jump to execute
        some benchmark_runs.
    """

    checksum_file = '/usr/local/osimage_checksum_file'
    try:
      rv, checksum, _ = command_executer.GetCommandExecuter().\
          CrosRunCommandWOutput(
              'cat ' + checksum_file,
              chromeos_root=self._sched.get_labels(0).chromeos_root,
              machine=self._dut.name,
              print_to_console=False)
      if rv == 0:
        checksum = checksum.strip()
        for l in self._sched.get_labels():
          if l.checksum == checksum:
            self._logger.LogOutput(
                "Dut '{}' is pre-installed with '{}'".format(self._dut.name, l))
            self._dut.label = l
            return
    except RuntimeError:
      traceback.print_exc(file=sys.stdout)
      self._dut.label = None

  def __str__(self):
    return 'DutWorker[dut="{}", label="{}"]'.format(
        self._dut.name, self._dut.label.name if self._dut.label else 'None')

  def dut(self):
    return self._dut

  def status_str(self):
    """Report thread status."""

    return ('Worker thread "{}", label="{}", benchmark_run={}, '
            'reimage={}, now {}'.format(
                self._dut.name, 'None' if self._dut.label is None else
                self._dut.label.name, self._stat_num_br_run,
                self._stat_num_reimage, self._stat_annotation))


class BenchmarkRunCacheReader(Thread):
  """The thread to read cache for a list of benchmark_runs.

    On creation, each instance of this class is given a br_list, which is a
    subset of experiment._benchmark_runs.
  """

  def __init__(self, schedv2, br_list):
    super(BenchmarkRunCacheReader, self).__init__()
    self._schedv2 = schedv2
    self._br_list = br_list
    self._logger = self._schedv2.get_logger()

  def run(self):
    for br in self._br_list:
      try:
        br.ReadCache()
        if br.cache_hit:
          self._logger.LogOutput('Cache hit - {}'.format(br))
          with self._schedv2.lock_on('_cached_br_list'):
            self._schedv2.get_cached_run_list().append(br)
        else:
          self._logger.LogOutput('Cache not hit - {}'.format(br))
      except RuntimeError:
        traceback.print_exc(file=sys.stderr)


class Schedv2(object):
  """New scheduler for crosperf."""

  def __init__(self, experiment):
    self._experiment = experiment
    self._logger = logger.GetLogger(experiment.log_dir)

    # Create shortcuts to nested data structure. "_duts" points to a list of
    # locked machines. _labels points to a list of all labels.
    self._duts = self._experiment.machine_manager.GetMachines()
    self._labels = self._experiment.labels

    # Bookkeeping for synchronization.
    self._workers_lock = Lock()
    # pylint: disable=unnecessary-lambda
    self._lock_map = defaultdict(lambda: Lock())

    # Test mode flag
    self._in_test_mode = test_flag.GetTestMode()

    # Read benchmarkrun cache.
    self._read_br_cache()

    # Mapping from label to a list of benchmark_runs.
    self._label_brl_map = dict((l, []) for l in self._labels)
    for br in self._experiment.benchmark_runs:
      assert br.label in self._label_brl_map
      # Only put no-cache-hit br into the map.
      if br not in self._cached_br_list:
        self._label_brl_map[br.label].append(br)

    # Use machine image manager to calculate initial label allocation.
    self._mim = MachineImageManager(self._labels, self._duts)
    self._mim.compute_initial_allocation()

    # Create worker thread, 1 per dut.
    self._active_workers = [DutWorker(dut, self) for dut in self._duts]
    self._finished_workers = []

    # Termination flag.
    self._terminated = False

  def run_sched(self):
    """Start all dut worker threads and return immediately."""

    for w in self._active_workers:
      w.start()

  def _read_br_cache(self):
    """Use multi-threading to read cache for all benchmarkruns.

        We do this by firstly creating a few threads, and then assign each
        thread a segment of all brs. Each thread will check cache status for
        each br and put those with cache into '_cached_br_list'.
    """

    self._cached_br_list = []
    n_benchmarkruns = len(self._experiment.benchmark_runs)
    if n_benchmarkruns <= 4:
      # Use single thread to read cache.
      self._logger.LogOutput(('Starting to read cache status for '
                              '{} benchmark runs ...').format(n_benchmarkruns))
      BenchmarkRunCacheReader(self, self._experiment.benchmark_runs).run()
      return

    # Split benchmarkruns set into segments. Each segment will be handled by
    # a thread. Note, we use (x+3)/4 to mimic math.ceil(x/4).
    n_threads = max(2, min(20, (n_benchmarkruns + 3) / 4))
    self._logger.LogOutput(('Starting {} threads to read cache status for '
                            '{} benchmark runs ...').format(
                                n_threads, n_benchmarkruns))
    benchmarkruns_per_thread = (n_benchmarkruns + n_threads - 1) / n_threads
    benchmarkrun_segments = []
    for i in range(n_threads - 1):
      start = i * benchmarkruns_per_thread
      end = (i + 1) * benchmarkruns_per_thread
      benchmarkrun_segments.append(self._experiment.benchmark_runs[start:end])
    benchmarkrun_segments.append(self._experiment.benchmark_runs[(
        n_threads - 1) * benchmarkruns_per_thread:])

    # Assert: aggregation of benchmarkrun_segments equals to benchmark_runs.
    assert sum(len(x) for x in benchmarkrun_segments) == n_benchmarkruns

    # Create and start all readers.
    cache_readers = [
        BenchmarkRunCacheReader(self, x) for x in benchmarkrun_segments
    ]

    for x in cache_readers:
      x.start()

    # Wait till all readers finish.
    for x in cache_readers:
      x.join()

    # Summarize.
    self._logger.LogOutput('Total {} cache hit out of {} benchmark_runs.'.
                           format(len(self._cached_br_list), n_benchmarkruns))

  def get_cached_run_list(self):
    return self._cached_br_list

  def get_label_map(self):
    return self._label_brl_map

  def get_experiment(self):
    return self._experiment

  def get_labels(self, i=None):
    if i == None:
      return self._labels
    return self._labels[i]

  def get_logger(self):
    return self._logger

  def get_cached_benchmark_run(self):
    """Get a benchmark_run with 'cache hit'.

    Returns:
      The benchmark that has cache hit, if any. Otherwise none.
    """

    with self.lock_on('_cached_br_list'):
      if self._cached_br_list:
        return self._cached_br_list.pop()
      return None

  def get_benchmark_run(self, dut):
    """Get a benchmark_run (br) object for a certain dut.

    Args:
      dut: the dut for which a br is returned.

    Returns:
      A br with its label matching that of the dut. If no such br could be
      found, return None (this usually means a reimage is required for the
      dut).
    """

    # If terminated, stop providing any br.
    if self._terminated:
      return None

    # If dut bears an unrecognized label, return None.
    if dut.label is None:
      return None

    # If br list for the dut's label is empty (that means all brs for this
    # label have been done), return None.
    with self.lock_on(dut.label):
      brl = self._label_brl_map[dut.label]
      if not brl:
        return None
      # Return the first br.
      return brl.pop(0)

  def allocate_label(self, dut):
    """Allocate a label to a dut.

        The work is delegated to MachineImageManager.

        The dut_worker calling this method is responsible for reimage the dut to
        this label.

    Args:
      dut: the new label that is to be reimaged onto the dut.

    Returns:
      The label or None.
    """

    if self._terminated:
      return None

    return self._mim.allocate(dut, self)

  def dut_worker_finished(self, dut_worker):
    """Notify schedv2 that the dut_worker thread finished.

    Args:
      dut_worker: the thread that is about to end.
    """

    self._logger.LogOutput('{} finished.'.format(dut_worker))
    with self._workers_lock:
      self._active_workers.remove(dut_worker)
      self._finished_workers.append(dut_worker)

  def is_complete(self):
    return len(self._active_workers) == 0

  def lock_on(self, my_object):
    return self._lock_map[my_object]

  def terminate(self):
    """Mark flag so we stop providing br/reimages.

        Also terminate each DutWorker, so they refuse to execute br or reimage.
    """

    self._terminated = True
    for dut_worker in self._active_workers:
      dut_worker.terminate()

  def threads_status_as_string(self):
    """Report the dut worker threads status."""

    status = '{} active threads, {} finished threads.\n'.format(
        len(self._active_workers), len(self._finished_workers))
    status += '  Active threads:'
    for dw in self._active_workers:
      status += '\n    ' + dw.status_str()
    if self._finished_workers:
      status += '\n  Finished threads:'
      for dw in self._finished_workers:
        status += '\n    ' + dw.status_str()
    return status
