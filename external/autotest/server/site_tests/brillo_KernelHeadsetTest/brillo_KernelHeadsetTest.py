# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server import test


# Max number of devices to check in /sys/class/input to see if one of them is an
# input device.
_MAX_DEVICES = 100


class brillo_KernelHeadsetTest(test.test):
    """Verify that a Brillo device supports headsets.

    This test is required if the Brillo board has an audio jack."""
    version = 1

    def run_once(self, host=None):
        """Runs the test.

        @param host: A host object representing the DUT.

        """
        # Check for headset support.
        found_headset = False
        for device_num in range(_MAX_DEVICES):
            result = host.run_output(
                    'cat sys/class/input/event%i/device/name' % device_num,
                    ignore_status=True)
            if 'Headset' in result:
                found_headset = True

        if not found_headset:
            raise error.TestNAError('Could not find headset input device.')

        # Check for h2w driver.
        result = host.run_output('cat /sys/class/switch/h2w/name',
                                 ignore_status=True)
        if 'h2w' not in result:
            raise error.TestNAError('h2w driver not found.')
