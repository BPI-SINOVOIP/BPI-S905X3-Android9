#/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""
Test script to test PAN testcases.

Test Script assumes that an internet connection
is available through a telephony provider that has
tethering allowed.

This device was not intended to run in a sheild box.
"""
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import bluetooth_enabled_check
from acts.test_utils.bt.bt_test_utils import orchestrate_and_verify_pan_connection
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from queue import Empty
import time


class BtPanTest(BluetoothBaseTest):
    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.pan_dut = self.android_devices[0]
        self.panu_dut = self.android_devices[1]

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='54f86e21-6d31-439c-bf57-426e9005cbc3')
    def test_pan_connection(self):
        """Test bluetooth PAN connection

        Test basic PAN connection between two devices.

        Steps:
        1. Enable Airplane mode on PANU device. Enable Bluetooth only.
        2. Enable Bluetooth tethering on PAN Service device.
        3. Pair the PAN Service device to the PANU device.
        4. Verify that Bluetooth tethering is enabled on PAN Service device.
        5. Enable PAN profile from PANU device to PAN Service device.
        6. Verify HTTP connection on PANU device.

        Expected Result:
        PANU device has internet access.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, PAN, tethering
        Priority: 1
        """
        return orchestrate_and_verify_pan_connection(self.pan_dut,
                                                     self.panu_dut)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='fa40a0b9-f326-4382-967a-fe4c73483a68')
    def test_pan_connection_then_disconnect(self):
        """Test bluetooth PAN connection then disconnect service

        Test basic PAN connection between two devices then disconnect
        service.

        Steps:
        1. Enable Airplane mode on PANU device. Enable Bluetooth only.
        2. Enable Bluetooth tethering on PAN Service device.
        3. Pair the PAN Service device to the PANU device.
        4. Verify that Bluetooth tethering is enabled on PAN Service device.
        5. Enable PAN profile from PANU device to PAN Service device.
        6. Verify HTTP connection on PANU device.
        7. Disable Bluetooth tethering on PAN Service device.
        8. Verify no HTTP connection on PANU device.

        Expected Result:
        PANU device does not have internet access.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, PAN, tethering
        Priority: 1
        """
        if not orchestrate_and_verify_pan_connection(self.pan_dut,
                                                     self.panu_dut):
            self.log.error("Could not establish a PAN connection.")
            return False
        self.pan_dut.droid.bluetoothPanSetBluetoothTethering(False)
        if not verify_http_connection(self.log, self.panu_dut,
                                      expected_state=False):
            self.log.error("PANU device still has internet access.")
            return False
        self.log.info("PANU device has no internet access as expected.")
        return True
