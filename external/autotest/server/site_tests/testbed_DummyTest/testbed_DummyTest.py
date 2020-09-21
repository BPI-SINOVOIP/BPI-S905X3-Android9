# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.server import adb_utils
from autotest_lib.server import constants
from autotest_lib.server import test


class testbed_DummyTest(test.test):
    """A dummy test to verify testbed can access connected Android devices."""
    version = 1

    def run_once(self, testbed=None, test_apk_install=False):
        """A dummy test to verify testbed can access connected Android devices.

        Prerequisite: All connected DUTs are in ADB mode.

        @param testbed: host object representing the testbed.
        @param test_apk_install: True to test installing sl4a.apk. Default is
                False.
        """
        self.testbed = testbed
        for device in self.testbed.get_all_hosts():
            device.run('true')

        if test_apk_install:
            for adb_host in testbed.get_adb_devices().values():
              adb_utils.install_apk_from_build(
                        adb_host, constants.SL4A_APK, constants.SL4A_ARTIFACT,
                        package_name=constants.SL4A_PACKAGE)
