# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros import vboot_constants as vboot


class firmware_ECUpdateId(FirmwareTest):
    """
    Servo based EC test for updating EC ID for verifying EC EFS.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False):
        # If EC isn't write-protected, it won't do EFS. Should enable WP.
        super(firmware_ECUpdateId, self).initialize(host, cmdline_args,
                                                    ec_wp=True)
        # In order to test software sync, it must be enabled.
        self.clear_set_gbb_flags(vboot.GBB_FLAG_DISABLE_EC_SOFTWARE_SYNC, 0)
        self.backup_firmware()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        # It makes updater-related RPCs to use the active AP/EC firmware,
        # instead of the firmware in the shellball.
        self.setup_firmwareupdate_shellball()
        self.setup_usbkey(usbkey=False)
        self.setup_rw_boot()
        self.setup_ec_rw_to_a()
        self.dev_mode = dev_mode

    def cleanup(self):
        # The superclass's cleanup() restores the original WP state.
        # Do it before restore the firmware.
        super(firmware_ECUpdateId, self).cleanup()
        try:
            self.restore_firmware()
        except Exception as e:
            logging.error("Caught exception: %s", str(e))

    def setup_ec_rw_to_a(self):
        """For EC EFS, make EC boot into RW A."""
        if self.faft_client.ec.is_efs():
            active_copy = self.servo.get_ec_active_copy()
            if active_copy == 'RW_B':
                from_section = 'rw_b'
                to_section = 'rw'
                logging.info("Copy EC RW from '%s' to '%s'",
                             from_section, to_section)
                self.faft_client.ec.copy_rw(from_section, to_section)

                logging.info("EC reboot to switch slot. Wait DUT up...")
                reboot = lambda: self.faft_client.ec.reboot_to_switch_slot()
                self.switcher.mode_aware_reboot('custom', reboot)

    def get_active_hash(self):
        """Return the current EC hash."""
        ec_hash = self.faft_client.ec.get_active_hash()
        logging.info("Current EC hash: %s", ec_hash)
        return ec_hash

    def active_hash_checker(self, expected_hash):
        """Check if the current EC hash equals to the given one."""
        ec_hash = self.get_active_hash()
        result = ec_hash == expected_hash
        if not result:
            logging.info("Expected EC hash %s but now %s",
                         expected_hash, ec_hash)
        return result

    def active_copy_checker(self, expected_copy):
        """Check if the EC active copy is matched."""
        # Get the active copy via servod (EC console).
        # The result of crossystem doesn't reflect RW_B.
        active_copy = self.servo.get_ec_active_copy()
        result = active_copy == expected_copy
        if not result:
            logging.info("Expected EC in %s but now in %s",
                         expected_copy, active_copy)
        return result

    def corrupt_active_rw(self):
        """Corrupt the active RW portion."""
        section = 'rw'
        if self.servo.get_ec_active_copy() == 'RW_B':
            section = 'rw_b'
        logging.info("Corrupt the EC section: %s", section)
        self.faft_client.ec.corrupt_body(section)

    def wait_software_sync_and_boot(self):
        """Wait for software sync to update EC."""
        if self.dev_mode:
            time.sleep(self.faft_config.software_sync_update +
                       self.faft_config.firmware_screen)
            self.servo.ctrl_d()
        else:
            time.sleep(self.faft_config.software_sync_update)

    def run_once(self):
        if not self.faft_client.ec.is_efs():
            raise error.TestNAError("Nothing needs to be tested for non-EFS")

        logging.info("Check the current state and record hash.")
        self.check_state((self.active_copy_checker, 'RW'))
        original_hash = self.get_active_hash()

        logging.info("Modify EC ID and flash it back to BIOS...")
        self.faft_client.updater.modify_ecid_and_flash_to_bios()
        modified_hash = self.faft_client.updater.get_ec_hash()

        logging.info("Reboot EC. Verify if EFS works as intended.")
        self.sync_and_ec_reboot('hard')
        self.wait_software_sync_and_boot()
        self.switcher.wait_for_client()

        logging.info("Expect EC in another RW slot (the modified hash).")
        self.check_state((self.active_copy_checker, 'RW_B'))
        self.check_state((self.active_hash_checker, modified_hash))

        logging.info("Disable EC WP (also reboot)")
        self.switcher.mode_aware_reboot(
                'custom',
                lambda:self.set_ec_write_protect_and_reboot(False))

        logging.info("Corrupt the active EC RW.")
        self.corrupt_active_rw()

        logging.info("Re-enable EC WP (also reboot)")
        self.switcher.mode_aware_reboot(
                'custom',
                lambda:self.set_ec_write_protect_and_reboot(True))

        logging.info("Expect EC recovered.")
        # * EC performs EC-EFS, jumps to A, and boots AP.
        # * AP software-sync's to EC B. Reboots.
        # * EC performs EC-EFS and jumps to B.
        self.check_state((self.active_copy_checker, 'RW_B'))
        self.check_state((self.active_hash_checker, modified_hash))

        logging.info("Restore the original AP firmware and reboot.")
        self.restore_firmware(restore_ec=False)

        logging.info("Expect EC restored back (the original hash).")
        # * EC performs EC-EFS, jumps to B, and boots AP.
        # * AP software-sync's to EC A. Reboots.
        # * EC performs EC-EFS and jumps to A.
        self.check_state((self.active_copy_checker, 'RW'))
        self.check_state((self.active_hash_checker, original_hash))
