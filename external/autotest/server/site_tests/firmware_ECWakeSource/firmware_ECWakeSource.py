# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ECWakeSource(FirmwareTest):
    """
    Servo based EC wake source test.
    """
    version = 1

    # Delay for waiting client to shut down
    SHUTDOWN_DELAY = 10

    def initialize(self, host, cmdline_args):
        super(firmware_ECWakeSource, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')

    def cleanup(self):
        # Restore the lid_open switch in case the test failed in the middle.
        self.servo.set('lid_open', 'yes')
        super(firmware_ECWakeSource, self).cleanup()

    def hibernate_and_wake_by_power_button(self):
        """Shutdown to G2/S5, then hibernate EC. Finally, wake by power button."""
        self.faft_client.system.run_shell_command("shutdown -H now")
        time.sleep(self.SHUTDOWN_DELAY)
        self.switcher.wait_for_client_offline()
        self.ec.send_command("hibernate 1000")
        time.sleep(self.WAKE_DELAY)
        self.servo.power_short_press()

    def run_once(self, host):
        # TODO(victoryang): make this test run on both x86 and arm
        if not self.check_ec_capability(['x86', 'lid']):
            raise error.TestNAError("Nothing needs to be tested on this device")

        # Login as a normal user and stay there, such that closing lid triggers
        # suspend, instead of shutdown.
        autotest_client = autotest.Autotest(host)
        autotest_client.run_test("desktopui_SimpleLogin",
                                 exit_without_logout=True)

        original_boot_id = host.get_boot_id()

        logging.info("Suspend and wake by power button.")
        self.suspend()
        self.switcher.wait_for_client_offline()
        self.servo.power_normal_press()
        self.switcher.wait_for_client()

        logging.info("Suspend and wake by lid switch.")
        self.suspend()
        self.switcher.wait_for_client_offline()
        self.servo.set('lid_open', 'no')
        time.sleep(self.LID_DELAY)
        self.servo.set('lid_open', 'yes')
        self.switcher.wait_for_client()

        logging.info("Close lid to suspend and wake by lid switch.")
        self.servo.set('lid_open', 'no')
        # Expect going to suspend, not pingable
        self.switcher.wait_for_client_offline()
        time.sleep(self.LID_DELAY)
        self.servo.set('lid_open', 'yes')
        self.switcher.wait_for_client()

        boot_id = host.get_boot_id()
        if boot_id != original_boot_id:
            raise error.TestFail('Different boot_id. Unexpected reboot.')

        use_ccd = 'ccd_cr50' in self.servo.get_servo_version()
        if use_ccd:
            logging.info("Using CCD, ignore waking by power button.")
        else:
            logging.info("EC hibernate and wake by power button.")
            self.hibernate_and_wake_by_power_button()
            self.switcher.wait_for_client()
