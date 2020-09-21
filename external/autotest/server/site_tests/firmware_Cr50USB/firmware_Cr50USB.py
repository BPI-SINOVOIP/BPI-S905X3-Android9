# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest, test
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_Cr50USB(FirmwareTest):
    """
    Stress the Cr50 usb connection to the DUT.

    This test is intended to be run with many iterations to ensure that the
    USB connection is not flaky.

    The iteration should be specified by the parameter -a "num_iterations=10".

    To exit on the first USB failure use  "exit_condition='immediately'".
    """
    version = 1

    SLEEP_DELAY = 20

    def cleanup(self):
        """Reenable CCD before cleanup"""
        if hasattr(self, "cr50"):
            self.cr50.ccd_enable()
        super(firmware_Cr50USB, self).cleanup()


    def run_once(self, host, cmdline_args, num_iterations=100,
                 exit_condition=None):
        self.host = host
        # Disable CCD so it doesn't interfere with the Cr50 AP usb connection.
        if hasattr(self, "cr50"):
            self.cr50.ccd_disable()

        # Make sure the device is logged in so TPM activity doesn't keep it
        # awake
        self.client_at = autotest.Autotest(self.host)
        self.client_at.run_test('login_LoginSuccess')

        failed_runs = []
        fail_count = 0
        logging.info("Running Cr50 USB stress test for %d iterations",
                     num_iterations)

        for iteration in xrange(num_iterations):
            if iteration:
                time.sleep(self.SLEEP_DELAY)

            i = iteration + 1
            logging.info("Run %d of %d%s", i, num_iterations,
                         " %d failures" % fail_count if fail_count else "")
            try:
                # Run usb_updater command.
                result = self.host.run("usb_updater -f")
            except Exception, e:
                failed_runs.append(str(i))
                fail_count += 1
                logging.debug(e)

                if exit_condition == "immediately":
                    raise error.TestFail("USB failure on run %d of %d" %
                        (i, num_iterations))

                logging.info("USB failure on %d out of %d runs: %s", fail_count,
                    i, ', '.join(failed_runs))

        if fail_count:
            raise error.TestFail('USB failure on %d runs out of %d: %s' %
                    (fail_count, num_iterations, ', '.join(failed_runs)))
