# Copyright 2011 Google Inc. All Rights Reserved.
#
"""Classes that help running commands in a subshell.

Commands can be run locally, or remotly using SSH connection.  You may log the
output of a command to a terminal or a file, or any other destination.
"""

__author__ = 'kbaclawski@google.com (Krystian Baclawski)'

import fcntl
import logging
import os
import select
import subprocess
import time

from automation.common import logger


class CommandExecuter(object):
  DRY_RUN = False

  def __init__(self, dry_run=False):
    self._logger = logging.getLogger(self.__class__.__name__)
    self._dry_run = dry_run or self.DRY_RUN

  @classmethod
  def Configure(cls, dry_run):
    cls.DRY_RUN = dry_run

  def RunCommand(self,
                 cmd,
                 machine=None,
                 username=None,
                 command_terminator=None,
                 command_timeout=None):
    cmd = str(cmd)

    if self._dry_run:
      return 0

    if not command_terminator:
      command_terminator = CommandTerminator()

    if command_terminator.IsTerminated():
      self._logger.warning('Command has been already terminated!')
      return 1

    # Rewrite command for remote execution.
    if machine:
      if username:
        login = '%s@%s' % (username, machine)
      else:
        login = machine

      self._logger.debug("Executing '%s' on %s.", cmd, login)

      # FIXME(asharif): Remove this after crosbug.com/33007 is fixed.
      cmd = "ssh -t -t %s -- '%s'" % (login, cmd)
    else:
      self._logger.debug("Executing: '%s'.", cmd)

    child = self._SpawnProcess(cmd, command_terminator, command_timeout)

    self._logger.debug('{PID: %d} Finished with %d code.', child.pid,
                       child.returncode)

    return child.returncode

  def _Terminate(self, child, command_timeout, wait_timeout=10):
    """Gracefully shutdown the child by sending SIGTERM."""

    if command_timeout:
      self._logger.warning('{PID: %d} Timeout of %s seconds reached since '
                           'process started.', child.pid, command_timeout)

    self._logger.warning('{PID: %d} Terminating child.', child.pid)

    try:
      child.terminate()
    except OSError:
      pass

    wait_started = time.time()

    while not child.poll():
      if time.time() - wait_started >= wait_timeout:
        break
      time.sleep(0.1)

    return child.poll()

  def _Kill(self, child):
    """Kill the child with immediate result."""
    self._logger.warning('{PID: %d} Process still alive.', child.pid)
    self._logger.warning('{PID: %d} Killing child.', child.pid)
    child.kill()
    child.wait()

  def _SpawnProcess(self, cmd, command_terminator, command_timeout):
    # Create a child process executing provided command.
    child = subprocess.Popen(cmd,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,
                             stdin=subprocess.PIPE,
                             shell=True)

    # Close stdin so the child won't be able to block on read.
    child.stdin.close()

    started_time = time.time()

    # Watch for data on process stdout, stderr.
    pipes = [child.stdout, child.stderr]

    # Put pipes into non-blocking mode.
    for pipe in pipes:
      fd = pipe.fileno()
      fd_flags = fcntl.fcntl(fd, fcntl.F_GETFL)
      fcntl.fcntl(fd, fcntl.F_SETFL, fd_flags | os.O_NONBLOCK)

    already_terminated = False

    while pipes:
      # Maybe timeout reached?
      if command_timeout and time.time() - started_time > command_timeout:
        command_terminator.Terminate()

      # Check if terminate request was received.
      if command_terminator.IsTerminated() and not already_terminated:
        if not self._Terminate(child, command_timeout):
          self._Kill(child)
        # Don't exit the loop immediately. Firstly try to read everything that
        # was left on stdout and stderr.
        already_terminated = True

      # Wait for pipes to become ready.
      ready_pipes, _, _ = select.select(pipes, [], [], 0.1)

      # Handle file descriptors ready to be read.
      for pipe in ready_pipes:
        fd = pipe.fileno()

        data = os.read(fd, 4096)

        # check for end-of-file
        if not data:
          pipes.remove(pipe)
          continue

        # read all data that's available
        while data:
          if pipe == child.stdout:
            self.DataReceivedOnOutput(data)
          elif pipe == child.stderr:
            self.DataReceivedOnError(data)

          try:
            data = os.read(fd, 4096)
          except OSError:
            # terminate loop if EWOULDBLOCK (EAGAIN) is received
            data = ''

    if not already_terminated:
      self._logger.debug('Waiting for command to finish.')
      child.wait()

    return child

  def DataReceivedOnOutput(self, data):
    """Invoked when the child process wrote data to stdout."""
    sys.stdout.write(data)

  def DataReceivedOnError(self, data):
    """Invoked when the child process wrote data to stderr."""
    sys.stderr.write(data)


class LoggingCommandExecuter(CommandExecuter):

  def __init__(self, *args, **kwargs):
    super(LoggingCommandExecuter, self).__init__(*args, **kwargs)

    # Create a logger for command's stdout/stderr streams.
    self._output = logging.getLogger('%s.%s' % (self._logger.name, 'Output'))

  def OpenLog(self, log_path):
    """The messages are going to be saved to gzip compressed file."""
    formatter = logging.Formatter('%(asctime)s %(prefix)s: %(message)s',
                                  '%Y-%m-%d %H:%M:%S')
    handler = logger.CompressedFileHandler(log_path, delay=True)
    handler.setFormatter(formatter)
    self._output.addHandler(handler)

    # Set a flag to prevent log records from being propagated up the logger
    # hierarchy tree.  We don't want for command output messages to appear in
    # the main log.
    self._output.propagate = 0

  def CloseLog(self):
    """Remove handlers and reattach the logger to its parent."""
    for handler in list(self._output.handlers):
      self._output.removeHandler(handler)
      handler.flush()
      handler.close()

    self._output.propagate = 1

  def DataReceivedOnOutput(self, data):
    """Invoked when the child process wrote data to stdout."""
    for line in data.splitlines():
      self._output.info(line, extra={'prefix': 'STDOUT'})

  def DataReceivedOnError(self, data):
    """Invoked when the child process wrote data to stderr."""
    for line in data.splitlines():
      self._output.warning(line, extra={'prefix': 'STDERR'})


class CommandTerminator(object):

  def __init__(self):
    self.terminated = False

  def Terminate(self):
    self.terminated = True

  def IsTerminated(self):
    return self.terminated
