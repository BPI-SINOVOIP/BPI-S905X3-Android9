# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.servo import pd_device


class firmware_PDResetSoft(FirmwareTest):
    """
    Servo based USB PD soft reset test.

    Soft resets are issued by both ends of the connection. If the DUT
    is dualrole capable, then a power role swap is executed, and the
    test is repeated with the DUT in the opposite power role. Pass
    criteria is that all attempted soft resets are successful.

    """
    version = 1
    RESET_ITERATIONS = 5


    def _test_soft_reset(self, port_pair):
        """Tests soft reset initated by both Plankton and the DUT

        @param port_pair: list of 2 connected PD devices
        """
        for dev in port_pair:
            for _ in xrange(self.RESET_ITERATIONS):
                try:
                    if dev.soft_reset() == False:
                        raise error.TestFail('Soft Reset Failed')
                except NotImplementedError:
                    logging.warn('Device cant soft reset ... skipping')
                    break

    def initialize(self, host, cmdline_args):
        super(firmware_PDResetSoft, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')
        # Turn off console prints, except for USBPD.
        self.usbpd.send_command('chan 0x08000000')

    def cleanup(self):
        self.usbpd.send_command('chan 0xffffffff')
        super(firmware_PDResetSoft, self).cleanup()

    def run_once(self):
        """Execute Power Role swap test.

        1. Verify that pd console is accessible
        2. Verify that DUT has a valid PD contract
        3. Make sure dualrole mode is enabled on both ends
        4. Test Soft Reset initiated by both ends of connection
        5. Attempt to change power roles
           If power role changed, then retest soft resets
           Else end test.

        """
        # Create list of available UART consoles
        consoles = [self.usbpd, self.plankton]
        port_partner = pd_device.PDPortPartner(consoles)
        # Identify a valid test port pair
        port_pair = port_partner.identify_pd_devices()
        if not port_pair:
            raise error.TestFail('No PD connection found!')

        # Test soft resets initiated by both ends
        self._test_soft_reset(port_pair)
        # Attempt to swap power roles
        try:
            if port_pair[0].pr_swap() == False:
                logging.warn('Power role not swapped, ending test')
                return
        except NotImplementedError:
            logging.warn('device cant send power role swap command, end test')
            return
        # Power role has been swapped, retest.
        self._test_soft_reset(port_pair)
