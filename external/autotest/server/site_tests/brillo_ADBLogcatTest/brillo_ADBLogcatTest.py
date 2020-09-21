# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cStringIO
import requests

from autotest_lib.client.common_lib import error
from autotest_lib.server import test


class brillo_ADBLogcatTest(test.test):
    """Verify that adb logcat and adb shell dmesg work correctly."""
    version = 1


    def run_once(self, host=None):
        """Body of the test."""
        logcat_log = cStringIO.StringIO()

        host.adb_run('logcat -d', stdout=logcat_log)
        result = host.run('dmesg')

        if not len(logcat_log.getvalue()):
            raise error.TestFail('No output from logcat')
        if not len(result.stdout):
            raise error.TestFail('No output from dmesg')
