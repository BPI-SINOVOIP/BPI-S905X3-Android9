# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This test isn't really HW specific. It's testing /dev/watchdog API
# to make sure it works as expected. The most robust implementations is
# based on real HW but it doesn't have to be.

import logging

# http://docs.python.org/2/library/errno.html
import errno

from autotest_lib.client.common_lib import error
from autotest_lib.server import test
from autotest_lib.server.cros.watchdog_tester import WatchdogTester


class platform_HWwatchdog(test.test):
    """Test to make sure that /dev/watchdog will reboot the system."""

    version = 1

    def _stop_watchdog(self, client, wd_dev):
        # HW watchdog is open and closed "properly".
        try:
            client.run('echo "V" > %s' % wd_dev)
        except error.AutoservRunError, e:
            raise error.TestError('write to %s failed (%s)' %
                                  (wd_dev, errno.errorcode[e.errno]))

    def run_once(self, host=None):
        tester = WatchdogTester(host)
        # If watchdog not present, just skip this test
        if not tester.is_supported():
            logging.info("INFO: %s not present. Skipping test." % tester.WD_DEV)
            return

        with tester:
            self._stop_watchdog(host, tester.WD_DEV)
            tester.trigger_watchdog()
