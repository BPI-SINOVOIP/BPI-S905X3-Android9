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

TVINPUT_V1_0_HAL = "android.hardware.tv.input@1.0::ITvInput"


class TvInputHidlTest(hal_hidl_host_test.HalHidlHostTest):
    """Two hello world test cases which use the shell driver."""

    TEST_HAL_SERVICES = {TVINPUT_V1_0_HAL}

    def setUpClass(self):
        """Creates a mirror and init tv input hal."""
        super(TvInputHidlTest, self).setUpClass()

        self.dut.hal.InitHidlHal(
            target_type="tv_input",
            target_basepaths=self.dut.libPaths,
            target_version=1.0,
            target_package="android.hardware.tv.input",
            target_component_name="ITvInput",
            hw_binder_service_name=self.getHalServiceName(TVINPUT_V1_0_HAL),
            bits=int(self.abi_bitness))

    def testGetStreamConfigurations(self):
        configs = self.dut.hal.tv_input.getStreamConfigurations(0)
        logging.info('return value of getStreamConfigurations(0): %s', configs)


if __name__ == "__main__":
    test_runner.main()
