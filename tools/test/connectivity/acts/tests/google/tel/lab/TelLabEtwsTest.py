#/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
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

from acts.controllers.anritsu_lib._anritsu_utils import AnritsuError
from acts.controllers.anritsu_lib.md8475a import MD8475A
from acts.controllers.anritsu_lib.md8475a import CBCHSetup
from acts.controllers.anritsu_lib.md8475a import CTCHSetup
from acts.test_utils.tel.anritsu_utils import ETWS_WARNING_EARTHQUAKETSUNAMI
from acts.test_utils.tel.anritsu_utils import ETWS_WARNING_OTHER_EMERGENCY
from acts.test_utils.tel.anritsu_utils import cb_serial_number
from acts.test_utils.tel.anritsu_utils import etws_receive_verify_message_lte_wcdma
from acts.test_utils.tel.anritsu_utils import set_system_model_gsm
from acts.test_utils.tel.anritsu_utils import set_system_model_lte
from acts.test_utils.tel.anritsu_utils import set_system_model_wcdma
from acts.test_utils.tel.anritsu_utils import set_usim_parameters
from acts.test_utils.tel.anritsu_utils import set_post_sim_params
from acts.test_utils.tel.tel_defines import NETWORK_MODE_CDMA
from acts.test_utils.tel.tel_defines import NETWORK_MODE_GSM_ONLY
from acts.test_utils.tel.tel_defines import NETWORK_MODE_GSM_UMTS
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_GSM_WCDMA
from acts.test_utils.tel.tel_defines import RAT_1XRTT
from acts.test_utils.tel.tel_defines import RAT_GSM
from acts.test_utils.tel.tel_defines import RAT_LTE
from acts.test_utils.tel.tel_defines import RAT_WCDMA
from acts.test_utils.tel.tel_defines import RAT_FAMILY_CDMA2000
from acts.test_utils.tel.tel_defines import RAT_FAMILY_GSM
from acts.test_utils.tel.tel_defines import RAT_FAMILY_LTE
from acts.test_utils.tel.tel_defines import RAT_FAMILY_UMTS
from acts.test_utils.tel.tel_test_utils import ensure_network_rat
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import start_qxdm_loggers
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_decorators import test_tracker_info

WAIT_TIME_BETWEEN_REG_AND_MSG = 15  # default 15 sec


