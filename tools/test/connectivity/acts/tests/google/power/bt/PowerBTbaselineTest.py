#!/usr/bin/env python3.4
#
#   Copyright 2018 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from acts import base_test
from acts.test_decorators import test_tracker_info
import acts.test_utils.power.PowerBTBaseTest as PBtBT


class PowerBTbaselineTest(PBtBT.PowerBTBaseTest):
    def __init__(self, controllers):

        base_test.BaseTestClass.__init__(self, controllers)

    def bt_baseline_test_func(self):
        """Base function for BT baseline measurement.

        Steps:
        1. Sets the phone in airplane mode, disables gestures and location
        2. Turns ON/OFF BT, BLE and screen according to test conditions
        3. Measures the power consumption
        4. Asserts pass/fail criteria based on measured power
        """

        # Decode the test params from test name
        attrs = ['screen_status', 'bt_status', 'ble_status', 'scan_status']
        indices = [2, 4, 6, 7]
        self.decode_test_configs(attrs, indices)
        # Setup the phoen at desired state
        self.phone_setup_for_BT(self.test_configs.bt_status,
                                self.test_configs.ble_status,
                                self.test_configs.screen_status)
        if self.test_configs.scan_status == 'connectable':
            self.dut.droid.bluetoothMakeConnectable()
        elif self.test_configs.scan_status == 'discoverable':
            self.dut.droid.bluetoothMakeDiscoverable(
                self.mon_info.duration + self.mon_info.offset)
        self.measure_power_and_validate()

    # Test cases- Baseline
    @test_tracker_info(uuid='3f8ac0cb-f20d-4569-a58e-6009c89ea049')
    def test_screen_OFF_bt_ON_ble_ON_connectable(self):
        self.bt_baseline_test_func()

    @test_tracker_info(uuid='d54a992e-37ed-460a-ada7-2c51941557fd')
    def test_screen_OFF_bt_ON_ble_ON_discoverable(self):
        self.bt_baseline_test_func()

    @test_tracker_info(uuid='8f4c36b5-b18e-4aa5-9fe5-aafb729c1034')
    def test_screen_ON_bt_ON_ble_ON_connectable(self):
        self.bt_baseline_test_func()

    @test_tracker_info(uuid='7128356f-67d8-46b3-9d6b-1a4c9a7a1745')
    def test_screen_ON_bt_ON_ble_ON_discoverable(self):
        self.bt_baseline_test_func()
