# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_WriteProtect(FirmwareTest):
    """
    This test checks whether the hardware write-protect state reported by
    crossystem matches the real write-protect state driven by Servo.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_WriteProtect, self).initialize(host, cmdline_args)
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self._original_wp = self.servo.get('fw_wp') == 'on'

    def cleanup(self):
        try:
            self.set_hardware_write_protect(self._original_wp)
        except Exception as e:
            logging.error('Caught exception: %s', str(e))
        super(firmware_WriteProtect, self).cleanup()

    def run_once(self):
        logging.info('Force write-protect on and reboot for a clean slate.')
        self.set_hardware_write_protect(True)
        self.switcher.mode_aware_reboot()
        self.check_state((self.checkers.crossystem_checker, {
                              'wpsw_boot': '1',
                              'wpsw_cur': '1',
                          }))
        logging.info('Now disable write-protect and check again.')
        self.set_hardware_write_protect(False)
        self.check_state((self.checkers.crossystem_checker, {
                              'wpsw_boot': '1',
                              'wpsw_cur': '0',
                          }))
        logging.info('Reboot so WP change takes effect for wpsw_boot.')
        self.switcher.mode_aware_reboot()
        self.check_state((self.checkers.crossystem_checker, {
                              'wpsw_boot': '0',
                              'wpsw_cur': '0',
                          }))
        logging.info('Enable write-protect again to observe final transition.')
        self.set_hardware_write_protect(True)
        self.check_state((self.checkers.crossystem_checker, {
                              'wpsw_boot': '0',
                              'wpsw_cur': '1',
                          }))
