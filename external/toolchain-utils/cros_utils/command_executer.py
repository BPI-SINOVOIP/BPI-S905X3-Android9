# Copyright 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Utilities to run commands in outside/inside chroot and on the board."""

from __future__ import print_function

import getpass
import os
import re
import select
import signal
import subprocess
import sys
import tempfile
import time

import logger
import misc

mock_default = False

LOG_LEVEL = ('none', 'quiet', 'average', 'verbose')


def InitCommandExecuter(mock=False):
  # pylint: disable=global-statement
  global mock_default
  # Whether to default to a mock command executer or not
  mock_default = mock


def GetCommandExecuter(logger_to_set=None, mock=False, log_level='verbose'):
  # If the default is a mock executer, always return one.
  if mock_default or mock:
    return MockCommandExecuter(log_level, logger_to_set)
  else:
    return CommandExecuter(log_level, logger_to_set)


class CommandExecuter(object):
  """Provides several methods to execute commands on several environments."""

  def __init__(self, log_level, logger_to_set=None):
    self.log_level = log_level
    if log_level == 'none':
      self.logger = None
    else:
      if logger_to_set is not None:
        self.logger = logger_to_set
      else:
        self.logger = logger.GetLogger()

  def GetLogLevel(self):
    return self.log_level

  def SetLogLevel(self, log_level):
    self.log_level = log_level

  def RunCommandGeneric(self,
                        cmd,
                        return_output=False,
                        machine=None,
                        username=None,
                        command_terminator=None,
                        command_timeout=None,
                        terminated_timeout=10,
                        print_to_console=True,
                        except_handler=lambda p, e: None):
    """Run a command.

    Returns triplet (returncode, stdout, stderr).
    """

    cmd = str(cmd)

    if self.log_level == 'quiet':
      print_to_console = False

    if self.log_level == 'verbose':
      self.logger.LogCmd(cmd, machine, username, print_to_console)
    elif self.logger:
      self.logger.LogCmdToFileOnly(cmd, machine, username)
    if command_terminator and command_terminator.IsTerminated():
      if self.logger:
        self.logger.LogError('Command was terminated!', print_to_console)
      return (1, '', '')

    if machine is not None:
      user = ''
      if username is not None:
        user = username + '@'
      cmd = "ssh -t -t %s%s -- '%s'" % (user, machine, cmd)

    # We use setsid so that the child will have a different session id
    # and we can easily kill the process group. This is also important
    # because the child will be disassociated from the parent terminal.
    # In this way the child cannot mess the parent's terminal.
    p = None
    try:
      p = subprocess.Popen(
          cmd,
          stdout=subprocess.PIPE,
          stderr=subprocess.PIPE,
          shell=True,
          preexec_fn=os.setsid,
          executable='/bin/bash')

      full_stdout = ''
      full_stderr = ''

      # Pull output from pipes, send it to file/stdout/string
      out = err = None
      pipes = [p.stdout, p.stderr]

      my_poll = select.poll()
      my_poll.register(p.stdout, select.POLLIN)
      my_poll.register(p.stderr, select.POLLIN)

      terminated_time = None
      started_time = time.time()

      while len(pipes):
        if command_terminator and command_terminator.IsTerminated():
          os.killpg(os.getpgid(p.pid), signal.SIGTERM)
          if self.logger:
            self.logger.LogError('Command received termination request. '
                                 'Killed child process group.',
                                 print_to_console)
          break

        l = my_poll.poll(100)
        for (fd, _) in l:
          if fd == p.stdout.fileno():
            out = os.read(p.stdout.fileno(), 16384)
            if return_output:
              full_stdout += out
            if self.logger:
              self.logger.LogCommandOutput(out, print_to_console)
            if out == '':
              pipes.remove(p.stdout)
              my_poll.unregister(p.stdout)
          if fd == p.stderr.fileno():
            err = os.read(p.stderr.fileno(), 16384)
            if return_output:
              full_stderr += err
            if self.logger:
              self.logger.LogCommandError(err, print_to_console)
            if err == '':
              pipes.remove(p.stderr)
              my_poll.unregister(p.stderr)

        if p.poll() is not None:
          if terminated_time is None:
            terminated_time = time.time()
          elif (terminated_timeout is not None and
                time.time() - terminated_time > terminated_timeout):
            if self.logger:
              self.logger.LogWarning('Timeout of %s seconds reached since '
                                     'process termination.' %
                                     terminated_timeout, print_to_console)
            break

        if (command_timeout is not None and
            time.time() - started_time > command_timeout):
          os.killpg(os.getpgid(p.pid), signal.SIGTERM)
          if self.logger:
            self.logger.LogWarning('Timeout of %s seconds reached since process'
                                   'started. Killed child process group.' %
                                   command_timeout, print_to_console)
          break

        if out == err == '':
          break

      p.wait()
      if return_output:
        return (p.returncode, full_stdout, full_stderr)
      return (p.returncode, '', '')
    except BaseException as e:
      except_handler(p, e)
      raise

  def RunCommand(self, *args, **kwargs):
    """Run a command.

    Takes the same arguments as RunCommandGeneric except for return_output.
    Returns a single value returncode.
    """
    # Make sure that args does not overwrite 'return_output'
    assert len(args) <= 1
    assert 'return_output' not in kwargs
    kwargs['return_output'] = False
    return self.RunCommandGeneric(*args, **kwargs)[0]

  def RunCommandWExceptionCleanup(self, *args, **kwargs):
    """Run a command and kill process if exception is thrown.

    Takes the same arguments as RunCommandGeneric except for except_handler.
    Returns same as RunCommandGeneric.
    """

    def KillProc(proc, _):
      if proc:
        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)

    # Make sure that args does not overwrite 'except_handler'
    assert len(args) <= 8
    assert 'except_handler' not in kwargs
    kwargs['except_handler'] = KillProc
    return self.RunCommandGeneric(*args, **kwargs)

  def RunCommandWOutput(self, *args, **kwargs):
    """Run a command.

    Takes the same arguments as RunCommandGeneric except for return_output.
    Returns a triplet (returncode, stdout, stderr).
    """
    # Make sure that args does not overwrite 'return_output'
    assert len(args) <= 1
    assert 'return_output' not in kwargs
    kwargs['return_output'] = True
    return self.RunCommandGeneric(*args, **kwargs)

  def RemoteAccessInitCommand(self, chromeos_root, machine):
    command = ''
    command += '\nset -- --remote=' + machine
    command += '\n. ' + chromeos_root + '/src/scripts/common.sh'
    command += '\n. ' + chromeos_root + '/src/scripts/remote_access.sh'
    command += '\nTMP=$(mktemp -d)'
    command += "\nFLAGS \"$@\" || exit 1"
    command += '\nremote_access_init'
    return command

  def WriteToTempShFile(self, contents):
    handle, command_file = tempfile.mkstemp(prefix=os.uname()[1], suffix='.sh')
    os.write(handle, '#!/bin/bash\n')
    os.write(handle, contents)
    os.close(handle)
    return command_file

  def CrosLearnBoard(self, chromeos_root, machine):
    command = self.RemoteAccessInitCommand(chromeos_root, machine)
    command += '\nlearn_board'
    command += '\necho ${FLAGS_board}'
    retval, output, _ = self.RunCommandWOutput(command)
    if self.logger:
      self.logger.LogFatalIf(retval, 'learn_board command failed')
    elif retval:
      sys.exit(1)
    return output.split()[-1]

  def CrosRunCommandGeneric(self,
                            cmd,
                            return_output=False,
                            machine=None,
                            command_terminator=None,
                            chromeos_root=None,
                            command_timeout=None,
                            terminated_timeout=10,
                            print_to_console=True):
    """Run a command on a ChromeOS box.

    Returns triplet (returncode, stdout, stderr).
    """

    if self.log_level != 'verbose':
      print_to_console = False

    if self.logger:
      self.logger.LogCmd(cmd, print_to_console=print_to_console)
      self.logger.LogFatalIf(not machine, 'No machine provided!')
      self.logger.LogFatalIf(not chromeos_root, 'chromeos_root not given!')
    else:
      if not chromeos_root or not machine:
        sys.exit(1)
    chromeos_root = os.path.expanduser(chromeos_root)

    # Write all commands to a file.
    command_file = self.WriteToTempShFile(cmd)
    retval = self.CopyFiles(
        command_file,
        command_file,
        dest_machine=machine,
        command_terminator=command_terminator,
        chromeos_root=chromeos_root,
        dest_cros=True,
        recursive=False,
        print_to_console=print_to_console)
    if retval:
      if self.logger:
        self.logger.LogError('Could not run remote command on machine.'
                             ' Is the machine up?')
      return (retval, '', '')

    command = self.RemoteAccessInitCommand(chromeos_root, machine)
    command += '\nremote_sh bash %s' % command_file
    command += "\nl_retval=$?; echo \"$REMOTE_OUT\"; exit $l_retval"
    retval = self.RunCommandGeneric(
        command,
        return_output,
        command_terminator=command_terminator,
        command_timeout=command_timeout,
        terminated_timeout=terminated_timeout,
        print_to_console=print_to_console)
    if return_output:
      connect_signature = (
          'Initiating first contact with remote host\n' + 'Connection OK\n')
      connect_signature_re = re.compile(connect_signature)
      modded_retval = list(retval)
      modded_retval[1] = connect_signature_re.sub('', retval[1])
      return modded_retval
    return retval

  def CrosRunCommand(self, *args, **kwargs):
    """Run a command on a ChromeOS box.

    Takes the same arguments as CrosRunCommandGeneric except for return_output.
    Returns a single value returncode.
    """
    # Make sure that args does not overwrite 'return_output'
    assert len(args) <= 1
    assert 'return_output' not in kwargs
    kwargs['return_output'] = False
    return self.CrosRunCommandGeneric(*args, **kwargs)[0]

  def CrosRunCommandWOutput(self, *args, **kwargs):
    """Run a command on a ChromeOS box.

    Takes the same arguments as CrosRunCommandGeneric except for return_output.
    Returns a triplet (returncode, stdout, stderr).
    """
    # Make sure that args does not overwrite 'return_output'
    assert len(args) <= 1
    assert 'return_output' not in kwargs
    kwargs['return_output'] = True
    return self.CrosRunCommandGeneric(*args, **kwargs)

  def ChrootRunCommandGeneric(self,
                              chromeos_root,
                              command,
                              return_output=False,
                              command_terminator=None,
                              command_timeout=None,
                              terminated_timeout=10,
                              print_to_console=True,
                              cros_sdk_options=''):
    """Runs a command within the chroot.

    Returns triplet (returncode, stdout, stderr).
    """

    if self.log_level != 'verbose':
      print_to_console = False

    if self.logger:
      self.logger.LogCmd(command, print_to_console=print_to_console)

    handle, command_file = tempfile.mkstemp(
        dir=os.path.join(chromeos_root, 'src/scripts'),
        suffix='.sh',
        prefix='in_chroot_cmd')
    os.write(handle, '#!/bin/bash\n')
    os.write(handle, command)
    os.write(handle, '\n')
    os.close(handle)

    os.chmod(command_file, 0777)

    # if return_output is set, run a dummy command first to make sure that
    # the chroot already exists. We want the final returned output to skip
    # the output from chroot creation steps.
    if return_output:
      ret = self.RunCommand('cd %s; cros_sdk %s -- true' % (chromeos_root,
                                                            cros_sdk_options))
      if ret:
        return (ret, '', '')

    # Run command_file inside the chroot, making sure that any "~" is expanded
    # by the shell inside the chroot, not outside.
    command = ("cd %s; cros_sdk %s -- bash -c '%s/%s'" %
               (chromeos_root, cros_sdk_options, misc.CHROMEOS_SCRIPTS_DIR,
                os.path.basename(command_file)))
    ret = self.RunCommandGeneric(
        command,
        return_output,
        command_terminator=command_terminator,
        command_timeout=command_timeout,
        terminated_timeout=terminated_timeout,
        print_to_console=print_to_console)
    os.remove(command_file)
    return ret

  def ChrootRunCommand(self, *args, **kwargs):
    """Runs a command within the chroot.

    Takes the same arguments as ChrootRunCommandGeneric except for
    return_output.
    Returns a single value returncode.
    """
    # Make sure that args does not overwrite 'return_output'
    assert len(args) <= 2
    assert 'return_output' not in kwargs
    kwargs['return_output'] = False
    return self.ChrootRunCommandGeneric(*args, **kwargs)[0]

  def ChrootRunCommandWOutput(self, *args, **kwargs):
    """Runs a command within the chroot.

    Takes the same arguments as ChrootRunCommandGeneric except for
    return_output.
    Returns a triplet (returncode, stdout, stderr).
    """
    # Make sure that args does not overwrite 'return_output'
    assert len(args) <= 2
    assert 'return_output' not in kwargs
    kwargs['return_output'] = True
    return self.ChrootRunCommandGeneric(*args, **kwargs)

  def RunCommands(self,
                  cmdlist,
                  machine=None,
                  username=None,
                  command_terminator=None):
    cmd = ' ;\n'.join(cmdlist)
    return self.RunCommand(
        cmd,
        machine=machine,
        username=username,
        command_terminator=command_terminator)

  def CopyFiles(self,
                src,
                dest,
                src_machine=None,
                dest_machine=None,
                src_user=None,
                dest_user=None,
                recursive=True,
                command_terminator=None,
                chromeos_root=None,
                src_cros=False,
                dest_cros=False,
                print_to_console=True):
    src = os.path.expanduser(src)
    dest = os.path.expanduser(dest)

    if recursive:
      src = src + '/'
      dest = dest + '/'

    if src_cros == True or dest_cros == True:
      if self.logger:
        self.logger.LogFatalIf(src_cros == dest_cros,
                               'Only one of src_cros and desc_cros can '
                               'be True.')
        self.logger.LogFatalIf(not chromeos_root, 'chromeos_root not given!')
      elif src_cros == dest_cros or not chromeos_root:
        sys.exit(1)
      if src_cros == True:
        cros_machine = src_machine
      else:
        cros_machine = dest_machine

      command = self.RemoteAccessInitCommand(chromeos_root, cros_machine)
      ssh_command = (
          'ssh -p ${FLAGS_ssh_port}' + ' -o StrictHostKeyChecking=no' +
          ' -o UserKnownHostsFile=$(mktemp)' + ' -i $TMP_PRIVATE_KEY')
      rsync_prefix = "\nrsync -r -e \"%s\" " % ssh_command
      if dest_cros == True:
        command += rsync_prefix + '%s root@%s:%s' % (src, dest_machine, dest)
        return self.RunCommand(
            command,
            machine=src_machine,
            username=src_user,
            command_terminator=command_terminator,
            print_to_console=print_to_console)
      else:
        command += rsync_prefix + 'root@%s:%s %s' % (src_machine, src, dest)
        return self.RunCommand(
            command,
            machine=dest_machine,
            username=dest_user,
            command_terminator=command_terminator,
            print_to_console=print_to_console)

    if dest_machine == src_machine:
      command = 'rsync -a %s %s' % (src, dest)
    else:
      if src_machine is None:
        src_machine = os.uname()[1]
        src_user = getpass.getuser()
      command = 'rsync -a %s@%s:%s %s' % (src_user, src_machine, src, dest)
    return self.RunCommand(
        command,
        machine=dest_machine,
        username=dest_user,
        command_terminator=command_terminator,
        print_to_console=print_to_console)

  def RunCommand2(self,
                  cmd,
                  cwd=None,
                  line_consumer=None,
                  timeout=None,
                  shell=True,
                  join_stderr=True,
                  env=None,
                  except_handler=lambda p, e: None):
    """Run the command with an extra feature line_consumer.

    This version allow developers to provide a line_consumer which will be
    fed execution output lines.

    A line_consumer is a callback, which is given a chance to run for each
    line the execution outputs (either to stdout or stderr). The
    line_consumer must accept one and exactly one dict argument, the dict
    argument has these items -
      'line'   -  The line output by the binary. Notice, this string includes
                  the trailing '\n'.
      'output' -  Whether this is a stdout or stderr output, values are either
                  'stdout' or 'stderr'. When join_stderr is True, this value
                  will always be 'output'.
      'pobject' - The object used to control execution, for example, call
                  pobject.kill().

    Note: As this is written, the stdin for the process executed is
    not associated with the stdin of the caller of this routine.

    Args:
      cmd: Command in a single string.
      cwd: Working directory for execution.
      line_consumer: A function that will ba called by this function. See above
        for details.
      timeout: terminate command after this timeout.
      shell: Whether to use a shell for execution.
      join_stderr: Whether join stderr to stdout stream.
      env: Execution environment.
      except_handler: Callback for when exception is thrown during command
        execution. Passed process object and exception.

    Returns:
      Execution return code.

    Raises:
      child_exception: if fails to start the command process (missing
                       permission, no such file, etc)
    """

    class StreamHandler(object):
      """Internal utility class."""

      def __init__(self, pobject, fd, name, line_consumer):
        self._pobject = pobject
        self._fd = fd
        self._name = name
        self._buf = ''
        self._line_consumer = line_consumer

      def read_and_notify_line(self):
        t = os.read(fd, 1024)
        self._buf = self._buf + t
        self.notify_line()

      def notify_line(self):
        p = self._buf.find('\n')
        while p >= 0:
          self._line_consumer(
              line=self._buf[:p + 1], output=self._name, pobject=self._pobject)
          if p < len(self._buf) - 1:
            self._buf = self._buf[p + 1:]
            p = self._buf.find('\n')
          else:
            self._buf = ''
            p = -1
            break

      def notify_eos(self):
        # Notify end of stream. The last line may not end with a '\n'.
        if self._buf != '':
          self._line_consumer(
              line=self._buf, output=self._name, pobject=self._pobject)
          self._buf = ''

    if self.log_level == 'verbose':
      self.logger.LogCmd(cmd)
    elif self.logger:
      self.logger.LogCmdToFileOnly(cmd)

    # We use setsid so that the child will have a different session id
    # and we can easily kill the process group. This is also important
    # because the child will be disassociated from the parent terminal.
    # In this way the child cannot mess the parent's terminal.
    pobject = None
    try:
      pobject = subprocess.Popen(
          cmd,
          cwd=cwd,
          bufsize=1024,
          env=env,
          shell=shell,
          universal_newlines=True,
          stdout=subprocess.PIPE,
          stderr=subprocess.STDOUT if join_stderr else subprocess.PIPE,
          preexec_fn=os.setsid)

      # We provide a default line_consumer
      if line_consumer is None:
        line_consumer = lambda **d: None
      start_time = time.time()
      poll = select.poll()
      outfd = pobject.stdout.fileno()
      poll.register(outfd, select.POLLIN | select.POLLPRI)
      handlermap = {
          outfd: StreamHandler(pobject, outfd, 'stdout', line_consumer)
      }
      if not join_stderr:
        errfd = pobject.stderr.fileno()
        poll.register(errfd, select.POLLIN | select.POLLPRI)
        handlermap[errfd] = StreamHandler(pobject, errfd, 'stderr',
                                          line_consumer)
      while len(handlermap):
        readables = poll.poll(300)
        for (fd, evt) in readables:
          handler = handlermap[fd]
          if evt & (select.POLLPRI | select.POLLIN):
            handler.read_and_notify_line()
          elif evt & (select.POLLHUP | select.POLLERR | select.POLLNVAL):
            handler.notify_eos()
            poll.unregister(fd)
            del handlermap[fd]

        if timeout is not None and (time.time() - start_time > timeout):
          os.killpg(os.getpgid(pobject.pid), signal.SIGTERM)

      return pobject.wait()
    except BaseException as e:
      except_handler(pobject, e)
      raise


