# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.server import test

class infra_FirmwareAutoupdate(test.test):
    """
    Update RW firmware in the target DUT.

    In production, consumer devices periodically update their read/write
    firmware during auto-update cycles.  This happens when the firmware
    bundled with a newly update image is different from the firmware
    currently installed on the device.

    In the test lab, this step is suppressed, to prevent devices from
    inadvertently updating to a new firmware version as a consequence of
    installing a new Chrome OS build for testing.  In particular,
    because the firmware is updated whenever the bundled firmware is
    _different_, and not merely _more recent_, suppressing the update
    prevents unexpectedly downgrading the firmware.

    Nonetheless, we sometimes want more recent firmware on a DUT than
    the image that was delivered from the factory.  This test allows
    updating the firmware to the installed firmware bundle, using the
    same procedure as is used during autoupdate.
    """
    version = 1

    def run_once(self, host):
        host.run('chromeos-firmwareupdate --mode=autoupdate')
        host.reboot()
