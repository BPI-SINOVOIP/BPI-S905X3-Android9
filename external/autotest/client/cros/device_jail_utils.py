# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import pyudev
import re
import select
import struct
import subprocess
import threading
import time
from autotest_lib.client.common_lib import error


JAIL_CONTROL_PATH = '/dev/jail-control'
JAIL_REQUEST_PATH = '/dev/jail-request'

# From linux/device_jail.h.
REQUEST_ALLOW = 0
REQUEST_ALLOW_WITH_LOCKDOWN = 1
REQUEST_ALLOW_WITH_DETACH = 2
REQUEST_DENY = 3


class OSFile:
    """Simple context manager for file descriptors."""
    def __init__(self, path, flag):
        self._fd = os.open(path, flag)

    def close(self):
        os.close(self._fd)

    def __enter__(self):
        """Returns the fd so it can be used in with-blocks."""
        return self._fd

    def __exit__(self, exc_type, exc_val, traceback):
        self.close()


class ConcurrentFunc:
    """Simple context manager that starts and joins a thread."""
    def __init__(self, target_func, timeout_func):
        self._thread = threading.Thread(target=target_func)
        self._timeout_func = timeout_func
        self._target_name = target_func.__name__

    def __enter__(self):
        self._thread.start()

    def __exit__(self, exc_type, exc_val, traceback):
        self._thread.join(self._timeout_func())
        if self._thread.is_alive() and not exc_val:
            raise error.TestError('Function %s timed out' % self._target_name)


class JailDevice:
    TIMEOUT_SEC = 3
    PATH_MAX = 4096

    def __init__(self, path_to_jail):
        self._path_to_jail = path_to_jail


    def __enter__(self):
        """
        Creates a jail device for the device located at self._path_to_jail.
        If the jail already exists, don't take ownership of it.
        """
        try:
            output = subprocess.check_output(
                ['device_jail_utility',
                 '--add={0}'.format(self._path_to_jail)],
                stderr=subprocess.STDOUT)

            match = re.search('created jail at (.*)', output)
            if match:
                self._path = match.group(1)
                self._owns_device = True
                return self

            match = re.search('jail already exists at (.*)', output)
            if match:
                self._path = match.group(1)
                self._owns_device = False
                return self

            raise error.TestError('Failed to create device jail')
        except subprocess.CalledProcessError as e:
            raise error.TestError('Failed to call device_jail_utility')


    def expect_open(self, verdict):
        """
        Tries to open the jail device. This method mocks out the
        device_jail request server which is normally run by permission_broker.
        This allows us to set the verdict we want to test. Since the open
        call will block until we return the verdict, we have to use a
        separate thread to perform the open call, as well.
        """
        # Python 2 does not support "nonlocal" so this closure can't
        # set the values of identifiers it closes over unless they
        # are in global scope. Work around this by using a list and
        # value-mutation.
        dev_file_wrapper = [None]
        def open_device():
            try:
                dev_file_wrapper[0] = OSFile(self._path, os.O_RDWR)
            except OSError as e:
                # We don't throw an error because this might be intentional,
                # such as when the verdict is REQUEST_DENY.
                logging.info("Failed to open jail device: %s", e.strerror)

        # timeout_sec should be used for the timeouts below.
        # This ensures we don't spend much longer than TIMEOUT_SEC in
        # this method.
        deadline = time.time() + self.TIMEOUT_SEC
        def timeout_sec():
            return max(deadline - time.time(), 0.01)

        # We have to use FDs because polling works with FDs and
        # buffering is silly.
        try:
            req_f = OSFile(JAIL_REQUEST_PATH, os.O_RDWR)
        except OSError as e:
            raise error.TestError(
                'Failed to open request device: %s' % e.strerror)

        with req_f as req_fd:
            poll_obj = select.poll()
            poll_obj.register(req_fd, select.POLLIN)

            # Starting open_device should ensure we have a request waiting
            # on the request device.
            with ConcurrentFunc(open_device, timeout_sec):
                ready_fds = poll_obj.poll(timeout_sec() * 1000)
                if not ready_fds:
                    raise error.TestError('Timed out waiting for jail-request')

                # Sanity check the request.
                path = os.read(req_fd, self.PATH_MAX)
                logging.info('Received jail-request for path %s', path)
                if path != self._path_to_jail:
                    raise error.TestError('Got request for the wrong path')

                os.write(req_fd, struct.pack('I', verdict))
                logging.info('Responded to jail-request')

        return dev_file_wrapper[0]


    def __exit__(self, exc_type, exc_val, traceback):
        if self._owns_device:
            subprocess.call(['device_jail_utility',
                             '--remove={0}'.format(self._path)])


def get_usb_devices():
    context = pyudev.Context()
    return [device for device in context.list_devices()
        if device.device_node and device.device_node.startswith('/dev/bus/usb')]
