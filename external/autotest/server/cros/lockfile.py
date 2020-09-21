
"""
lockfile.py - Platform-independent advisory file locks.

Forked from python2.7/dist-packages/lockfile version 0.8.

Usage:

>>> lock = FileLock('somefile')
>>> try:
...     lock.acquire()
... except AlreadyLocked:
...     print 'somefile', 'is locked already.'
... except LockFailed:
...     print 'somefile', 'can\\'t be locked.'
... else:
...     print 'got lock'
got lock
>>> print lock.is_locked()
True
>>> lock.release()

>>> lock = FileLock('somefile')
>>> print lock.is_locked()
False
>>> with lock:
...    print lock.is_locked()
True
>>> print lock.is_locked()
False
>>> # It is okay to lock twice from the same thread...
>>> with lock:
...     lock.acquire()
...
>>> # Though no counter is kept, so you can't unlock multiple times...
>>> print lock.is_locked()
False

Exceptions:

    Error - base class for other exceptions
        LockError - base class for all locking exceptions
            AlreadyLocked - Another thread or process already holds the lock
            LockFailed - Lock failed for some other reason
        UnlockError - base class for all unlocking exceptions
            AlreadyUnlocked - File was not locked.
            NotMyLock - File was locked but not by the current thread/process
"""

from __future__ import division

import logging
import socket
import os
import threading
import time
import urllib

# Work with PEP8 and non-PEP8 versions of threading module.
if not hasattr(threading, "current_thread"):
    threading.current_thread = threading.currentThread
if not hasattr(threading.Thread, "get_name"):
    threading.Thread.get_name = threading.Thread.getName

__all__ = ['Error', 'LockError', 'LockTimeout', 'AlreadyLocked',
           'LockFailed', 'UnlockError', 'LinkFileLock']

class Error(Exception):
    """
    Base class for other exceptions.

    >>> try:
    ...   raise Error
    ... except Exception:
    ...   pass
    """
    pass

class LockError(Error):
    """
    Base class for error arising from attempts to acquire the lock.

    >>> try:
    ...   raise LockError
    ... except Error:
    ...   pass
    """
    pass

class LockTimeout(LockError):
    """Raised when lock creation fails within a user-defined period of time.

    >>> try:
    ...   raise LockTimeout
    ... except LockError:
    ...   pass
    """
    pass

class AlreadyLocked(LockError):
    """Some other thread/process is locking the file.

    >>> try:
    ...   raise AlreadyLocked
    ... except LockError:
    ...   pass
    """
    pass

class LockFailed(LockError):
    """Lock file creation failed for some other reason.

    >>> try:
    ...   raise LockFailed
    ... except LockError:
    ...   pass
    """
    pass

class UnlockError(Error):
    """
    Base class for errors arising from attempts to release the lock.

    >>> try:
    ...   raise UnlockError
    ... except Error:
    ...   pass
    """
    pass

class LockBase(object):
    """Base class for platform-specific lock classes."""
    def __init__(self, path):
        """
        Unlike the original implementation we always assume the threaded case.
        """
        self.path = path
        self.lock_file = os.path.abspath(path) + ".lock"
        self.hostname = socket.gethostname()
        self.pid = os.getpid()
        name = threading.current_thread().get_name()
        tname = "%s-" % urllib.quote(name, safe="")
        dirname = os.path.dirname(self.lock_file)
        self.unique_name = os.path.join(dirname, "%s.%s%s" % (self.hostname,
                                                              tname, self.pid))

    def __del__(self):
        """Paranoia: We are trying hard to not leave any file behind. This
        might possibly happen in very unusual acquire exception cases."""
        if os.path.exists(self.unique_name):
            logging.warning("Removing unexpected file %s", self.unique_name)
            os.unlink(self.unique_name)

    def acquire(self, timeout=None):
        """
        Acquire the lock.

        * If timeout is omitted (or None), wait forever trying to lock the
          file.

        * If timeout > 0, try to acquire the lock for that many seconds.  If
          the lock period expires and the file is still locked, raise
          LockTimeout.

        * If timeout <= 0, raise AlreadyLocked immediately if the file is
          already locked.
        """
        raise NotImplementedError("implement in subclass")

    def release(self):
        """
        Release the lock.

        If the file is not locked, raise NotLocked.
        """
        raise NotImplementedError("implement in subclass")

    def is_locked(self):
        """
        Tell whether or not the file is locked.
        """
        raise NotImplementedError("implement in subclass")

    def i_am_locking(self):
        """
        Return True if this object is locking the file.
        """
        raise NotImplementedError("implement in subclass")

    def break_lock(self):
        """
        Remove a lock.  Useful if a locking thread failed to unlock.
        """
        raise NotImplementedError("implement in subclass")

    def age_of_lock(self):
        """
        Return the time since creation of lock in seconds.
        """
        raise NotImplementedError("implement in subclass")

    def __enter__(self):
        """
        Context manager support.
        """
        self.acquire()
        return self

    def __exit__(self, *_exc):
        """
        Context manager support.
        """
        self.release()


class LinkFileLock(LockBase):
    """Lock access to a file using atomic property of link(2)."""

    def acquire(self, timeout=None):
        try:
            open(self.unique_name, "wb").close()
        except IOError:
            raise LockFailed("failed to create %s" % self.unique_name)

        end_time = time.time()
        if timeout is not None and timeout > 0:
            end_time += timeout

        while True:
            # Try and create a hard link to it.
            try:
                os.link(self.unique_name, self.lock_file)
            except OSError:
                # Link creation failed.  Maybe we've double-locked?
                nlinks = os.stat(self.unique_name).st_nlink
                if nlinks == 2:
                    # The original link plus the one I created == 2.  We're
                    # good to go.
                    return
                else:
                    # Otherwise the lock creation failed.
                    if timeout is not None and time.time() > end_time:
                        os.unlink(self.unique_name)
                        if timeout > 0:
                            raise LockTimeout
                        else:
                            raise AlreadyLocked
                    # IHF: The original code used integer division/10.
                    time.sleep(timeout is not None and timeout / 10.0 or 0.1)
            else:
                # Link creation succeeded.  We're good to go.
                return

    def release(self):
        # IHF: I think original cleanup was not correct when somebody else broke
        # our lock and took it. Then we released the new process' lock causing
        # a cascade of wrong lock releases. Notice the SQLiteFileLock::release()
        # doesn't seem to run into this problem as it uses i_am_locking().
        if self.i_am_locking():
            # We own the lock and clean up both files.
            os.unlink(self.unique_name)
            os.unlink(self.lock_file)
            return
        if os.path.exists(self.unique_name):
            # We don't own lock_file but clean up after ourselves.
            os.unlink(self.unique_name)
        raise UnlockError

    def is_locked(self):
        """Check if anybody is holding the lock."""
        return os.path.exists(self.lock_file)

    def i_am_locking(self):
        """Check if we are holding the lock."""
        return (self.is_locked() and
                os.path.exists(self.unique_name) and
                os.stat(self.unique_name).st_nlink == 2)

    def break_lock(self):
        """Break (another processes) lock."""
        if os.path.exists(self.lock_file):
            os.unlink(self.lock_file)

    def age_of_lock(self):
        """Returns the time since creation of lock in seconds."""
        try:
            # Creating the hard link for the lock updates the change time.
            age = time.time() - os.stat(self.lock_file).st_ctime
        except OSError:
            age = -1.0
        return age


FileLock = LinkFileLock
