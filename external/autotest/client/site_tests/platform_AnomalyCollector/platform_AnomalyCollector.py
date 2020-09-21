# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from autotest_lib.client.bin import test, utils

class platform_AnomalyCollector(test.test):
    "Tests the anomaly collector daemon"
    version = 1

    def run_once(self):
        "Runs the test once"

        # Restart the anomaly collector daemon, trigger a test kernel warning,
        # and verify that a warning file is created.
        utils.system("stop anomaly-collector")
        utils.system("rm -rf /run/anomaly-collector")
        utils.system("start anomaly-collector")
        utils.system("sleep 0.1")
        lkdtm = "/sys/kernel/debug/provoke-crash/DIRECT"
        if os.path.exists(lkdtm):
            utils.system("echo WARNING > %s" % (lkdtm))
        else:
            utils.system("echo warning > /proc/breakme")
        utils.system("sleep 0.1")
        utils.system("test -f /run/anomaly-collector/warning")
