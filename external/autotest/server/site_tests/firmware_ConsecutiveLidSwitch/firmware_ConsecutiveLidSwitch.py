# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.server import autotest
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ConsecutiveLidSwitch(FirmwareTest):
    """
    Servo based consecutive lid switch test.

    This test is intended to be run with many iterations to ensure that closing
    DUT lid triggers suspend and opening lid wakes it up.

    The iteration should be specified by the parameter -a "faft_iterations=10".

    Checking the boot_id ensures DUT won't reboot unexpectedly.
    """
    version = 1


    def initialize(self, host, cmdline_args):
        # Parse arguments from command line
        dict_args = utils.args_to_dict(cmdline_args)
        self.faft_iterations = int(dict_args.get('faft_iterations', 1))
        super(firmware_ConsecutiveLidSwitch, self).initialize(host,
                                                              cmdline_args)
        self.setup_usbkey(usbkey=False)


    def cleanup(self):
        # Restore the lid_open switch in case the test failed in the middle.
        try:
            self.servo.set('lid_open', 'yes')
        except Exception as e:
            logging.error("Caught exception: %s", str(e))
        super(firmware_ConsecutiveLidSwitch, self).cleanup()


    def run_once(self, host):
        # Login as a normal user and stay there. Closing lid at the login
        # screen may shut the machine down, that also verifies the lid switch
        # but take more time. Once logged in, closing lid triggers suspend.
        autotest_client = autotest.Autotest(host)
        autotest_client.run_test("desktopui_SimpleLogin",
                                 exit_without_logout=True)

        original_boot_id = host.get_boot_id()

        for i in xrange(self.faft_iterations):
            logging.info('======== Running FAFT ITERATION %d/%d ========',
                         i + 1, self.faft_iterations)

            logging.info("Close lid to suspend.")
            self.servo.set('lid_open', 'no')
            logging.info("Expected going to suspend. Waiting DUT offline...")
            self.switcher.wait_for_client_offline()
            time.sleep(self.LID_DELAY)

            logging.info("Wake DUT by lid switch.")
            self.servo.set('lid_open', 'yes')
            logging.info("Expected going to resume. Waiting DUT online...")
            self.switcher.wait_for_client()
            time.sleep(self.LID_DELAY)

            boot_id = host.get_boot_id()
            if boot_id != original_boot_id:
                raise error.TestFail('Different boot_id. Unexpected reboot.')
