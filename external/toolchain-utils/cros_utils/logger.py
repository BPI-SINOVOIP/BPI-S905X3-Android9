# Copyright 2010 Google Inc. All Rights Reserved.
"""Logging helper module."""

from __future__ import print_function

# System modules
import os.path
import sys
import traceback


#TODO(yunlian@google.com): Use GetRoot from misc
def GetRoot(scr_name):
  """Break up pathname into (dir+name)."""
  abs_path = os.path.abspath(scr_name)
  return (os.path.dirname(abs_path), os.path.basename(abs_path))


class Logger(object):
  """Logging helper class."""

  MAX_LOG_FILES = 10

  def __init__(self, rootdir, basefilename, print_console, subdir='logs'):
    logdir = os.path.join(rootdir, subdir)
    basename = os.path.join(logdir, basefilename)

    try:
      os.makedirs(logdir)
    except OSError:
      pass
      # print("Warning: Logs directory '%s' already exists." % logdir)

    self.print_console = print_console

    self._CreateLogFileHandles(basename)

    self._WriteTo(self.cmdfd, ' '.join(sys.argv), True)

  def _AddSuffix(self, basename, suffix):
    return '%s%s' % (basename, suffix)

  def _FindSuffix(self, basename):
    timestamps = []
    found_suffix = None
    for i in range(self.MAX_LOG_FILES):
      suffix = str(i)
      suffixed_basename = self._AddSuffix(basename, suffix)
      cmd_file = '%s.cmd' % suffixed_basename
      if not os.path.exists(cmd_file):
        found_suffix = suffix
        break
      timestamps.append(os.stat(cmd_file).st_mtime)

    if found_suffix:
      return found_suffix

    # Try to pick the oldest file with the suffix and return that one.
    suffix = str(timestamps.index(min(timestamps)))
    # print ("Warning: Overwriting log file: %s" %
    #       self._AddSuffix(basename, suffix))
    return suffix

  def _CreateLogFileHandle(self, name):
    fd = None
    try:
      fd = open(name, 'w')
    except IOError:
      print('Warning: could not open %s for writing.' % name)
    return fd

  def _CreateLogFileHandles(self, basename):
    suffix = self._FindSuffix(basename)
    suffixed_basename = self._AddSuffix(basename, suffix)

    self.cmdfd = self._CreateLogFileHandle('%s.cmd' % suffixed_basename)
    self.stdout = self._CreateLogFileHandle('%s.out' % suffixed_basename)
    self.stderr = self._CreateLogFileHandle('%s.err' % suffixed_basename)

    self._CreateLogFileSymlinks(basename, suffixed_basename)

  # Symlink unsuffixed basename to currently suffixed one.
  def _CreateLogFileSymlinks(self, basename, suffixed_basename):
    try:
      for extension in ['cmd', 'out', 'err']:
        src_file = '%s.%s' % (os.path.basename(suffixed_basename), extension)
        dest_file = '%s.%s' % (basename, extension)
        if os.path.exists(dest_file):
          os.remove(dest_file)
        os.symlink(src_file, dest_file)
    except Exception as ex:
      print('Exception while creating symlinks: %s' % str(ex))

  def _WriteTo(self, fd, msg, flush):
    if fd:
      fd.write(msg)
      if flush:
        fd.flush()

  def LogStartDots(self, print_to_console=True):
    term_fd = self._GetStdout(print_to_console)
    if term_fd:
      term_fd.flush()
      term_fd.write('. ')
      term_fd.flush()

  def LogAppendDot(self, print_to_console=True):
    term_fd = self._GetStdout(print_to_console)
    if term_fd:
      term_fd.write('. ')
      term_fd.flush()

  def LogEndDots(self, print_to_console=True):
    term_fd = self._GetStdout(print_to_console)
    if term_fd:
      term_fd.write('\n')
      term_fd.flush()

  def LogMsg(self, file_fd, term_fd, msg, flush=True):
    if file_fd:
      self._WriteTo(file_fd, msg, flush)
    if self.print_console:
      self._WriteTo(term_fd, msg, flush)

  def _GetStdout(self, print_to_console):
    if print_to_console:
      return sys.stdout
    return None

  def _GetStderr(self, print_to_console):
    if print_to_console:
      return sys.stderr
    return None

  def LogCmdToFileOnly(self, cmd, machine='', user=None):
    if not self.cmdfd:
      return

    host = ('%s@%s' % (user, machine)) if user else machine
    flush = True
    cmd_string = 'CMD (%s): %s\n' % (host, cmd)
    self._WriteTo(self.cmdfd, cmd_string, flush)

  def LogCmd(self, cmd, machine='', user=None, print_to_console=True):
    if user:
      host = '%s@%s' % (user, machine)
    else:
      host = machine

    self.LogMsg(self.cmdfd, self._GetStdout(print_to_console),
                'CMD (%s): %s\n' % (host, cmd))

  def LogFatal(self, msg, print_to_console=True):
    self.LogMsg(self.stderr, self._GetStderr(print_to_console),
                'FATAL: %s\n' % msg)
    self.LogMsg(self.stderr, self._GetStderr(print_to_console),
                '\n'.join(traceback.format_stack()))
    sys.exit(1)

  def LogError(self, msg, print_to_console=True):
    self.LogMsg(self.stderr, self._GetStderr(print_to_console),
                'ERROR: %s\n' % msg)

  def LogWarning(self, msg, print_to_console=True):
    self.LogMsg(self.stderr, self._GetStderr(print_to_console),
                'WARNING: %s\n' % msg)

  def LogOutput(self, msg, print_to_console=True):
    self.LogMsg(self.stdout, self._GetStdout(print_to_console),
                'OUTPUT: %s\n' % msg)

  def LogFatalIf(self, condition, msg):
    if condition:
      self.LogFatal(msg)

  def LogErrorIf(self, condition, msg):
    if condition:
      self.LogError(msg)

  def LogWarningIf(self, condition, msg):
    if condition:
      self.LogWarning(msg)

  def LogCommandOutput(self, msg, print_to_console=True):
    self.LogMsg(self.stdout,
                self._GetStdout(print_to_console),
                msg,
                flush=False)

  def LogCommandError(self, msg, print_to_console=True):
    self.LogMsg(self.stderr,
                self._GetStderr(print_to_console),
                msg,
                flush=False)

  def Flush(self):
    self.cmdfd.flush()
    self.stdout.flush()
    self.stderr.flush()


