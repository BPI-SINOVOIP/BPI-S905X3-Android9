#!/usr/bin/env python
#
# Copyright (C) 2016 The Android Open Source Project
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
from vts.runners.host import test_runner
from vts.runners.host import utils
from vts.utils.python.controllers import adb


class RebootRootRemountTest(base_test.BaseTestClass):
    """Tests if device can root and remount /system partition after reboot.

    Attributes:
        dut: AndroidDevice, the device under test as config.
        verity: boolean, True if verity was enabled before test,
            False otherwise.
    """

    def setUpClass(self):
        self.dut = self.android_devices[0]
        self.verity = self.dut.verityEnabled

    def testRebootRootRemount(self):
        """Tests if /system partition can be remounted as r/w after reboot."""
        # Disable verity if it's enabled.
        if self.verity:
            logging.info("Disable verity.")
            self.dut.adb.disable_verity()

        try:
            self.dut.reboot()
            self.dut.waitForBootCompletion()
        except utils.TimeoutError:
            asserts.fail("Reboot failed.")

        try:
            self.dut.adb.root()
            self.dut.adb.wait_for_device()
            # Remount /system partition as r/w.
            self.dut.adb.remount()
            self.dut.adb.wait_for_device()
        except adb.AdbError():
            asserts.fail("Root/remount failed.")

        # Restore verity to its original state.
        if self.verity:
            logging.info("Enable verity.")
            self.dut.adb.enable_verity()
            try:
                self.dut.reboot()
                self.dut.waitForBootCompletion()
            except utils.TimeoutError:
                asserts.fail("Reboot failed after re-enabling verity.")

        asserts.assertEqual(self.verity, self.dut.verityEnabled,
                            "Verity state was successfully restored.")


if __name__ == "__main__":
    test_runner.main()
