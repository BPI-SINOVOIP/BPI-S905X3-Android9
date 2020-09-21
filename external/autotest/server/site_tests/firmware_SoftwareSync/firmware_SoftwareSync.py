# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros import vboot_constants as vboot


class firmware_SoftwareSync(FirmwareTest):
    """
    Servo based EC software sync test.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False):
        # This test tries to corrupt EC firmware. Should disable EC WP.
        super(firmware_SoftwareSync, self).initialize(host, cmdline_args,
                                                      ec_wp=False)
        # In order to test software sync, it must be enabled.
        self.clear_set_gbb_flags(vboot.GBB_FLAG_DISABLE_EC_SOFTWARE_SYNC, 0)
        self.backup_firmware()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=False)
        self.setup_rw_boot()
        self.dev_mode = dev_mode

    def cleanup(self):
        try:
            self.restore_firmware()
        except Exception as e:
            logging.error("Caught exception: %s", str(e))
        super(firmware_SoftwareSync, self).cleanup()

    def record_hash(self):
        """Record current EC hash."""
        self._ec_hash = self.faft_client.ec.get_active_hash()
        logging.info("Stored EC hash: %s", self._ec_hash)

    def corrupt_active_rw(self):
        """Corrupt the active RW portion."""
        section = 'rw'
        try:
            if self.servo.get_ec_active_copy() == 'RW_B':
                section = 'rw_b'
        except error.TestFail:
            # Skip the failure, as ec_active_copy is new.
            # TODO(waihong): Remove this except clause.
            pass
        logging.info("Corrupt the EC section: %s", section)
        self.faft_client.ec.corrupt_body(section)

    def software_sync_checker(self):
        """Check EC firmware is restored by software sync."""
        ec_hash = self.faft_client.ec.get_active_hash()
        logging.info("Current EC hash: %s", ec_hash)
        if self._ec_hash != ec_hash:
            return False
        return self.checkers.ec_act_copy_checker('RW')

    def wait_software_sync_and_boot(self):
        """Wait for software sync to update EC."""
        if self.dev_mode:
            time.sleep(self.faft_config.software_sync_update +
                       self.faft_config.firmware_screen)
            self.servo.ctrl_d()
        else:
            time.sleep(self.faft_config.software_sync_update)

    def run_once(self):
        logging.info("Corrupt EC firmware RW body.")
        self.check_state((self.checkers.ec_act_copy_checker, 'RW'))
        self.record_hash()
        self.corrupt_active_rw()
        logging.info("Reboot AP, check EC hash, and software sync it.")
        self.switcher.simple_reboot(reboot_type='warm')
        self.wait_software_sync_and_boot()
        self.switcher.wait_for_client()

        logging.info("Expect EC in RW and RW is restored.")
        self.check_state(self.software_sync_checker)
