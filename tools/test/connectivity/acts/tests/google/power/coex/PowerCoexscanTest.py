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

import math
import acts.test_utils.power.PowerCoexBaseTest as PCoBT
from acts.test_decorators import test_tracker_info


class PowerCoexscanTest(PCoBT.PowerCoexBaseTest):
    def setup_class(self):

        super().setup_class()
        iterations = math.floor((self.mon_duration + self.mon_offset + 10) /
                                self.wifi_scan_interval)

        self.PERIODIC_WIFI_SCAN = (
            'am instrument -w -r -e scan-interval \"%d\" -e scan-iterations'
            ' \"%d\" -e class com.google.android.platform.powertests.'
            'WifiTests#testGScanAllChannels com.google.android.platform.'
            'powertests/android.test.InstrumentationTestRunner > /dev/null &' %
            (self.wifi_scan_interval, iterations))

    def coex_scan_test_func(self):
        """Base test function for coex scan tests.

        Steps:
        1. Setup phone in correct state
        2. Start desired scans
        3. Measure power and validate result
        """
        attrs = [
            'screen_status', 'wifi_status', 'wifi_band', 'wifi_scan',
            'bt_status', 'ble_status', 'ble_scan_mode', 'cellular_status',
            'cellular_band'
        ]
        indices = [2, 4, 6, 8, 10, 12, 15, 17, 19]
        self.decode_test_configs(attrs, indices)
        if self.test_configs.ble_scan_mode == 'lowpower':
            ble_scan_mode = 'low_power'
        elif self.test_configs.ble_scan_mode == 'lowlatency':
            ble_scan_mode = 'low_latency'
        else:
            ble_scan_mode = self.test_configs.ble_scan_mode
        self.phone_setup_for_BT(self.test_configs.bt_status,
                                self.test_configs.ble_status,
                                self.test_configs.screen_status)
        self.coex_scan_setup(self.test_configs.wifi_scan, ble_scan_mode,
                             self.PERIODIC_WIFI_SCAN)
        self.measure_power_and_validate()

    @test_tracker_info(uuid='a998dd2b-f5f1-4361-b5da-83e42a69e80b')
    def test_screen_ON_WiFi_Connected_band_2g_scan_OFF_bt_ON_ble_ON_default_scan_balanced_cellular_OFF_band_None(
            self):
        self.coex_scan_test_func()

    @test_tracker_info(uuid='87146825-787a-4ea7-9622-30e9286c8a76')
    def test_screen_OFF_WiFi_Connected_band_2g_scan_OFF_bt_ON_ble_ON_filtered_scan_lowpower_cellular_OFF_band_None(
            self):
        self.coex_scan_test_func()

    @test_tracker_info(uuid='2e645deb-b744-4272-8578-5d4cb159d5aa')
    def test_screen_OFF_WiFi_Connected_band_5g_scan_OFF_bt_ON_ble_ON_filtered_scan_lowpower_cellular_OFF_band_None(
            self):
        self.coex_scan_test_func()

    @test_tracker_info(uuid='d458bc41-f1c8-4ed6-a7b5-0bec34780dda')
    def test_screen_OFF_WiFi_Disconnected_band_2g_scan_ON_bt_ON_ble_ON_page_scan_None_cellular_OFF_band_None(
            self):
        self.coex_scan_test_func()

    @test_tracker_info(uuid='6d9c0e8e-6a0f-458b-84d2-7d60fc254170')
    def test_screen_OFF_WiFi_Disconnected_band_2g_scan_ON_bt_ON_ble_ON_filtered_scan_lowpower_cellular_OFF_band_None(
            self):
        self.coex_scan_test_func()

    @test_tracker_info(uuid='ba52317f-426a-4688-a0a5-1394bcc7b092')
    def test_screen_OFF_WiFi_Disconnected_band_2g_scan_ON_bt_ON_ble_ON_filtered_scan_lowlatency_cellular_OFF_band_None(
            self):
        self.coex_scan_test_func()

    @test_tracker_info(uuid='b4c63eac-bc77-4e76-afff-ade98dde4411')
    def test_screen_OFF_WiFi_Connected_band_2g_scan_PNO_bt_ON_ble_ON_filtered_scan_lowlatency_cellular_OFF_band_None(
            self):
        self.coex_scan_test_func()

    @test_tracker_info(uuid='798796dc-960c-42b2-a835-2b2aefa028d5')
    def test_screen_OFF_WiFi_Disconnected_band_5g_scan_ON_bt_OFF_ble_OFF_no_scan_None_cellular_ON_band_Verizon(
            self):
        self.coex_scan_test_func()

    @test_tracker_info(uuid='6ae44d84-0e68-4524-99b2-d3bfbd2253b8')
    def test_screen_OFF_WiFi_Disconnected_band_5g_scan_ON_bt_OFF_ble_ON_background_scan_lowpower_cellular_ON_band_Verizon(
            self):
        self.coex_scan_test_func()

    @test_tracker_info(uuid='2cb915a3-6319-4ac4-9e4d-9325b3b731c8')
    def test_screen_OFF_WiFi_Disconnected_band_2g_scan_ON_bt_OFF_ble_ON_background_scan_lowlatency_cellular_ON_band_Verizon(
            self):
        self.coex_scan_test_func()
