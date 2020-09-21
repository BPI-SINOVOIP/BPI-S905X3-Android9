# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import device_jail_test_base
from autotest_lib.client.cros import device_jail_utils


class security_DeviceJail_AllowDeny(device_jail_test_base.DeviceJailTestBase):
    """
    Ensures that if device jail is present, it is functioning properly
    in that it allows access if and only if instructed (generally
    by permission_broker) and correctly locks down devices or detaches
    kernel drivers as instructed.
    """
    version = 1

    def run_once(self):
        usb_devices = device_jail_utils.get_usb_devices()
        if not usb_devices:
            error.TestNAError('No USB devices found')

        dev_path = usb_devices[0].device_node
        with device_jail_utils.JailDevice(dev_path) as jail:
            # This should succeed and return a file.
            f = jail.expect_open(device_jail_utils.REQUEST_ALLOW)
            if not f:
                raise error.TestError('Failed to open allowed jail')
            else:
                f.close()

            # This should not return a file.
            f = jail.expect_open(device_jail_utils.REQUEST_DENY)
            if f:
                raise error.TestError('Successfully opened denied jail')
