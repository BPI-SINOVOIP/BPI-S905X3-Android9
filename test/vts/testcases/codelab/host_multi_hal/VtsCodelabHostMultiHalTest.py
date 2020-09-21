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
import threading
import time

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import test_runner


class CodelabHostMultiHalTest(base_test.BaseTestClass):
    """A simple testcase for testing multiple HALs."""

    def setUpClass(self):
        """Creates hal mirrors."""
        self.dut = self.android_devices[0]

        self.dut.hal.InitHidlHal(
            target_type="light",
            target_basepaths=self.dut.libPaths,
            target_version=2.0,
            target_package="android.hardware.light",
            target_component_name="ILight",
            bits=int(self.abi_bitness))

        self.dut.hal.InitHidlHal(
            target_type="thermal",
            target_basepaths=self.dut.libPaths,
            target_version=1.0,
            target_package="android.hardware.thermal",
            target_component_name="IThermal",
            bits=int(self.abi_bitness))

        self.light_types = self.dut.hal.light.GetHidlTypeInterface("types")
        self.thermal_types = self.dut.hal.thermal.GetHidlTypeInterface("types")

    def LightTest(self):
        """A sample test for Light HAL."""
        whiteState = {
            "color": 0xFFFFFFFF,
            "flashMode": self.light_types.Flash.TIMED,
            "flashOnMs": 100,
            "flashOffMs": 50,
            "brightnessMode": self.light_types.Brightness.USER,
        }
        whiteState_pb = self.light_types.Py2Pb("LightState", whiteState)
        status = self.dut.hal.light.setLight(self.light_types.Type.BACKLIGHT,
                                             whiteState_pb)
        asserts.assertEqual(status, self.light_types.Status.SUCCESS)

    def ThermalTest(self):
        """A sample test for Thermal HAL."""
        status, cpuUsages = self.dut.hal.thermal.getCpuUsages()
        asserts.assertEqual(status['code'],
                            self.thermal_types.ThermalStatusCode.SUCCESS)

    def testBase(self):
        """A basic test case which tests APIs of two HALs."""
        self.LightTest()
        self.ThermalTest()

    def testMutlThread(self):
        """A basic test case which tests two HALs in parallel."""
        def LightWorker():
            for i in range(20):
                logging.info("Light test round: %s", i)
                self.LightTest()
                time.sleep(1)
            logging.info("Light test exiting.")

        def ThermalWorker():
            for i in range(10):
                logging.info("Thermal test round: %s", i)
                self.ThermalTest()
                time.sleep(2)
            logging.info("Thermal test exiting.")

        t1 = threading.Thread(name='light_test', target=LightWorker)
        t2 = threading.Thread(name='thermal_test', target=ThermalWorker)
        t1.start()
        t2.start()

        t1.join()
        t2.join()


if __name__ == "__main__":
    test_runner.main()
