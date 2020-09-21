# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_Cr50OpenWhileAPOff(FirmwareTest):
    """Verify the console can be opened while the AP is off.

    Make sure it runs ok when cr50 saw the AP turn off and when it resets while
    the AP is off.

    This test would work the same with any cr50 ccd command that uses vendor
    commands. 'ccd open' is just one.
    """
    version = 1

    SLEEP_DELAY = 20

    def initialize(self, host, cmdline_args):
        """Initialize the test"""
        super(firmware_Cr50OpenWhileAPOff, self).initialize(host, cmdline_args)

        if not hasattr(self, 'cr50'):
            raise error.TestNAError('Test can only be run on devices with '
                                    'access to the Cr50 console')


        # TODO(mruthven): replace with dependency on servo v4 with servo micro
        # and type c cable.
        if 'servo_v4_with_servo_micro' != self.servo.get_servo_version():
            raise error.TestNAError('Run using servo v4 with servo micro')

        # Verify DTS mode while the EC is off. DTS mode needs to work to control
        # deep sleep. Some devices' rdd doesn't work when the EC is off. Make
        # sure it works on this device before running the test.
        self.servo.set('cold_reset', 'on')
        dts_mode_works = self.cr50.servo_v4_supports_dts_mode()
        self.servo.set('cold_reset', 'off')
        if not dts_mode_works:
            raise error.TestNAError('Plug in servo v4 type c cable into ccd '
                    'port')

        if not self.cr50.has_command('ccdstate'):
            raise error.TestNAError('Cannot test on Cr50 with old CCD version')


    def restore_dut(self):
        """Turn on the device and reset cr50

        Do a deep sleep reset to fix the cr50 console. Then turn the device on.

        Raises:
            TestFail if the cr50 console doesn't work
        """
        logging.info('attempt cr50 console recovery')

        # Make sure the device is off
        self.servo.set('cold_reset', 'on')

        # Do a deep sleep reset to restore the cr50 console.
        self.deep_sleep_reset()

        # Deassert EC reset to turn the device back on
        self.servo.set('cold_reset', 'off')

        # Verify the cr50 console responds to commands.
        try:
            logging.info(self.cr50.send_command_get_output('ccdstate', ['.*>']))
        except error.TestFail, e:
            if 'Timeout waiting for response' in e.message:
                raise error.TestFail('Could not restore Cr50 console')
            raise


    def deep_sleep_reset(self):
        """Toggle ccd to get to do a deep sleep reset"""
        # Make sure cr50 has been up long enough to enter deep sleep
        time.sleep(self.SLEEP_DELAY)
        # We cant use cr50 ccd_disable/enable, because those uses the cr50
        # console. Call servo_v4_dts_mode directly.
        self.servo.set_nocheck('servo_v4_dts_mode', 'off')
        time.sleep(self.SLEEP_DELAY)
        self.servo.set_nocheck('servo_v4_dts_mode', 'on')


    def try_ccd_open(self, cr50_reset):
        """Try 'ccd open' and make sure the console doesn't hang"""
        self.cr50.set_ccd_level('lock')

        try:
            # Hold the EC in reset so the AP wont turn on when we press the
            # power button
            self.servo.set('cold_reset', 'on')

            # Do a deep sleep reset before running ccd open
            if cr50_reset:
                self.cr50.clear_deep_sleep_count()
                self.deep_sleep_reset()
                if self.cr50.get_deep_sleep_count() == 0:
                    raise error.TestFail('Did not detect a cr50 reset')

            # Verify ccd open
            self.cr50.set_ccd_level('open')
        finally:
            self.restore_dut()


    def run_once(self):
        """Turn off the AP and try ccd open."""
        self.try_ccd_open(False)
        self.try_ccd_open(True)
