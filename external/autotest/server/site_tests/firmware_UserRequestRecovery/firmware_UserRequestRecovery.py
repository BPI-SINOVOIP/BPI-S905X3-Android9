# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_UserRequestRecovery(FirmwareTest):
    """
    Servo based user request recovery boot test.

    This test requires a USB disk plugged-in, which contains a Chrome OS test
    image (built by "build_image --test"). On runtime, this test first requests
    a recovery mode on next boot by setting the crossystem recovery_request
    flag. It then triggers recovery mode by unplugging and plugging in the USB
    disk and checks success of it.
    """
    version = 1

    def ensure_normal_boot(self):
        """Ensure normal mode boot this time.

        If not, it may be a test failure during step 2, try to recover to
        normal mode by simply rebooting the machine.
        """
        if not self.checkers.crossystem_checker(
                {'mainfw_type': ('normal', 'developer')}):
            self.switcher.mode_aware_reboot()

    def initialize(self, host, cmdline_args, dev_mode=False, ec_wp=None):
        super(firmware_UserRequestRecovery, self).initialize(host, cmdline_args,
                                                             ec_wp=ec_wp)
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=True, host=True)

    def cleanup(self):
        try:
            self.ensure_normal_boot()
        except Exception as e:
            logging.error("Caught exception: %s", str(e))
        super(firmware_UserRequestRecovery, self).cleanup()

    def run_once(self, dev_mode=False):
        logging.info("Request recovery boot.")
        self.check_state((self.checkers.crossystem_checker, {
                           'mainfw_type': 'developer' if dev_mode else 'normal',
                           }))
        # Execute on DUT: crossystem recovery_request=193
        self.faft_client.system.request_recovery_boot()
        # Execute from desktop:
        #   dut-control warm_reset:on sleep:0.5000 warm_reset:off
        self.switcher.simple_reboot()

        # DUT should be waiting for USB after reboot.
        # Connect servo USB to DUT and DUT should boot from USB.
        # dut-control usb_mux_sel1:dut_sees_usbkey
        # bypass_rec_mode will issue power_state:rec if host did not response in
        # delay_reboot_to_ping seconds, which may reset recovery_reason and
        # fail this tests.
        self.switcher.bypass_rec_mode()
        self.switcher.wait_for_client()

        logging.info("Expected recovery boot, request recovery again.")
        self.check_state((self.checkers.crossystem_checker, {
                           'mainfw_type': 'recovery',
                           'recovery_reason' : vboot.RECOVERY_REASON['US_TEST'],
                           }))

        self.faft_client.system.request_recovery_boot()
        self.switcher.simple_reboot()
        self.switcher.bypass_rec_mode()
        self.switcher.wait_for_client()

        logging.info("Expected recovery boot.")
        self.check_state((self.checkers.crossystem_checker, {
                           'mainfw_type': 'recovery',
                           'recovery_reason' : vboot.RECOVERY_REASON['US_TEST'],
                           }))
        self.switcher.mode_aware_reboot()

        logging.info("Expected normal boot.")
        self.check_state((self.checkers.crossystem_checker, {
                           'mainfw_type': 'developer' if dev_mode else 'normal',
                           }))
