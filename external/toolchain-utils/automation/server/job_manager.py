# Copyright 2010 Google Inc. All Rights Reserved.
#

import logging
import os
import re
import threading

from automation.common import job
from automation.common import logger
from automation.server.job_executer import JobExecuter


class IdProducerPolicy(object):
  """Produces series of unique integer IDs.

  Example:
      id_producer = IdProducerPolicy()
      id_a = id_producer.GetNextId()
      id_b = id_producer.GetNextId()
      assert id_a != id_b
  """

  def __init__(self):
    self._counter = 1

  def Initialize(self, home_prefix, home_pattern):
    """Find first available ID based on a directory listing.

    Args:
      home_prefix: A directory to be traversed.
      home_pattern: A regexp describing all files/directories that will be
        considered. The regexp must contain exactly one match group with name
        "id", which must match an integer number.

    Example:
      id_producer.Initialize(JOBDIR_PREFIX, 'job-(?P<id>\d+)')
    """
    harvested_ids = []

    if os.path.isdir(home_prefix):
      for filename in os.listdir(home_prefix):
        path = os.path.join(home_prefix, filename)

        if os.path.isdir(path):
          match = re.match(home_pattern, filename)

          if match:
            harvested_ids.append(int(match.group('id')))

    self._counter = max(harvested_ids or [0]) + 1

  def GetNextId(self):
    """Calculates another ID considered to be unique."""
    new_id = self._counter
    self._counter += 1
    return new_id


class JobManager(threading.Thread):

  def __init__(self, machine_manager):
    threading.Thread.__init__(self, name=self.__class__.__name__)
    self.all_jobs = []
    self.ready_jobs = []
    self.job_executer_mapping = {}

    self.machine_manager = machine_manager

    self._lock = threading.Lock()
    self._jobs_available = threading.Condition(self._lock)
    self._exit_request = False

    self.listeners = []
    self.listeners.append(self)

    self._id_producer = IdProducerPolicy()
    self._id_producer.Initialize(job.Job.WORKDIR_PREFIX, 'job-(?P<id>\d+)')

    self._logger = logging.getLogger(self.__class__.__name__)

  def StartJobManager(self):
    self._logger.info('Starting...')

    with self._lock:
      self.start()
      self._jobs_available.notifyAll()

  def StopJobManager(self):
    self._logger.info('Shutdown request received.')

    with self._lock:
      for job_ in self.all_jobs:
        self._KillJob(job_.id)

      # Signal to die
      self._exit_request = True
      self._jobs_available.notifyAll()

    # Wait for all job threads to finish
    for executer in self.job_executer_mapping.values():
      executer.join()

  def KillJob(self, job_id):
    """Kill a job by id.

    Does not block until the job is completed.
    """
    with self._lock:
      self._KillJob(job_id)

  def GetJob(self, job_id):
    for job_ in self.all_jobs:
      if job_.id == job_id:
        return job_
    return None

  def _KillJob(self, job_id):
    self._logger.info('Killing [Job: %d].', job_id)

    if job_id in self.job_executer_mapping:
      self.job_executer_mapping[job_id].Kill()
    for job_ in self.ready_jobs:
      if job_.id == job_id:
        self.ready_jobs.remove(job_)
        break

  def AddJob(self, job_):
    with self._lock:
      job_.id = self._id_producer.GetNextId()

      self.all_jobs.append(job_)
      # Only queue a job as ready if it has no dependencies
      if job_.is_ready:
        self.ready_jobs.append(job_)

      self._jobs_available.notifyAll()

    return job_.id

  def CleanUpJob(self, job_):
    with self._lock:
      if job_.id in self.job_executer_mapping:
        self.job_executer_mapping[job_.id].CleanUpWorkDir()
        del self.job_executer_mapping[job_.id]
      # TODO(raymes): remove job from self.all_jobs

  def NotifyJobComplete(self, job_):
    self.machine_manager.ReturnMachines(job_.machines)

    with self._lock:
      self._logger.debug('Handling %r completion event.', job_)

      if job_.status == job.STATUS_SUCCEEDED:
        for succ in job_.successors:
          if succ.is_ready:
            if succ not in self.ready_jobs:
              self.ready_jobs.append(succ)

      self._jobs_available.notifyAll()

  def AddListener(self, listener):
    self.listeners.append(listener)

  @logger.HandleUncaughtExceptions
  def run(self):
    self._logger.info('Started.')

    while not self._exit_request:
      with self._lock:
        # Get the next ready job, block if there are none
        self._jobs_available.wait()

        while self.ready_jobs:
          ready_job = self.ready_jobs.pop()

          required_machines = ready_job.machine_dependencies
          for pred in ready_job.predecessors:
            required_machines[0].AddPreferredMachine(
                pred.primary_machine.hostname)

          machines = self.machine_manager.GetMachines(required_machines)
          if not machines:
            # If we can't get the necessary machines right now, simply wait
            # for some jobs to complete
            self.ready_jobs.insert(0, ready_job)
            break
          else:
            # Mark as executing
            executer = JobExecuter(ready_job, machines, self.listeners)
            executer.start()
            self.job_executer_mapping[ready_job.id] = executer

    self._logger.info('Stopped.')
