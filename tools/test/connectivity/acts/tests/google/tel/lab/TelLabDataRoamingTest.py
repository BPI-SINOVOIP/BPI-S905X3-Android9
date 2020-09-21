#/usr/bin/env python3.4
#
#   Copyright 2017 - The Android Open Source Project
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
Sanity tests for voice tests in telephony
"""
import time

from acts.test_decorators import test_tracker_info
from acts.controllers.anritsu_lib._anritsu_utils import AnritsuError
from acts.controllers.anritsu_lib.md8475a import MD8475A
from acts.controllers.anritsu_lib.md8475a import BtsServiceState
from acts.controllers.anritsu_lib.md8475a import BtsPacketRate
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.anritsu_utils import set_system_model_lte_wcdma
from acts.test_utils.tel.anritsu_utils import set_usim_parameters
from acts.test_utils.tel.anritsu_utils import set_post_sim_params
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_GSM_WCDMA
from acts.test_utils.tel.tel_defines import RAT_FAMILY_LTE
from acts.test_utils.tel.tel_test_utils import ensure_network_rat
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import toggle_cell_data_roaming
from acts.test_utils.tel.tel_test_utils import set_preferred_apn_by_adb
from acts.test_utils.tel.tel_test_utils import start_qxdm_loggers
from acts.utils import adb_shell_ping

PING_DURATION = 5  # Number of packets to ping
PING_TARGET = "192.168.1.2"  # Set to Eth0 IP address of MD8475A
TIME_TO_WAIT_BEFORE_PING = 10  # Time(sec) to wait before ping


class TelLabDataRoamingTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.ad = self.android_devices[0]
        self.ad.sim_card = getattr(self.ad, "sim_card", None)
        self.md8475a_ip_address = self.user_params[
            "anritsu_md8475a_ip_address"]
        self.wlan_option = self.user_params.get("anritsu_wlan_option", False)
        if self.ad.sim_card == "VzW12349":
            set_preferred_apn_by_adb(self.ad, "VZWINTERNET")

    def setup_class(self):
        try:
            self.anritsu = MD8475A(self.md8475a_ip_address, self.log,
                                   self.wlan_option)
        except AnritsuError:
            self.log.error("Error in connecting to Anritsu Simulator")
            return False
        return True

    def setup_test(self):
        if getattr(self, "qxdm_log", True):
            start_qxdm_loggers(self.log, self.android_devices)
        ensure_phones_idle(self.log, self.android_devices)
        toggle_airplane_mode(self.log, self.ad, True)
        self.ad.adb.shell(
            "setprop net.lte.ims.volte.provisioned 1", ignore_status=True)
        return True

    def teardown_test(self):
        self.log.info("Stopping Simulation")
        self.anritsu.stop_simulation()
        toggle_airplane_mode(self.log, self.ad, True)
        return True

    def teardown_class(self):
        self.anritsu.disconnect()
        return True

    def phone_setup_data_roaming(self):
        return ensure_network_rat(
            self.log,
            self.ad,
            NETWORK_MODE_LTE_GSM_WCDMA,
            RAT_FAMILY_LTE,
            toggle_apm_after_setting=True)

    def LTE_WCDMA_data_roaming(self, mcc, mnc, lte_band, wcdma_band):
        try:
            [self.bts1, self.bts2] = set_system_model_lte_wcdma(
                self.anritsu, self.user_params, self.ad.sim_card)
            set_usim_parameters(self.anritsu, self.ad.sim_card)
            set_post_sim_params(self.anritsu, self.user_params,
                                self.ad.sim_card)
            self.bts1.mcc = mcc
            self.bts1.mnc = mnc
            self.bts2.mcc = mcc
            self.bts2.mnc = mnc
            self.bts1.band = lte_band
            self.bts2.band = wcdma_band
            self.bts2.packet_rate = BtsPacketRate.WCDMA_DLHSAUTO_REL8_ULHSAUTO
            self.anritsu.start_simulation()
            self.bts2.service_state = BtsServiceState.SERVICE_STATE_OUT
            self.log.info("Toggle Mobile Data On")
            self.ad.droid.telephonyToggleDataConnection(True)

            if not self.phone_setup_data_roaming():
                self.log.warning("phone_setup_func failed. Rebooting UE")
                self.ad.reboot()
                time.sleep(30)
                if self.ad.sim_card == "VzW12349":
                    set_preferred_apn_by_adb(self.ad, "VZWINTERNET")
                if not self.phone_setup_data_roaming():
                    self.log.error(
                        "Failed to set rat family {}, preferred network:{}".
                        format(RAT_FAMILY_LTE, NETWORK_MODE_LTE_GSM_WCDMA))
                    return False

            toggle_cell_data_roaming(self.ad, True)
            self.anritsu.wait_for_registration_state(1)  # for BTS1 LTE

            time.sleep(TIME_TO_WAIT_BEFORE_PING)
            for i in range(3):
                self.ad.log.info("Verify internet connection - attempt %d",
                                 i + 1)
                result = adb_shell_ping(self.ad, PING_DURATION, PING_TARGET)
                if result:
                    self.ad.log.info("PING SUCCESS")
                    break
                elif i == 2:
                    self.log.error(
                        "Test Fail: Phone {} can not ping {} with Data Roaming On"
                        .format(self.ad.serial, PING_TARGET))
                    return False

            toggle_cell_data_roaming(self.ad, False)
            time.sleep(TIME_TO_WAIT_BEFORE_PING)
            if adb_shell_ping(self.ad, PING_DURATION, PING_TARGET):
                self.log.error(
                    "Test Fail: Phone {} can ping {} with Data Roaming Off"
                    .format(self.ad.serial, PING_TARGET))
                return False

            toggle_airplane_mode(self.log, self.ad, True)
            time.sleep(2)
            self.bts2.service_state = BtsServiceState.SERVICE_STATE_IN
            self.bts1.service_state = BtsServiceState.SERVICE_STATE_OUT

            toggle_airplane_mode(self.log, self.ad, False)
            toggle_cell_data_roaming(self.ad, True)
            self.anritsu.wait_for_registration_state(2)  # for BTS2 WCDMA

            time.sleep(TIME_TO_WAIT_BEFORE_PING)
            for i in range(3):
                self.ad.log.info("Verify internet connection - attempt %d",
                                 i + 1)
                result = adb_shell_ping(self.ad, PING_DURATION, PING_TARGET)
                if result:
                    self.ad.log.info("PING SUCCESS")
                    break
                elif i == 2:
                    self.log.error(
                        "Test Fail: Phone {} can not ping {} with Data Roaming On"
                        .format(self.ad.serial, PING_TARGET))
                    return False

            toggle_cell_data_roaming(self.ad, False)
            time.sleep(TIME_TO_WAIT_BEFORE_PING)
            if adb_shell_ping(self.ad, PING_DURATION, PING_TARGET):
                self.log.error(
                    "Test Fail: Phone {} can ping {} with Data Roaming Off"
                    .format(self.ad.serial, PING_TARGET))
                return False

        except AnritsuError as e:
            self.log.error("Error in connection with Anritsu Simulator: " +
                           str(e))
            return False
        except Exception as e:
            self.log.error("Exception during data roaming: " + str(e))
            return False
        return True

    """ Tests Begin """

    @test_tracker_info(uuid="46d49bff-9671-4ab0-a90d-b49d870af6f0")
    @TelephonyBaseTest.tel_test_wrap
    def test_data_roaming_optus(self):
        """Data roaming test for Optus LTE and WCDMA networks

        Tests if data roaming enabled/disabled work correctly with
        Optus(Australia) LTE and WCDMA networks

        Steps:
        1. Setup Optus LTE network and make sure UE registers
        2. Turn on data roaming, and check if Ping succeeds
        3. Turn off data roaming, and check if Ping fails
        4. Turn Airplane Mode On in UE
        5. Setup Optus WCDMA network and make sure UE registers
        6. Turn on data roaming, and check if Ping succeeds
        7. Turn off data roaming, and check if Ping fails

        Expected Result:
        Ping succeeds when data roaming is turned on,
        and Ping fails when data roaming is turned off

        Returns:
            True if pass; False if fail
        """
        return self.LTE_WCDMA_data_roaming(
            mcc="505", mnc="02F", lte_band="3", wcdma_band="1")

    @test_tracker_info(uuid="68a6313c-d95a-4cae-8e35-2fdf3c94df56")
    @TelephonyBaseTest.tel_test_wrap
    def test_data_roaming_telus(self):
        """Data roaming test for Telus LTE and WCDMA networks

        Tests if data roaming enabled/disabled work correctly with
        Telus(Canada) LTE and WCDMA networks

        Steps:
        1. Setup Telus LTE network and make sure UE registers
        2. Turn on data roaming, and check if Ping succeeds
        3. Turn off data roaming, and check if Ping fails
        4. Turn Airplane Mode On in UE
        5. Setup Telus WCDMA network and make sure UE registers
        6. Turn on data roaming, and check if Ping succeeds
        7. Turn off data roaming, and check if Ping fails

        Expected Result:
        Ping succeeds when data roaming is turned on,
        and Ping fails when data roaming is turned off

        Returns:
            True if pass; False if fail
        """
        return self.LTE_WCDMA_data_roaming(
            mcc="302", mnc="220", lte_band="4", wcdma_band="2")

    @test_tracker_info(uuid="16de850a-6511-42d4-8d8f-d800477aba6b")
    @TelephonyBaseTest.tel_test_wrap
    def test_data_roaming_vodafone(self):
        """Data roaming test for Vodafone LTE and WCDMA networks

        Tests if data roaming enabled/disabled work correctly with
        Vodafone(UK) LTE and WCDMA networks

        Steps:
        1. Setup Vodafone LTE network and make sure UE registers
        2. Turn on data roaming, and check if Ping succeeds
        3. Turn off data roaming, and check if Ping fails
        4. Turn Airplane Mode On in UE
        5. Setup Vodafone WCDMA network and make sure UE registers
        6. Turn on data roaming, and check if Ping succeeds
        7. Turn off data roaming, and check if Ping fails

        Expected Result:
        Ping succeeds when data roaming is turned on,
        and Ping fails when data roaming is turned off

        Returns:
            True if pass; False if fail
        """
        return self.LTE_WCDMA_data_roaming(
            mcc="234", mnc="15F", lte_band="20", wcdma_band="1")

    @test_tracker_info(uuid="e9050f3d-b53c-4a87-9363-b88a842a3479")
    @TelephonyBaseTest.tel_test_wrap
    def test_data_roaming_o2(self):
        """Data roaming test for O2 LTE and WCDMA networks

        Tests if data roaming enabled/disabled work correctly with
        O2(UK) LTE and WCDMA networks

        Steps:
        1. Setup O2 LTE network and make sure UE registers
        2. Turn on data roaming, and check if Ping succeeds
        3. Turn off data roaming, and check if Ping fails
        4. Turn Airplane Mode On in UE
        5. Setup O2 WCDMA network and make sure UE registers
        6. Turn on data roaming, and check if Ping succeeds
        7. Turn off data roaming, and check if Ping fails

        Expected Result:
        Ping succeeds when data roaming is turned on,
        and Ping fails when data roaming is turned off

        Returns:
            True if pass; False if fail
        """
        return self.LTE_WCDMA_data_roaming(
            mcc="234", mnc="02F", lte_band="20", wcdma_band="1")

    @test_tracker_info(uuid="a3f56da1-6a51-45b0-8016-3a492661e1f4")
    @TelephonyBaseTest.tel_test_wrap
    def test_data_roaming_orange(self):
        """Data roaming test for Orange LTE and WCDMA networks

        Tests if data roaming enabled/disabled work correctly with
        Orange(UK) LTE and WCDMA networks

        Steps:
        1. Setup Orange LTE network and make sure UE registers
        2. Turn on data roaming, and check if Ping succeeds
        3. Turn off data roaming, and check if Ping fails
        4. Turn Airplane Mode On in UE
        5. Setup Orange WCDMA network and make sure UE registers
        6. Turn on data roaming, and check if Ping succeeds
        7. Turn off data roaming, and check if Ping fails

        Expected Result:
        Ping succeeds when data roaming is turned on,
        and Ping fails when data roaming is turned off

        Returns:
            True if pass; False if fail
        """
        return self.LTE_WCDMA_data_roaming(
            mcc="234", mnc="33F", lte_band="20", wcdma_band="1")

    @test_tracker_info(uuid="dcde16c1-730c-41ee-ad29-286f4962c66f")
    @TelephonyBaseTest.tel_test_wrap
    def test_data_roaming_idea(self):
        """Data roaming test for Idea LTE and WCDMA networks

        Tests if data roaming enabled/disabled work correctly with
        Idea(India) LTE and WCDMA networks

        Steps:
        1. Setup Idea LTE network and make sure UE registers
        2. Turn on data roaming, and check if Ping succeeds
        3. Turn off data roaming, and check if Ping fails
        4. Turn Airplane Mode On in UE
        5. Setup Idea WCDMA network and make sure UE registers
        6. Turn on data roaming, and check if Ping succeeds
        7. Turn off data roaming, and check if Ping fails

        Expected Result:
        Ping succeeds when data roaming is turned on,
        and Ping fails when data roaming is turned off

        Returns:
            True if pass; False if fail
        """
        return self.LTE_WCDMA_data_roaming(
            mcc="404", mnc="24F", lte_band="3", wcdma_band="1")

    """ Tests End """
