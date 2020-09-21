#!/usr/bin/env python3.4
#
#   Copyright 2018 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the 'License');
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an 'AS IS' BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import time
from acts.test_decorators import test_tracker_info
from acts.test_utils.power import PowerWiFiBaseTest as PWBT
from acts.test_utils.wifi import wifi_test_utils as wutils

UNLOCK_SCREEN = 'input keyevent 82'


class PowerWiFiscanTest(PWBT.PowerWiFiBaseTest):
    def setup_class(self):
        super().setup_class()
        # Setup scan command
        SINGLE_SHOT_SCAN = (
            'am instrument -w -r  -e min_scan_count \"700\"'
            ' -e WifiScanTest-testWifiSingleShotScan %d'
            ' -e class com.google.android.platform.powertests.'
            'WifiScanTest#testWifiSingleShotScan'
            ' com.google.android.platform.powertests/'
            'android.test.InstrumentationTestRunner > /dev/null &' %
            (self.mon_duration + self.mon_offset + 10))
        BACKGROUND_SCAN = ('am instrument -w -r -e min_scan_count \"1\" -e '
                           'WifiScanTest-testWifiBackgroundScan %d -e class '
                           'com.google.android.platform.powertests.WifiScan'
                           'Test#testWifiBackgroundScan com.google.android.'
                           'platform.powertests/android.test.Instrumentation'
                           'TestRunner > /dev/null &' %
                           (self.mon_duration + self.mon_offset + 10))
        WIFI_SCAN = ('am instrument -w -r -e min_scan_count \"1\" -e '
                     'WifiScanTest-testWifiScan %d -e class '
                     'com.google.android.platform.powertests.WifiScanTest#'
                     'testWifiScan com.google.android.platform.powertests/'
                     'android.test.InstrumentationTestRunner > /dev/null &' %
                     (self.mon_duration + self.mon_offset + 10))
        self.APK_SCAN_CMDS = {
            'singleshot': SINGLE_SHOT_SCAN,
            'background': BACKGROUND_SCAN,
            'wifi': WIFI_SCAN
        }

    def scan_setup(self):
        """Setup for scan based on the type of scan.

        Trigger the desired scan.
        """
        self.log.info('Trigger {} scans'.format(self.test_configs.scan_mode))
        if self.test_configs.scan_type == 'apk':
            atten_setting = self.test_configs.wifi_band + '_' + self.test_configs.rssi
            self.set_attenuation(self.atten_level[atten_setting])
            self.dut.adb.shell_nb(
                self.APK_SCAN_CMDS[self.test_configs.scan_mode])
        else:
            self.mon_info.offset = 0
            if self.test_configs.scan_mode == 'pno':
                self.log.info('Set attenuation to trigger PNO scan')
                self.set_attenuation(self.atten_level['max_atten'])
            elif self.test_configs.scan_mode == 'connectivity':
                self.dut.droid.wakeUpNow()
                self.log.info(
                    'Now turn on screen to trigger connectivity scans')
                self.dut.adb.shell(UNLOCK_SCREEN)
            elif self.test_configs.scan_mode == 'roaming':
                atten_setting = self.test_configs.wifi_band + '_roaming'
                self.set_attenuation(self.atten_level[atten_setting])

    def wifi_scan_test_func(self):

        attrs = [
            'screen_status', 'wifi_status', 'wifi_band', 'rssi', 'scan_type',
            'scan_mode'
        ]
        indices = [2, 4, 6, 8, 10, 11]
        self.decode_test_configs(attrs, indices)
        if self.test_configs.wifi_status == 'Disconnected':
            self.setup_ap_connection(
                self.main_network[self.test_configs.wifi_band], connect=False)
        elif self.test_configs.wifi_status == 'Connected':
            self.setup_ap_connection(
                self.main_network[self.test_configs.wifi_band])
        else:
            wutils.wifi_toggle_state(self.dut, True)
        if self.test_configs.screen_status == 'OFF':
            self.dut.droid.goToSleepNow()
            self.dut.log.info('Screen is OFF')
        time.sleep(2)
        self.scan_setup()
        self.measure_power_and_validate()

    # Test cases
    # Power.apk triggered scans
    @test_tracker_info(uuid='e5539b01-e208-43c6-bebf-6f1e73d8d8cb')
    def test_screen_OFF_WiFi_Disconnected_band_2g_RSSI_high_scan_apk_singleshot(
            self):
        self.wifi_scan_test_func()

    @test_tracker_info(uuid='14c5a762-95bc-40ea-9fd4-27126df7d86c')
    def test_screen_OFF_WiFi_Disconnected_band_2g_RSSI_low_scan_apk_singleshot(
            self):
        self.wifi_scan_test_func()

    @test_tracker_info(uuid='a6506600-c567-43b5-9c25-86b505099b97')
    def test_screen_OFF_WiFi_Disconnected_band_2g_RSSI_none_scan_apk_singleshot(
            self):
        self.wifi_scan_test_func()

    @test_tracker_info(uuid='1a458248-1159-4c8e-a39f-92fc9e69c4dd')
    def test_screen_OFF_WiFi_Disconnected_band_5g_RSSI_high_scan_apk_singleshot(
            self):
        self.wifi_scan_test_func()

    @test_tracker_info(uuid='bd4da426-a621-4131-9f89-6e5a77f321d2')
    def test_screen_OFF_WiFi_Disconnected_band_5g_RSSI_low_scan_apk_singleshot(
            self):
        self.wifi_scan_test_func()

    @test_tracker_info(uuid='288b3add-8925-4803-81c0-53debf157ffc')
    def test_screen_OFF_WiFi_Disconnected_band_5g_RSSI_none_scan_apk_singleshot(
            self):
        self.wifi_scan_test_func()

    @test_tracker_info(uuid='f401c66c-e515-4f51-8ef2-2a03470d8ff2')
    def test_screen_OFF_WiFi_Disconnected_band_5g_RSSI_high_scan_apk_background(
            self):
        self.wifi_scan_test_func()

    @test_tracker_info(uuid='fe38c1c7-937c-42c0-9381-98356639df8f')
    def test_screen_OFF_WiFi_Disconnected_band_2g_RSSI_high_scan_apk_wifi(
            self):
        self.wifi_scan_test_func()

    @test_tracker_info(uuid='8eedefd1-3a08-4ac2-ba55-5eb438def3d4')
    def test_screen_OFF_WiFi_Disconnected_band_5g_RSSI_high_scan_apk_wifi(
            self):
        self.wifi_scan_test_func()

    # Firmware/framework scans
    @test_tracker_info(uuid='ff5ea952-ee31-4968-a190-82935ce7a8cb')
    def test_screen_OFF_WiFi_ON_band_5g_RSSI_high_scan_system_connectivity(
            self):
        """WiFi disconected, turn on Screen to trigger connectivity scans.

        """
        self.wifi_scan_test_func()

    @test_tracker_info(uuid='9a836e5b-8128-4dd2-8e96-e79177810bdd')
    def test_screen_OFF_WiFi_Connected_band_2g_RSSI_high_scan_system_connectivity(
            self):
        """WiFi connected to 2g, turn on screen to trigger connectivity scans.

        """
        self.wifi_scan_test_func()

    @test_tracker_info(uuid='51e3c4f1-742b-45af-afd5-ae3552a03272')
    def test_screen_OFF_WiFi_Connected_band_2g_RSSI_high_scan_system_roaming(
            self):
        """WiFi connected to 2g, low RSSI to be below roaming threshold.

        """
        self.wifi_scan_test_func()

    @test_tracker_info(uuid='a16ae337-326f-4d09-990f-42232c3c0dc4')
    def test_screen_OFF_WiFi_Connected_band_2g_RSSI_high_scan_system_pno(self):
        """WiFi connected to 2g, trigger pno scan.

        """
        self.wifi_scan_test_func()
