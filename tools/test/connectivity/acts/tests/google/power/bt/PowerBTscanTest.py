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

import acts.test_utils.power.PowerBTBaseTest as PBtBT
from acts.test_decorators import test_tracker_info


class PowerBTscanTest(PBtBT.PowerBTBaseTest):
    def ble_scan_base_func(self):
        """Base function to start a generic BLE scan and measures the power

        Steps:
        1. Sets the phone in airplane mode, disables gestures and location
        2. Turns ON/OFF BT, BLE and screen according to test conditions
        3. Sends the adb shell command to PMC to start scan
        4. Measures the power consumption
        5. Asserts pass/fail criteria based on measured power
        """
        # Decode the test params from test name
        attrs = ['screen_status', 'bt_status', 'ble_status', 'scan_mode']
        indices = [2, 4, 6, -1]
        self.decode_test_configs(attrs, indices)
        if self.test_configs.scan_mode == 'lowpower':
            scan_mode = 'low_power'
        elif self.test_configs.scan_mode == 'lowlatency':
            scan_mode = 'low_latency'
        else:
            scan_mode = self.test_configs.scan_mode
        self.phone_setup_for_BT(self.test_configs.bt_status,
                                self.test_configs.ble_status,
                                self.test_configs.screen_status)
        self.start_pmc_ble_scan(scan_mode, self.mon_info.offset,
                                self.mon_info.duration)
        self.measure_power_and_validate()

    # Test Cases: BLE Scans + Filtered scans
    @test_tracker_info(uuid='e9a36161-1d0c-4b9a-8bd8-80fef8cdfe28')
    def test_screen_ON_bt_ON_ble_ON_default_scan_balanced(self):
        self.ble_scan_base_func()

    @test_tracker_info(uuid='5fa61bf4-5f04-40bf-af52-6644b534d02e')
    def test_screen_OFF_bt_ON_ble_ON_filter_scan_opportunistic(self):
        self.ble_scan_base_func()

    @test_tracker_info(uuid='512b6cde-be83-43b0-b799-761380ba69ff')
    def test_screen_OFF_bt_ON_ble_ON_filter_scan_lowpower(self):
        self.ble_scan_base_func()

    @test_tracker_info(uuid='3a526838-ae7b-4cdb-bc29-89a5503d2306')
    def test_screen_OFF_bt_ON_ble_ON_filter_scan_balanced(self):
        self.ble_scan_base_func()

    @test_tracker_info(uuid='03a57cfd-4269-4a09-8544-84f878d2e801')
    def test_screen_OFF_bt_ON_ble_ON_filter_scan_lowlatency(self):
        self.ble_scan_base_func()

    # Test Cases: Background scans
    @test_tracker_info(uuid='20145317-e362-4bfd-9860-4ceddf764784')
    def test_screen_ON_bt_OFF_ble_ON_background_scan_lowlatency(self):
        self.ble_scan_base_func()

    @test_tracker_info(uuid='00a53dc3-2c33-43c4-b356-dba93249b823')
    def test_screen_ON_bt_OFF_ble_ON_background_scan_lowpower(self):
        self.ble_scan_base_func()

    @test_tracker_info(uuid='b7185d64-631f-4b18-8d0b-4e14b80db375')
    def test_screen_OFF_bt_OFF_ble_ON_background_scan_lowlatency(self):
        self.ble_scan_base_func()

    @test_tracker_info(uuid='93eb05da-a577-409c-8208-6af1899a10c2')
    def test_screen_OFF_bt_OFF_ble_ON_background_scan_lowpower(self):
        self.ble_scan_base_func()
