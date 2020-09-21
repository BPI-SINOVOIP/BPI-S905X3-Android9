# Copyright 2010 Google Inc. All Rights Reserved.
#

import getpass
import os

from automation.common.state_machine import BasicStateMachine

STATUS_NOT_EXECUTED = 'NOT_EXECUTED'
STATUS_EXECUTING = 'EXECUTING'
STATUS_SUCCEEDED = 'SUCCEEDED'
STATUS_FAILED = 'FAILED'


class JobGroupStateMachine(BasicStateMachine):
  state_machine = {
      STATUS_NOT_EXECUTED: [STATUS_EXECUTING],
      STATUS_EXECUTING: [STATUS_SUCCEEDED, STATUS_FAILED]
  }

  final_states = [STATUS_SUCCEEDED, STATUS_FAILED]


class JobGroup(object):
  HOMEDIR_PREFIX = os.path.join('/home', getpass.getuser(), 'www', 'automation')

  def __init__(self,
               label,
               jobs=None,
               cleanup_on_completion=True,
               cleanup_on_failure=False,
               description=''):
    self._state = JobGroupStateMachine(STATUS_NOT_EXECUTED)
    self.id = 0
    self.label = label
    self.jobs = []
    self.cleanup_on_completion = cleanup_on_completion
    self.cleanup_on_failure = cleanup_on_failure
    self.description = description

    if jobs:
      for job in jobs:
        self.AddJob(job)

  def _StateGet(self):
    return self._state

  def _StateSet(self, new_state):
    self._state.Change(new_state)

  status = property(_StateGet, _StateSet)

  @property
  def home_dir(self):
    return os.path.join(self.HOMEDIR_PREFIX, 'job-group-%d' % self.id)

  @property
  def time_submitted(self):
    try:
      return self.status.timeline[1].time_started
    except IndexError:
      return None

  def __repr__(self):
    return '{%s: %s}' % (self.__class__.__name__, self.id)

  def __str__(self):
    return '\n'.join(['Job-Group:', 'ID: %s' % self.id] + [str(
        job) for job in self.jobs])

  def AddJob(self, job):
    self.jobs.append(job)
    job.group = self
