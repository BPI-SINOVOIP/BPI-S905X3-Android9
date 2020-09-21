# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno
import logging
import os
import os.path
import pyudev
import stat
import subprocess
import tempfile
import threading
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import device_jail_utils


class security_DeviceJail_Filesystem(test.test):
    """
    Ensures that if the device jail filesystem is present, it restricts
    the devices that can be seen and instantiates jails.
    """
    version = 1

    SHUTDOWN_TIMEOUT_SEC = 5

    def warmup(self):
        super(security_DeviceJail_Filesystem, self).warmup()
        if not os.path.exists(device_jail_utils.JAIL_CONTROL_PATH):
            raise error.TestNAError('Device jail is not present')

        self._mount = tempfile.mkdtemp(prefix='djfs_test_')
        logging.debug('Attempting to mount device_jail_fs on %s', self._mount)
        try:
            self._subprocess = subprocess.Popen(['device_jail_fs', self._mount])
        except OSError as e:
            if e.errno == errno.ENOENT:
                raise error.TestNAError('Device jail FS is not present')
            else:
                raise error.TestError(
                    'Device jail FS failed to start: %s' % e.strerror)


    def _is_jail_device(self, filename):
        context = pyudev.Context()
        device = pyudev.Device.from_device_file(context, filename)
        return device.subsystem == 'device_jail'


    def _check_device(self, filename):
        logging.debug('Checking device %s', filename)
        # Devices outside of the FS are fine.
        if not filename.startswith(self._mount):
            return

        # Remove mount from full path.
        relative_dev_path = filename[len(self._mount) + 1:]

        # Check if this is a passthrough device.
        if relative_dev_path in ['null', 'full', 'zero', 'urandom']:
            return

        # Ensure USB devices are jailed.
        if relative_dev_path.startswith('bus/usb'):
            if self._is_jail_device(filename):
                return
            else:
                raise error.TestError('Device should be jailed: %s' % filename)

        # All other devices should be hidden.
        raise error.TestError('Device should be hidden: %s' % filename)


    def run_once(self):
        for dirpath, _, filenames in os.walk(self._mount):
            for filename in filenames:
                real_path = os.path.realpath(os.path.join(dirpath, filename))
                try:
                    mode = os.stat(real_path).st_mode
                except OSError as e:
                    continue

                if stat.S_ISCHR(mode):
                    self._check_device(real_path)


    def _clean_shutdown(self):
        subprocess.check_call(['fusermount', '-u', self._mount])
        self._subprocess.wait()


    def _hard_shutdown(self):
        logging.warn('Timeout expired, killing device_jail_fs')
        self._subprocess.kill()
        self._subprocess.wait()


    def cleanup(self):
        super(security_DeviceJail_Filesystem, self).cleanup()
        if hasattr(self, '_subprocess'):
            logging.info('Waiting %d seconds for device_jail_fs shutdown',
                         self.SHUTDOWN_TIMEOUT_SEC)
            timeout = threading.Timer(self.SHUTDOWN_TIMEOUT_SEC,
                                      self._hard_shutdown)
            timeout.start()
            self._clean_shutdown()
            timeout.cancel()

        try:
            os.rmdir(self._mount)
        except OSError as e:
            raise error.TestError('Failed to remove temp dir: %s' % e.strerror)