class MockLogger(object):
  """Logging helper class."""

  MAX_LOG_FILES = 10

  def __init__(self, *_args, **_kwargs):
    self.stdout = sys.stdout
    self.stderr = sys.stderr
    return None

  def _AddSuffix(self, basename, suffix):
    return '%s%s' % (basename, suffix)

  def _FindSuffix(self, basename):
    timestamps = []
    found_suffix = None
    for i in range(self.MAX_LOG_FILES):
      suffix = str(i)
      suffixed_basename = self._AddSuffix(basename, suffix)
      cmd_file = '%s.cmd' % suffixed_basename
      if not os.path.exists(cmd_file):
        found_suffix = suffix
        break
      timestamps.append(os.stat(cmd_file).st_mtime)

    if found_suffix:
      return found_suffix

    # Try to pick the oldest file with the suffix and return that one.
    suffix = str(timestamps.index(min(timestamps)))
    # print ("Warning: Overwriting log file: %s" %
    #       self._AddSuffix(basename, suffix))
    return suffix

  def _CreateLogFileHandle(self, name):
    print('MockLogger: creating open file handle for %s (writing)' % name)

  def _CreateLogFileHandles(self, basename):
    suffix = self._FindSuffix(basename)
    suffixed_basename = self._AddSuffix(basename, suffix)

    print('MockLogger: opening file %s.cmd' % suffixed_basename)
    print('MockLogger: opening file %s.out' % suffixed_basename)
    print('MockLogger: opening file %s.err' % suffixed_basename)

    self._CreateLogFileSymlinks(basename, suffixed_basename)

  # Symlink unsuffixed basename to currently suffixed one.
  def _CreateLogFileSymlinks(self, basename, suffixed_basename):
    for extension in ['cmd', 'out', 'err']:
      src_file = '%s.%s' % (os.path.basename(suffixed_basename), extension)
      dest_file = '%s.%s' % (basename, extension)
      print('MockLogger: Calling os.symlink(%s, %s)' % (src_file, dest_file))

  def _WriteTo(self, _fd, msg, _flush):
    print('MockLogger: %s' % msg)

  def LogStartDots(self, _print_to_console=True):
    print('. ')

  def LogAppendDot(self, _print_to_console=True):
    print('. ')

  def LogEndDots(self, _print_to_console=True):
    print('\n')

  def LogMsg(self, _file_fd, _term_fd, msg, **_kwargs):
    print('MockLogger: %s' % msg)

  def _GetStdout(self, _print_to_console):
    return None

  def _GetStderr(self, _print_to_console):
    return None

  def LogCmdToFileOnly(self, *_args, **_kwargs):
    return

  # def LogCmdToFileOnly(self, cmd, machine='', user=None):
  #   host = ('%s@%s' % (user, machine)) if user else machine
  #   cmd_string = 'CMD (%s): %s\n' % (host, cmd)
  #   print('MockLogger: Writing to file ONLY: %s' % cmd_string)

  def LogCmd(self, cmd, machine='', user=None, print_to_console=True):
    if user:
      host = '%s@%s' % (user, machine)
    else:
      host = machine

    self.LogMsg(0, self._GetStdout(print_to_console),
                'CMD (%s): %s\n' % (host, cmd))

  def LogFatal(self, msg, print_to_console=True):
    self.LogMsg(0, self._GetStderr(print_to_console), 'FATAL: %s\n' % msg)
    self.LogMsg(0, self._GetStderr(print_to_console),
                '\n'.join(traceback.format_stack()))
    print('MockLogger: Calling sysexit(1)')

  def LogError(self, msg, print_to_console=True):
    self.LogMsg(0, self._GetStderr(print_to_console), 'ERROR: %s\n' % msg)

  def LogWarning(self, msg, print_to_console=True):
    self.LogMsg(0, self._GetStderr(print_to_console), 'WARNING: %s\n' % msg)

  def LogOutput(self, msg, print_to_console=True):
    self.LogMsg(0, self._GetStdout(print_to_console), 'OUTPUT: %s\n' % msg)

  def LogFatalIf(self, condition, msg):
    if condition:
      self.LogFatal(msg)

  def LogErrorIf(self, condition, msg):
    if condition:
      self.LogError(msg)

  def LogWarningIf(self, condition, msg):
    if condition:
      self.LogWarning(msg)

  def LogCommandOutput(self, msg, print_to_console=True):
    self.LogMsg(self.stdout,
                self._GetStdout(print_to_console),
                msg,
                flush=False)

  def LogCommandError(self, msg, print_to_console=True):
    self.LogMsg(self.stderr,
                self._GetStderr(print_to_console),
                msg,
                flush=False)

  def Flush(self):
    print('MockLogger: Flushing cmdfd, stdout, stderr')


main_logger = None


def InitLogger(script_name, log_dir, print_console=True, mock=False):
  """Initialize a global logger. To be called only once."""
  # pylint: disable=global-statement
  global main_logger
  assert not main_logger, 'The logger has already been initialized'
  rootdir, basefilename = GetRoot(script_name)
  if not log_dir:
    log_dir = rootdir
  if not mock:
    main_logger = Logger(log_dir, basefilename, print_console)
  else:
    main_logger = MockLogger(log_dir, basefilename, print_console)


def GetLogger(log_dir='', mock=False):
  if not main_logger:
    InitLogger(sys.argv[0], log_dir, mock=mock)
  return main_logger


def HandleUncaughtExceptions(fun):
  """Catches all exceptions that would go outside decorated fun scope."""

  def _Interceptor(*args, **kwargs):
    try:
      return fun(*args, **kwargs)
    except StandardError:
      GetLogger().LogFatal('Uncaught exception:\n%s' % traceback.format_exc())

  return _Interceptor
