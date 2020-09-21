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


class SensorsHidlTest(hal_hidl_host_test.HalHidlHostTest):
    """Host testcase class for the SENSORS HIDL HAL.

    This class set-up/tear-down the webDB host test framwork and contains host test cases for
    sensors HIDL HAL.
    """

    TEST_HAL_SERVICES = {"android.hardware.sensors@1.0::ISensors"}
    def setUpClass(self):
        """Creates a mirror and turns on the framework-layer SENSORS service."""
        super(SensorsHidlTest, self).setUpClass()

        self.dut.hal.InitHidlHal(
            target_type="sensors",
            target_basepaths=self.dut.libPaths,
            target_version=1.0,
            target_package="android.hardware.sensors",
            target_component_name="ISensors",
            bits=int(self.abi_bitness))

    def testSensorsBasic(self):
        """Test the basic operation of test framework and sensor HIDL HAL

        This test obtains predefined enum values via sensors HIDL HAL host test framework and
        compares them to known values as a sanity check to make sure both sensors HAL
        and the test framework are working properly.
        """
        sensors_types = self.dut.hal.sensors.GetHidlTypeInterface("types")
        logging.info("sensors_types: %s", sensors_types)
        logging.info("OK: %s", sensors_types.Result.OK)
        logging.info("BAD_VALUE: %s", sensors_types.Result.BAD_VALUE)
        logging.info("NO_MEMORY: %s", sensors_types.Result.NO_MEMORY)
        logging.info("PERMISSION_DENIED: %s", sensors_types.Result.PERMISSION_DENIED)
        logging.info("INVALID_OPERATION: %s", sensors_types.Result.INVALID_OPERATION)
        asserts.assertEqual(sensors_types.Result.OK, 0)
        asserts.assertEqual(sensors_types.Result.NO_MEMORY, -12)
        asserts.assertEqual(sensors_types.Result.BAD_VALUE, -22)
        asserts.assertEqual(sensors_types.Result.INVALID_OPERATION, -38)

        logging.info("sensor types:")
        logging.info("ACCELEROMETER: %s", sensors_types.SensorType.ACCELEROMETER)
        logging.info("MAGNETIC_FIELD: %s", sensors_types.SensorType.MAGNETIC_FIELD)
        logging.info("GYROSCOPE: %s", sensors_types.SensorType.GYROSCOPE)
        asserts.assertEqual(sensors_types.SensorType.ACCELEROMETER, 1)
        asserts.assertEqual(sensors_types.SensorType.MAGNETIC_FIELD, 2)
        asserts.assertEqual(sensors_types.SensorType.GYROSCOPE, 4)

if __name__ == "__main__":
    test_runner.main()
