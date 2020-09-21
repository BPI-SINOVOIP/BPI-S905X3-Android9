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
import time

from vts.runners.host import asserts
from vts.runners.host import test_runner
from vts.testcases.template.hal_hidl_host_test import hal_hidl_host_test


class VibratorHidlTest(hal_hidl_host_test.HalHidlHostTest):
    """A simple testcase for the VIBRATOR HIDL HAL."""

    TEST_HAL_SERVICES = {"android.hardware.vibrator@1.0::IVibrator"}
    def setUpClass(self):
        """Creates a mirror and turns on the framework-layer VIBRATOR service."""
        super(VibratorHidlTest, self).setUpClass()

        self.dut.hal.InitHidlHal(
            target_type="vibrator",
            target_basepaths=self.dut.libPaths,
            target_version=1.0,
            target_package="android.hardware.vibrator",
            target_component_name="IVibrator",
            bits=int(self.abi_bitness))

    def testVibratorBasic(self):
        """A simple test case which just calls each registered function."""
        vibrator_types = self.dut.hal.vibrator.GetHidlTypeInterface("types")
        logging.info("vibrator_types: %s", vibrator_types)
        logging.info("OK: %s", vibrator_types.Status.OK)
        logging.info("UNKNOWN_ERROR: %s", vibrator_types.Status.UNKNOWN_ERROR)
        logging.info("BAD_VALUE: %s", vibrator_types.Status.BAD_VALUE)
        logging.info("UNSUPPORTED_OPERATION: %s",
            vibrator_types.Status.UNSUPPORTED_OPERATION)

        result = self.dut.hal.vibrator.on(10000)
        logging.info("on result: %s", result)
        asserts.assertEqual(vibrator_types.Status.OK, result)

        time.sleep(1)

        result = self.dut.hal.vibrator.off()
        logging.info("off result: %s", result)
        asserts.assertEqual(vibrator_types.Status.OK, result)

if __name__ == "__main__":
    test_runner.main()
