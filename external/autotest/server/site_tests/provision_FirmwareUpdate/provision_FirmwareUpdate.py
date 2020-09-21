# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


""" The autotest performing FW update, both EC and AP."""


import logging
import sys

from autotest_lib.client.common_lib import error
from autotest_lib.server import test


class provision_FirmwareUpdate(test.test):
    """A test that can provision a machine to the correct firmware version."""

    version = 1


    def stage_image_to_usb(self, host):
        """Stage the current ChromeOS image on the USB stick connected to the
        servo.

        @param host:  a CrosHost object of the machine to update.
        """
        info = host.host_info_store.get()
        if not info.build:
            logging.warning('Failed to get build label from the DUT, skip '
                            'staging ChromeOS image on the servo USB stick.')
        else:
            host.servo.image_to_servo_usb(
                    host.stage_image_for_servo(info.build))
            logging.debug('ChromeOS image %s is staged on the USB stick.',
                          info.build)

    def get_ro_firmware_ver(self, host):
        """Get the RO firmware version from the host."""
        result = host.run('crossystem ro_fwid', ignore_status=True)
        if result.exit_status == 0:
            # The firmware ID is something like "Google_Board.1234.56.0".
            # Remove the prefix "Google_Board".
            return result.stdout.split('.', 1)[1]
        else:
            return None

    def get_rw_firmware_ver(self, host):
        """Get the RW firmware version from the host."""
        result = host.run('crossystem fwid', ignore_status=True)
        if result.exit_status == 0:
            # The firmware ID is something like "Google_Board.1234.56.0".
            # Remove the prefix "Google_Board".
            return result.stdout.split('.', 1)[1]
        else:
            return None

    def run_once(self, host, value, rw_only=False, stage_image_to_usb=False):
        """The method called by the control file to start the test.

        @param host:  a CrosHost object of the machine to update.
        @param value: the provisioning value, which is the build version
                      to which we want to provision the machine,
                      e.g. 'link-firmware/R22-2695.1.144'.
        @param rw_only: True to only update the RW firmware.
        @param stage_image_to_usb: True to stage the current ChromeOS image on
                the USB stick connected to the servo. Default is False.
        """
        try:
            host.repair_servo()

            # Stage the current CrOS image to servo USB stick.
            if stage_image_to_usb:
                self.stage_image_to_usb(host)

            host.firmware_install(build=value, rw_only=rw_only)
        except Exception as e:
            logging.error(e)
            raise error.TestFail, str(e), sys.exc_info()[2]

        # DUT reboots after the above firmware_install(). Wait it to boot.
        host.test_wait_for_boot()

        # Only care about the version number.
        firmware_ver = value.rsplit('-', 1)[1]
        if not rw_only:
            current_ro_ver = self.get_ro_firmware_ver(host)
            if current_ro_ver != firmware_ver:
                raise error.TestFail('Failed to update RO, still version %s' %
                                     current_ro_ver)
        current_rw_ver = self.get_rw_firmware_ver(host)
        if current_rw_ver != firmware_ver:
            raise error.TestFail('Failed to update RW, still version %s' %
                                 current_rw_ver)
