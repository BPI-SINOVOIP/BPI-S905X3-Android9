# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import dbus
import logging
import os
import subprocess

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import autotemp, error
from autotest_lib.client.common_lib.cros import dbus_send


class vm_CrosVmStart(test.test):
    """Tests crosvm."""

    version = 1
    BUS_NAME = 'org.chromium.ComponentUpdaterService'
    BUS_PATH = '/org/chromium/ComponentUpdaterService'
    BUS_INTERFACE = 'org.chromium.ComponentUpdaterService'
    LOAD_COMPONENT = 'LoadComponent'
    TERMINA_COMPONENT_NAME = 'cros-termina'
    USER = 'chronos'


    def _load_component(self, name):
        args = [dbus.String(name)]
        return dbus_send.dbus_send(
            self.BUS_NAME,
            self.BUS_INTERFACE,
            self.BUS_PATH,
            self.LOAD_COMPONENT,
            timeout_seconds=60,
            user=self.USER,
            args=args).response


    def run_once(self):
        """
        Runs a basic test to see if crosvm starts.
        """
        mnt_path = self._load_component(self.TERMINA_COMPONENT_NAME)
        if not mnt_path:
            raise error.TestError('Component Updater LoadComponent failed')
        kernel_path = mnt_path + '/vm_kernel'
        rootfs_path = mnt_path + '/vm_rootfs.img'
        crosvm_socket_path = '/tmp/vm_CrosVmStart.sock'

        # Running /bin/ls as init causes the VM to exit immediately, crosvm
        # will have a successful exit code.
        cmd = ['/usr/bin/crosvm', 'run', '-c', '1', '-m', '1024',
                '--disk', rootfs_path,
                '--socket',  crosvm_socket_path,
                '-p', 'init=/bin/bash root=/dev/vda ro',
                kernel_path]
        proc = subprocess.Popen(cmd)
        if proc.pid <= 0:
            raise error.TestFail('Failed: crosvm did not start.')

        # Tell the VM to stop.
        stop_cmd = ['/usr/bin/crosvm', 'stop', '--socket', crosvm_socket_path]
        stop_proc = subprocess.Popen(cmd)
        if stop_proc.pid <= 0:
            raise error.TestFail('Failed: crosvm stop command failed.')
        stop_proc.wait();
        if stop_proc.returncode == None:
            raise error.TestFail('Failed: crosvm stop did not exit.')
        if stop_proc.returncode != 0:
            raise error.TestFail('Failed: crosvm stop returned an error %d.' %
                                 stop_proc.returncode)

        # Wait for the VM to exit.
        proc.wait()
        if proc.returncode == None:
            raise error.TestFail('Failed: crosvm did not exit.')
        if proc.returncode != 0:
            raise error.TestFail('Failed: crosvm returned an error %d.' %
                                 proc.returncode)
