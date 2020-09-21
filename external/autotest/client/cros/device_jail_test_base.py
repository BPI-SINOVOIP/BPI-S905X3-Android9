# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os.path
import upstart
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import device_jail_utils


class DeviceJailTestBase(test.test):
    """
    An abstract base class for device jail tests. This checks to
    make sure the device jail module is loaded in the kernel and
    makes it easier to use.
    """

    def warmup(self):
        super(DeviceJailTestBase, self).warmup()
        if not (os.path.exists(device_jail_utils.JAIL_CONTROL_PATH) and
                os.path.exists(device_jail_utils.JAIL_REQUEST_PATH)):
            raise error.TestNAError('Device jail is not present')
        if upstart.is_running('permission_broker'):
            upstart.stop_job('permission_broker')


    def cleanup(self):
        super(DeviceJailTestBase, self).cleanup()
        upstart.restart_job('permission_broker')
