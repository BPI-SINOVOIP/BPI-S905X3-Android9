# Copyright 2010 Google Inc. All Rights Reserved.
#

import copy
import logging
import threading

from automation.common import command as cmd
from automation.common import logger
from automation.common.command_executer import CommandExecuter
from automation.common import job
from automation.common import job_group
from automation.server.job_manager import IdProducerPolicy


class JobGroupManager(object):

  def __init__(self, job_manager):
    self.all_job_groups = []

    self.job_manager = job_manager
    self.job_manager.AddListener(self)

    self._lock = threading.Lock()
    self._job_group_finished = threading.Condition(self._lock)

    self._id_producer = IdProducerPolicy()
    self._id_producer.Initialize(job_group.JobGroup.HOMEDIR_PREFIX,
                                 'job-group-(?P<id>\d+)')

    self._logger = logging.getLogger(self.__class__.__name__)

  def GetJobGroup(self, group_id):
    with self._lock:
      for group in self.all_job_groups:
        if group.id == group_id:
          return group

      return None

  def GetAllJobGroups(self):
    with self._lock:
      return copy.deepcopy(self.all_job_groups)

  def AddJobGroup(self, group):
    with self._lock:
      group.id = self._id_producer.GetNextId()

    self._logger.debug('Creating runtime environment for %r.', group)

    CommandExecuter().RunCommand(cmd.Chain(
        cmd.RmTree(group.home_dir), cmd.MakeDir(group.home_dir)))

    with self._lock:
      self.all_job_groups.append(group)

      for job_ in group.jobs:
        self.job_manager.AddJob(job_)

      group.status = job_group.STATUS_EXECUTING

    self._logger.info('Added %r to queue.', group)

    return group.id

  def KillJobGroup(self, group):
    with self._lock:
      self._logger.debug('Killing all jobs that belong to %r.', group)

      for job_ in group.jobs:
        self.job_manager.KillJob(job_)

      self._logger.debug('Waiting for jobs to quit.')

      # Lets block until the group is killed so we know it is completed
      # when we return.
      while group.status not in [job_group.STATUS_SUCCEEDED,
                                 job_group.STATUS_FAILED]:
        self._job_group_finished.wait()

  def NotifyJobComplete(self, job_):
    self._logger.debug('Handling %r completion event.', job_)

    group = job_.group

    with self._lock:
      # We need to perform an action only if the group hasn't already failed.
      if group.status != job_group.STATUS_FAILED:
        if job_.status == job.STATUS_FAILED:
          # We have a failed job, abort the job group
          group.status = job_group.STATUS_FAILED
          if group.cleanup_on_failure:
            for job_ in group.jobs:
              # TODO(bjanakiraman): We should probably only kill dependent jobs
              # instead of the whole job group.
              self.job_manager.KillJob(job_)
              self.job_manager.CleanUpJob(job_)
        else:
          # The job succeeded successfully -- lets check to see if we are done.
          assert job_.status == job.STATUS_SUCCEEDED
          finished = True
          for other_job in group.jobs:
            assert other_job.status != job.STATUS_FAILED
            if other_job.status != job.STATUS_SUCCEEDED:
              finished = False
              break

          if finished and group.status != job_group.STATUS_SUCCEEDED:
            # TODO(kbaclawski): Without check performed above following code
            # could be called more than once. This would trigger StateMachine
            # crash, because it cannot transition from STATUS_SUCCEEDED to
            # STATUS_SUCCEEDED. Need to address that bug in near future.
            group.status = job_group.STATUS_SUCCEEDED
            if group.cleanup_on_completion:
              for job_ in group.jobs:
                self.job_manager.CleanUpJob(job_)

        self._job_group_finished.notifyAll()