class TelLabEtwsTest(TelephonyBaseTest):
    SERIAL_NO = cb_serial_number()

    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.ad = self.android_devices[0]
        self.ad.sim_card = getattr(self.ad, "sim_card", None)
        self.md8475a_ip_address = self.user_params[
            "anritsu_md8475a_ip_address"]
        self.wlan_option = self.user_params.get("anritsu_wlan_option", False)
        self.ad.adb.shell("settings put secure cmas_additional_broadcast_pkg "
                          "com.googlecode.android_scripting")
        self.wait_time_between_reg_and_msg = self.user_params.get(
            "wait_time_between_reg_and_msg", WAIT_TIME_BETWEEN_REG_AND_MSG)

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
        return True

    def teardown_test(self):
        self.log.info("Stopping Simulation")
        self.anritsu.stop_simulation()
        toggle_airplane_mode(self.log, self.ad, True)

    def teardown_class(self):
        self.anritsu.disconnect()
        return True

    def _send_receive_etws_message(self, set_simulation_func, rat, message_id,
                                   warning_message):
        try:
            [self.bts1] = set_simulation_func(self.anritsu, self.user_params,
                                              self.ad.sim_card)
            set_usim_parameters(self.anritsu, self.ad.sim_card)
            if rat == RAT_LTE:
                set_post_sim_params(self.anritsu, self.user_params,
                                    self.ad.sim_card)
            self.anritsu.start_simulation()

            if rat == RAT_LTE:
                preferred_network_setting = NETWORK_MODE_LTE_GSM_WCDMA
                rat_family = RAT_FAMILY_LTE
            elif rat == RAT_WCDMA:
                self.bts1.wcdma_ctch = CTCHSetup.CTCH_ENABLE
                self.ad.droid.telephonyToggleDataConnection(False)
                preferred_network_setting = NETWORK_MODE_GSM_UMTS
                rat_family = RAT_FAMILY_UMTS
            elif rat == RAT_GSM:
                self.bts1.gsm_cbch = CBCHSetup.CBCH_ENABLE
                self.ad.droid.telephonyToggleDataConnection(False)
                preferred_network_setting = NETWORK_MODE_GSM_ONLY
                rat_family = RAT_FAMILY_GSM
            elif rat == RAT_1XRTT:
                preferred_network_setting = NETWORK_MODE_CDMA
                rat_family = RAT_FAMILY_CDMA2000
            else:
                self.log.error("No valid RAT provided for ETWS test.")
                return False

            if not ensure_network_rat(
                    self.log,
                    self.ad,
                    preferred_network_setting,
                    rat_family,
                    toggle_apm_after_setting=True):
                self.log.error(
                    "Failed to set rat family {}, preferred network:{}".format(
                        rat_family, preferred_network_setting))
                return False

            self.anritsu.wait_for_registration_state()
            if not etws_receive_verify_message_lte_wcdma(
                    self.log, self.ad, self.anritsu,
                    next(TelLabEtwsTest.SERIAL_NO), message_id,
                    warning_message):
                self.log.error("Phone {} Failed to receive ETWS message"
                               .format(self.ad.serial))
                return False
        except AnritsuError as e:
            self.log.error("Error in connection with Anritsu Simulator: " +
                           str(e))
            return False
        except Exception as e:
            self.log.error("Exception during ETWS send/receive: " + str(e))
            return False
        return True

    """ Tests Begin """

    @test_tracker_info(uuid="af4a00d0-9a91-45d5-9f65-9541e64a57f2")
    @TelephonyBaseTest.tel_test_wrap
    def test_etws_earthquake_tsunami_lte(self):
        """ETWS Earthquake and Tsunami warning message reception on LTE

        Tests the capability of device to receive and inform the user
        about the ETWS Earthquake and Tsunami warning message when camped on
        LTE newtork

        Steps:
        1. Make Sure Phone is camped on LTE network
        2. Send ETWS Earthquake and Tsunami warning message from Anritsu

        Expected Result:
        Phone receives ETWS Earthquake and Tsunami warning message

        Returns:
            True if pass; False if fail
        """
        return self._send_receive_etws_message(set_system_model_lte, RAT_LTE,
                                               ETWS_WARNING_EARTHQUAKETSUNAMI,
                                               "LTE Earthquake and Tsunami")

    @test_tracker_info(uuid="03785878-0319-413c-9190-d4e08f0edc33")
    @TelephonyBaseTest.tel_test_wrap
    def test_etws_other_emergency_lte(self):
        """ETWS Other emergency warning message reception on LTE

        Tests the capability of device to receive and inform the user
        about the ETWS Other emergency warning message when camped on
        LTE newtork

        Steps:
        1. Make Sure Phone is camped on LTE network
        2. Send ETWS Earthquake and Tsunami warning message from Anritsu

        Expected Result:
        Phone receives ETWS Earthquake and Tsunami warning message

        Returns:
            True if pass; False if fail
        """
        return self._send_receive_etws_message(set_system_model_lte, RAT_LTE,
                                               ETWS_WARNING_OTHER_EMERGENCY,
                                               "LTE ETWS Other Emergency")

    @test_tracker_info(uuid="1ef4a5d7-9ceb-49eb-8ec7-5538625c8bd4")
    @TelephonyBaseTest.tel_test_wrap
    def test_etws_earthquake_tsunami_wcdma(self):
        """ETWS Earthquake and Tsunami warning message reception on WCDMA

        Tests the capability of device to receive and inform the user
        about the ETWS Earthquake and Tsunami warning message when camped on
        WCDMA newtork

        Steps:
        1. Make Sure Phone is camped on WCDMA network
        2. Send ETWS Earthquake and Tsunami warning message from Anritsu

        Expected Result:
        Phone receives ETWS Earthquake and Tsunami warning message

        Returns:
            True if pass; False if fail
        """
        return self._send_receive_etws_message(
            set_system_model_wcdma, RAT_WCDMA, ETWS_WARNING_EARTHQUAKETSUNAMI,
            "WCDMA Earthquake and Tsunami")

    @test_tracker_info(uuid="71dc9650-d00a-4533-99f5-5cc301c21334")
    @TelephonyBaseTest.tel_test_wrap
    def test_etws_other_emergency_wcdma(self):
        """ETWS Other emergency warning message reception on WCDMA

        Tests the capability of device to receive and inform the user
        about the ETWS Other emergency warning message when camped on
        WCDMA newtork

        Steps:
        1. Make Sure Phone is camped on WCDMA network
        2. Send ETWS Earthquake and Tsunami warning message from Anritsu

        Expected Result:
        Phone receives ETWS Earthquake and Tsunami warning message

        Returns:
            True if pass; False if fail
        """
        return self._send_receive_etws_message(
            set_system_model_wcdma, RAT_WCDMA, ETWS_WARNING_OTHER_EMERGENCY,
            "WCDMA ETWS Other Emergency")

    @test_tracker_info(uuid="a9fd9c0e-21bf-41d1-81d2-c34679052fe0")
    @TelephonyBaseTest.tel_test_wrap
    def test_etws_earthquake_tsunami_gsm(self):
        """ETWS Earthquake and Tsunami warning message reception on GSM

        Tests the capability of device to receive and inform the user
        about the ETWS Earthquake and Tsunami warning message when camped on
        GSM newtork

        Steps:
        1. Make Sure Phone is camped on GSM network
        2. Send ETWS Earthquake and Tsunami warning message from Anritsu

        Expected Result:
        Phone receives ETWS Earthquake and Tsunami warning message

        Returns:
            True if pass; False if fail
        """
        return self._send_receive_etws_message(set_system_model_gsm, RAT_GSM,
                                               ETWS_WARNING_EARTHQUAKETSUNAMI,
                                               "GSM Earthquake and Tsunami")

    @test_tracker_info(uuid="0ae42f8d-1720-449c-9200-e88f7f1d2cbe")
    @TelephonyBaseTest.tel_test_wrap
    def test_etws_other_emergency_gsm(self):
        """ETWS Other emergency warning message reception on GSM

        Tests the capability of device to receive and inform the user
        about the ETWS Other emergency warning message when camped on
        GSM newtork

        Steps:
        1. Make Sure Phone is camped on GSM network
        2. Send ETWS Earthquake and Tsunami warning message from Anritsu

        Expected Result:
        Phone receives ETWS Earthquake and Tsunami warning message

        Returns:
            True if pass; False if fail
        """
        return self._send_receive_etws_message(set_system_model_gsm, RAT_GSM,
                                               ETWS_WARNING_OTHER_EMERGENCY,
                                               "GSM ETWS Other Emergency")

    """ Tests End """