class MockCommandExecuter(CommandExecuter):
  """Mock class for class CommandExecuter."""

  def __init__(self, log_level, logger_to_set=None):
    super(MockCommandExecuter, self).__init__(log_level, logger_to_set)

  def RunCommandGeneric(self,
                        cmd,
                        return_output=False,
                        machine=None,
                        username=None,
                        command_terminator=None,
                        command_timeout=None,
                        terminated_timeout=10,
                        print_to_console=True,
                        except_handler=lambda p, e: None):
    assert not command_timeout
    cmd = str(cmd)
    if machine is None:
      machine = 'localhost'
    if username is None:
      username = 'current'
    logger.GetLogger().LogCmd('(Mock) ' + cmd, machine, username,
                              print_to_console)
    return (0, '', '')

  def RunCommand(self, *args, **kwargs):
    assert 'return_output' not in kwargs
    kwargs['return_output'] = False
    return self.RunCommandGeneric(*args, **kwargs)[0]

  def RunCommandWOutput(self, *args, **kwargs):
    assert 'return_output' not in kwargs
    kwargs['return_output'] = True
    return self.RunCommandGeneric(*args, **kwargs)


class CommandTerminator(object):
  """Object to request termination of a command in execution."""

  def __init__(self):
    self.terminated = False

  def Terminate(self):
    self.terminated = True

  def IsTerminated(self):
    return self.terminated
