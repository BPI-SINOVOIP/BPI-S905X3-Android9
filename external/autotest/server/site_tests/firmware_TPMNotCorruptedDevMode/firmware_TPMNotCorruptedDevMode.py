# Copyright (c) 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest

class firmware_TPMNotCorruptedDevMode(FirmwareTest):
    """
    Checks the firmware and kernel version stored in TPMC and boots to USB and
    checks the firmware version and kernel version in crossystem.

    This test requires a USB disk plugged-in, which contains a Chrome OS test
    image (built by "build_image test").
    """
    version = 1

    TPMC_KNOWN_VALUES = set(['1 4c 57 52 47 1 0 1 0 0 0 0 0',
                             '2 4c 57 52 47 1 0 1 0 0 0 0 55'])

    def initialize(self, host, cmdline_args, ec_wp=None):
        dict_args = utils.args_to_dict(cmdline_args)
        super(firmware_TPMNotCorruptedDevMode, self).initialize(
            host, cmdline_args, ec_wp=ec_wp)

        self.assert_test_image_in_usb_disk()
        self.switcher.setup_mode('dev')
        self.setup_usbkey(usbkey=True, host=False)

        self.original_dev_boot_usb = self.faft_client.system.get_dev_boot_usb()
        logging.info(
            'Original dev_boot_usb value: %s',
            str(self.original_dev_boot_usb))

    def cleanup(self):
        try:
            self.ensure_dev_internal_boot(self.original_dev_boot_usb)
        except Exception as e:
            logging.error("Caught exception: %s", str(e))
        super(firmware_TPMNotCorruptedDevMode, self).cleanup()

    def ensure_usb_device_boot(self):
        """Ensure USB device boot and if not reboot into USB."""
        if not self.faft_client.system.is_removable_device_boot():
            logging.info('Reboot into USB...')
            self.faft_client.system.set_dev_boot_usb(1)
            self.switcher.simple_reboot()
            self.switcher.bypass_dev_boot_usb()
            self.switcher.wait_for_client()

        self.check_state((self.checkers.dev_boot_usb_checker, (True, True),
                          'Device not booted from USB image properly.'))

    def read_tmpc(self):
        """First checks if internal device boot and if not reboots into it.
        Then stops the tcsd then reads the value of `tpmc read 0x1008 0x0d` then
        checks if the output of that command is what is expected.
        """
        self.ensure_dev_internal_boot(self.original_dev_boot_usb)
        logging.info('Reading tpmc data.')
        self.faft_client.tpm.stop_daemon()
        tpmc_output = self.faft_client.system.run_shell_command_get_output(
            'tpmc read 0x1008 0x0d')
        self.faft_client.tpm.restart_daemon()

        logging.info('===== TPMC OUTPUT: %s =====' % tpmc_output)
        if self.check_tpmc(tpmc_output):
          raise error.TestFail(
              'TPMC version is incorrect. Actual: %s Expected one of: %s' %
                  (tpmc_output, self.TPMC_KNOWN_VALUES))

    def read_kern_fw_ver(self):
        """First ensures that we are booted on a USB device. Then checks the
        firmware and kernel version reported by crossystem.
        """
        self.ensure_usb_device_boot()
        logging.info('Checking kernel and fw version in crossystem.')

        if self.checkers.crossystem_checker({
                    'tpm_fwver': '0xffffffff',
                    'tpm_kernver': '0xffffffff', }):
            raise error.TestFail('tpm versions invalid.')

    def check_tpmc(self, tpmc_output):
      """Checks that the kern and fw version from the tpmc read command is one
      of the expected values.
      """
      if len(tpmc_output) != 1:
        return False

      tpmc_fw_kern_version = set(tpmc_output)

      return (tpmc_fw_kern_version in self.TPMC_KNOWN_VALUES)

    def run_once(self):
        self.read_tmpc()
        self.read_kern_fw_ver()
