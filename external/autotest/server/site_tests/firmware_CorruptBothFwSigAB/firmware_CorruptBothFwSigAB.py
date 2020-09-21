# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_CorruptBothFwSigAB(FirmwareTest):
    """
    Servo based both firmware signature A and B corruption test.

    This test requires a USB disk plugged-in, which contains a Chrome OS test
    image (built by "build_image --test"). On runtime, this test corrupts
    both firmware signature A and B. On next reboot, the firmware verification
    fails and enters recovery mode. This test then checks the success of the
    recovery boot.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_CorruptBothFwSigAB, self).initialize(host, cmdline_args)
        self.backup_firmware()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=True, host=False)

    def cleanup(self):
        try:
            self.restore_firmware()
        except Exception as e:
            logging.error("Caught exception: %s", str(e))
        super(firmware_CorruptBothFwSigAB, self).cleanup()

    def run_once(self, dev_mode=False):
        logging.info("Corrupt both firmware signature A and B.")
        self.check_state((self.checkers.crossystem_checker, {
                          'mainfw_type': 'developer' if dev_mode else 'normal',
                          }))
        self.faft_client.bios.corrupt_sig(('a', 'b'),)

        # Older devices (without BROKEN screen) didn't wait for removal in
        # dev mode. Make sure the USB key is not plugged in so they won't
        # start booting immediately and get interrupted by unplug/replug.
        self.servo.switch_usbkey('host')
        self.switcher.simple_reboot()
        self.switcher.bypass_rec_mode()
        self.switcher.wait_for_client()

        logging.info("Expected recovery boot and set fwb_tries flag.")
        self.check_state((self.checkers.crossystem_checker, {
                          'mainfw_type': 'recovery',
                          'recovery_reason': (
                              vboot.RECOVERY_REASON['RO_INVALID_RW'],
                              vboot.RECOVERY_REASON['RW_VERIFY_KEYBLOCK']),
                          }))
        self.faft_client.system.set_try_fw_b()

        self.servo.switch_usbkey('host')
        self.switcher.simple_reboot(sync_before_boot=False)
        self.switcher.bypass_rec_mode()
        self.switcher.wait_for_client()

        logging.info("Still expected recovery boot and restore firmware.")
        self.check_state((self.checkers.crossystem_checker, {
                          'mainfw_type': 'recovery',
                          'recovery_reason': (
                              vboot.RECOVERY_REASON['RO_INVALID_RW'],
                              vboot.RECOVERY_REASON['RW_VERIFY_KEYBLOCK']),
                          }))
        self.faft_client.bios.restore_sig(('a', 'b'),)
        self.switcher.mode_aware_reboot()

        logging.info("Expected normal boot, done.")
        self.check_state((self.checkers.crossystem_checker, {
                          'mainfw_type': 'developer' if dev_mode else 'normal',
                          }))
