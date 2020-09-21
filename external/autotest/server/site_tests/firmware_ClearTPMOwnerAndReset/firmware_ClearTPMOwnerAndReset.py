# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.common_lib.cros import tpm_utils
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ClearTPMOwnerAndReset(FirmwareTest):
    """
    Reboot the EC almost immediately after the tpm owner has been cleared.
    Verify the device can handle this and that it does not boot into recovery.
    """
    TIMEOUT=60


    def run_once(self, host):
        """Clear the tpm owner, reboot the EC, and check the device boots"""
        tpm_utils.ClearTPMOwnerRequest(host)

        logging.info(tpm_utils.TPMStatus(host))

        self.servo.get_power_state_controller().reset()

        end_time = time.time() + self.TIMEOUT
        while utils.ping(host.ip, deadline=5, tries=1):
            if time.time() > end_time:
                self.ec.reboot()
                raise error.TestFail('DUT failed to boot')
            logging.info('DUT is still down (no response to ping)')

        logging.info('DUT is up')

        self.check_state((self.checkers.crossystem_checker,
                          {'mainfw_type': 'normal'}))
