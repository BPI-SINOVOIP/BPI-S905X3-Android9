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
Test script to test various airplane mode scenarios and how it
affects Bluetooth state.
"""

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import bluetooth_enabled_check
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode_by_adb
from queue import Empty
import time


class BtAirplaneModeTest(BluetoothBaseTest):
    default_timeout = 10
    grace_timeout = 4
    WAIT_TIME_ANDROID_STATE_SETTLING = 5

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.dut = self.android_devices[0]

    def setup_test(self):
        super(BluetoothBaseTest, self).setup_test()
        # Ensure testcase starts with Airplane mode off
        if not toggle_airplane_mode_by_adb(self.log, self.dut, False):
            return False
        time.sleep(self.WAIT_TIME_ANDROID_STATE_SETTLING)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='11209b74-f27f-44cc-b336-8cf7f168f653')
    def test_bt_on_toggle_airplane_mode_on(self):
        """Test that toggles airplane mode on while BT on

        Turning airplane mode on should toggle Bluetooth off
        successfully.

        Steps:
        1. Verify that Bluetooth state is on
        2. Turn airplane mode on
        3. Verify that Bluetooth state is off

        Expected Result:
        Bluetooth should toggle off successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Bluetooth, Airplane
        Priority: 3
        """
        if not bluetooth_enabled_check(self.dut):
            self.log.error("Failed to set Bluetooth state to enabled")
            return False
        if not toggle_airplane_mode_by_adb(self.log, self.dut, True):
            self.log.error("Failed to toggle airplane mode on")
            return False
        return not self.dut.droid.bluetoothCheckState()

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='823bb1e1-ce39-43a9-9f2c-0bd2a9b8793f')
    def test_bt_on_toggle_airplane_mode_on_bt_remains_off(self):
        """Test that verifies BT remains off after airplane mode toggles

        Turning airplane mode on should toggle Bluetooth off
        successfully and Bluetooth state should remain off. For
        this test we will use 60 seconds as a baseline.

        Steps:
        1. Verify that Bluetooth state is on
        2. Turn airplane mode on
        3. Verify that Bluetooth state is off
        3. Verify tat Bluetooth state remains off for 60 seconds

        Expected Result:
        Bluetooth should remain toggled off.

        Returns:
          Pass if True
          Fail if False

        TAGS: Bluetooth, Airplane
        Priority: 3
        """
        if not bluetooth_enabled_check(self.dut):
            self.log.error("Failed to set Bluetooth state to enabled")
            return False
        if not toggle_airplane_mode_by_adb(self.log, self.dut, True):
            self.log.error("Failed to toggle airplane mode on")
            return False
        toggle_timeout = 60
        self.log.info(
            "Waiting {} seconds until verifying Bluetooth state.".format(
                toggle_timeout))
        time.sleep(toggle_timeout)
        return not self.dut.droid.bluetoothCheckState()

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d3977a15-c4b8-4dad-b4e4-98e7c3216688')
    def test_bt_on_toggle_airplane_mode_on_then_off(self):
        """Test that toggles airplane mode both on and off

        Turning airplane mode on should toggle Bluetooth off
        successfully. Turning airplane mode off should toggle
        Bluetooth back on.

        Steps:
        1. Verify that Bluetooth state is on
        2. Turn airplane mode on
        3. Verify that Bluetooth state is off
        4. Turn airplane mode off
        5. Verify that Bluetooth state is on

        Expected Result:
        Bluetooth should toggle off successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Bluetooth, Airplane
        Priority: 3
        """
        if not bluetooth_enabled_check(self.dut):
            self.log.error("Failed to set Bluetooth state to enabled")
            return False
        if not toggle_airplane_mode_by_adb(self.log, self.dut, True):
            self.log.error("Failed to toggle airplane mode on")
            return False
        if not toggle_airplane_mode_by_adb(self.log, self.dut, False):
            self.log.error("Failed to toggle airplane mode off")
            return False
        time.sleep(self.WAIT_TIME_ANDROID_STATE_SETTLING)
        return self.dut.droid.bluetoothCheckState()
