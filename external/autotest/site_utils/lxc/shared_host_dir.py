# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.site_utils.lxc import constants
from autotest_lib.site_utils.lxc import utils as lxc_utils


# Cleaning up the bind mount can sometimes be blocked if a process is active in
# the directory.  Give cleanup operations about 10 seconds to complete.  This is
# only an approximate measure.
_RETRY_MAX_SECONDS = 10


class SharedHostDir(object):
    """A class that manages the shared host directory.

    Instantiating this class sets up a shared host directory at the specified
    path.  The directory is cleaned up and unmounted when cleanup is called.
    """

    def __init__(self,
                 path = constants.DEFAULT_SHARED_HOST_PATH,
                 force_delete = False):
        """Sets up the shared host directory.

        @param shared_host_path: The location of the shared host path.
        @param force_delete: If True, the host dir will be cleared and
                             reinitialized if it already exists.
        """
        self.path = os.path.realpath(path)

        # If the host dir exists and is valid and force_delete is not set, there
        # is nothing to do.  Otherwise, clear the host dir if it exists, then
        # recreate it.  Do not use lxc_utils.path_exists as that forces a sudo
        # call - the SharedHostDir is used all over the place, and
        # instantiatinng one should not cause the user to have to enter their
        # password if the host dir already exists.  The host dir is created with
        # open permissions so it should be accessible without sudo.
        if os.path.isdir(self.path):
            if not force_delete and self._host_dir_is_valid():
                return
            else:
                self.cleanup()

        utils.run('sudo mkdir "%(path)s" && '
                  'sudo chmod 777 "%(path)s" && '
                  'sudo mount --bind "%(path)s" "%(path)s" && '
                  'sudo mount --make-shared "%(path)s"' %
                  {'path': self.path})


    def cleanup(self, timeout=_RETRY_MAX_SECONDS):
        """Removes the shared host directory.

        This should only be called after all containers have been destroyed
        (i.e. all host mounts have been disconnected and removed, so the shared
        host directory should be empty).

        @param timeout: Unmounting and deleting the mount point can run into
                        race conditions vs the kernel sometimes.  This parameter
                        specifies the number of seconds for which to keep
                        waiting and retrying the umount/rm commands before
                        raising a CmdError.  The default of _RETRY_MAX_SECONDS
                        should work; this parameter is for tests to substitute a
                        different time out.

        @raises CmdError: If any of the commands involved in unmounting or
                          deleting the mount point fail even after retries.
        """
        if not os.path.exists(self.path):
            return

        # Unmount and delete everything in the host path.
        for info in lxc_utils.get_mount_info():
            if lxc_utils.is_subdir(self.path, info.mount_point):
                utils.run('sudo umount "%s"' % info.mount_point)

        # It's possible that the directory is no longer mounted (e.g. if the
        # system was rebooted), so check before unmounting.
        if utils.run('findmnt %s > /dev/null' % self.path,
                     ignore_status=True).exit_status == 0:
            self._try_umount(timeout)
        self._try_rm(timeout)


    def _try_umount(self, timeout):
        """Tries to unmount the shared host dir.

        If the unmount fails, it is retried approximately once a second, for
        <timeout> seconds.  If the command still fails, a CmdError is raised.

        @param timeout: A timeout in seconds for which to retry the command.

        @raises CmdError: If the command has not succeeded after
                          _RETRY_MAX_SECONDS.
        """
        @retry.retry(error.CmdError, timeout_min=timeout/60.0,
                     delay_sec=1)
        def run_with_retry():
            """Actually unmounts the shared host dir.  Internal function."""
            utils.run('sudo umount %s' % self.path)
        run_with_retry()


    def _try_rm(self, timeout):
        """Tries to remove the shared host dir.

        If the rm command fails, it is retried approximately once a second, for
        <timeout> seconds.  If the command still fails, a CmdError is raised.

        @param timeout: A timeout in seconds for which to retry the command.

        @raises CmdError: If the command has not succeeded after
                          _RETRY_MAX_SECONDS.
        """
        @retry.retry(error.CmdError, timeout_min=timeout/60.0,
                     delay_sec=1)
        def run_with_retry():
            """Actually removes the shared host dir.  Internal function."""
            utils.run('sudo rm -r "%s"' % self.path)
        run_with_retry()


    def _host_dir_is_valid(self):
        """Verifies that the shared host directory is set up correctly."""
        logging.debug('Verifying existing host path: %s', self.path)
        host_mount = list(lxc_utils.get_mount_info(self.path))

        # Check that the host mount exists and is shared
        if host_mount:
            if 'shared' in host_mount[0].tags:
                return True
            else:
                logging.debug('Host mount not shared (%r).', host_mount)
        else:
            logging.debug('Host mount not found.')

        return False
