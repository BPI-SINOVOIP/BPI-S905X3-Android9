#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google
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
"""
    Test Script for Telephony Settings
"""

import os
import time

from acts import signals
from acts.keys import Config
from acts.utils import create_dir
from acts.utils import unzip_maintain_permissions
from acts.utils import get_current_epoch_time
from acts.utils import exe_cmd
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_WIFI_CONNECTION
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_IMS_REGISTRATION
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_VOLTE_ENABLED
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_WFC_ENABLED
from acts.test_utils.tel.tel_defines import RAT_FAMILY_WLAN
from acts.test_utils.tel.tel_defines import WFC_MODE_CELLULAR_PREFERRED
from acts.test_utils.tel.tel_defines import WFC_MODE_DISABLED
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_ONLY
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import ensure_phone_subscription
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import flash_radio
from acts.test_utils.tel.tel_test_utils import get_outgoing_voice_sub_id
from acts.test_utils.tel.tel_test_utils import get_slot_index_from_subid
from acts.test_utils.tel.tel_test_utils import is_droid_in_rat_family
from acts.test_utils.tel.tel_test_utils import is_sim_locked
from acts.test_utils.tel.tel_test_utils import is_wfc_enabled
from acts.test_utils.tel.tel_test_utils import multithread_func
from acts.test_utils.tel.tel_test_utils import power_off_sim
from acts.test_utils.tel.tel_test_utils import power_on_sim
from acts.test_utils.tel.tel_test_utils import print_radio_info
from acts.test_utils.tel.tel_test_utils import set_qxdm_logger_command
from acts.test_utils.tel.tel_test_utils import set_wfc_mode
from acts.test_utils.tel.tel_test_utils import system_file_push
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode_by_adb
from acts.test_utils.tel.tel_test_utils import toggle_volte
from acts.test_utils.tel.tel_test_utils import unlock_sim
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_test_utils import wait_for_ims_registered
from acts.test_utils.tel.tel_test_utils import wait_for_network_rat
from acts.test_utils.tel.tel_test_utils import wait_for_not_network_rat
from acts.test_utils.tel.tel_test_utils import wait_for_volte_enabled
from acts.test_utils.tel.tel_test_utils import wait_for_wfc_disabled
from acts.test_utils.tel.tel_test_utils import wait_for_wfc_enabled
from acts.test_utils.tel.tel_test_utils import wait_for_wifi_data_connection
from acts.test_utils.tel.tel_test_utils import wifi_reset
from acts.test_utils.tel.tel_test_utils import wifi_toggle_state
from acts.test_utils.tel.tel_test_utils import set_wifi_to_default
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.tel.tel_voice_utils import phone_idle_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_idle_volte
from acts.utils import set_mobile_data_always_on


