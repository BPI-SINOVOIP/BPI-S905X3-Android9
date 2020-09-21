#!/usr/bin/env python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Script to lock/unlock machines."""

from __future__ import print_function

__author__ = 'asharif@google.com (Ahmad Sharif)'

import argparse
import datetime
import fcntl
import getpass
import glob
import json
import os
import socket
import sys
import time

from cros_utils import logger

LOCK_SUFFIX = '_check_lock_liveness'

# The locks file directory REQUIRES that 'group' only has read/write
# privileges and 'world' has no privileges.  So the mask must be
# '0027': 0777 - 0027 = 0750.
LOCK_MASK = 0027


def FileCheckName(name):
  return name + LOCK_SUFFIX


def OpenLiveCheck(file_name):
  with FileCreationMask(LOCK_MASK):
    fd = open(file_name, 'a')
  try:
    fcntl.lockf(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
  except IOError:
    raise
  return fd


class FileCreationMask(object):
  """Class for the file creation mask."""

  def __init__(self, mask):
    self._mask = mask
    self._old_mask = None

  def __enter__(self):
    self._old_mask = os.umask(self._mask)

  def __exit__(self, typ, value, traceback):
    os.umask(self._old_mask)


class LockDescription(object):
  """The description of the lock."""

  def __init__(self, desc=None):
    try:
      self.owner = desc['owner']
      self.exclusive = desc['exclusive']
      self.counter = desc['counter']
      self.time = desc['time']
      self.reason = desc['reason']
      self.auto = desc['auto']
    except (KeyError, TypeError):
      self.owner = ''
      self.exclusive = False
      self.counter = 0
      self.time = 0
      self.reason = ''
      self.auto = False

  def IsLocked(self):
    return self.counter or self.exclusive

  def __str__(self):
    return ' '.join([
        'Owner: %s' % self.owner, 'Exclusive: %s' % self.exclusive,
        'Counter: %s' % self.counter, 'Time: %s' % self.time,
        'Reason: %s' % self.reason, 'Auto: %s' % self.auto
    ])


class FileLock(object):
  """File lock operation class."""
  FILE_OPS = []

  def __init__(self, lock_filename):
    self._filepath = lock_filename
    lock_dir = os.path.dirname(lock_filename)
    assert os.path.isdir(lock_dir), ("Locks dir: %s doesn't exist!" % lock_dir)
    self._file = None
    self._description = None

  def getDescription(self):
    return self._description

  def getFilePath(self):
    return self._filepath

  def setDescription(self, desc):
    self._description = desc

  @classmethod
  def AsString(cls, file_locks):
    stringify_fmt = '%-30s %-15s %-4s %-4s %-15s %-40s %-4s'
    header = stringify_fmt % ('machine', 'owner', 'excl', 'ctr', 'elapsed',
                              'reason', 'auto')
    lock_strings = []
    for file_lock in file_locks:

      elapsed_time = datetime.timedelta(
          seconds=int(time.time() - file_lock.getDescription().time))
      elapsed_time = '%s ago' % elapsed_time
      lock_strings.append(
          stringify_fmt %
          (os.path.basename(file_lock.getFilePath),
           file_lock.getDescription().owner,
           file_lock.getDescription().exclusive,
           file_lock.getDescription().counter, elapsed_time,
           file_lock.getDescription().reason, file_lock.getDescription().auto))
    table = '\n'.join(lock_strings)
    return '\n'.join([header, table])

  @classmethod
  def ListLock(cls, pattern, locks_dir):
    if not locks_dir:
      locks_dir = Machine.LOCKS_DIR
    full_pattern = os.path.join(locks_dir, pattern)
    file_locks = []
    for lock_filename in glob.glob(full_pattern):
      if LOCK_SUFFIX in lock_filename:
        continue
      file_lock = FileLock(lock_filename)
      with file_lock as lock:
        if lock.IsLocked():
          file_locks.append(file_lock)
    logger.GetLogger().LogOutput('\n%s' % cls.AsString(file_locks))

  def __enter__(self):
    with FileCreationMask(LOCK_MASK):
      try:
        self._file = open(self._filepath, 'a+')
        self._file.seek(0, os.SEEK_SET)

        if fcntl.flock(self._file.fileno(), fcntl.LOCK_EX) == -1:
          raise IOError('flock(%s, LOCK_EX) failed!' % self._filepath)

        try:
          desc = json.load(self._file)
        except (EOFError, ValueError):
          desc = None
        self._description = LockDescription(desc)

        if self._description.exclusive and self._description.auto:
          locked_byself = False
          for fd in self.FILE_OPS:
            if fd.name == FileCheckName(self._filepath):
              locked_byself = True
              break
          if not locked_byself:
            try:
              fp = OpenLiveCheck(FileCheckName(self._filepath))
            except IOError:
              pass
            else:
              self._description = LockDescription()
              fcntl.lockf(fp, fcntl.LOCK_UN)
              fp.close()
        return self._description
      # Check this differently?
      except IOError as ex:
        logger.GetLogger().LogError(ex)
        return None

  def __exit__(self, typ, value, traceback):
    self._file.truncate(0)
    self._file.write(json.dumps(self._description.__dict__, skipkeys=True))
    self._file.close()

  def __str__(self):
    return self.AsString([self])


class Lock(object):
  """Lock class"""

  def __init__(self, lock_file, auto=True):
    self._to_lock = os.path.basename(lock_file)
    self._lock_file = lock_file
    self._logger = logger.GetLogger()
    self._auto = auto

  def NonBlockingLock(self, exclusive, reason=''):
    with FileLock(self._lock_file) as lock:
      if lock.exclusive:
        self._logger.LogError(
            'Exclusive lock already acquired by %s. Reason: %s' % (lock.owner,
                                                                   lock.reason))
        return False

      if exclusive:
        if lock.counter:
          self._logger.LogError('Shared lock already acquired')
          return False
        lock_file_check = FileCheckName(self._lock_file)
        fd = OpenLiveCheck(lock_file_check)
        FileLock.FILE_OPS.append(fd)

        lock.exclusive = True
        lock.reason = reason
        lock.owner = getpass.getuser()
        lock.time = time.time()
        lock.auto = self._auto
      else:
        lock.counter += 1
    self._logger.LogOutput('Successfully locked: %s' % self._to_lock)
    return True

  def Unlock(self, exclusive, force=False):
    with FileLock(self._lock_file) as lock:
      if not lock.IsLocked():
        self._logger.LogWarning("Can't unlock unlocked machine!")
        return True

      if lock.exclusive != exclusive:
        self._logger.LogError('shared locks must be unlocked with --shared')
        return False

      if lock.exclusive:
        if lock.owner != getpass.getuser() and not force:
          self._logger.LogError("%s can't unlock lock owned by: %s" %
                                (getpass.getuser(), lock.owner))
          return False
        if lock.auto != self._auto:
          self._logger.LogError("Can't unlock lock with different -a"
                                ' parameter.')
          return False
        lock.exclusive = False
        lock.reason = ''
        lock.owner = ''

        if self._auto:
          del_list = [
              i for i in FileLock.FILE_OPS
              if i.name == FileCheckName(self._lock_file)
          ]
          for i in del_list:
            FileLock.FILE_OPS.remove(i)
          for f in del_list:
            fcntl.lockf(f, fcntl.LOCK_UN)
            f.close()
          del del_list
          os.remove(FileCheckName(self._lock_file))

      else:
        lock.counter -= 1
    return True


class Machine(object):
  """Machine class"""

  LOCKS_DIR = '/google/data/rw/users/mo/mobiletc-prebuild/locks'

  def __init__(self, name, locks_dir=LOCKS_DIR, auto=True):
    self._name = name
    self._auto = auto
    try:
      self._full_name = socket.gethostbyaddr(name)[0]
    except socket.error:
      self._full_name = self._name
    self._full_name = os.path.join(locks_dir, self._full_name)

  def Lock(self, exclusive=False, reason=''):
    lock = Lock(self._full_name, self._auto)
    return lock.NonBlockingLock(exclusive, reason)

  def TryLock(self, timeout=300, exclusive=False, reason=''):
    locked = False
    sleep = timeout / 10
    while True:
      locked = self.Lock(exclusive, reason)
      if locked or not timeout >= 0:
        break
      print('Lock not acquired for {0}, wait {1} seconds ...'.format(
          self._name, sleep))
      time.sleep(sleep)
      timeout -= sleep
    return locked

  def Unlock(self, exclusive=False, ignore_ownership=False):
    lock = Lock(self._full_name, self._auto)
    return lock.Unlock(exclusive, ignore_ownership)


def Main(argv):
  """The main function."""

  parser = argparse.ArgumentParser()
  parser.add_argument(
      '-r', '--reason', dest='reason', default='', help='The lock reason.')
  parser.add_argument(
      '-u',
      '--unlock',
      dest='unlock',
      action='store_true',
      default=False,
      help='Use this to unlock.')
  parser.add_argument(
      '-l',
      '--list_locks',
      dest='list_locks',
      action='store_true',
      default=False,
      help='Use this to list locks.')
  parser.add_argument(
      '-f',
      '--ignore_ownership',
      dest='ignore_ownership',
      action='store_true',
      default=False,
      help="Use this to force unlock on a lock you don't own.")
  parser.add_argument(
      '-s',
      '--shared',
      dest='shared',
      action='store_true',
      default=False,
      help='Use this for a shared (non-exclusive) lock.')
  parser.add_argument(
      '-d',
      '--dir',
      dest='locks_dir',
      action='store',
      default=Machine.LOCKS_DIR,
      help='Use this to set different locks_dir')
  parser.add_argument('args', nargs='*', help='Machine arg.')

  options = parser.parse_args(argv)

  options.locks_dir = os.path.abspath(options.locks_dir)
  exclusive = not options.shared

  if not options.list_locks and len(options.args) != 2:
    logger.GetLogger().LogError(
        'Either --list_locks or a machine arg is needed.')
    return 1

  if len(options.args) > 1:
    machine = Machine(options.args[1], options.locks_dir, auto=False)
  else:
    machine = None

  if options.list_locks:
    FileLock.ListLock('*', options.locks_dir)
    retval = True
  elif options.unlock:
    retval = machine.Unlock(exclusive, options.ignore_ownership)
  else:
    retval = machine.Lock(exclusive, options.reason)

  if retval:
    return 0
  else:
    return 1


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
