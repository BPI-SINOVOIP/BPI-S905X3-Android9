# Copyright 2010 Google Inc. All Rights Reserved.
#
"""A module for a job in the infrastructure."""

__author__ = 'raymes@google.com (Raymes Khoury)'

import os.path

from automation.common import state_machine

STATUS_NOT_EXECUTED = 'NOT_EXECUTED'
STATUS_SETUP = 'SETUP'
STATUS_COPYING = 'COPYING'
STATUS_RUNNING = 'RUNNING'
STATUS_SUCCEEDED = 'SUCCEEDED'
STATUS_FAILED = 'FAILED'


class FolderDependency(object):

  def __init__(self, job, src, dest=None):
    if not dest:
      dest = src

    # TODO(kbaclawski): rename to producer
    self.job = job
    self.src = src
    self.dest = dest

  @property
  def read_only(self):
    return self.dest == self.src


class JobStateMachine(state_machine.BasicStateMachine):
  state_machine = {
      STATUS_NOT_EXECUTED: [STATUS_SETUP],
      STATUS_SETUP: [STATUS_COPYING, STATUS_FAILED],
      STATUS_COPYING: [STATUS_RUNNING, STATUS_FAILED],
      STATUS_RUNNING: [STATUS_SUCCEEDED, STATUS_FAILED]
  }

  final_states = [STATUS_SUCCEEDED, STATUS_FAILED]


class JobFailure(Exception):

  def __init__(self, message, exit_code):
    Exception.__init__(self, message)
    self.exit_code = exit_code


class Job(object):
  """A class representing a job whose commands will be executed."""

  WORKDIR_PREFIX = '/usr/local/google/tmp/automation'

  def __init__(self, label, command, timeout=4 * 60 * 60):
    self._state = JobStateMachine(STATUS_NOT_EXECUTED)
    self.predecessors = set()
    self.successors = set()
    self.machine_dependencies = []
    self.folder_dependencies = []
    self.id = 0
    self.machines = []
    self.command = command
    self._has_primary_machine_spec = False
    self.group = None
    self.dry_run = None
    self.label = label
    self.timeout = timeout

  def _StateGet(self):
    return self._state

  def _StateSet(self, new_state):
    self._state.Change(new_state)

  status = property(_StateGet, _StateSet)

  @property
  def timeline(self):
    return self._state.timeline

  def __repr__(self):
    return '{%s: %s}' % (self.__class__.__name__, self.id)

  def __str__(self):
    res = []
    res.append('%d' % self.id)
    res.append('Predecessors:')
    res.extend(['%d' % pred.id for pred in self.predecessors])
    res.append('Successors:')
    res.extend(['%d' % succ.id for succ in self.successors])
    res.append('Machines:')
    res.extend(['%s' % machine for machine in self.machines])
    res.append(self.PrettyFormatCommand())
    res.append('%s' % self.status)
    res.append(self.timeline.GetTransitionEventReport())
    return '\n'.join(res)

  @staticmethod
  def _FormatCommand(cmd, substitutions):
    for pattern, replacement in substitutions:
      cmd = cmd.replace(pattern, replacement)

    return cmd

  def GetCommand(self):
    substitutions = [
        ('$JOB_ID', str(self.id)), ('$JOB_TMP', self.work_dir),
        ('$JOB_HOME', self.home_dir),
        ('$PRIMARY_MACHINE', self.primary_machine.hostname)
    ]

    if len(self.machines) > 1:
      for num, machine in enumerate(self.machines[1:]):
        substitutions.append(('$SECONDARY_MACHINES[%d]' % num, machine.hostname
                             ))

    return self._FormatCommand(str(self.command), substitutions)

  def PrettyFormatCommand(self):
    # TODO(kbaclawski): This method doesn't belong here, but rather to
    # non existing Command class. If one is created then PrettyFormatCommand
    # shall become its method.
    return self._FormatCommand(self.GetCommand(), [
        ('\{ ', ''), ('; \}', ''), ('\} ', '\n'), ('\s*&&\s*', '\n')
    ])

  def DependsOnFolder(self, dependency):
    self.folder_dependencies.append(dependency)
    self.DependsOn(dependency.job)

  @property
  def results_dir(self):
    return os.path.join(self.work_dir, 'results')

  @property
  def logs_dir(self):
    return os.path.join(self.home_dir, 'logs')

  @property
  def log_filename_prefix(self):
    return 'job-%d.log' % self.id

  @property
  def work_dir(self):
    return os.path.join(self.WORKDIR_PREFIX, 'job-%d' % self.id)

  @property
  def home_dir(self):
    return os.path.join(self.group.home_dir, 'job-%d' % self.id)

  @property
  def primary_machine(self):
    return self.machines[0]

  def DependsOn(self, job):
    """Specifies Jobs to be finished before this job can be launched."""
    self.predecessors.add(job)
    job.successors.add(self)

  @property
  def is_ready(self):
    """Check that all our dependencies have been executed."""
    return all(pred.status == STATUS_SUCCEEDED for pred in self.predecessors)

  def DependsOnMachine(self, machine_spec, primary=True):
    # Job will run on arbitrarily chosen machine specified by
    # MachineSpecification class instances passed to this method.
    if primary:
      if self._has_primary_machine_spec:
        raise RuntimeError('Only one primary machine specification allowed.')
      self._has_primary_machine_spec = True
      self.machine_dependencies.insert(0, machine_spec)
    else:
      self.machine_dependencies.append(machine_spec)
