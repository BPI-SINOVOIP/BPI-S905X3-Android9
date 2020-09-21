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

import acts.test_utils.power.PowerCoexBaseTest as PCoBT
from acts.test_decorators import test_tracker_info


class PowerCoexbaselineTest(PCoBT.PowerCoexBaseTest):
    def coex_baseline_test_func(self):
        """Base function to do coex baseline tests.

        Steps:
        1. Set the phone into desired state (WiFi, BT/BLE, cellular)
        2. Measures the power consumption
        3. Asserts pass/fail criteria based on measured power
        """
        attrs = [
            'screen_status', 'wifi_status', 'wifi_band', 'bt_status',
            'ble_status', 'cellular_status', 'cellular_band'
        ]
        indices = [2, 4, 6, 8, 10, 12, 14]
        self.decode_test_configs(attrs, indices)
        self.coex_test_phone_setup(
            self.test_configs.screen_status, self.test_configs.wifi_status,
            self.test_configs.wifi_band, self.test_configs.bt_status,
            self.test_configs.ble_status, self.test_configs.cellular_status,
            self.test_configs.cellular_band)
        self.measure_power_and_validate()

    @test_tracker_info(uuid='f3fc6667-73d8-4fb5-bdf3-0253e52043b1')
    def test_screen_OFF_WiFi_ON_band_None_bt_ON_ble_ON_cellular_OFF_band_None(
            self):
        self.coex_baseline_test_func()

    @test_tracker_info(uuid='1bec36d1-f7b2-4a4b-9f5d-dfb5ed985649')
    def test_screen_OFF_WiFi_Connected_band_2g_bt_ON_ble_ON_cellular_OFF_band_None(
            self):
        self.coex_baseline_test_func()

    @test_tracker_info(uuid='88170cad-8336-4dff-8e53-3cc693d01b72')
    def test_screen_OFF_WiFi_Connected_band_5g_bt_ON_ble_ON_cellular_OFF_band_None(
            self):
        self.coex_baseline_test_func()

    @test_tracker_info(uuid='b82e59a9-9b27-4ba2-88f6-48d7917066f4')
    def test_screen_OFF_WiFi_OFF_band_None_bt_ON_ble_ON_cellular_ON_band_Verizon(
            self):
        self.coex_baseline_test_func()

    @test_tracker_info(uuid='6409a02e-d63a-4c46-a210-1d5f1b006556')
    def test_screen_OFF_WiFi_Connected_band_5g_bt_OFF_ble_OFF_cellular_ON_band_Verizon(
            self):
        self.coex_baseline_test_func()

    @test_tracker_info(uuid='6f22792f-b304-4804-853d-e41484d442ab')
    def test_screen_OFF_WiFi_Connected_band_2g_bt_OFF_ble_OFF_cellular_ON_band_Verizon(
            self):
        self.coex_baseline_test_func()

    @test_tracker_info(uuid='11bb1683-4544-46b4-ad4a-875e31323729')
    def test_screen_OFF_WiFi_Connected_band_5g_bt_ON_ble_ON_cellular_ON_band_Verizon(
            self):
        self.coex_baseline_test_func()
