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


class VrHidlTest(hal_hidl_host_test.HalHidlHostTest):
    """A simple testcase for the VR HIDL HAL."""

    TEST_HAL_SERVICES = {"android.hardware.vr@1.0::IVr"}

    def setUpClass(self):
        """Creates a mirror and turns on the framework-layer VR service."""
        super(VrHidlTest, self).setUpClass()
        self.dut.hal.InitHidlHal(
            target_type="vr",
            target_basepaths=self.dut.libPaths,
            target_version=1.0,
            target_package="android.hardware.vr",
            target_component_name="IVr",
            bits=int(self.abi_bitness))

    def testVrBasic(self):
        """A simple test case which just calls each registered function."""
        result = self.dut.hal.vr.init()
        logging.info("init result: %s", result)

        time.sleep(1)

        result = self.dut.hal.vr.setVrMode(True)
        logging.info("setVrMode(true) result: %s", result)

        time.sleep(1)

        result = self.dut.hal.vr.setVrMode(False)
        logging.info("setVrMode(false) result: %s", result)


if __name__ == "__main__":
    test_runner.main()
