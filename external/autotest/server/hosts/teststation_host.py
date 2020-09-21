# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This class defines the TestStationHost class."""

import logging
import os

import common

from autotest_lib.client.bin import local_host
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.cros import constants as cros_constants
from autotest_lib.server.hosts import base_classes
from autotest_lib.server.hosts import moblab_host
from autotest_lib.server.hosts import ssh_host


# TODO(kevcheng): Update the creation method so it's not a research project
# determining the class inheritance model (same for factory.create_host).
def create_teststationhost(hostname, **kwargs):
    """Creates the TestStationHost object.

    @param hostname: Hostname of the test station.
    @param kwargs: Keyword args to pass to the testbed initialization.

    @return: A Test Station Host object.
    """
    classes = [TestStationHost]
    if hostname == 'localhost':
        classes.append(local_host.LocalHost)
    else:
        classes.append(ssh_host.SSHHost)
    host_class = type('new_teststationhost', tuple(classes), {})
    return host_class(hostname, **kwargs)


class TestStationHost(base_classes.Host):
    """This class represents a linux box accessible via ssh."""


    def check_credentials(self, hostname):
        """Make sure teststation credentials work if we're doing ssh.

        @param hostname: Hostname of the machine.
        """
        if hostname != 'localhost':
            try:
                self.run('true')
            except error.AutoservRunError:
                # Some test stations may not have root access, try user adb.
                logging.debug('Switching to user adb.')
                self.user = 'adb'


    def _initialize(self, hostname='localhost', *args, **dargs):
        """Initialize a Test Station Host.

        This will create a Test Station Host. Hostname should always refer
        to the host machine connected to the devices under test.

        @param hostname: Hostname of the machine, default to localhost.
        """
        logging.debug('Initializing Test Station Host running on host: %s.',
                      hostname)

        # Do parent class initializations.
        super(TestStationHost, self)._initialize(hostname=hostname, *args,
                                                 **dargs)

        self.check_credentials(hostname)

        # We'll want to do certain things differently if we're on a moblab.
        self._is_host_moblab = None
        # Keep track of whether the host was closed since multiple AdbHost
        # might have an instance of this teststation.
        self._is_closed = False


    @property
    def is_moblab(self):
        """Check if the host running adb command is a Moblab.

        @return: True if the host running adb command is a Moblab, False
                 otherwise.
        """
        if self._is_host_moblab is None:
            try:
                self.run('cat %s | grep -q moblab' % cros_constants.LSB_RELEASE)
                self._is_host_moblab = True
            except (error.AutoservRunError, error.AutotestHostRunError):
                self._is_host_moblab = False
        return self._is_host_moblab


    def get_tmp_dir(self, parent='/var/tmp'):
        """Return pathname of a temporary directory on the test station.

        If parent folder is supplied and the teststation is a moblab.  Then
        the parent will have the moblab tmp directory prepended to it.

        @param parent: The parent dir to create the temporary dir.

        @return: Path of the newly created temporary dir.
        """
        if self.is_moblab:
            parent = (moblab_host.MOBLAB_TMP_DIR if parent == '/tmp'
                      else os.path.join(moblab_host.MOBLAB_TMP_DIR,
                                        parent.lstrip('/')))
        return super(TestStationHost, self).get_tmp_dir(parent=parent)


    def run(self, cmd, force_tty=True, *args, **dargs):
        """Run a command on the adb device.

        This will run the command on the test station.  This method only
        exists to modify the command supplied if we're running a fastboot
        command on a moblab, otherwise we leave the command untouched.

        @param cmd: The command line string.
        @param force_tty: Set to True to force pseudo-terminal allocation to
                run the command. This allows the command running on remote host
                to abort when the ssh command is timed out. Default is True.

        @returns A CMDResult object or None if the call timed out and
                 ignore_timeout is True.
        """
        # TODO (sbasi/kevcheng) - Make teststation_host check if running
        # on Chrome OS, rather than MobLab when prepending sudo to fastboot.
        if cmd.startswith('fastboot ') and self.is_moblab:
            cmd = 'sudo -n ' + cmd
        if force_tty:
            dargs['options'] = dargs.get('options', '') + ' -t '
        return super(TestStationHost, self).run(cmd, *args, **dargs)

    @retry.retry(error.GenericHostRunError, timeout_min=10)
    def download_file(self, src_url, dest_file, unzip=False, unzip_dest=None):
        """Download the given url.

        @param src_url: The url to download from.
        @param dest_file: Destination for the file to be downloaded to.
        @param unzip: If True, unzip the downloaded file.
        @param unzip_dest: Location to unzip the downloaded file to. If not
                           provided, dest_file's directory is used.

        @returns: The path of the downloaded file on the teststation.
        """
        try:
            self.run('wget -q -O "%s" "%s"' % (dest_file, src_url))

            readlink_result = self.run('readlink -f "%s"' % dest_file)
            full_path = readlink_result.stdout.splitlines()[0]

            if unzip:
                unzip_dest = unzip_dest or os.path.dirname(full_path)
                self.run('unzip "%s" -x -d "%s"' % (dest_file, unzip_dest))

            return full_path
        except:
            # Delete the destination file if download failed.
            self.run('rm -f "%s"' % dest_file)
            raise
