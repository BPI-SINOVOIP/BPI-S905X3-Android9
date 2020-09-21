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

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import test_runner
from vts.runners.host import utils


class RebootTest(base_test.BaseTestClass):
    """Tests if device survives reboot.

    Attributes:
        dut: AndroidDevice, the device under test as config
    """

    def setUpClass(self):
        self.dut = self.android_devices[0]

    def testReboot(self):
        """Tests if device is still responsive after reboot."""
        try:
            self.dut.reboot()
            # If waitForBootCompletion() returns, the device must have
            # responded to an adb shell command.
            self.dut.waitForBootCompletion()
        except utils.TimeoutError:
            asserts.fail("Reboot failed.")


if __name__ == "__main__":
    test_runner.main()
