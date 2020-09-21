#!/usr/bin/env python3.4
#
# Copyright 2016 - The Android Open Source Project
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

from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_atten_utils import set_rssi
from acts.test_utils.tel.tel_defines import MAX_RSSI_RESERVED_VALUE
from acts.test_utils.tel.tel_defines import MIN_RSSI_RESERVED_VALUE
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_NW_SELECTION
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import GEN_4G
from acts.test_utils.tel.tel_test_utils import ensure_network_generation
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_test_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_test_utils import wait_for_wifi_data_connection
from acts.test_utils.tel.tel_test_utils import run_multithread_func
from acts.test_utils.tel.tel_test_utils import active_file_download_test
from acts.test_utils.tel.tel_test_utils import get_telephony_signal_strength
from acts.test_utils.tel.tel_test_utils import get_wifi_signal_strength
from acts.utils import adb_shell_ping

# Attenuator name
ATTEN_NAME_FOR_WIFI_2G = 'wifi0'
ATTEN_NAME_FOR_WIFI_5G = 'wifi1'
ATTEN_NAME_FOR_CELL_3G = 'cell0'
ATTEN_NAME_FOR_CELL_4G = 'cell1'

DEFAULT_PING_DURATION = 120
DEFAULT_IRAT_DURATION = 60


class TelWifiDataTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)

        self.stress_test_number = self.get_stress_test_number()
        self.live_network_ssid = self.user_params["wifi_network_ssid"]
        self.live_network_pwd = self.user_params.get("wifi_network_pass")

        self.attens = {}
        for atten in self.attenuators:
            self.attens[atten.path] = atten
        attentuator_name_list = [
            ATTEN_NAME_FOR_WIFI_2G, ATTEN_NAME_FOR_WIFI_5G,
            ATTEN_NAME_FOR_CELL_3G, ATTEN_NAME_FOR_CELL_4G
        ]
        for atten_name in attentuator_name_list:
            set_rssi(self.log, self.attens[atten_name], 0,
                     MAX_RSSI_RESERVED_VALUE)

    def teardown_test(self):
        super().teardown_test()
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI_2G], 0,
                 MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI_5G], 0,
                 MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL_3G], 0,
                 MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL_4G], 0,
                 MAX_RSSI_RESERVED_VALUE)
        return True

    def _basic_connectivity_check(self):
        """
        Set Attenuator Value for WiFi and Cell to 0
        Make sure DUT get Cell Data coverage (LTE)
        Make sure DUT WiFi is connected
        """
        ad = self.android_devices[0]
        toggle_airplane_mode(self.log, ad, False)
        if not ensure_network_generation(self.log, ad,
                                         GEN_4G, NETWORK_SERVICE_DATA):
            return False

        if not ensure_wifi_connected(self.log, ad,
                                     self.live_network_ssid,
                                     self.live_network_pwd):
            ad.log.error("connect WiFi failed")
            return False
        return True

    def _atten_setup_wifi_cell(self):
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI_2G], 0,
                 MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI_5G], 0,
                 MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL_3G], 0,
                 MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL_4G], 0,
                 MAX_RSSI_RESERVED_VALUE)

    def _atten_setup_cell_only(self):
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI_2G], 0,
                 MIN_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI_5G], 0,
                 MIN_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL_3G], 0,
                 MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL_4G], 0,
                 MAX_RSSI_RESERVED_VALUE)

    def _atten_setup_lte_only(self):
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI_2G], 0,
                 MIN_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI_5G], 0,
                 MIN_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL_3G], 0,
                 MIN_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL_4G], 0,
                 MAX_RSSI_RESERVED_VALUE)

    def _atten_setup_wcdma_only(self):
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI_2G], 0,
                 MIN_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI_5G], 0,
                 MIN_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL_3G], 0,
                 MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL_4G], 0,
                 MIN_RSSI_RESERVED_VALUE)

    def _atten_setup_wifi_only(self):
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI_2G], 0,
                 MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI_5G], 0,
                 MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL_3G], 0,
                 MIN_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL_4G], 0,
                 MIN_RSSI_RESERVED_VALUE)

    @TelephonyBaseTest.tel_test_wrap
    def _wifi_cell_irat_task(self, ad, irat_wait_time=60):
        """
        Atten only WiFi to MIN and MAX
        WiFi --> Cellular
        """
        self._atten_setup_wifi_cell()
        if (not wait_for_wifi_data_connection(self.log, ad, True,
                                              irat_wait_time) or
                not verify_http_connection(self.log, ad)):
            ad.log.error("Data not on WiFi")
            get_telephony_signal_strength(ad)
            get_wifi_signal_strength(ad)
            return False

        ad.log.info("Triggering WiFi to Cellular IRAT")
        self._atten_setup_cell_only()
        if (not wait_for_cell_data_connection(self.log, ad, True,
                                              irat_wait_time) or
                not verify_http_connection(self.log, ad)):
            ad.log.error("Data not on Cell")
            get_telephony_signal_strength(ad)
            get_wifi_signal_strength(ad)
            return False
        return True

    @test_tracker_info(uuid="b223f74b-59f4-4eec-8785-67420bd96bd1")
    @TelephonyBaseTest.tel_test_wrap
    def test_wifi_cell_irat_stress_ping_continuous(self):
        """Test for data switch between WiFi and Cell. DUT go in and out WiFi
        coverage for multiple times.

        Steps:
        1. Set WiFi and Cellular signal to good (attenuation value to MIN).
        2. Make sure DUT get Cell data coverage (LTE) and WiFi connected.
        3. Set WiFi RSSI to MAX (WiFi attenuator value to MIN).
        4. Verify DUT report WiFi connected and Internet access OK.
        5. Set WiFi RSSI to MIN (WiFi attenuator value to MAX).
        6. Verify DUT report Cellular Data connected and Internet access OK.
        7. Repeat Step 3~6 for stress number.

        Expected Results:
        4. DUT report WiFi connected and Internet access OK.
        6. DUT report Cellular Data connected and Internet access OK.
        7. Stress test should pass.

        Returns:
        True if Pass. False if fail.
        """
        if not self._basic_connectivity_check():
            self.log.error("Basic Connectivity Check Failed")
            return False

        total_iteration = self.stress_test_number
        ad = self.android_devices[0]
        ping_task = (adb_shell_ping, (ad, DEFAULT_PING_DURATION,
                                      "www.google.com", 200, 40))
        irat_task = (self._wifi_cell_irat_task, (ad, DEFAULT_IRAT_DURATION))
        current_iteration = 1
        while (current_iteration <= total_iteration):
            self.log.info(">----Current iteration = %d/%d----<",
                          current_iteration, total_iteration)
            results = run_multithread_func(self.log, [ping_task, irat_task])
            if not results[1]:
                ad.log.error("Data IRAT failed in active ICMP transfer")
                break
            if results[0]:
                ad.log.info("ICMP transfer succeeded with parallel IRAT")
            else:
                ad.log.error("ICMP transfer failed with parallel IRAT")
                break
            self.log.info(">----Iteration : %d/%d succeed.----<",
                          current_iteration, total_iteration)
            current_iteration += 1
        if current_iteration <= total_iteration:
            self.log.info(">----Iteration : %d/%d failed.----<",
                          current_iteration, total_iteration)
            return False
        else:
            return True

    @test_tracker_info(uuid="72d2aa4d-c395-417e-99c5-12dc22ea90a1")
    @TelephonyBaseTest.tel_test_wrap
    def test_wifi_cell_irat_stress_http_dl(self):
        """Test for data switch between WiFi and Cell. DUT go in and out WiFi
        coverage for multiple times.

        Steps:
        1. Set WiFi and Cellular signal to good (attenuation value to MIN).
        2. Make sure DUT get Cell data coverage (LTE) and WiFi connected.
        3. Set WiFi RSSI to MAX (WiFi attenuator value to MIN).
        4. Verify DUT report WiFi connected and able to download file
        5. Set WiFi RSSI to MIN (WiFi attenuator value to MAX).
        6. Verify DUT report Cellular Data connected and able to download file
        7. Repeat Step 3~6 for stress number.

        Expected Results:
        4. DUT report WiFi connected and able to download file
        6. DUT report Cellular Data connected and able to download file
        7. Stress test should pass.

        Returns:
        True if Pass. False if fail.
        """
        ad = self.android_devices[0]
        if not self._basic_connectivity_check():
            self.log.error("Basic Connectivity Check Failed")
            return False

        total_iteration = self.stress_test_number
        self.log.info("Stress test. Total iteration = %d.", total_iteration)
        current_iteration = 1
        while (current_iteration <= total_iteration):
            self.log.info(">----Current iteration = %d/%d----<",
                          current_iteration, total_iteration)

            self._atten_setup_wifi_cell()
            if (not wait_for_wifi_data_connection(self.log, ad, True)):
                ad.log.error("Data not on WiFi")
                get_telephony_signal_strength(ad)
                get_wifi_signal_strength(ad)
                break

            ad.on_mobile_data = False
            if not active_file_download_test(self.log, ad):
                ad.log.error("HTTP file download failed on WiFi")
                get_telephony_signal_strength(ad)
                get_wifi_signal_strength(ad)
                break

            self._atten_setup_cell_only()
            if (not wait_for_cell_data_connection(self.log, ad, True)):
                ad.log.error("Data not on Cell")
                get_telephony_signal_strength(ad)
                get_wifi_signal_strength(ad)
                break

            ad.on_mobile_data = True
            if not active_file_download_test(self.log, ad):
                ad.log.error("HTTP file download failed on cell")
                get_telephony_signal_strength(ad)
                get_wifi_signal_strength(ad)
                break

            self.log.info(">----Iteration : %d/%d succeed.----<",
                          current_iteration, total_iteration)
            current_iteration += 1

        if current_iteration <= total_iteration:
            self.log.info(">----Iteration : %d/%d failed.----<",
                          current_iteration, total_iteration)
            return False
        else:
            return True

    @test_tracker_info(uuid="bce71469-114c-489f-b9c4-26c53c29a553")
    @TelephonyBaseTest.tel_test_wrap
    def test_wifi_cell_irat_stress_ping(self):
        """Test for data switch between WiFi and Cell. DUT go in and out WiFi
        coverage for multiple times.

        Steps:
        1. Set WiFi and Cellular signal to good (attenuation value to MIN).
        2. Make sure DUT get Cell data coverage (LTE) and WiFi connected.
        3. Set WiFi RSSI to MAX (WiFi attenuator value to MIN).
        4. Verify DUT report WiFi connected and Internet access OK.
        5. Set WiFi RSSI to MIN (WiFi attenuator value to MAX).
        6. Verify DUT report Cellular Data connected and Internet access OK.
        7. Repeat Step 3~6 for stress number.

        Expected Results:
        4. DUT report WiFi connected and Internet access OK.
        6. DUT report Cellular Data connected and Internet access OK.
        7. Stress test should pass.

        Returns:
        True if Pass. False if fail.
        """
        ad = self.android_devices[0]
        if not self._basic_connectivity_check():
            self.log.error("Basic Connectivity Check Failed")
            return False

        total_iteration = self.stress_test_number
        self.log.info("Stress test. Total iteration = %d.", total_iteration)
        current_iteration = 1
        while (current_iteration <= total_iteration):
            self.log.info(">----Current iteration = %d/%d----<",
                          current_iteration, total_iteration)

            self._atten_setup_wifi_cell()
            if (not wait_for_wifi_data_connection(self.log, ad, True) or
                    not verify_http_connection(self.log, ad)):
                ad.log.error("Data not on WiFi")
                get_telephony_signal_strength(ad)
                get_wifi_signal_strength(ad)
                break

            self._atten_setup_cell_only()
            if (not wait_for_cell_data_connection(self.log, ad, True) or
                    not verify_http_connection(self.log, ad)):
                ad.log.error("Data not on Cell")
                get_telephony_signal_strength(ad)
                get_wifi_signal_strength(ad)
                break

            self.log.info(">----Iteration : %d/%d succeed.----<",
                          current_iteration, total_iteration)
            current_iteration += 1
        if current_iteration <= total_iteration:
            self.log.info(">----Iteration : %d/%d failed.----<",
                          current_iteration, total_iteration)
            return False
        else:
            return True

    @test_tracker_info(uuid="696f22ef-39cd-4e15-bbb2-f836d2ee47f1")
    @TelephonyBaseTest.tel_test_wrap
    def test_wifi_only_http_dl(self):
        """Test for 10MB file download on WiFi Only

        Steps:
        1. Set WiFi atten to MIN and Cellular to MAX
        2. Start downloading 1GB file from net
        3. Verify is the download is successfull

        Expected Results:
        1. File should download over WiFi

        Returns:
        True if Pass. False if fail.
        """
        ad = self.android_devices[0]
        if not self._basic_connectivity_check():
            self.log.error("Basic Connectivity Check Failed")
            return False
        self._atten_setup_wifi_only()
        if (not wait_for_wifi_data_connection(self.log, ad, True) or
                not verify_http_connection(self.log, ad)):
            ad.log.error("Data not on WiFi")
            get_telephony_signal_strength(ad)
            get_wifi_signal_strength(ad)
            return False
        ad.on_mobile_data = False
        if not active_file_download_test(self.log, ad, "10MB"):
            ad.log.error("HTTP file download failed on WiFi")
            get_telephony_signal_strength(ad)
            get_wifi_signal_strength(ad)
            return False
        return True

    @test_tracker_info(uuid="6c9bf89b-5469-4b08-acf4-0ef651b1a318")
    @TelephonyBaseTest.tel_test_wrap
    def test_lte_only_http_dl(self):
        """Test for 1GB file download on WiFi Only

        Steps:
        1. Set WiFi atten to MIN and Cellular to MAX
        2. Start downloading 1GB file from net
        3. Verify is the download is successfull

        Expected Results:
        1. File should download over WiFi

        Returns:
        True if Pass. False if fail.
        """
        ad = self.android_devices[0]
        if not self._basic_connectivity_check():
            self.log.error("Basic Connectivity Check Failed")
            return False
        self._atten_setup_lte_only()
        if (not wait_for_cell_data_connection(self.log, ad, True) or
                not verify_http_connection(self.log, ad)):
            ad.log.error("Data not on LTE")
            get_telephony_signal_strength(ad)
            get_wifi_signal_strength(ad)
            return False
        ad.on_mobile_data = True
        if not active_file_download_test(self.log, ad, "512MB"):
            ad.log.error("HTTP file download failed on LTE")
            get_telephony_signal_strength(ad)
            get_wifi_signal_strength(ad)
            return False
        return True


if __name__ == "__main__":
    raise Exception("Cannot run this class directly")
