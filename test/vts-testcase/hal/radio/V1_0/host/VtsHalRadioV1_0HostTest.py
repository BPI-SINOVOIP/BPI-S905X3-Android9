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
from vts.runners.host import test_runner
from vts.testcases.template.hal_hidl_host_test import hal_hidl_host_test


class VtsHalRadioV1_0HostTest(hal_hidl_host_test.HalHidlHostTest):
    """A simple testcase for the Radio HIDL HAL."""

    TEST_HAL_SERVICES = {"android.hardware.radio@1.0::IRadio"}
    def setUpClass(self):
        """Creates a mirror and init radio hal."""
        super(VtsHalRadioV1_0HostTest, self).setUpClass()

        self.dut.hal.InitHidlHal(
            target_type="radio",
            target_basepaths=self.dut.libPaths,
            target_version=1.0,
            target_package="android.hardware.radio",
            target_component_name="IRadio",
            hw_binder_service_name="slot1",
            bits=int(self.abi_bitness))

        self.radio = self.dut.hal.radio  # shortcut
        self.radio_types = self.dut.hal.radio.GetHidlTypeInterface("types")
        logging.info("Radio types: %s", self.radio_types)

    def testHelloWorld(self):
        logging.info('hello world')


if __name__ == "__main__":
    test_runner.main()