class TelLiveSettingsTest(TelephonyBaseTest):

    _TEAR_DOWN_OPERATION_DISCONNECT_WIFI = "disconnect_wifi"
    _TEAR_DOWN_OPERATION_RESET_WIFI = "reset_wifi"
    _TEAR_DOWN_OPERATION_DISABLE_WFC = "disable_wfc"

    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)

        self.ad = self.android_devices[0]
        self.wifi_network_ssid = self.user_params["wifi_network_ssid"]
        try:
            self.wifi_network_pass = self.user_params["wifi_network_pass"]
        except KeyError:
            self.wifi_network_pass = None
        self.number_of_devices = 1
        self.stress_test_number = self.get_stress_test_number()

    def _wifi_connected_enable_wfc_teardown_wfc(
            self,
            tear_down_operation,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED,
            check_volte_after_wfc_disabled=False):
        if initial_setup_wifi and not ensure_wifi_connected(
                self.log, self.ad, self.wifi_network_ssid,
                self.wifi_network_pass):
            self.log.error("Failed to connect WiFi")
            return False
        if initial_setup_wfc_mode and not set_wfc_mode(self.log, self.ad,
                                                       initial_setup_wfc_mode):
            self.log.error("Failed to set WFC mode.")
            return False
        if not phone_idle_iwlan(self.log, self.ad):
            self.log.error("WFC is not available.")
            return False

        # Tear Down WFC based on tear_down_operation
        if tear_down_operation == self._TEAR_DOWN_OPERATION_DISCONNECT_WIFI:
            if not wifi_toggle_state(self.log, self.ad, False):
                self.ad.log.error("Failed to turn off WiFi.")
                return False
        elif tear_down_operation == self._TEAR_DOWN_OPERATION_RESET_WIFI:
            if not wifi_reset(self.log, self.ad, False):
                self.ad.log.error("Failed to reset WiFi")
                return False
        elif tear_down_operation == self._TEAR_DOWN_OPERATION_DISABLE_WFC:
            if not set_wfc_mode(self.log, self.ad, WFC_MODE_DISABLED):
                self.ad.log.error("Failed to turn off WFC.")
                return False
        else:
            self.log.info("No tear down operation")
            return True

        if not wait_for_wfc_disabled(self.log, self.ad):
            self.log.error(
                "WFC is still available after turn off WFC or WiFi.")
            return False

        #For SMS over Wifi, data will be in IWLAN with WFC off
        if tear_down_operation != self._TEAR_DOWN_OPERATION_DISABLE_WFC and (
                not wait_for_not_network_rat(
                    self.log,
                    self.ad,
                    RAT_FAMILY_WLAN,
                    voice_or_data=NETWORK_SERVICE_DATA)):
            self.log.error("Data Rat is still iwlan.")
            return False

        # If VoLTE was previous available, after tear down WFC, DUT should have
        # VoLTE service.
        if check_volte_after_wfc_disabled and not wait_for_volte_enabled(
                self.log, self.ad, MAX_WAIT_TIME_VOLTE_ENABLED):
            self.log.error("Device failed to acquire VoLTE service")
            return False
        return True

    def _wifi_connected_set_wfc_mode_change_wfc_mode(
            self,
            initial_wfc_mode,
            new_wfc_mode,
            is_wfc_available_in_initial_wfc_mode,
            is_wfc_available_in_new_wfc_mode,
            initial_setup_wifi=True,
            check_volte_after_wfc_disabled=False):
        if initial_setup_wifi and not ensure_wifi_connected(
                self.log, self.ad, self.wifi_network_ssid,
                self.wifi_network_pass):
            self.log.error("Failed to connect WiFi")
            return False
        # Set to initial_wfc_mode first, then change to new_wfc_mode
        for (wfc_mode, is_wfc_available) in \
            [(initial_wfc_mode, is_wfc_available_in_initial_wfc_mode),
             (new_wfc_mode, is_wfc_available_in_new_wfc_mode)]:
            current_wfc_status = is_wfc_enabled(self.log, self.ad)
            self.log.info("Current WFC: {}, Set WFC to {}".format(
                current_wfc_status, wfc_mode))
            if not set_wfc_mode(self.log, self.ad, wfc_mode):
                self.log.error("Failed to set WFC mode.")
                return False
            if is_wfc_available:
                if current_wfc_status:
                    # Previous is True, after set it still need to be true
                    # wait and check if DUT WFC got disabled.
                    if wait_for_wfc_disabled(self.log, self.ad):
                        self.log.error("WFC is not available.")
                        return False
                else:
                    # Previous is False, after set it will be true,
                    # wait and check if DUT WFC got enabled.
                    if not wait_for_wfc_enabled(self.log, self.ad):
                        self.log.error("WFC is not available.")
                        return False
            else:
                if current_wfc_status:
                    # Previous is True, after set it will be false,
                    # wait and check if DUT WFC got disabled.
                    if not wait_for_wfc_disabled(self.log, self.ad):
                        self.log.error("WFC is available.")
                        return False
                else:
                    # Previous is False, after set it still need to be false
                    # Wait and check if DUT WFC got enabled.
                    if wait_for_wfc_enabled(self.log, self.ad):
                        self.log.error("WFC is available.")
                        return False
                if check_volte_after_wfc_disabled and not wait_for_volte_enabled(
                        self.log, self.ad, MAX_WAIT_TIME_VOLTE_ENABLED):
                    self.log.error("Device failed to acquire VoLTE service")
                    return False
        return True

    def _wifi_connected_set_wfc_mode_turn_off_apm(
            self, wfc_mode, is_wfc_available_after_turn_off_apm):
        if not ensure_wifi_connected(self.log, self.ad, self.wifi_network_ssid,
                                     self.wifi_network_pass):
            self.log.error("Failed to connect WiFi")
            return False
        if not set_wfc_mode(self.log, self.ad, wfc_mode):
            self.log.error("Failed to set WFC mode.")
            return False
        if not phone_idle_iwlan(self.log, self.ad):
            self.log.error("WFC is not available.")
            return False
        if not toggle_airplane_mode_by_adb(self.log, self.ad, False):
            self.log.error("Failed to turn off airplane mode")
            return False
        is_wfc_not_available = wait_for_wfc_disabled(self.log, self.ad)
        if is_wfc_available_after_turn_off_apm and is_wfc_not_available:
            self.log.error("WFC is not available.")
            return False
        elif (not is_wfc_available_after_turn_off_apm
              and not is_wfc_not_available):
            self.log.error("WFC is available.")
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a3a680ba-d1e0-4770-a38c-4de8f15f9171")
    def test_lte_volte_wifi_connected_toggle_wfc(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WiFi Connected, Toggling WFC

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi connected, WFC disabled.
        3. Set DUT WFC enabled (WiFi Preferred), verify DUT WFC available,
            report iwlan rat.
        4. Set DUT WFC disabled, verify DUT WFC unavailable,
            not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISABLE_WFC,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED,
            check_volte_after_wfc_disabled=True)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d3ffae75-ae4a-4ed8-9337-9155c413311d")
    def test_lte_wifi_connected_toggle_wfc(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Disabled + WiFi Connected, Toggling WFC

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE disabled.
        2. Make sure DUT WiFi connected, WFC disabled.
        3. Set DUT WFC enabled (WiFi Preferred), verify DUT WFC available,
            report iwlan rat.
        4. Set DUT WFC disabled, verify DUT WFC unavailable,
            not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not phone_setup_csfb(self.log, self.ad):
            self.log.error("Failed to setup LTE")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISABLE_WFC,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="29d2d7b7-1c31-4a2c-896a-3f6756c620ac")
    def test_3g_wifi_connected_toggle_wfc(self):
        """Test for WiFi Calling settings:
        3G + WiFi Connected, Toggling WFC

        Steps:
        1. Setup DUT Idle, 3G network type.
        2. Make sure DUT WiFi connected, WFC disabled.
        3. Set DUT WFC enabled (WiFi Preferred), verify DUT WFC available,
            report iwlan rat.
        4. Set DUT WFC disabled, verify DUT WFC unavailable,
            not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """
        set_wifi_to_default(self.log, self.ad)
        if not phone_setup_voice_3g(self.log, self.ad):
            self.log.error("Failed to setup 3G")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISABLE_WFC,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ce2c0208-9ea0-4b31-91f4-d06a62cb927a")
    def test_apm_wifi_connected_toggle_wfc(self):
        """Test for WiFi Calling settings:
        APM + WiFi Connected, Toggling WFC

        Steps:
        1. Setup DUT Idle, Airplane mode.
        2. Make sure DUT WiFi connected, WFC disabled.
        3. Set DUT WFC enabled (WiFi Preferred), verify DUT WFC available,
            report iwlan rat.
        4. Set DUT WFC disabled, verify DUT WFC unavailable,
            not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not toggle_airplane_mode_by_adb(self.log, self.ad, True):
            self.log.error("Failed to turn on airplane mode")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISABLE_WFC,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="681e2448-32a2-434d-abd6-0bc2ab5afd9c")
    def test_lte_volte_wfc_enabled_toggle_wifi(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WFC enabled, Toggling WiFi

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi disconnected, WFC enabled (WiFi Preferred).
        3. DUT connect WiFi, verify DUT WFC available, report iwlan rat.
        4. DUT disconnect WiFi,verify DUT WFC unavailable, not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISCONNECT_WIFI,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED,
            check_volte_after_wfc_disabled=True)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="63922066-9caa-42e6-bc9f-49f5ac01cbe2")
    def test_lte_wfc_enabled_toggle_wifi(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Disabled + WFC enabled, Toggling WiFi

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE disabled.
        2. Make sure DUT WiFi disconnected, WFC enabled (WiFi Preferred).
        3. DUT connect WiFi, verify DUT WFC available, report iwlan rat.
        4. DUT disconnect WiFi,verify DUT WFC unavailable, not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not phone_setup_csfb(self.log, self.ad):
            self.log.error("Failed to setup LTE")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISCONNECT_WIFI,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="8a80a446-2116-4b19-b0ef-f771f30a6d15")
    def test_3g_wfc_enabled_toggle_wifi(self):
        """Test for WiFi Calling settings:
        3G + WFC enabled, Toggling WiFi

        Steps:
        1. Setup DUT Idle, 3G network type.
        2. Make sure DUT WiFi disconnected, WFC enabled (WiFi Preferred).
        3. DUT connect WiFi, verify DUT WFC available, report iwlan rat.
        4. DUT disconnect WiFi,verify DUT WFC unavailable, not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """
        set_wifi_to_default(self.log, self.ad)
        if not phone_setup_voice_3g(self.log, self.ad):
            self.log.error("Failed to setup 3G")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISCONNECT_WIFI,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="9889eebf-cde6-4f47-aec0-9cb204fdf2e5")
    def test_apm_wfc_enabled_toggle_wifi(self):
        """Test for WiFi Calling settings:
        APM + WFC enabled, Toggling WiFi

        Steps:
        1. Setup DUT Idle, Airplane mode.
        2. Make sure DUT WiFi disconnected, WFC enabled (WiFi Preferred).
        3. DUT connect WiFi, verify DUT WFC available, report iwlan rat.
        4. DUT disconnect WiFi,verify DUT WFC unavailable, not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not toggle_airplane_mode_by_adb(self.log, self.ad, True):
            self.log.error("Failed to turn on airplane mode")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISCONNECT_WIFI,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="9b23e04b-4f70-4e73-88e7-6376262c739d")
    def test_lte_wfc_enabled_wifi_connected_toggle_volte(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WiFi Connected + WFC enabled, toggle VoLTE setting

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi connected, WFC enabled (WiFi Preferred).
            Verify DUT WFC available, report iwlan rat.
        3. Disable VoLTE on DUT, verify in 2 minutes period,
            DUT does not lost WiFi Calling, DUT still report WFC available,
            rat iwlan.
        4. Enable VoLTE on DUT, verify in 2 minutes period,
            DUT does not lost WiFi Calling, DUT still report WFC available,
            rat iwlan.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return True, network rat is iwlan.
        """
        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE.")
            return False
        if not phone_setup_iwlan(
                self.log, self.ad, False, WFC_MODE_WIFI_PREFERRED,
                self.wifi_network_ssid, self.wifi_network_pass):
            self.log.error("Failed to setup WFC.")
            return False
        # Turn Off VoLTE, then Turn On VoLTE
        for i in range(2):
            if not toggle_volte(self.log, self.ad):
                self.log.error("Failed to toggle VoLTE.")
                return False
            if wait_for_wfc_disabled(self.log, self.ad):
                self.log.error("WFC is not available.")
                return False
            if not is_droid_in_rat_family(self.log, self.ad, RAT_FAMILY_WLAN,
                                          NETWORK_SERVICE_DATA):
                self.log.error("Data Rat is not iwlan.")
                return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="04bdfda4-06f7-41df-9352-a8534bc2a67a")
    def test_lte_volte_wfc_wifi_preferred_to_cellular_preferred(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WiFi Connected + WiFi Preferred,
        change WFC to Cellular Preferred

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi connected, WFC is set to WiFi Preferred.
            Verify DUT WFC available, report iwlan rat.
        3. Change WFC setting to Cellular Preferred.
        4. Verify DUT report WFC not available.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFI Calling feature bit return False, network rat is not iwlan.
        """
        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE.")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_WIFI_PREFERRED,
            WFC_MODE_CELLULAR_PREFERRED,
            True,
            False,
            check_volte_after_wfc_disabled=True)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="80d26bdb-992a-4b30-ad51-68308d5af168")
    def test_lte_wfc_wifi_preferred_to_cellular_preferred(self):
        """Test for WiFi Calling settings:
        LTE + WiFi Connected + WiFi Preferred, change WFC to Cellular Preferred

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE disabled.
        2. Make sure DUT WiFi connected, WFC is set to WiFi Preferred.
            Verify DUT WFC available, report iwlan rat.
        3. Change WFC setting to Cellular Preferred.
        4. Verify DUT report WFC not available.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFI Calling feature bit return False, network rat is not iwlan.
        """
        if not phone_setup_csfb(self.log, self.ad):
            self.log.error("Failed to setup LTE.")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_WIFI_PREFERRED,
            WFC_MODE_CELLULAR_PREFERRED,
            True,
            False,
            check_volte_after_wfc_disabled=False)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d486c7e3-3d2b-4552-8af8-7b19f6347427")
    def test_3g_wfc_wifi_preferred_to_cellular_preferred(self):
        """Test for WiFi Calling settings:
        3G + WiFi Connected + WiFi Preferred, change WFC to Cellular Preferred

        Steps:
        1. Setup DUT Idle, 3G network type.
        2. Make sure DUT WiFi connected, WFC is set to WiFi Preferred.
            Verify DUT WFC available, report iwlan rat.
        3. Change WFC setting to Cellular Preferred.
        4. Verify DUT report WFC not available.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFI Calling feature bit return False, network rat is not iwlan.
        """
        set_wifi_to_default(self.log, self.ad)
        if not phone_setup_voice_3g(self.log, self.ad):
            self.log.error("Failed to setup 3G.")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_WIFI_PREFERRED, WFC_MODE_CELLULAR_PREFERRED, True, False)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="0feb0add-8e22-4c86-b13e-be68659cdd87")
    def test_apm_wfc_wifi_preferred_to_cellular_preferred(self):
        """Test for WiFi Calling settings:
        APM + WiFi Connected + WiFi Preferred, change WFC to Cellular Preferred

        Steps:
        1. Setup DUT Idle, airplane mode.
        2. Make sure DUT WiFi connected, WFC is set to WiFi Preferred.
            Verify DUT WFC available, report iwlan rat.
        3. Change WFC setting to Cellular Preferred.
        4. Verify DUT report WFC not available.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFI Calling feature bit return True, network rat is iwlan.
        """
        if not toggle_airplane_mode_by_adb(self.log, self.ad, True):
            self.log.error("Failed to turn on airplane mode")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_WIFI_PREFERRED, WFC_MODE_CELLULAR_PREFERRED, True, True)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="9c8f359f-a084-4413-b8a9-34771af166c5")
    def test_lte_volte_wfc_cellular_preferred_to_wifi_preferred(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WiFi Connected + Cellular Preferred,
        change WFC to WiFi Preferred

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi connected, WFC is set to Cellular Preferred.
            Verify DUT WFC not available.
        3. Change WFC setting to WiFi Preferred.
        4. Verify DUT report WFC available.

        Expected Results:
        2. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        4. DUT WiFI Calling feature bit return True, network rat is iwlan.
        """
        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE.")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_CELLULAR_PREFERRED,
            WFC_MODE_WIFI_PREFERRED,
            False,
            True,
            check_volte_after_wfc_disabled=True)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="1894e685-63cf-43aa-91ed-938782ca35a9")
    def test_lte_wfc_cellular_preferred_to_wifi_preferred(self):
        """Test for WiFi Calling settings:
        LTE + WiFi Connected + Cellular Preferred, change WFC to WiFi Preferred

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE disabled.
        2. Make sure DUT WiFi connected, WFC is set to Cellular Preferred.
            Verify DUT WFC not available.
        3. Change WFC setting to WiFi Preferred.
        4. Verify DUT report WFC available.

        Expected Results:
        2. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        4. DUT WiFI Calling feature bit return True, network rat is iwlan.
        """
        if not phone_setup_csfb(self.log, self.ad):
            self.log.error("Failed to setup LTE.")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_CELLULAR_PREFERRED,
            WFC_MODE_WIFI_PREFERRED,
            False,
            True,
            check_volte_after_wfc_disabled=False)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e7fb6a6c-4672-44da-bca2-78b4d96dea9e")
    def test_3g_wfc_cellular_preferred_to_wifi_preferred(self):
        """Test for WiFi Calling settings:
        3G + WiFi Connected + Cellular Preferred, change WFC to WiFi Preferred

        Steps:
        1. Setup DUT Idle, 3G network type.
        2. Make sure DUT WiFi connected, WFC is set to Cellular Preferred.
            Verify DUT WFC not available.
        3. Change WFC setting to WiFi Preferred.
        4. Verify DUT report WFC available.

        Expected Results:
        2. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        4. DUT WiFI Calling feature bit return True, network rat is iwlan.
        """
        set_wifi_to_default(self.log, self.ad)
        if not phone_setup_voice_3g(self.log, self.ad):
            self.log.error("Failed to setup 3G.")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_CELLULAR_PREFERRED, WFC_MODE_WIFI_PREFERRED, False, True)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="46262b2d-5de9-4984-87e8-42f44469289e")
    def test_apm_wfc_cellular_preferred_to_wifi_preferred(self):
        """Test for WiFi Calling settings:
        APM + WiFi Connected + Cellular Preferred, change WFC to WiFi Preferred

        Steps:
        1. Setup DUT Idle, airplane mode.
        2. Make sure DUT WiFi connected, WFC is set to Cellular Preferred.
            Verify DUT WFC not available.
        3. Change WFC setting to WiFi Preferred.
        4. Verify DUT report WFC available.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFI Calling feature bit return True, network rat is iwlan.
        """
        if not toggle_airplane_mode_by_adb(self.log, self.ad, True):
            self.log.error("Failed to turn on airplane mode")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_CELLULAR_PREFERRED, WFC_MODE_WIFI_PREFERRED, True, True)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="5b514f51-fed9-475e-99d3-17d2165e11a1")
    def test_apm_wfc_wifi_preferred_turn_off_apm(self):
        """Test for WiFi Calling settings:
        APM + WiFi Connected + WiFi Preferred + turn off APM

        Steps:
        1. Setup DUT Idle in Airplane mode.
        2. Make sure DUT WiFi connected, set WFC mode to WiFi preferred.
        3. verify DUT WFC available, report iwlan rat.
        4. Turn off airplane mode.
        5. Verify DUT WFC still available, report iwlan rat

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        5. DUT WiFI Calling feature bit return True, network rat is iwlan.
        """
        if not toggle_airplane_mode_by_adb(self.log, self.ad, True):
            self.log.error("Failed to turn on airplane mode")
            return False
        return self._wifi_connected_set_wfc_mode_turn_off_apm(
            WFC_MODE_WIFI_PREFERRED, True)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f328cff2-9dec-44b3-ba74-a662b76fcf2a")
    def test_apm_wfc_cellular_preferred_turn_off_apm(self):
        """Test for WiFi Calling settings:
        APM + WiFi Connected + Cellular Preferred + turn off APM

        Steps:
        1. Setup DUT Idle in Airplane mode.
        2. Make sure DUT WiFi connected, set WFC mode to Cellular preferred.
        3. verify DUT WFC available, report iwlan rat.
        4. Turn off airplane mode.
        5. Verify DUT WFC not available, not report iwlan rat

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        5. DUT WiFI Calling feature bit return False, network rat is not iwlan.
        """
        if not toggle_airplane_mode_by_adb(self.log, self.ad, True):
            self.log.error("Failed to turn on airplane mode")
            return False
        return self._wifi_connected_set_wfc_mode_turn_off_apm(
            WFC_MODE_CELLULAR_PREFERRED, False)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="7e30d219-42ee-4309-a95c-2b45b8831d26")
    def test_wfc_setup_timing(self):
        """ Measures the time delay in enabling WiFi calling

        Steps:
        1. Make sure DUT idle.
        2. Turn on Airplane Mode, Set WiFi Calling to WiFi_Preferred.
        3. Turn on WiFi, connect to WiFi AP and measure time delay.
        4. Wait for WiFi connected, verify Internet and measure time delay.
        5. Wait for rat to be reported as iwlan and measure time delay.
        6. Wait for ims registered and measure time delay.
        7. Wait for WiFi Calling feature bit to be True and measure time delay.

        Expected results:
        Time Delay in each step should be within pre-defined limit.

        Returns:
            Currently always return True.
        """
        # TODO: b/26338119 Set pass/fail criteria
        ad = self.android_devices[0]

        time_values = {
            'start': 0,
            'wifi_enabled': 0,
            'wifi_connected': 0,
            'wifi_data': 0,
            'iwlan_rat': 0,
            'ims_registered': 0,
            'wfc_enabled': 0,
            'mo_call_success': 0
        }

        wifi_reset(self.log, ad)
        toggle_airplane_mode_by_adb(self.log, ad, True)

        set_wfc_mode(self.log, ad, WFC_MODE_WIFI_PREFERRED)

        time_values['start'] = time.time()

        self.log.info("Start Time {}s".format(time_values['start']))

        wifi_toggle_state(self.log, ad, True)
        time_values['wifi_enabled'] = time.time()
        self.log.info("WiFi Enabled After {}s".format(
            time_values['wifi_enabled'] - time_values['start']))

        ensure_wifi_connected(self.log, ad, self.wifi_network_ssid,
                              self.wifi_network_pass)
        ad.droid.wakeUpNow()

        if not wait_for_wifi_data_connection(self.log, ad, True,
                                             MAX_WAIT_TIME_WIFI_CONNECTION):
            self.log.error("Failed WiFi connection, aborting!")
            return False
        time_values['wifi_connected'] = time.time()

        self.log.info("WiFi Connected After {}s".format(
            time_values['wifi_connected'] - time_values['wifi_enabled']))

        if not verify_http_connection(self.log, ad, 'http://www.google.com',
                                      100, .1):
            self.log.error("Failed to get user-plane traffic, aborting!")
            return False

        time_values['wifi_data'] = time.time()
        self.log.info("WifiData After {}s".format(
            time_values['wifi_data'] - time_values['wifi_connected']))

        if not wait_for_network_rat(
                self.log, ad, RAT_FAMILY_WLAN,
                voice_or_data=NETWORK_SERVICE_DATA):
            self.log.error("Failed to set-up iwlan, aborting!")
            if is_droid_in_rat_family(self.log, ad, RAT_FAMILY_WLAN,
                                      NETWORK_SERVICE_DATA):
                self.log.error("Never received the event, but droid in iwlan")
            else:
                return False
        time_values['iwlan_rat'] = time.time()
        self.log.info("iWLAN Reported After {}s".format(
            time_values['iwlan_rat'] - time_values['wifi_data']))

        if not wait_for_ims_registered(self.log, ad,
                                       MAX_WAIT_TIME_IMS_REGISTRATION):
            self.log.error("Never received IMS registered, aborting")
            return False
        time_values['ims_registered'] = time.time()
        self.log.info("Ims Registered After {}s".format(
            time_values['ims_registered'] - time_values['iwlan_rat']))

        if not wait_for_wfc_enabled(self.log, ad, MAX_WAIT_TIME_WFC_ENABLED):
            self.log.error("Never received WFC feature, aborting")
            return False

        time_values['wfc_enabled'] = time.time()
        self.log.info("Wifi Calling Feature Enabled After {}s".format(
            time_values['wfc_enabled'] - time_values['ims_registered']))

        set_wfc_mode(self.log, ad, WFC_MODE_DISABLED)

        wait_for_not_network_rat(
            self.log, ad, RAT_FAMILY_WLAN, voice_or_data=NETWORK_SERVICE_DATA)

        self.log.info("\n\n------------------summary-----------------")
        self.log.info("WiFi Enabled After {0:.2f} s".format(
            time_values['wifi_enabled'] - time_values['start']))
        self.log.info("WiFi Connected After {0:.2f} s".format(
            time_values['wifi_connected'] - time_values['wifi_enabled']))
        self.log.info("WifiData After {0:.2f} s".format(
            time_values['wifi_data'] - time_values['wifi_connected']))
        self.log.info("iWLAN Reported After {0:.2f} s".format(
            time_values['iwlan_rat'] - time_values['wifi_data']))
        self.log.info("Ims Registered After {0:.2f} s".format(
            time_values['ims_registered'] - time_values['iwlan_rat']))
        self.log.info("Wifi Calling Feature Enabled After {0:.2f} s".format(
            time_values['wfc_enabled'] - time_values['ims_registered']))
        self.log.info("\n\n")
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="4e0bf35f-b4e1-44f8-b657-e9c71878d1f6")
    def test_lte_volte_wfc_enabled_toggle_wifi_stress(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WFC enabled, Toggling WiFi Stress test

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi disconnected, WFC enabled (WiFi Preferred).
        3. DUT connect WiFi, verify DUT WFC available, report iwlan rat.
        4. DUT disconnect WiFi, verify DUT WFC unavailable, not report iwlan rat.
        5. Verify DUT report VoLTE available.
        6. Repeat steps 3~5 for N times.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        5. DUT report VoLTE available.
        """

        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE")
            return False
        set_wfc_mode(self.log, self.ad, WFC_MODE_WIFI_PREFERRED)

        for i in range(1, self.stress_test_number + 1):
            self.log.info("Start Iteration {}.".format(i))
            result = self._wifi_connected_enable_wfc_teardown_wfc(
                tear_down_operation=self._TEAR_DOWN_OPERATION_DISCONNECT_WIFI,
                initial_setup_wifi=True,
                initial_setup_wfc_mode=None,
                check_volte_after_wfc_disabled=True)
            if not result:
                self.log.error("Test Failed in iteration: {}.".format(i))
                return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c6ef1dfd-29d4-4fc8-9fc0-27e35bb377fb")
    def test_lte_volte_wfc_enabled_reset_wifi_stress(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WFC enabled, Reset WiFi Stress test

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi disconnected, WFC enabled (WiFi Preferred).
        3. DUT connect WiFi, verify DUT WFC available, report iwlan rat.
        4. DUT Reset WiFi, verify DUT WFC unavailable, not report iwlan rat.
        5. Verify DUT report VoLTE available.
        6. Repeat steps 3~5 for N times.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        5. DUT report VoLTE available.
        """

        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE")
            return False
        set_wfc_mode(self.log, self.ad, WFC_MODE_WIFI_PREFERRED)

        for i in range(1, self.stress_test_number + 1):
            self.log.info("Start Iteration {}.".format(i))
            result = self._wifi_connected_enable_wfc_teardown_wfc(
                tear_down_operation=self._TEAR_DOWN_OPERATION_RESET_WIFI,
                initial_setup_wifi=True,
                initial_setup_wfc_mode=None,
                check_volte_after_wfc_disabled=True)
            if not result:
                self.log.error("Test Failed in iteration: {}.".format(i))
                return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a7d2b9fc-d676-4ada-84b9-08e99659da78")
    def test_lte_volte_wfc_wifi_preferred_to_cellular_preferred_stress(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WiFi Connected + WiFi Preferred,
        change WFC to Cellular Preferred stress

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi connected, WFC is set to WiFi Preferred.
            Verify DUT WFC available, report iwlan rat.
        3. Change WFC setting to Cellular Preferred.
        4. Verify DUT report WFC not available.
        5. Verify DUT report VoLTE available.
        6. Repeat steps 3~5 for N times.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFI Calling feature bit return False, network rat is not iwlan.
        5. DUT report VoLTE available.
        """
        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE.")
            return False
        if not ensure_wifi_connected(self.log, self.ad, self.wifi_network_ssid,
                                     self.wifi_network_pass):
            self.log.error("Failed to connect WiFi")
            return False

        for i in range(1, self.stress_test_number + 1):
            self.log.info("Start Iteration {}.".format(i))
            result = self._wifi_connected_set_wfc_mode_change_wfc_mode(
                WFC_MODE_WIFI_PREFERRED,
                WFC_MODE_CELLULAR_PREFERRED,
                True,
                False,
                initial_setup_wifi=False,
                check_volte_after_wfc_disabled=True)
            if not result:
                self.log.error("Test Failed in iteration: {}.".format(i))
                return False
        return True

    def verify_volte_on_wfc_off(self):
        result = True
        if self.ad.droid.imsGetWfcMode() != WFC_MODE_DISABLED:
            self.ad.log.error(
                "WFC mode is not disabled after IMS factory reset")
            result = False
        else:
            self.ad.log.info("WFC mode is disabled as expected")
        if not self.ad.droid.imsIsEnhanced4gLteModeSettingEnabledByUser():
            self.ad.log.error("VoLTE mode is not on")
            result = False
        else:
            self.ad.log.info("VoLTE mode is turned on as expected")
        if not phone_idle_volte(self.log, self.ad):
            self.ad.log.error("Voice RAT is not in LTE")
            result = False
        if not call_setup_teardown(self.log, self.ad, self.android_devices[1],
                                   self.ad, is_phone_in_call_volte):
            self.ad.log.error("Voice call in VoLTE failed")
            result = False
        return result

    def verify_volte_off_wfc_off(self):
        result = True
        if self.ad.droid.imsGetWfcMode() != WFC_MODE_DISABLED:
            self.ad.log.error(
                "WFC mode is not disabled after IMS factory reset")
            result = False
        else:
            self.ad.log.info("WFC mode is disabled as expected")
        if self.ad.droid.imsIsEnhanced4gLteModeSettingEnabledByUser():
            self.ad.log.error("VoLTE mode is on")
            result = False
        else:
            self.ad.log.info("VoLTE mode is turned off as expected")
        if not call_setup_teardown(self.log, self.ad, self.android_devices[1],
                                   self.ad, None):
            self.ad.log.error("Voice call failed")
            result = False
        return result

    def revert_default_telephony_setting(self):
        toggle_airplane_mode_by_adb(self.log, self.ad, True)
        default_data_roaming = int(
            self.ad.adb.getprop("ro.com.android.dataroaming") == 'true')
        default_network_preference = int(
            self.ad.adb.getprop("ro.telephony.default_network"))
        self.ad.log.info("Default data roaming %s, network preference %s",
                         default_data_roaming, default_network_preference)
        new_data_roaming = abs(default_data_roaming - 1)
        new_network_preference = abs(default_network_preference - 1)
        self.ad.log.info(
            "Set data roaming = %s, mobile data = 0, network preference = %s",
            new_data_roaming, new_network_preference)
        self.ad.adb.shell("settings put global mobile_data 0")
        self.ad.adb.shell(
            "settings put global data_roaming %s" % new_data_roaming)
        self.ad.adb.shell("settings put global preferred_network_mode %s" %
                          new_network_preference)

    def verify_default_telephony_setting(self):
        default_data_roaming = int(
            self.ad.adb.getprop("ro.com.android.dataroaming") == 'true')
        default_network_preference = int(
            self.ad.adb.getprop("ro.telephony.default_network"))
        self.ad.log.info("Default data roaming %s, network preference %s",
                         default_data_roaming, default_network_preference)
        data_roaming = int(
            self.ad.adb.shell("settings get global data_roaming"))
        mobile_data = int(self.ad.adb.shell("settings get global mobile_data"))
        network_preference = int(
            self.ad.adb.shell("settings get global preferred_network_mode"))
        airplane_mode = int(
            self.ad.adb.shell("settings get global airplane_mode_on"))
        result = True
        self.ad.log.info("data_roaming = %s, mobile_data = %s, "
                         "network_perference = %s, airplane_mode = %s",
                         data_roaming, mobile_data, network_preference,
                         airplane_mode)
        if airplane_mode:
            self.ad.log.error("Airplane mode is on")
            result = False
        if data_roaming != default_data_roaming:
            self.ad.log.error("Data roaming is %s, expecting %s", data_roaming,
                              default_data_roaming)
            result = False
        if not mobile_data:
            self.ad.log.error("Mobile data is off")
            result = False
        if network_preference != default_network_preference:
            self.ad.log.error("preferred_network_mode is %s, expecting %s",
                              network_preference, default_network_preference)
            result = False
        return result

    @test_tracker_info(uuid="135301ea-6d00-4233-98fd-cda706d61eb2")
    @TelephonyBaseTest.tel_test_wrap
    def test_ims_factory_reset_to_volte_on_wfc_off(self):
        """Test VOLTE is enabled WFC is disabled after ims factory reset.

        Steps:
        1. Setup VoLTE, WFC, APM is various mode.
        2. Call IMS factory reset.
        3. Verify VoLTE is on, WFC is off after IMS factory reset.
        4. Verify VoLTE Voice call can be made successful.

        Expected Results: VoLTE is on, WFC is off after IMS factory reset.
        """
        result = True
        for airplane_mode in (True, False):
            for volte_mode in (True, False):
                for wfc_mode in (WFC_MODE_DISABLED,
                                 WFC_MODE_CELLULAR_PREFERRED,
                                 WFC_MODE_WIFI_PREFERRED):
                    self.ad.log.info("Set VoLTE %s, WFC %s, APM %s",
                                     volte_mode, wfc_mode, airplane_mode)
                    toggle_airplane_mode_by_adb(self.log, self.ad,
                                                airplane_mode)
                    toggle_volte(self.log, self.ad, volte_mode)
                    set_wfc_mode(self.log, self.ad, wfc_mode)
                    self.ad.log.info("Call IMS factory reset")
                    self.ad.droid.imsFactoryReset()
                    self.ad.log.info("Ensure airplane mode is off")
                    toggle_airplane_mode_by_adb(self.log, self.ad, False)
                    if not self.verify_volte_on_wfc_off(): result = False
        return result

    @test_tracker_info(uuid="5318bf7a-4210-4b49-b361-9539d28f3e38")
    @TelephonyBaseTest.tel_test_wrap
    def test_ims_factory_reset_to_volte_off_wfc_off(self):
        """Test VOLTE is enabled WFC is disabled after ims factory reset.

        Steps:
        1. Setup VoLTE, WFC, APM is various mode.
        2. Call IMS factory reset.
        3. Verify VoLTE is on, WFC is off after IMS factory reset.
        4. Verify VoLTE Voice call can be made successful.

        Expected Results: VoLTE is on, WFC is off after IMS factory reset.
        """
        result = True
        for airplane_mode in (True, False):
            for volte_mode in (True, False):
                for wfc_mode in (WFC_MODE_DISABLED,
                                 WFC_MODE_CELLULAR_PREFERRED,
                                 WFC_MODE_WIFI_PREFERRED):
                    self.ad.log.info("Set VoLTE %s, WFC %s, APM %s",
                                     volte_mode, wfc_mode, airplane_mode)
                    toggle_airplane_mode_by_adb(self.log, self.ad,
                                                airplane_mode)
                    toggle_volte(self.log, self.ad, volte_mode)
                    set_wfc_mode(self.log, self.ad, wfc_mode)
                    self.ad.log.info("Call IMS factory reset")
                    self.ad.droid.imsFactoryReset()
                    self.ad.log.info("Ensure airplane mode is off")
                    toggle_airplane_mode_by_adb(self.log, self.ad, False)
                    if not self.verify_volte_off_wfc_off(): result = False
        return result

    @test_tracker_info(uuid="c6149bd6-7080-453d-af37-1f9bd350a764")
    @TelephonyBaseTest.tel_test_wrap
    def test_telephony_factory_reset(self):
        """Test VOLTE is enabled WFC is disabled after telephony factory reset.

        Steps:
        1. Setup DUT with various dataroaming, mobiledata, and default_network.
        2. Call telephony factory reset.
        3. Verify DUT back to factory default.

        Expected Results: dataroaming is off, mobiledata is on, network
                          preference is back to default.
        """
        self.ad.log.info("Call telephony factory reset")
        self.revert_default_telephony_setting()
        self.ad.droid.telephonyFactoryReset()
        return self.verify_default_telephony_setting()

    @test_tracker_info(uuid="ce60740f-4d8e-4013-a7cf-65589e8a0893")
    @TelephonyBaseTest.tel_test_wrap
    def test_factory_reset_by_wipe_to_volte_on_wfc_off(self):
        """Verify the network setting after factory reset by wipe.

        Steps:
        1. Config VoLTE off, WFC on, APM on, data_roaming on, mobile_data on
           preferred_network_mode.
        2. Factory reset by .
        3. Verify VoLTE is on, WFC is off after IMS factory reset.
        4. Verify VoLTE Voice call can be made successful.

        Expected Results: VoLTE is on, WFC is off after IMS factory reset.
        """
        self.ad.log.info("Set VoLTE off, WFC wifi preferred, APM on")
        toggle_airplane_mode_by_adb(self.log, self.ad, True)
        toggle_volte(self.log, self.ad, False)
        set_wfc_mode(self.log, self.ad, WFC_MODE_WIFI_PREFERRED)
        self.revert_default_telephony_setting()
        self.ad.log.info("Wipe in fastboot")
        fastboot_wipe(self.ad)
        result = self.verify_volte_on_wfc_off()
        if not self.verify_default_telephony_setting(): result = False
        return result

    @test_tracker_info(uuid="44e9291e-949b-4db1-a209-c6d41552ec27")
    @TelephonyBaseTest.tel_test_wrap
    def test_factory_reset_by_wipe_to_volte_off_wfc_off(self):
        """Verify the network setting after factory reset by wipe.

        Steps:
        1. Config VoLTE on, WFC on, APM on, data_roaming on, mobile_data on
           preferred_network_mode.
        2. Factory reset by .
        3. Verify VoLTE is on, WFC is off after IMS factory reset.
        4. Verify VoLTE Voice call can be made successful.

        Expected Results: VoLTE is on, WFC is off after IMS factory reset.
        """
        self.ad.log.info("Set VoLTE on, WFC wifi preferred, APM on")
        toggle_airplane_mode_by_adb(self.log, self.ad, True)
        toggle_volte(self.log, self.ad, True)
        set_wfc_mode(self.log, self.ad, WFC_MODE_WIFI_PREFERRED)
        self.revert_default_telephony_setting()
        self.ad.log.info("Wipe in fastboot")
        fastboot_wipe(self.ad)
        result = self.verify_volte_off_wfc_off()
        if not self.verify_default_telephony_setting(): result = False
        return result

    @test_tracker_info(uuid="64deba57-c1c2-422f-b771-639c95edfbc0")
    @TelephonyBaseTest.tel_test_wrap
    def test_disable_mobile_data_always_on(self):
        """Verify mobile_data_always_on can be disabled.

        Steps:
        1. Disable mobile_data_always_on by adb.
        2. Verify the mobile data_always_on state.

        Expected Results: mobile_data_always_on return 0.
        """
        self.ad.log.info("Disable mobile_data_always_on")
        set_mobile_data_always_on(self.ad, False)
        time.sleep(1)
        return self.ad.adb.shell(
            "settings get global mobile_data_always_on") == "0"

    @test_tracker_info(uuid="56ddcd5a-92b0-46c7-9c2b-d743794efb7c")
    @TelephonyBaseTest.tel_test_wrap
    def test_enable_mobile_data_always_on(self):
        """Verify mobile_data_always_on can be enabled.

        Steps:
        1. Enable mobile_data_always_on by adb.
        2. Verify the mobile data_always_on state.

        Expected Results: mobile_data_always_on return 1.
        """
        self.ad.log.info("Enable mobile_data_always_on")
        set_mobile_data_always_on(self.ad, True)
        time.sleep(1)
        return "1" in self.ad.adb.shell(
            "settings get global mobile_data_always_on")

    @test_tracker_info(uuid="c2cc5b66-40af-4ba6-81cb-6c44ae34cbbb")
    @TelephonyBaseTest.tel_test_wrap
    def test_push_new_radio_or_mbn(self):
        """Verify new mdn and radio can be push to device.

        Steps:
        1. If new radio path is given, flash new radio on the device.
        2. Verify the radio version.
        3. If new mbn path is given, push new mbn to device.
        4. Verify the installed mbn version.

        Expected Results:
        radio and mbn can be pushed to device and mbn.ver is available.
        """
        result = True
        paths = {}
        for path_key, dst_name in zip(["radio_image", "mbn_path"],
                                      ["radio.img", "mcfg_sw"]):
            path = self.user_params.get(path_key)
            if not path:
                continue
            elif isinstance(path, list):
                if not path[0]:
                    continue
                path = path[0]
            if "dev/null" in path:
                continue
            if not os.path.exists(path):
                self.log.error("path %s does not exist", path)
                self.log.info(self.user_params)
                path = os.path.join(self.user_params[Config.key_config_path],
                                    path)
                if not os.path.exists(path):
                    self.log.error("path %s does not exist", path)
                    continue

            self.log.info("%s path = %s", path_key, path)
            if "zip" in path:
                self.log.info("Unzip %s", path)
                file_path, file_name = os.path.split(path)
                dest_path = os.path.join(file_path, dst_name)
                os.system("rm -rf %s" % dest_path)
                unzip_maintain_permissions(path, file_path)
                path = dest_path
            os.system("chmod -R 777 %s" % path)
            paths[path_key] = path
        if not paths:
            self.log.info("No radio_path or mbn_path is provided")
            raise signals.TestSkip("No radio_path or mbn_path is provided")
        self.log.info("paths = %s", paths)
        for ad in self.android_devices:
            if paths.get("radio_image"):
                print_radio_info(ad, "Before flash radio, ")
                flash_radio(ad, paths["radio_image"])
                print_radio_info(ad, "After flash radio, ")
            if not paths.get("mbn_path") or "mbn" not in ad.adb.shell(
                    "ls /vendor"):
                ad.log.info("No need to push mbn files")
                continue
            push_result = True
            try:
                mbn_ver = ad.adb.shell(
                    "cat /vendor/mbn/mcfg/configs/mcfg_sw/mbn.ver")
                if mbn_ver:
                    ad.log.info("Before push mbn, mbn.ver = %s", mbn_ver)
                else:
                    ad.log.info(
                        "There is no mbn.ver before push, unmatching device")
                    continue
            except:
                ad.log.info(
                    "There is no mbn.ver before push, unmatching device")
                continue
            print_radio_info(ad, "Before push mbn, ")
            for i in range(2):
                if not system_file_push(ad, paths["mbn_path"],
                                        "/vendor/mbn/mcfg/configs/"):
                    if i == 1:
                        ad.log.error("Failed to push mbn file")
                        push_result = False
                else:
                    ad.log.info("The mbn file is pushed to device")
                    break
            if not push_result:
                result = False
                continue
            print_radio_info(ad, "After push mbn, ")
            try:
                new_mbn_ver = ad.adb.shell(
                    "cat /vendor/mbn/mcfg/configs/mcfg_sw/mbn.ver")
                if new_mbn_ver:
                    ad.log.info("new mcfg_sw mbn.ver = %s", new_mbn_ver)
                    if new_mbn_ver == mbn_ver:
                        ad.log.error(
                            "mbn.ver is the same before and after push")
                        result = False
                else:
                    ad.log.error("Unable to get new mbn.ver")
                    result = False
            except Exception as e:
                ad.log.error("cat mbn.ver with error %s", e)
                result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    def test_set_qxdm_log_mask_ims(self):
        """Set the QXDM Log mask to IMS_DS_CNE_LnX_Golden.cfg"""
        tasks = [(set_qxdm_logger_command, [ad, "IMS_DS_CNE_LnX_Golden.cfg"])
                 for ad in self.android_devices]
        return multithread_func(self.log, tasks)

    @TelephonyBaseTest.tel_test_wrap
    def test_set_qxdm_log_mask_qc_default(self):
        """Set the QXDM Log mask to QC_Default.cfg"""
        tasks = [(set_qxdm_logger_command, [ad, " QC_Default.cfg"])
                 for ad in self.android_devices]
        return multithread_func(self.log, tasks)

    @test_tracker_info(uuid="e2734d66-6111-4e76-aa7b-d3b4cbcde4f1")
    @TelephonyBaseTest.tel_test_wrap
    def test_check_carrier_id(self):
        """Verify mobile_data_always_on can be enabled.

        Steps:
        1. Enable mobile_data_always_on by adb.
        2. Verify the mobile data_always_on state.

        Expected Results: mobile_data_always_on return 1.
        """
        result = True
        if self.ad.adb.getprop("ro.build.version.release")[0] in ("8", "O",
                                                                  "7", "N"):
            raise signals.TestSkip("Not supported in this build")
        old_carrier_id = self.ad.droid.telephonyGetSubscriptionCarrierId()
        old_carrier_name = self.ad.droid.telephonyGetSubscriptionCarrierName()
        self.result_detail = "carrier_id = %s, carrier_name = %s" % (
            old_carrier_id, old_carrier_name)
        self.ad.log.info(self.result_detail)
        sub_id = get_outgoing_voice_sub_id(self.ad)
        slot_index = get_slot_index_from_subid(self.log, self.ad, sub_id)
        if power_off_sim(self.ad, slot_index):
            for i in range(3):
                carrier_id = self.ad.droid.telephonyGetSubscriptionCarrierId()
                carrier_name = self.ad.droid.telephonyGetSubscriptionCarrierName(
                )
                msg = "After SIM power down, carrier_id = %s(expecting -1), " \
                      "carrier_name = %s(expecting None)" % (carrier_id, carrier_name)
                if carrier_id != -1 or carrier_name:
                    if i == 2:
                        self.ad.log.error(msg)
                        self.result_detail = "%s %s" % (self.result_detail,
                                                        msg)
                        result = False
                    else:
                        time.sleep(5)
                else:
                    self.ad.log.info(msg)
                    break
        else:
            if self.ad.model in ("taimen", "walleye"):
                msg = "Power off SIM slot is not working"
                self.ad.log.error(msg)
                result = False
            else:
                msg = "Power off SIM slot is not supported"
                self.ad.log.warning(msg)
            self.result_detail = "%s %s" % (self.result_detail, msg)

        if not power_on_sim(self.ad, slot_index):
            self.ad.log.error("Fail to power up SIM")
            result = False
            setattr(self.ad, "reboot_to_recover", True)
        else:
            if is_sim_locked(self.ad):
                self.ad.log.info("Sim is locked")
                carrier_id = self.ad.droid.telephonyGetSubscriptionCarrierId()
                carrier_name = self.ad.droid.telephonyGetSubscriptionCarrierName(
                )
                msg = "In locked SIM, carrier_id = %s(expecting -1), " \
                      "carrier_name = %s(expecting None)" % (carrier_id, carrier_name)
                if carrier_id != -1 or carrier_name:
                    self.ad.log.error(msg)
                    self.result_detail = "%s %s" % (self.result_detail, msg)
                    result = False
                else:
                    self.ad.log.info(msg)
                unlock_sim(self.ad)
            elif getattr(self.ad, "is_sim_locked", False):
                self.ad.log.error(
                    "After SIM slot power cycle, SIM in not in locked state")
                return False

            if not ensure_phone_subscription(self.log, self.ad):
                self.ad.log.error("Unable to find a valid subscription!")
                result = False
            new_carrier_id = self.ad.droid.telephonyGetSubscriptionCarrierId()
            new_carrier_name = self.ad.droid.telephonyGetSubscriptionCarrierName(
            )
            msg = "After SIM power up, new_carrier_id = %s, " \
                  "new_carrier_name = %s" % (new_carrier_id, new_carrier_name)
            if old_carrier_id != new_carrier_id or (old_carrier_name !=
                                                    new_carrier_name):
                self.ad.log.error(msg)
                self.result_detail = "%s %s" % (self.result_detail, msg)
                result = False
            else:
                self.ad.log.info(msg)
        return result

    @TelephonyBaseTest.tel_test_wrap
    def test_modem_power_anomaly_file_existence(self):
        """Verify if the power anomaly file exists

        1. Collect Bugreport
        2. unzip bugreport
        3. remane the .bin file to .tar
        4. unzip dumpstate.tar
        5. Verify if the file exists

        """
        ad = self.android_devices[0]
        begin_time = get_current_epoch_time()
        for i in range(3):
            try:
                bugreport_path = os.path.join(ad.log_path, self.test_name)
                create_dir(bugreport_path)
                ad.take_bug_report(self.test_name, begin_time)
                break
            except Exception as e:
                ad.log.error("bugreport attempt %s error: %s", i + 1, e)
        ad.log.info("Bugreport Path is %s" % bugreport_path)
        try:
            list_of_files = os.listdir(bugreport_path)
            ad.log.info(list_of_files)
            for filename in list_of_files:
                if ".zip" in filename:
                    ad.log.info(filename)
                    file_path = os.path.join(bugreport_path, filename)
                    ad.log.info(file_path)
                    unzip_maintain_permissions(file_path, bugreport_path)
            dumpstate_path = os.path.join(bugreport_path,
                                          "dumpstate_board.bin")
            if os.path.isfile(dumpstate_path):
                os.rename(dumpstate_path,
                          bugreport_path + "/dumpstate_board.tar")
                os.chmod(bugreport_path + "/dumpstate_board.tar", 0o777)
                current_dir = os.getcwd()
                os.chdir(bugreport_path)
                exe_cmd("tar -xvf %s" %
                        (bugreport_path + "/dumpstate_board.tar"))
                os.chdir(current_dir)
                if os.path.isfile(bugreport_path + "/power_anomaly_data.txt"):
                    ad.log.info("Modem Power Anomaly File Exists!!")
                    return True
            return False
        except Exception as e:
            ad.log.error(e)
            return False
