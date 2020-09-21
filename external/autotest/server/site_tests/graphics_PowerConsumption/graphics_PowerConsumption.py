# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging
from autotest_lib.server import autotest
from autotest_lib.server import test
from autotest_lib.client.common_lib import error


class graphics_PowerConsumption(test.test):
    version = 1

    def run_once(self, host, client_test):
        """Runs client test with battery actively discharging."""
        if not host.has_power():
            raise error.TestError("This test requires RPM support.")

        try:
            logging.debug("Powering off client machine before running %s test.",
                          client_test)
            host.power_off()
            client = autotest.Autotest(host)
            client.run_test(client_test, power_test=True)
        finally:
            host.power_on()
