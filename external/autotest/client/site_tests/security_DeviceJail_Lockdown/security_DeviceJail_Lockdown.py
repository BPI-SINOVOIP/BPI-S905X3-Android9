# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno
import fcntl
import logging
import struct
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import device_jail_test_base
from autotest_lib.client.cros import device_jail_utils


# Definitions here are found in the uapi header linux/usbdevice_fs.h.

# struct usbdevfs_ioctl {
#         int     ifno;
#         int     ioctl_code;
#         void __user *data;
# };
def usbdevfs_ioctl(ifno, ioctl_code):
    return struct.pack('iiP', ifno, ioctl_code, 0)
# #define USBDEVFS_IOCTL             _IOWR('U', 18, struct usbdevfs_ioctl)
USBDEVFS_IOCTL = (3 << 30) + \
                 (ord('U') << 8) + \
                 18 + \
                 (struct.calcsize('iiP') << 16)
# #define USBDEVFS_DISCONNECT        _IO('U', 22)
USBDEVFS_DISCONNECT = (ord('U') << 8) + 22


class security_DeviceJail_Lockdown(device_jail_test_base.DeviceJailTestBase):
    """
    Simulate permission_broker locking the device down before letting us
    open it, and then try to perform a privileged operation such as
    disconnecting the kernel driver. If we are allowed to perform the
    operation, something is broken.
    """
    version = 1

    def _find_device_with_interface(self):
        usb_devices = device_jail_utils.get_usb_devices()
        if not usb_devices:
            error.TestNAError('No USB devices found')

        for device in usb_devices:
            if not device.children:
                continue
            for child in device.children:
                if child.device_type != 'usb_interface':
                    continue
                return (device.device_node,
                        child.attributes.asint('bInterfaceNumber'))

        return (None, None)


    def run_once(self):
        dev_path, dev_intf = self._find_device_with_interface()
        if not dev_path:
            raise error.TestNAError('No suitable USB devices found')
        logging.info('Using device %s, interface %d', dev_path, dev_intf)

        with device_jail_utils.JailDevice(dev_path) as jail:
            f = jail.expect_open(device_jail_utils.REQUEST_ALLOW_WITH_LOCKDOWN)
            if not f:
                raise error.TestError('Failed to open allowed jail')

            with f as fd:
                try:
                    fcntl.ioctl(fd, USBDEVFS_IOCTL,
                                usbdevfs_ioctl(dev_intf, USBDEVFS_DISCONNECT))
                except IOError as e:
                    if e.errno != errno.EACCES:
                        raise error.TestError(
                            'Got wrong error from ioctl: %s' % e.strerror)
                else:
                    raise error.TestError('ioctl was incorrectly allowed')
