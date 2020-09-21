# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.server import test

class platform_BootDevice(test.test):
    version = 1

    def run_once(self, reboot_iterations=1, host=None):
        for i in xrange(reboot_iterations):
            logging.info('======== Running BOOTDEVICE REBOOT ITERATION %d/%d '
                         '========', i+1, reboot_iterations)
            # Reboot the client
            logging.info('BootDevice: reboot %s', host.hostname)
            host.reboot()
