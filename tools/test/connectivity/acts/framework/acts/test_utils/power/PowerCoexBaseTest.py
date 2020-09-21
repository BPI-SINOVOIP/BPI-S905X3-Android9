#!/usr/bin/env python3.4
#
#   Copyright 2018 - The Android Open Source Project
8
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

import acts.test_utils.power.PowerBTBaseTest as PBtBT
import acts.test_utils.power.PowerWiFiBaseTest as PWBT
from acts import utils
from acts.test_utils.wifi import wifi_test_utils as wutils


class PowerCoexBaseTest(PBtBT.PowerBTBaseTest, PWBT.PowerWiFiBaseTest):
    """Base class for BT power related tests.

    Inherited from the PowerBaseTest class
    """

    def coex_test_phone_setup(self, Screen_status, WiFi_status, WiFi_band,
                              BT_status, BLE_status, Cellular_status,
                              Celluar_band):
        """Setup the phone in desired state for coex tests.

        Args:
            Screen_status: 'ON' or 'OFF'
            WiFi_status: 'ON', 'Connected', 'Disconnected', or 'OFF'
            WiFi_band: '2g', '5g' or None, the band of AP
            BT_status: 'ON' or 'OFF'
            BLE_status: 'ON' or 'OFF'
            Cellular_status: 'ON' or 'OFF'
            Celluar_band: 'Verizon', 'Tmobile', or 'ATT' for live network,
                actual band for callbox setup; 'None' when celluar is OFF
        """
        # Setup WiFi
        if WiFi_status is 'ON':
            wutils.wifi_toggle_state(self.dut, True)
        elif WiFi_status is 'Connected':
            self.setup_ap_connection(self.main_network[WiFi_band])
        elif WiFi_status is 'Disconnected':
            self.setup_ap_connection(
                self.main_network[WiFi_band], connect=False)

        # Setup BT/BLE
        self.phone_setup_for_BT(BT_status, BLE_status, Screen_status)

        # Setup Cellular
        if Cellular_status is 'ON':
            self.dut.droid.connectivityToggleAirplaneMode(False)
            utils.set_mobile_data_always_on(self.dut, True)

    def coex_scan_setup(self, WiFi_scan, BLE_scan_mode, wifi_scan_command):
        """Setup for scan activities on WiFi, BT/BLE, and cellular.

        Args:
            WiFi_scan: 'ON', 'OFF' or 'PNO'
            BLE_scan_mode: 'balanced', 'opportunistic', 'low_power', or 'low_latency'
        """
        if WiFi_scan is 'ON':
            self.dut.adb.shell(wifi_scan_command)
        if WiFi_scan is 'PNO':
            self.log.info(
                'Set attenuation so device loses connection to trigger PNO scans'
            )
            # Set to maximum attenuation 95 dB to cut down connection
            [self.attenuators[i].set_atten(95) for i in range(self.num_atten)]
        if BLE_scan_mode is not None:
            self.start_pmc_ble_scan(BLE_scan_mode, self.mon_info.offset,
                                    self.mon_info.duration)
