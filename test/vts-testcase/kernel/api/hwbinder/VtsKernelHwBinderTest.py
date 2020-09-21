#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.utils.python.file import target_file_utils

HWBINDER_PATH = "/dev/hwbinder"


class VtsKernelHwBinderTest(base_test.BaseTestClass):
    """Test case to validate existence of hwbinder node.
    """

    def setUpClass(self):
        self.dut = self.android_devices[0]
        self.shell = self.dut.shell

    def testHwbinderExistence(self):
        """Checks that hwbinder node exists.
        """
        logging.info("Testing existence of %s", HWBINDER_PATH)
        asserts.assertTrue(
            target_file_utils.Exists(HWBINDER_PATH, self.shell),
            "%s: File does not exist." % HWBINDER_PATH)

        try:
            permissions = target_file_utils.GetPermission(
                HWBINDER_PATH, self.shell)
            asserts.assertTrue(
                target_file_utils.IsReadWrite(permissions),
                "%s: File has invalid permissions (%s)" % (HWBINDER_PATH,
                                                           permissions))
        except (ValueError, IOError) as e:
            asserts.fail("Failed to assert permissions: %s" % str(e))


if __name__ == "__main__":
    test_runner.main()
