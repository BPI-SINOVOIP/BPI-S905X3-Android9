# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno
import logging
import os.path
import pyudev
import struct
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import device_jail_test_base
from autotest_lib.client.cros import device_jail_utils


class security_DeviceJail_Detach(device_jail_test_base.DeviceJailTestBase):
    """
    Simulate permission_broker detaching the kernel driver before we
    open it, and then check to make sure it is really detached. Also,
    check to make sure the driver is reattached after we close it.
    """
    version = 1

    def _get_multi_intf_device(self):
        usb_devices = device_jail_utils.get_usb_devices()
        if not usb_devices:
            error.TestNAError('No USB devices found')

        for device in usb_devices:
            if device.attributes.asint('bNumInterfaces') > 1:
                return device

        return None


    def _force_reattach_driver(self, intf, driver):
        try:
            with open('/sys/bus/usb/drivers/%s/bind' % driver,
                      'w') as bind_file:
                bind_file.write(os.path.basename(intf))
        except IOError as e:
            # This might happen if force-reattaching an earlier
            # interface also reattached this one. Just ignore it.
            pass


    def _get_interface_drivers(self, device):
        return { child.device_path : child.driver for child in device.children
            if child.device_type == 'usb_interface' }


    def run_once(self):
        device = self._get_multi_intf_device()
        if not device:
            raise error.TestNAError('No suitable USB device found')

        dev_path = device.device_node
        pre_detach_drivers = self._get_interface_drivers(device)
        logging.info('Found device %s', dev_path)

        with device_jail_utils.JailDevice(dev_path) as jail:
            # This should succeed and return a file.
            f = jail.expect_open(device_jail_utils.REQUEST_ALLOW_WITH_DETACH)
            if not f:
                raise error.TestError('Failed to open allowed jail')
            with f:
                detach_drivers = self._get_interface_drivers(device)
            post_detach_drivers = self._get_interface_drivers(device)

        failed_detach = {}
        failed_reattach = {}
        for intf, driver in pre_detach_drivers.items():
            if detach_drivers[intf]:
                failed_detach[intf] = driver
            if driver != post_detach_drivers[intf]:
                self._force_reattach_driver(intf, driver)
                failed_reattach[intf] = driver

        if failed_detach:
            raise error.TestError('Drivers failed to detach from interfaces:\n'
                + '\n'.join(intf for intf in failed_detach))
        if failed_reattach:
            raise error.TestError('Drivers failed to reattach to interfaces:\n'
                + '\n'.join(intf for intf in failed_reattach))
