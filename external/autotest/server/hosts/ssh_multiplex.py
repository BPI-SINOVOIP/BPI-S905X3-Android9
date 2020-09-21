# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import multiprocessing
import os
import threading
import time

from autotest_lib.client.common_lib import autotemp
from autotest_lib.server import utils

_MASTER_SSH_COMMAND_TEMPLATE = (
    '/usr/bin/ssh -a -x -N '
    '-o ControlMaster=yes '  # Create multiplex socket.
    '-o ControlPath=%(socket)s '
    '-o StrictHostKeyChecking=no '
    '-o UserKnownHostsFile=/dev/null '
    '-o BatchMode=yes '
    '-o ConnectTimeout=30 '
    '-o ServerAliveInterval=900 '
    '-o ServerAliveCountMax=3 '
    '-o ConnectionAttempts=4 '
    '-o Protocol=2 '
    '-l %(user)s -p %(port)d %(hostname)s')


class MasterSsh(object):
    """Manages multiplex ssh connection."""

    def __init__(self, hostname, user, port):
        self._hostname = hostname
        self._user = user
        self._port = port

        self._master_job = None
        self._master_tempdir = None

        self._lock = multiprocessing.Lock()

    def __del__(self):
        self.close()

    @property
    def _socket_path(self):
        return os.path.join(self._master_tempdir.name, 'socket')

    @property
    def ssh_option(self):
        """Returns the ssh option to use this multiplexed ssh.

        If background process is not running, returns an empty string.
        """
        if not self._master_tempdir:
            return ''
        return '-o ControlPath=%s' % (self._socket_path,)

    def maybe_start(self, timeout=5):
        """Starts the background process to run multiplex ssh connection.

        If there already is a background process running, this does nothing.
        If there is a stale process or a stale socket, first clean them up,
        then create a background process.

        @param timeout: timeout in seconds (default 5) to wait for master ssh
                        connection to be established. If timeout is reached, a
                        warning message is logged, but no other action is
                        taken.
        """
        # Multiple processes might try in parallel to clean up the old master
        # ssh connection and create a new one, therefore use a lock to protect
        # against race conditions.
        with self._lock:
            # If a previously started master SSH connection is not running
            # anymore, it needs to be cleaned up and then restarted.
            if (self._master_job and (not os.path.exists(self._socket_path) or
                                      self._master_job.sp.poll() is not None)):
                logging.info(
                        'Master ssh connection to %s is down.', self._hostname)
                self._close_internal()

            # Start a new master SSH connection.
            if not self._master_job:
                # Create a shared socket in a temp location.
                self._master_tempdir = autotemp.tempdir(unique_id='ssh-master')

                # Start the master SSH connection in the background.
                master_cmd = _MASTER_SSH_COMMAND_TEMPLATE % {
                        'hostname': self._hostname,
                        'user': self._user,
                        'port': self._port,
                        'socket': self._socket_path,
                }
                logging.info(
                        'Starting master ssh connection \'%s\'', master_cmd)
                self._master_job = utils.BgJob(
                         master_cmd, nickname='master-ssh',
                         stdout_tee=utils.DEVNULL, stderr_tee=utils.DEVNULL,
                         unjoinable=True)

                # To prevent a race between the the master ssh connection
                # startup and its first attempted use, wait for socket file to
                # exist before returning.
                end_time = time.time() + timeout
                while time.time() < end_time:
                    if os.path.exists(self._socket_path):
                        break
                    time.sleep(.2)
                else:
                    logging.info('Timed out waiting for master-ssh connection '
                       'to be established.')

    def close(self):
        """Releases all resources used by multiplexed ssh connection."""
        with self._lock:
            self._close_internal()

    def _close_internal(self):
        # Assume that when this is called, _lock should be acquired, already.
        if self._master_job:
            logging.debug('Nuking ssh master_job')
            utils.nuke_subprocess(self._master_job.sp)
            self._master_job = None

        if self._master_tempdir:
            logging.debug('Cleaning ssh master_tempdir')
            self._master_tempdir.clean()
            self._master_tempdir = None


class ConnectionPool(object):
    """Holds SSH multiplex connection instance."""

    def __init__(self):
        self._pool = {}
        self._lock = threading.Lock()

    def get(self, hostname, user, port):
        """Returns MasterSsh instance for the given endpoint.

        If the pool holds the instance already, returns it. If not, create the
        instance, and returns it.

        Caller has the responsibility to call maybe_start() before using it.

        @param hostname: Host name of the endpoint.
        @param user: User name to log in.
        @param port: Port number sshd is listening.
        """
        key = (hostname, user, port)
        logging.debug('Get master ssh connection for %s@%s:%d', user, hostname,
                      port)

        with self._lock:
            conn = self._pool.get(key)
            if not conn:
                conn = MasterSsh(hostname, user, port)
                self._pool[key] = conn
            return conn

    def shutdown(self):
        """Closes all ssh multiplex connections."""
        for ssh in self._pool.itervalues():
            ssh.close()
