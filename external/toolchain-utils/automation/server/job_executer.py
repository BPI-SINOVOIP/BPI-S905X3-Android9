# Copyright 2010 Google Inc. All Rights Reserved.
#

import logging
import os.path
import threading

from automation.common import command as cmd
from automation.common import job
from automation.common import logger
from automation.common.command_executer import LoggingCommandExecuter
from automation.common.command_executer import CommandTerminator


class JobExecuter(threading.Thread):

  def __init__(self, job_to_execute, machines, listeners):
    threading.Thread.__init__(self)

    assert machines

    self.job = job_to_execute
    self.listeners = listeners
    self.machines = machines

    # Set Thread name.
    self.name = '%s-%s' % (self.__class__.__name__, self.job.id)

    self._logger = logging.getLogger(self.__class__.__name__)
    self._executer = LoggingCommandExecuter(self.job.dry_run)
    self._terminator = CommandTerminator()

  def _RunRemotely(self, command, fail_msg, command_timeout=1 * 60 * 60):
    exit_code = self._executer.RunCommand(command,
                                          self.job.primary_machine.hostname,
                                          self.job.primary_machine.username,
                                          command_terminator=self._terminator,
                                          command_timeout=command_timeout)
    if exit_code:
      raise job.JobFailure(fail_msg, exit_code)

  def _RunLocally(self, command, fail_msg, command_timeout=1 * 60 * 60):
    exit_code = self._executer.RunCommand(command,
                                          command_terminator=self._terminator,
                                          command_timeout=command_timeout)
    if exit_code:
      raise job.JobFailure(fail_msg, exit_code)

  def Kill(self):
    self._terminator.Terminate()

  def CleanUpWorkDir(self):
    self._logger.debug('Cleaning up %r work directory.', self.job)
    self._RunRemotely(cmd.RmTree(self.job.work_dir), 'Cleanup workdir failed.')

  def CleanUpHomeDir(self):
    self._logger.debug('Cleaning up %r home directory.', self.job)
    self._RunLocally(cmd.RmTree(self.job.home_dir), 'Cleanup homedir failed.')

  def _PrepareRuntimeEnvironment(self):
    self._RunRemotely(
        cmd.MakeDir(self.job.work_dir, self.job.logs_dir, self.job.results_dir),
        'Creating new job directory failed.')

    # The log directory is ready, so we can prepare to log command's output.
    self._executer.OpenLog(os.path.join(self.job.logs_dir,
                                        self.job.log_filename_prefix))

  def _SatisfyFolderDependencies(self):
    for dependency in self.job.folder_dependencies:
      to_folder = os.path.join(self.job.work_dir, dependency.dest)
      from_folder = os.path.join(dependency.job.work_dir, dependency.src)
      from_machine = dependency.job.primary_machine

      if from_machine == self.job.primary_machine and dependency.read_only:
        # No need to make a copy, just symlink it
        self._RunRemotely(
            cmd.MakeSymlink(from_folder, to_folder),
            'Failed to create symlink to required directory.')
      else:
        self._RunRemotely(
            cmd.RemoteCopyFrom(from_machine.hostname,
                               from_folder,
                               to_folder,
                               username=from_machine.username),
            'Failed to copy required files.')

  def _LaunchJobCommand(self):
    command = self.job.GetCommand()

    self._RunRemotely('%s; %s' % ('PS1=. TERM=linux source ~/.bashrc',
                                  cmd.Wrapper(command,
                                              cwd=self.job.work_dir)),
                      "Command failed to execute: '%s'." % command,
                      self.job.timeout)

  def _CopyJobResults(self):
    """Copy test results back to directory."""
    self._RunLocally(
        cmd.RemoteCopyFrom(self.job.primary_machine.hostname,
                           self.job.results_dir,
                           self.job.home_dir,
                           username=self.job.primary_machine.username),
        'Failed to copy results.')

  def run(self):
    self.job.status = job.STATUS_SETUP
    self.job.machines = self.machines
    self._logger.debug('Executing %r on %r in directory %s.', self.job,
                       self.job.primary_machine.hostname, self.job.work_dir)

    try:
      self.CleanUpWorkDir()

      self._PrepareRuntimeEnvironment()

      self.job.status = job.STATUS_COPYING

      self._SatisfyFolderDependencies()

      self.job.status = job.STATUS_RUNNING

      self._LaunchJobCommand()
      self._CopyJobResults()

      # If we get here, the job succeeded.
      self.job.status = job.STATUS_SUCCEEDED
    except job.JobFailure as ex:
      self._logger.error('Job failed. Exit code %s. %s', ex.exit_code, ex)
      if self._terminator.IsTerminated():
        self._logger.info('%r was killed', self.job)

      self.job.status = job.STATUS_FAILED

    self._executer.CloseLog()

    for listener in self.listeners:
      listener.NotifyJobComplete(self.job)
