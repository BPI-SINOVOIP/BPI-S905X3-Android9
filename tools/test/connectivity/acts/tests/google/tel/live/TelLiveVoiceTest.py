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
    Test Script for Telephony Pre Check In Sanity
"""

import time

from acts import signals
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import DIRECTION_MOBILE_ORIGINATED
from acts.test_utils.tel.tel_defines import DIRECTION_MOBILE_TERMINATED
from acts.test_utils.tel.tel_defines import GEN_2G
from acts.test_utils.tel.tel_defines import GEN_4G
from acts.test_utils.tel.tel_defines import CALL_STATE_ACTIVE
from acts.test_utils.tel.tel_defines import CALL_STATE_HOLDING
from acts.test_utils.tel.tel_defines import GEN_3G
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_NW_SELECTION
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import PHONE_TYPE_CDMA
from acts.test_utils.tel.tel_defines import PHONE_TYPE_GSM
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL_FOR_IMS
from acts.test_utils.tel.tel_defines import WFC_MODE_CELLULAR_PREFERRED
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_ONLY
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_subscription_utils import \
    get_incoming_voice_sub_id
from acts.test_utils.tel.tel_subscription_utils import \
    get_outgoing_voice_sub_id
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import \
    call_voicemail_erase_all_pending_voicemail
from acts.test_utils.tel.tel_test_utils import active_file_download_task
from acts.utils import adb_shell_ping
from acts.test_utils.tel.tel_test_utils import ensure_network_generation
from acts.test_utils.tel.tel_test_utils import get_mobile_data_usage
from acts.test_utils.tel.tel_test_utils import get_phone_number
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import initiate_call
from acts.test_utils.tel.tel_test_utils import multithread_func
from acts.test_utils.tel.tel_test_utils import num_active_calls
from acts.test_utils.tel.tel_test_utils import phone_number_formatter
from acts.test_utils.tel.tel_test_utils import remove_mobile_data_usage_limit
from acts.test_utils.tel.tel_test_utils import run_multithread_func
from acts.test_utils.tel.tel_test_utils import set_mobile_data_usage_limit
from acts.test_utils.tel.tel_test_utils import set_phone_number
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_test_utils import verify_incall_state
from acts.test_utils.tel.tel_test_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_test_utils import wait_for_ringing_call
from acts.test_utils.tel.tel_test_utils import wait_for_state
from acts.test_utils.tel.tel_test_utils import start_adb_tcpdump
from acts.test_utils.tel.tel_test_utils import stop_adb_tcpdump
from acts.test_utils.tel.tel_test_utils import set_wifi_to_default
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_1x
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_2g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_csfb
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_not_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_wcdma
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts.test_utils.tel.tel_voice_utils import \
    phone_setup_iwlan_cellular_preferred
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_2g
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_general
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.tel.tel_voice_utils import phone_idle_2g
from acts.test_utils.tel.tel_voice_utils import phone_idle_3g
from acts.test_utils.tel.tel_voice_utils import phone_idle_csfb
from acts.test_utils.tel.tel_voice_utils import phone_idle_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_idle_volte
from acts.test_utils.tel.tel_voice_utils import two_phone_call_leave_voice_mail
from acts.test_utils.tel.tel_voice_utils import two_phone_call_long_seq
from acts.test_utils.tel.tel_voice_utils import two_phone_call_short_seq

DEFAULT_LONG_DURATION_CALL_TOTAL_DURATION = 1 * 60 * 60  # default value 1 hour
DEFAULT_PING_DURATION = 120  # in seconds


class TelLiveVoiceTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)

        self.stress_test_number = self.get_stress_test_number()
        self.wifi_network_ssid = self.user_params["wifi_network_ssid"]
        self.wifi_network_pass = self.user_params.get("wifi_network_pass")
        self.long_duration_call_total_duration = self.user_params.get(
            "long_duration_call_total_duration",
            DEFAULT_LONG_DURATION_CALL_TOTAL_DURATION)
        self.tcpdump_proc = [None, None]
        self.number_of_devices = 2

    """ Tests Begin """

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="fca3f9e1-447a-416f-9a9c-50b7161981bf")
    def test_call_mo_voice_general(self):
        """ General voice to voice call.

        1. Make Sure PhoneA attached to voice network.
        2. Make Sure PhoneB attached to voice network.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(self.log, ads[0], None, None, ads[1],
                                        None, None)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="69faeb84-3830-47c0-ad80-dc657381a83b")
    def test_call_mt_voice_general(self):
        """ General voice to voice call.

        1. Make Sure PhoneA attached to voice network.
        2. Make Sure PhoneB attached to voice network.
        3. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        4. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(self.log, ads[1], None, None, ads[0],
                                        None, None)

    @test_tracker_info(uuid="b2de097b-70e1-4242-b555-c1aa0a5acd8c")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte(self):
        """ VoLTE to VoLTE call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_volte, is_phone_in_call_volte, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="3c7f5a09-0177-4469-9994-cd5e7dd7c7fe")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_7_digit_dialing(self):
        """ VoLTE to VoLTE call test, dial with 7 digit number

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB by 7-digit phone number, accept on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        if self.android_devices[0].droid.telephonyGetSimCountryIso() == "ca":
            raise signals.TestSkip("7 digit dialing not supported")

        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        callee_default_number = get_phone_number(self.log, ads[1])
        caller_dialing_number = phone_number_formatter(callee_default_number,
                                                       7)
        try:
            set_phone_number(self.log, ads[1], caller_dialing_number)
            return call_setup_teardown(
                self.log, ads[0], ads[1], ads[0], is_phone_in_call_volte,
                is_phone_in_call_volte, WAIT_TIME_IN_CALL_FOR_IMS)
        except Exception as e:
            self.log.error("Exception happened: {}".format(e))
        finally:
            set_phone_number(self.log, ads[1], callee_default_number)

    @test_tracker_info(uuid="721ef935-a03c-4d0f-85b9-4753d857162f")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_10_digit_dialing(self):
        """ VoLTE to VoLTE call test, dial with 10 digit number

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB by 10-digit phone number, accept on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        if self.android_devices[0].droid.telephonyGetSimCountryIso() == "ca":
            raise signals.TestSkip("10 digit dialing not supported")

        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        callee_default_number = get_phone_number(self.log, ads[1])
        caller_dialing_number = phone_number_formatter(callee_default_number,
                                                       10)
        try:
            set_phone_number(self.log, ads[1], caller_dialing_number)
            return call_setup_teardown(
                self.log, ads[0], ads[1], ads[0], is_phone_in_call_volte,
                is_phone_in_call_volte, WAIT_TIME_IN_CALL_FOR_IMS)
        except Exception as e:
            self.log.error("Exception happened: {}".format(e))
        finally:
            set_phone_number(self.log, ads[1], callee_default_number)

    @test_tracker_info(uuid="4fd3aa62-2398-4cee-994e-7fc5cadbcbc1")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_11_digit_dialing(self):
        """ VoLTE to VoLTE call test, dial with 11 digit number

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB by 11-digit phone number, accept on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        callee_default_number = get_phone_number(self.log, ads[1])
        caller_dialing_number = phone_number_formatter(callee_default_number,
                                                       11)
        try:
            set_phone_number(self.log, ads[1], caller_dialing_number)
            return call_setup_teardown(
                self.log, ads[0], ads[1], ads[0], is_phone_in_call_volte,
                is_phone_in_call_volte, WAIT_TIME_IN_CALL_FOR_IMS)
        except Exception as e:
            self.log.error("Exception happened: {}".format(e))
        finally:
            set_phone_number(self.log, ads[1], callee_default_number)

    @test_tracker_info(uuid="969abdac-6a57-442a-9c40-48199bd8d556")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_12_digit_dialing(self):
        """ VoLTE to VoLTE call test, dial with 12 digit number

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB by 12-digit phone number, accept on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        callee_default_number = get_phone_number(self.log, ads[1])
        caller_dialing_number = phone_number_formatter(callee_default_number,
                                                       12)
        try:
            set_phone_number(self.log, ads[1], caller_dialing_number)
            return call_setup_teardown(
                self.log, ads[0], ads[1], ads[0], is_phone_in_call_volte,
                is_phone_in_call_volte, WAIT_TIME_IN_CALL_FOR_IMS)
        except Exception as e:
            self.log.error("Exception happened: {}".format(e))
        finally:
            set_phone_number(self.log, ads[1], callee_default_number)

    @test_tracker_info(uuid="6b13a03d-c9ff-43d7-9798-adbead7688a4")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_csfb_3g(self):
        """ VoLTE to CSFB 3G call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (without VoLTE).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_csfb,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_csfb, is_phone_in_call_csfb, None)

    @test_tracker_info(uuid="38096fdb-324a-4ce0-8836-8bbe713cffc2")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_csfb_for_tmo(self):
        """ VoLTE to CSFB 3G call test for TMobile

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (without VoLTE, CSFB to WCDMA/GSM).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_csfb,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(self.log, ads[0], phone_idle_volte,
                                        None, ads[1], phone_idle_csfb,
                                        is_phone_in_call_csfb, None)

    @test_tracker_info(uuid="82f9515d-a52b-4dec-93a5-997ffdbca76c")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_csfb_1x_long(self):
        """ VoLTE to CSFB 1x call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (without VoLTE, CSFB to 1x).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # Make Sure PhoneB is CDMA phone.
        if ads[1].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA:
            self.log.error(
                "PhoneB not cdma phone, can not csfb 1x. Stop test.")
            return False

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_csfb,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_csfb, is_phone_in_call_1x, None)

    @test_tracker_info(uuid="2e57fad6-5eaf-4e7d-8353-8aa6f4c52776")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_csfb_long(self):
        """ VoLTE to CSFB WCDMA call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (without VoLTE, CSFB to WCDMA/GSM).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # Make Sure PhoneB is GSM phone.
        if ads[1].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM:
            self.log.error(
                "PhoneB not gsm phone, can not csfb wcdma. Stop test.")
            return False

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_csfb,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_csfb, is_phone_in_call_csfb, None)

    @test_tracker_info(uuid="4bab759f-7610-4cec-893c-0a8aed95f70c")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_3g(self):
        """ VoLTE to 3G call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_3g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_3g, is_phone_in_call_3g, None)

    @test_tracker_info(uuid="b394cdc5-d88d-4659-8a26-0e58fde69974")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_3g_1x_long(self):
        """ VoLTE to 3G 1x call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in 3G 1x mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # Make Sure PhoneB is CDMA phone.
        if ads[1].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA:
            self.log.error("PhoneB not cdma phone, can not 3g 1x. Stop test.")
            return False

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_3g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_3g, is_phone_in_call_1x, None)

    @test_tracker_info(uuid="b39a74a9-2a89-4c0b-ac4e-71ed9317bd75")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_3g_wcdma_long(self):
        """ VoLTE to 3G WCDMA call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in 3G WCDMA mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # Make Sure PhoneB is GSM phone.
        if ads[1].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM:
            self.log.error(
                "PhoneB not gsm phone, can not 3g wcdma. Stop test.")
            return False

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_3g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_3g, is_phone_in_call_wcdma, None)

    @test_tracker_info(uuid="573bbcf1-6cbd-4084-9cb7-e14fb6c9521e")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_2g(self):
        """ VoLTE to 2G call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in 2G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_2g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_2g, is_phone_in_call_2g, None)

    def _call_epdg_to_epdg_wfc(self, ads, apm_mode, wfc_mode, wifi_ssid,
                               wifi_pwd):
        """ Test epdg<->epdg call functionality.

        Make Sure PhoneA is set to make epdg call.
        Make Sure PhoneB is set to make epdg call.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Args:
            ads: list of android objects, this list should have two ad.
            apm_mode: phones' airplane mode.
                if True, phones are in airplane mode during test.
                if False, phones are not in airplane mode during test.
            wfc_mode: phones' wfc mode.
                Valid mode includes: WFC_MODE_WIFI_ONLY, WFC_MODE_CELLULAR_PREFERRED,
                WFC_MODE_WIFI_PREFERRED, WFC_MODE_DISABLED.
            wifi_ssid: WiFi ssid to connect during test.
            wifi_pwd: WiFi password.

        Returns:
            True if pass; False if fail.
        """
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            self.tcpdump_proc[1] = start_adb_tcpdump(ads[1], self.test_name)
            tasks = [(phone_setup_iwlan, (self.log, ads[0], apm_mode, wfc_mode,
                                          wifi_ssid, wifi_pwd)),
                     (phone_setup_iwlan, (self.log, ads[1], apm_mode, wfc_mode,
                                          wifi_ssid, wifi_pwd))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return False

            ad_ping = ads[0]

            call_task = (two_phone_call_short_seq,
                         (self.log, ads[0], phone_idle_iwlan,
                          is_phone_in_call_iwlan, ads[1], phone_idle_iwlan,
                          is_phone_in_call_iwlan, None,
                          WAIT_TIME_IN_CALL_FOR_IMS))
            ping_task = (adb_shell_ping, (ad_ping, DEFAULT_PING_DURATION))

            results = run_multithread_func(self.log, [ping_task, call_task])

            if not results[1]:
                self.log.error("Call setup failed in active ICMP transfer.")
            if results[0]:
                self.log.info(
                    "ICMP transfer succeeded with parallel phone call.")
            else:
                self.log.error(
                    "ICMP transfer failed with parallel phone call.")
            result = all(results)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None
            if self.tcpdump_proc[1] is not None:
                stop_adb_tcpdump(ads[1], self.tcpdump_proc[1], not result,
                                 self.test_name)
                self.tcpdump_proc[1] = None

    @test_tracker_info(uuid="a4a043c0-f4ba-4405-9262-42c752cc4487")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling to WiFi Calling test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Setup PhoneB WFC mode: WIFI_ONLY.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        return self._call_epdg_to_epdg_wfc(
            self.android_devices, False, WFC_MODE_WIFI_ONLY,
            self.wifi_network_ssid, self.wifi_network_pass)

    @test_tracker_info(uuid="ae171d58-d4c1-43f7-aa93-4860b4b28d53")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling to WiFi Calling test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Setup PhoneB WFC mode: WIFI_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        return self._call_epdg_to_epdg_wfc(
            self.android_devices, False, WFC_MODE_WIFI_PREFERRED,
            self.wifi_network_ssid, self.wifi_network_pass)

    @test_tracker_info(uuid="ece58857-fedc-49a9-bf10-b76bd78a51f2")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_wfc_cellular_preferred(self):
        """ Cellular Preferred, WiFi calling to WiFi Calling test

        1. Setup PhoneA WFC mode: CELLULAR_PREFERRED.
        2. Setup PhoneB WFC mode: CELLULAR_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            self.tcpdump_proc[1] = start_adb_tcpdump(ads[1], self.test_name)
            tasks = [(phone_setup_iwlan_cellular_preferred,
                      (self.log, ads[0], self.wifi_network_ssid,
                       self.wifi_network_pass)),
                     (phone_setup_iwlan_cellular_preferred,
                      (self.log, ads[1], self.wifi_network_ssid,
                       self.wifi_network_pass))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return False

            result = two_phone_call_short_seq(
                self.log, ads[0], None, is_phone_in_call_not_iwlan, ads[1],
                None, is_phone_in_call_not_iwlan, None,
                WAIT_TIME_IN_CALL_FOR_IMS)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None
            if self.tcpdump_proc[1] is not None:
                stop_adb_tcpdump(ads[1], self.tcpdump_proc[1], not result,
                                 self.test_name)
                self.tcpdump_proc[1] = None

    @test_tracker_info(uuid="0d63c250-d9e7-490c-8c48-0a6afbad5f88")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Setup PhoneB in airplane mode, WFC mode: WIFI_ONLY.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        return self._call_epdg_to_epdg_wfc(
            self.android_devices, True, WFC_MODE_WIFI_ONLY,
            self.wifi_network_ssid, self.wifi_network_pass)

    @test_tracker_info(uuid="7678e4ee-29c6-4319-93ab-d555501d1876")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Setup PhoneB in airplane mode, WFC mode: WIFI_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        return self._call_epdg_to_epdg_wfc(
            self.android_devices, True, WFC_MODE_WIFI_PREFERRED,
            self.wifi_network_ssid, self.wifi_network_pass)

    @test_tracker_info(uuid="8f5c637e-683a-448d-9443-b2b39626ab19")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_apm_wfc_cellular_preferred(self):
        """ Airplane + Cellular Preferred, WiFi calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: CELLULAR_PREFERRED.
        2. Setup PhoneB in airplane mode, WFC mode: CELLULAR_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        return self._call_epdg_to_epdg_wfc(
            self.android_devices, True, WFC_MODE_CELLULAR_PREFERRED,
            self.wifi_network_ssid, self.wifi_network_pass)

    @test_tracker_info(uuid="0b51666e-c83c-40b5-ba0f-737e64bc82a2")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_volte_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling to VoLTE test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Make Sure PhoneB is in LTE mode (with VoLTE enabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_volte, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return result

            result = two_phone_call_short_seq(
                self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                ads[1], phone_idle_volte, is_phone_in_call_volte, None,
                WAIT_TIME_IN_CALL_FOR_IMS)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None

    @test_tracker_info(uuid="6e0630a9-63b2-4ea1-8ec9-6560f001905c")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_volte_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling to VoLTE test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Make Sure PhoneB is in LTE mode (with VoLTE enabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_volte, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return result

            result = two_phone_call_short_seq(
                self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                ads[1], phone_idle_volte, is_phone_in_call_volte, None,
                WAIT_TIME_IN_CALL_FOR_IMS)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None

    @test_tracker_info(uuid="51077985-2229-491f-9a54-1ff53871758c")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_volte_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling to VoLTE test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Make Sure PhoneB is in LTE mode (with VoLTE enabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_volte, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return result

            result = two_phone_call_short_seq(
                self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                ads[1], phone_idle_volte, is_phone_in_call_volte, None,
                WAIT_TIME_IN_CALL_FOR_IMS)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None

    @test_tracker_info(uuid="fff9edcd-1ace-4f2d-a09b-06f3eea56cca")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_volte_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling to VoLTE test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Make Sure PhoneB is in LTE mode (with VoLTE enabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_volte, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return result

            result = two_phone_call_short_seq(
                self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                ads[1], phone_idle_volte, is_phone_in_call_volte, None,
                WAIT_TIME_IN_CALL_FOR_IMS)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None

    @test_tracker_info(uuid="8591554e-4e38-406c-97bf-8921d5329c47")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_csfb_3g_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling to CSFB 3G test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Make Sure PhoneB is in LTE mode (with VoLTE disabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            # Turn OFF WiFi for Phone B
            set_wifi_to_default(self.log, ads[1])
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_csfb, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return result

            result = two_phone_call_short_seq(
                self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                ads[1], phone_idle_csfb, is_phone_in_call_csfb, None)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None

    @test_tracker_info(uuid="9711888d-5b1e-4d05-86e9-98f94f46098b")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_csfb_3g_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling to CSFB 3G test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Make Sure PhoneB is in LTE mode (with VoLTE disabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            # Turn OFF WiFi for Phone B
            set_wifi_to_default(self.log, ads[1])
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_csfb, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return result

            result = two_phone_call_short_seq(
                self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                ads[1], phone_idle_csfb, is_phone_in_call_csfb, None)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None

    @test_tracker_info(uuid="902c96a4-858f-43ff-bd56-6d7d27004320")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_csfb_3g_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling to CSFB 3G test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Make Sure PhoneB is in LTE mode (with VoLTE disabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            # Turn OFF WiFi for Phone B
            set_wifi_to_default(self.log, ads[1])
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_csfb, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return result

            result = two_phone_call_short_seq(
                self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                ads[1], phone_idle_csfb, is_phone_in_call_csfb, None)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None

    @test_tracker_info(uuid="362a5396-ebda-4706-a73a-d805e5028fd7")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_csfb_3g_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling to CSFB 3G test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Make Sure PhoneB is in LTE mode (with VoLTE disabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            # Turn OFF WiFi for Phone B
            set_wifi_to_default(self.log, ads[1])
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_csfb, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return result

            result = two_phone_call_short_seq(
                self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                ads[1], phone_idle_csfb, is_phone_in_call_csfb, None)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None

    @test_tracker_info(uuid="647bb859-46bc-4e3e-b6ab-7944d3bbcc26")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_3g_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling to 3G test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            # Turn OFF WiFi for Phone B
            set_wifi_to_default(self.log, ads[1])
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_voice_3g, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return result

            result = two_phone_call_short_seq(
                self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                ads[1], phone_idle_3g, is_phone_in_call_3g, None)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None

    @test_tracker_info(uuid="3688ea1f-a52d-4a35-9df4-d5ed0985e49b")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_3g_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling to 3G test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            # Turn OFF WiFi for Phone B
            set_wifi_to_default(self.log, ads[1])
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_voice_3g, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return result

            result = two_phone_call_short_seq(
                self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                ads[1], phone_idle_3g, is_phone_in_call_3g, None)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None

    @test_tracker_info(uuid="f4efc821-fbaf-4ec2-b89b-5a47354344f0")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_3g_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling to 3G test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            # Turn OFF WiFi for Phone B
            set_wifi_to_default(self.log, ads[1])
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_voice_3g, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return False

            result = two_phone_call_short_seq(
                self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                ads[1], phone_idle_3g, is_phone_in_call_3g, None)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None

    @test_tracker_info(uuid="2b1345b7-3b62-44bd-91ad-9c5a4925b0e1")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_3g_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling to 3G test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        result = True
        try:
            self.tcpdump_proc[0] = start_adb_tcpdump(ads[0], self.test_name)
            # Turn OFF WiFi for Phone B
            set_wifi_to_default(self.log, ads[1])
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_voice_3g, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                result = False
                return False

            result = two_phone_call_short_seq(
                self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                ads[1], phone_idle_3g, is_phone_in_call_3g, None)
            return result
        finally:
            if self.tcpdump_proc[0] is not None:
                stop_adb_tcpdump(ads[0], self.tcpdump_proc[0], not result,
                                 self.test_name)
                self.tcpdump_proc[0] = None

    @test_tracker_info(uuid="7b3fea22-114a-442e-aa12-dde3b6001681")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_csfb_3g_to_csfb_3g(self):
        """ CSFB 3G to CSFB 3G call test

        1. Make Sure PhoneA is in LTE mode, VoLTE disabled.
        2. Make Sure PhoneB is in LTE mode, VoLTE disabled.
        3. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # Turn OFF WiFi for Phone B
        set_wifi_to_default(self.log, ads[1])
        tasks = [(phone_setup_csfb, (self.log, ads[0])), (phone_setup_csfb,
                                                          (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_csfb, is_phone_in_call_csfb, ads[1],
            phone_idle_csfb, is_phone_in_call_csfb, None)

    @test_tracker_info(uuid="91d751ea-40c8-4ffc-b9d3-03d0ad0902bd")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_3g_to_3g(self):
        """ 3G to 3G call test

        1. Make Sure PhoneA is in 3G mode.
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # Turn OFF WiFi for Phone B
        set_wifi_to_default(self.log, ads[1])
        tasks = [(phone_setup_voice_3g, (self.log, ads[0])),
                 (phone_setup_voice_3g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_3g, is_phone_in_call_3g, ads[1],
            phone_idle_3g, is_phone_in_call_3g, None)

    @test_tracker_info(uuid="df57c481-010a-4d21-a5c1-5116917871b2")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_long(self):
        """ VoLTE to VoLTE call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_volte, is_phone_in_call_volte, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="b0712d8a-71cf-405f-910c-8592da082660")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_long_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling to WiFi Calling test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Setup PhoneB WFC mode: WIFI_ONLY.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_iwlan, is_phone_in_call_iwlan, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="a7293d6c-0fdb-4842-984a-e4c6395fd41d")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_long_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling to WiFi Calling test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Setup PhoneB WFC mode: WIFI_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_iwlan, is_phone_in_call_iwlan, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="029af2a7-aba4-406b-9095-b32da57a7cdb")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_long_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Setup PhoneB in airplane mode, WFC mode: WIFI_ONLY.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_iwlan, is_phone_in_call_iwlan, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="3c751d79-7159-4407-a63c-96f835dd6cb0")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_long_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Setup PhoneB in airplane mode, WFC mode: WIFI_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_iwlan, is_phone_in_call_iwlan, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @test_tracker_info(uuid="9deab765-e2da-4826-bae8-ba8755551a1b")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_csfb_3g_to_csfb_3g_long(self):
        """ CSFB 3G to CSFB 3G call test

        1. Make Sure PhoneA is in LTE mode, VoLTE disabled.
        2. Make Sure PhoneB is in LTE mode, VoLTE disabled.
        3. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # Turn OFF WiFi for Phone B
        set_wifi_to_default(self.log, ads[1])
        tasks = [(phone_setup_csfb, (self.log, ads[0])), (phone_setup_csfb,
                                                          (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_csfb, is_phone_in_call_csfb, ads[1],
            phone_idle_csfb, is_phone_in_call_csfb, None)

    @test_tracker_info(uuid="54768178-818f-4126-9e50-4f49e43a6fd3")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_3g_to_3g_long(self):
        """ 3G to 3G call test

        1. Make Sure PhoneA is in 3G mode.
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # Turn OFF WiFi for Phone B
        set_wifi_to_default(self.log, ads[1])
        tasks = [(phone_setup_voice_3g, (self.log, ads[0])),
                 (phone_setup_voice_3g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_3g, is_phone_in_call_3g, ads[1],
            phone_idle_3g, is_phone_in_call_3g, None)

    @test_tracker_info(uuid="")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_loop(self):
        """ Stress test: VoLTE to VoLTE call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_volte, is_phone_in_call_volte,
                    ads[1], phone_idle_volte, is_phone_in_call_volte, None,
                    WAIT_TIME_IN_CALL_FOR_IMS):
                success_count += 1
                result_str = "Succeeded"

            else:
                fail_count += 1
                result_str = "Failed"

            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {} - {}%".format(
            success_count, fail_count,
            str(100 * success_count / (success_count + fail_count))))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    @test_tracker_info(uuid="dfa2c1a7-0e9a-42f2-b3ba-7e196df87e1b")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_loop_wfc_wifi_only(self):
        """ Stress test: WiFi Only, WiFi calling to WiFi Calling test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Setup PhoneB WFC mode: WIFI_ONLY.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                    ads[1], phone_idle_iwlan, is_phone_in_call_iwlan, None,
                    WAIT_TIME_IN_CALL_FOR_IMS):
                success_count += 1
                result_str = "Succeeded"

            else:
                fail_count += 1
                result_str = "Failed"

            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {} - {}%".format(
            success_count, fail_count,
            str(100 * success_count / (success_count + fail_count))))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    @test_tracker_info(uuid="382f97ad-65d4-4ebb-a31b-aa243e01bce4")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_loop_wfc_wifi_preferred(self):
        """ Stress test: WiFi Preferred, WiFi Calling to WiFi Calling test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Setup PhoneB WFC mode: WIFI_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                    ads[1], phone_idle_iwlan, is_phone_in_call_iwlan, None,
                    WAIT_TIME_IN_CALL_FOR_IMS):
                success_count += 1
                result_str = "Succeeded"

            else:
                fail_count += 1
                result_str = "Failed"

            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {} - {}%".format(
            success_count, fail_count,
            str(100 * success_count / (success_count + fail_count))))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    @test_tracker_info(uuid="c820e2ea-8a14-421c-b608-9074b716f7dd")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_loop_apm_wfc_wifi_only(self):
        """ Stress test: Airplane + WiFi Only, WiFi Calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Setup PhoneB in airplane mode, WFC mode: WIFI_ONLY.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                    ads[1], phone_idle_iwlan, is_phone_in_call_iwlan, None,
                    WAIT_TIME_IN_CALL_FOR_IMS):
                success_count += 1
                result_str = "Succeeded"

            else:
                fail_count += 1
                result_str = "Failed"

            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {} - {}%".format(
            success_count, fail_count,
            str(100 * success_count / (success_count + fail_count))))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    @test_tracker_info(uuid="3b8cb344-1551-4244-845d-b864501f2fb4")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_loop_apm_wfc_wifi_preferred(self):
        """ Stress test: Airplane + WiFi Preferred, WiFi Calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Setup PhoneB in airplane mode, WFC mode: WIFI_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                    ads[1], phone_idle_iwlan, is_phone_in_call_iwlan, None,
                    WAIT_TIME_IN_CALL_FOR_IMS):
                success_count += 1
                result_str = "Succeeded"

            else:
                fail_count += 1
                result_str = "Failed"

            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {} - {}%".format(
            success_count, fail_count,
            str(100 * success_count / (success_count + fail_count))))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    @test_tracker_info(uuid="")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_csfb_3g_to_csfb_3g_loop(self):
        """ Stress test: CSFB 3G to CSFB 3G call test

        1. Make Sure PhoneA is in LTE mode, VoLTE disabled.
        2. Make Sure PhoneB is in LTE mode, VoLTE disabled.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_csfb, (self.log, ads[0])), (phone_setup_csfb,
                                                          (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_csfb, is_phone_in_call_csfb,
                    ads[1], phone_idle_csfb, is_phone_in_call_csfb, None):
                success_count += 1
                result_str = "Succeeded"

            else:
                fail_count += 1
                result_str = "Failed"

            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {}".format(
            success_count, fail_count))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    @test_tracker_info(uuid="")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_3g_to_3g_loop(self):
        """ Stress test: 3G to 3G call test

        1. Make Sure PhoneA is in 3G mode
        2. Make Sure PhoneB is in 3G mode
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_voice_3g, (self.log, ads[0])),
                 (phone_setup_voice_3g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_3g, is_phone_in_call_3g,
                    ads[1], phone_idle_3g, is_phone_in_call_3g, None):
                success_count += 1
                result_str = "Succeeded"

            else:
                fail_count += 1
                result_str = "Failed"

            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {}".format(
            success_count, fail_count))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    def _hold_unhold_test(self, ads):
        """ Test hold/unhold functionality.

        PhoneA is in call with PhoneB. The call on PhoneA is active.
        Get call list on PhoneA.
        Hold call_id on PhoneA.
        Check call_id state.
        Unhold call_id on PhoneA.
        Check call_id state.

        Args:
            ads: List of android objects.
                This list should contain 2 android objects.
                ads[0] is the ad to do hold/unhold operation.

        Returns:
            True if pass; False if fail.
        """
        call_list = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(call_list))
        if num_active_calls(self.log, ads[0]) != 1:
            return False
        call_id = call_list[0]

        if ads[0].droid.telecomCallGetCallState(call_id) != CALL_STATE_ACTIVE:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_id, ads[0].droid.telecomCallGetCallState(call_id)))
            return False
        # TODO: b/26296375 add voice check.

        self.log.info("Hold call_id {} on PhoneA".format(call_id))
        ads[0].droid.telecomCallHold(call_id)
        time.sleep(WAIT_TIME_IN_CALL)
        if ads[0].droid.telecomCallGetCallState(call_id) != CALL_STATE_HOLDING:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_HOLDING".format(
                    call_id, ads[0].droid.telecomCallGetCallState(call_id)))
            return False
        # TODO: b/26296375 add voice check.

        self.log.info("Unhold call_id {} on PhoneA".format(call_id))
        ads[0].droid.telecomCallUnhold(call_id)
        time.sleep(WAIT_TIME_IN_CALL)
        if ads[0].droid.telecomCallGetCallState(call_id) != CALL_STATE_ACTIVE:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_id, ads[0].droid.telecomCallGetCallState(call_id)))
            return False
        # TODO: b/26296375 add voice check.

        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            self.log.error("Caller/Callee dropped call.")
            return False

        return True

    @test_tracker_info(uuid="4043c68a-c5d4-4e1d-9010-ef65b205cab1")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mo_hold_unhold_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling MO call hold/unhold test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ad_hangup=None,
                verify_caller_func=is_phone_in_call_iwlan,
                verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="0667535e-dcad-49f0-9b4b-fa45d6c75f5b")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mo_hold_unhold_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling MO call hold/unhold test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ad_hangup=None,
                verify_caller_func=is_phone_in_call_iwlan,
                verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="cf318b4c-c920-4e80-b73f-2f092c03a144")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mo_hold_unhold_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling MO call hold/unhold test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ad_hangup=None,
                verify_caller_func=is_phone_in_call_iwlan,
                verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="ace36801-1e7b-4f06-aa0b-17affc8df069")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mo_hold_unhold_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling MO call hold/unhold test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ad_hangup=None,
                verify_caller_func=is_phone_in_call_iwlan,
                verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="2ad32874-0d39-4475-8ae3-d6dccda675f5")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mt_hold_unhold_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling MT call hold/unhold test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[1],
                ads[0],
                ad_hangup=None,
                verify_caller_func=None,
                verify_callee_func=is_phone_in_call_iwlan):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="3efd5d59-30ee-45f5-8966-56ce8fadf9a1")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mt_hold_unhold_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling MT call hold/unhold test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[1],
                ads[0],
                ad_hangup=None,
                verify_caller_func=None,
                verify_callee_func=is_phone_in_call_iwlan):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="35ed0f89-7435-4d3b-9ebc-c5cdc3f7e32b")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mt_hold_unhold_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling MT call hold/unhold test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[1],
                ads[0],
                ad_hangup=None,
                verify_caller_func=None,
                verify_callee_func=is_phone_in_call_iwlan):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info("37ad003b-6426-42f7-b528-ec7c1842fd18")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mt_hold_unhold_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling MT call hold/unhold test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[1],
                ads[0],
                ad_hangup=None,
                verify_caller_func=None,
                verify_callee_func=is_phone_in_call_iwlan):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="fa37cd37-c30a-4caa-80b4-52507995ec77")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_mo_hold_unhold(self):
        """ VoLTE MO call hold/unhold test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is able to make/receive call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ad_hangup=None,
                verify_caller_func=is_phone_in_call_volte,
                verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="28a9acb3-83e8-4dd1-82bf-173da8bd2eca")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_mt_hold_unhold(self):
        """ VoLTE MT call hold/unhold test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is able to make/receive call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[1],
                ads[0],
                ad_hangup=None,
                verify_caller_func=None,
                verify_callee_func=is_phone_in_call_volte):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="ffe724ae-4223-4c15-9fed-9aba17de9a63")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_wcdma_mo_hold_unhold(self):
        """ MO WCDMA hold/unhold test

        1. Make Sure PhoneA is in 3G WCDMA mode.
        2. Make Sure PhoneB is able to make/receive call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma hold/unhold test.")
            return False

        tasks = [(phone_setup_voice_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ad_hangup=None,
                verify_caller_func=is_phone_in_call_3g,
                verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="23805165-01ce-4351-83d3-73c9fb3bda76")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_wcdma_mt_hold_unhold(self):
        """ MT WCDMA hold/unhold test

        1. Make Sure PhoneA is in 3G WCDMA mode.
        2. Make Sure PhoneB is able to make/receive call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma hold/unhold test.")
            return False

        tasks = [(phone_setup_voice_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[1],
                ads[0],
                ad_hangup=None,
                verify_caller_func=None,
                verify_callee_func=is_phone_in_call_3g):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="08c846c7-1978-4ece-8f2c-731129947699")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_csfb_mo_hold_unhold(self):
        """ MO CSFB WCDMA/GSM hold/unhold test

        1. Make Sure PhoneA is in LTE mode (VoLTE disabled).
        2. Make Sure PhoneB is able to make/receive call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma hold/unhold test.")
            return False

        tasks = [(phone_setup_csfb, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ad_hangup=None,
                verify_caller_func=is_phone_in_call_csfb,
                verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="a6405fe6-c732-4ae6-bbae-e912a124f4a2")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_csfb_mt_hold_unhold(self):
        """ MT CSFB WCDMA/GSM hold/unhold test

        1. Make Sure PhoneA is in LTE mode (VoLTE disabled).
        2. Make Sure PhoneB is able to make/receive call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma hold/unhold test.")
            return False

        tasks = [(phone_setup_csfb, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[1],
                ads[0],
                ad_hangup=None,
                verify_caller_func=None,
                verify_callee_func=is_phone_in_call_csfb):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="5edc5034-90ef-4113-926f-05407ed60a87")
    @TelephonyBaseTest.tel_test_wrap
    def test_erase_all_pending_voicemail(self):
        """Script for TMO/ATT/SPT phone to erase all pending voice mail.
        This script only works if phone have already set up voice mail options,
        and phone should disable password protection for voice mail.

        1. If phone don't have pending voice message, return True.
        2. Dial voice mail number.
            For TMO, the number is '123'.
            For ATT, the number is phone's number.
            For SPT, the number is phone's number.
        3. Use DTMF to delete all pending voice messages.
        4. Check telephonyGetVoiceMailCount result. it should be 0.

        Returns:
            False if error happens. True is succeed.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return call_voicemail_erase_all_pending_voicemail(
            self.log, self.android_devices[1])

    @test_tracker_info(uuid="c81156a2-089b-4b10-ba80-7afea61d06c6")
    @TelephonyBaseTest.tel_test_wrap
    def test_voicemail_indicator_volte(self):
        """Test Voice Mail notification in LTE (VoLTE enabled).
        This script currently only works for TMO now.

        1. Make sure DUT (ads[1]) in VoLTE mode. Both PhoneB (ads[0]) and DUT idle.
        2. Make call from PhoneB to DUT, reject on DUT.
        3. On PhoneB, leave a voice mail to DUT.
        4. Verify DUT receive voice mail notification.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_volte, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_voicemail_erase_all_pending_voicemail(self.log, ads[1]):
            self.log.error("Failed to clear voice mail.")
            return False

        return two_phone_call_leave_voice_mail(self.log, ads[0], None, None,
                                               ads[1], phone_idle_volte)

    @test_tracker_info(uuid="529e12cb-3178-4d2c-b155-d5cfb1eac0c9")
    @TelephonyBaseTest.tel_test_wrap
    def test_voicemail_indicator_lte(self):
        """Test Voice Mail notification in LTE (VoLTE disabled).
        This script currently only works for TMO/ATT/SPT now.

        1. Make sure DUT (ads[1]) in LTE (No VoLTE) mode. Both PhoneB (ads[0]) and DUT idle.
        2. Make call from PhoneB to DUT, reject on DUT.
        3. On PhoneB, leave a voice mail to DUT.
        4. Verify DUT receive voice mail notification.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_csfb, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_voicemail_erase_all_pending_voicemail(self.log, ads[1]):
            self.log.error("Failed to clear voice mail.")
            return False

        return two_phone_call_leave_voice_mail(self.log, ads[0], None, None,
                                               ads[1], phone_idle_csfb)

    @test_tracker_info(uuid="60cef7dd-f990-4913-af9a-75e9336fc80a")
    @TelephonyBaseTest.tel_test_wrap
    def test_voicemail_indicator_3g(self):
        """Test Voice Mail notification in 3G
        This script currently only works for TMO/ATT/SPT now.

        1. Make sure DUT (ads[1]) in 3G mode. Both PhoneB (ads[0]) and DUT idle.
        2. Make call from PhoneB to DUT, reject on DUT.
        3. On PhoneB, leave a voice mail to DUT.
        4. Verify DUT receive voice mail notification.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_voice_3g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_voicemail_erase_all_pending_voicemail(self.log, ads[1]):
            self.log.error("Failed to clear voice mail.")
            return False

        return two_phone_call_leave_voice_mail(self.log, ads[0], None, None,
                                               ads[1], phone_idle_3g)

    @test_tracker_info(uuid="e4c83cfa-db60-4258-ab69-15f7de3614b0")
    @TelephonyBaseTest.tel_test_wrap
    def test_voicemail_indicator_2g(self):
        """Test Voice Mail notification in 2G
        This script currently only works for TMO/ATT/SPT now.

        1. Make sure DUT (ads[0]) in 2G mode. Both PhoneB (ads[1]) and DUT idle.
        2. Make call from PhoneB to DUT, reject on DUT.
        3. On PhoneB, leave a voice mail to DUT.
        4. Verify DUT receive voice mail notification.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_2g, (self.log, ads[0]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_voicemail_erase_all_pending_voicemail(self.log, ads[0]):
            self.log.error("Failed to clear voice mail.")
            return False

        return two_phone_call_leave_voice_mail(self.log, ads[1], None, None,
                                               ads[0], phone_idle_2g)

    @test_tracker_info(uuid="f0cb02fb-a028-43da-9c87-5b21b2f8549b")
    @TelephonyBaseTest.tel_test_wrap
    def test_voicemail_indicator_iwlan(self):
        """Test Voice Mail notification in WiFI Calling
        This script currently only works for TMO now.

        1. Make sure DUT (ads[1]) in WFC mode. Both PhoneB (ads[0]) and DUT idle.
        2. Make call from PhoneB to DUT, reject on DUT.
        3. On PhoneB, leave a voice mail to DUT.
        4. Verify DUT receive voice mail notification.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_voicemail_erase_all_pending_voicemail(self.log, ads[1]):
            self.log.error("Failed to clear voice mail.")
            return False

        return two_phone_call_leave_voice_mail(self.log, ads[0], None, None,
                                               ads[1], phone_idle_iwlan)

    @test_tracker_info(uuid="9bd0550e-abfd-436b-912f-571810f973d7")
    @TelephonyBaseTest.tel_test_wrap
    def test_voicemail_indicator_apm_iwlan(self):
        """Test Voice Mail notification in WiFI Calling
        This script currently only works for TMO now.

        1. Make sure DUT (ads[1]) in APM WFC mode. Both PhoneB (ads[0]) and DUT idle.
        2. Make call from PhoneB to DUT, reject on DUT.
        3. On PhoneB, leave a voice mail to DUT.
        4. Verify DUT receive voice mail notification.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_voicemail_erase_all_pending_voicemail(self.log, ads[1]):
            self.log.error("Failed to clear voice mail.")
            return False

        return two_phone_call_leave_voice_mail(self.log, ads[0], None, None,
                                               ads[1], phone_idle_iwlan)

    @test_tracker_info(uuid="6bd5cf0f-522e-4e4a-99bf-92ae46261d8c")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_2g_to_2g(self):
        """ Test 2g<->2g call functionality.

        Make Sure PhoneA is in 2g mode.
        Make Sure PhoneB is in 2g mode.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_2g, (self.log, ads[0])),
                 (phone_setup_voice_2g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_2g, is_phone_in_call_2g, ads[1],
            phone_idle_2g, is_phone_in_call_2g, None)

    @test_tracker_info(uuid="947f3178-735b-4ac2-877c-a06a94972457")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_2g_to_2g_long(self):
        """ Test 2g<->2g call functionality.

        Make Sure PhoneA is in 2g mode.
        Make Sure PhoneB is in 2g mode.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_2g, (self.log, ads[0])),
                 (phone_setup_voice_2g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_2g, is_phone_in_call_2g, ads[1],
            phone_idle_2g, is_phone_in_call_2g, None)

    @test_tracker_info(uuid="d109df55-ac2f-493f-9324-9be1d3d7d6d3")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_gsm_mo_hold_unhold(self):
        """ Test GSM call hold/unhold functionality.

        Make Sure PhoneA is in 2g mode (GSM).
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma hold/unhold test.")
            return False

        tasks = [(phone_setup_voice_2g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ad_hangup=None,
                verify_caller_func=is_phone_in_call_2g,
                verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @test_tracker_info(uuid="a8279cda-73b3-470a-8ca7-a331ef99270b")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_gsm_mt_hold_unhold(self):
        """ Test GSM call hold/unhold functionality.

        Make Sure PhoneA is in 2g mode (GSM).
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneB to PhoneA, accept on PhoneA, hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma hold/unhold test.")
            return False

        tasks = [(phone_setup_voice_2g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty.".format(
                ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(
                self.log,
                ads[1],
                ads[0],
                ad_hangup=None,
                verify_caller_func=None,
                verify_callee_func=is_phone_in_call_2g):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    def _test_call_long_duration(self, dut_incall_check_func, total_duration):
        ads = self.android_devices
        self.log.info("Long Duration Call Test. Total duration = {}".format(
            total_duration))
        return call_setup_teardown(
            self.log,
            ads[0],
            ads[1],
            ads[0],
            verify_caller_func=dut_incall_check_func,
            wait_time_in_call=total_duration)

    @test_tracker_info(uuid="d0008b51-25ed-414a-9b82-3ffb139a6e0d")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_long_duration_volte(self):
        """ Test call drop rate for VoLTE long duration call.

        Steps:
        1. Setup VoLTE for DUT.
        2. Make VoLTE call from DUT to PhoneB.
        3. For <total_duration> time, check if DUT drop call or not.

        Expected Results:
        DUT should not drop call.

        Returns:
        False if DUT call dropped during test.
        Otherwise True.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._test_call_long_duration(
            is_phone_in_call_volte, self.long_duration_call_total_duration)

    @test_tracker_info(uuid="d4c1aec0-df05-403f-954c-496faf18605a")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_long_duration_wfc(self):
        """ Test call drop rate for WiFi Calling long duration call.

        Steps:
        1. Setup WFC for DUT.
        2. Make WFC call from DUT to PhoneB.
        3. For <total_duration> time, check if DUT drop call or not.

        Expected Results:
        DUT should not drop call.

        Returns:
        False if DUT call dropped during test.
        Otherwise True.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._test_call_long_duration(
            is_phone_in_call_iwlan, self.long_duration_call_total_duration)

    @test_tracker_info(uuid="bc44f3ca-2616-4024-b959-3a5a85503dfd")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_long_duration_3g(self):
        """ Test call drop rate for 3G long duration call.

        Steps:
        1. Setup 3G for DUT.
        2. Make CS call from DUT to PhoneB.
        3. For <total_duration> time, check if DUT drop call or not.

        Expected Results:
        DUT should not drop call.

        Returns:
        False if DUT call dropped during test.
        Otherwise True.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._test_call_long_duration(
            is_phone_in_call_3g, self.long_duration_call_total_duration)

    def _test_call_hangup_while_ringing(self, ad_caller, ad_callee):
        """ Call a phone and verify ringing, then hangup from the originator

        1. Setup PhoneA and PhoneB to ensure voice service.
        2. Call from PhoneA to PhoneB and wait for ringing.
        3. End the call on PhoneA.

        Returns:
            True if pass; False if fail.
        """

        caller_number = ad_caller.cfg['subscription'][
            get_outgoing_voice_sub_id(ad_caller)]['phone_num']
        callee_number = ad_callee.cfg['subscription'][
            get_incoming_voice_sub_id(ad_callee)]['phone_num']

        tasks = [(phone_setup_voice_general, (self.log, ad_caller)),
                 (phone_setup_voice_general, (self.log, ad_callee))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ad_caller.droid.telecomCallClearCallList()
        if num_active_calls(self.log, ad_caller) != 0:
            self.log.error("Phone {} has ongoing calls.".format(
                ad_caller.serial))
            return False

        if not initiate_call(self.log, ad_caller, callee_number):
            self.log.error("Phone was {} unable to initate a call".format(
                ads[0].serial))
            return False

        if not wait_for_ringing_call(self.log, ad_callee, caller_number):
            self.log.error("Phone {} never rang.".format(ad_callee.serial))
            return False

        if not hangup_call(self.log, ad_caller):
            self.log.error("Unable to hang up the call")
            return False

        return True

    @test_tracker_info(uuid="ef4fb42d-9040-46f2-9626-d0a2e1dd854f")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_hangup_while_ringing(self):
        """ Call a phone and verify ringing, then hangup from the originator

        1. Setup PhoneA and PhoneB to ensure voice service.
        2. Call from PhoneA to PhoneB and wait for ringing.
        3. End the call on PhoneA.

        Returns:
            True if pass; False if fail.
        """

        return self._test_call_hangup_while_ringing(self.android_devices[0],
                                                    self.android_devices[1])

    @test_tracker_info(uuid="f514ac72-d551-4e21-b5af-bd87b6cdf34a")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_hangup_while_ringing(self):
        """ Call a phone and verify ringing, then hangup from the originator

        1. Setup PhoneA and PhoneB to ensure voice service.
        2. Call from PhoneB to PhoneA and wait for ringing.
        3. End the call on PhoneB.

        Returns:
            True if pass; False if fail.
        """

        return self._test_call_hangup_while_ringing(self.android_devices[1],
                                                    self.android_devices[0])

    def _test_call_setup_in_active_data_transfer(
            self,
            nw_gen=None,
            call_direction=DIRECTION_MOBILE_ORIGINATED,
            allow_data_transfer_interruption=False):
        """Test call can be established during active data connection.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting downloading file from Internet.
        Initiate a voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.
        Note: file download will be suspended when call is initiated if voice
              is using voice channel and voice channel and data channel are
              on different RATs.

        Returns:
            True if success.
            False if failed.
        """

        def _call_setup_teardown(log, ad_caller, ad_callee, ad_hangup,
                                 caller_verifier, callee_verifier,
                                 wait_time_in_call):
            #wait time for active data transfer
            time.sleep(5)
            return call_setup_teardown(log, ad_caller, ad_callee, ad_hangup,
                                       caller_verifier, callee_verifier,
                                       wait_time_in_call)

        if nw_gen:
            if not ensure_network_generation(
                    self.log, self.android_devices[0], nw_gen,
                    MAX_WAIT_TIME_NW_SELECTION, NETWORK_SERVICE_DATA):
                self.log.error("Device failed to reselect in %s.",
                               MAX_WAIT_TIME_NW_SELECTION)
                return False

            self.android_devices[0].droid.telephonyToggleDataConnection(True)
            if not wait_for_cell_data_connection(
                    self.log, self.android_devices[0], True):
                self.log.error("Data connection is not on cell")
                return False

        if not verify_http_connection(self.log, self.android_devices[0]):
            self.log.error("HTTP connection is not available")
            return False

        if call_direction == DIRECTION_MOBILE_ORIGINATED:
            ad_caller = self.android_devices[0]
            ad_callee = self.android_devices[1]
        else:
            ad_caller = self.android_devices[1]
            ad_callee = self.android_devices[0]
        ad_download = self.android_devices[0]

        ad_download.ensure_screen_on()
        ad_download.adb.shell('am start -a android.intent.action.VIEW -d '
                              '"https://www.youtube.com/watch?v=VHF-XK0Vg1s"')
        if wait_for_state(ad_download.droid.audioIsMusicActive, True, 15, 1):
            ad_download.log.info("Before call, audio is in MUSIC_state")
        else:
            ad_download.log.warning("Before call, audio is not in MUSIC state")
        call_task = (_call_setup_teardown, (self.log, ad_caller, ad_callee,
                                            ad_caller, None, None, 30))
        download_task = active_file_download_task(self.log, ad_download)
        results = run_multithread_func(self.log, [download_task, call_task])
        if wait_for_state(ad_download.droid.audioIsMusicActive, True, 15, 1):
            ad_download.log.info("After call hangup, audio is back to music")
        else:
            ad_download.log.warning(
                "After call hang up, audio is not back to music")
        ad_download.force_stop_apk("com.google.android.youtube")
        if not results[1]:
            self.log.error("Call setup failed in active data transfer.")
            return False
        if results[0]:
            self.log.info("Data transfer succeeded.")
            return True
        elif not allow_data_transfer_interruption:
            self.log.error("Data transfer failed with parallel phone call.")
            return False
        else:
            ad_download.log.info("Retry data transfer after call hung up")
            return download_task[0](*download_task[1])

    @test_tracker_info(uuid="aa40e7e1-e64a-480b-86e4-db2242449555")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_general_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting downloading file from Internet.
        Initiate a MO voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.
        Note: file download will be suspended when call is initiated if voice
              is using voice channel and voice channel and data channel are
              on different RATs.

        Returns:
            True if success.
            False if failed.
        """
        return self._test_call_setup_in_active_data_transfer(
            None, DIRECTION_MOBILE_ORIGINATED)

    @test_tracker_info(uuid="d750d66b-2091-4e8d-baa2-084b9d2bbff5")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_general_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting downloading file from Internet.
        Initiate a MT voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.
        Note: file download will be suspended when call is initiated if voice
              is using voice channel and voice channel and data channel are
              on different RATs.

        Returns:
            True if success.
            False if failed.
        """
        return self._test_call_setup_in_active_data_transfer(
            None, DIRECTION_MOBILE_TERMINATED)

    @test_tracker_info(uuid="35703e83-b3e6-40af-aeaf-6b983d6205f4")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_volte_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting downloading file from Internet.
        Initiate a MO voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.
        Note: file download will be suspended when call is initiated if voice
              is using voice channel and voice channel and data channel are
              on different RATs.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_volte(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup VoLTE")
            return False
        return self._test_call_setup_in_active_data_transfer(
            GEN_4G, DIRECTION_MOBILE_ORIGINATED)

    @test_tracker_info(uuid="a0f658d9-4212-44db-b3e8-7202f1eec04d")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_volte_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting downloading file from Internet.
        Initiate a MT voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.
        Note: file download will be suspended when call is initiated if voice
              is using voice channel and voice channel and data channel are
              on different RATs.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_volte(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup VoLTE")
            return False
        return self._test_call_setup_in_active_data_transfer(
            GEN_4G, DIRECTION_MOBILE_TERMINATED)

    @test_tracker_info(uuid="e0b264ec-fc29-411e-b018-684b7ff5a37e")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_csfb_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting downloading file from Internet.
        Initiate a MO voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.
        Note: file download will be suspended when call is initiated if voice
              is using voice channel and voice channel and data channel are
              on different RATs.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_csfb(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup VoLTE")
            return False
        return self._test_call_setup_in_active_data_transfer(
            GEN_4G,
            DIRECTION_MOBILE_ORIGINATED,
            allow_data_transfer_interruption=True)

    @test_tracker_info(uuid="98f04a27-74e1-474d-90d1-a4a45cdb6f5b")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_csfb_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting downloading file from Internet.
        Initiate a MT voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.
        Note: file download will be suspended when call is initiated if voice
              is using voice channel and voice channel and data channel are
              on different RATs.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_csfb(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup VoLTE")
            return False
        return self._test_call_setup_in_active_data_transfer(
            GEN_4G,
            DIRECTION_MOBILE_TERMINATED,
            allow_data_transfer_interruption=True)

    @test_tracker_info(uuid="359b1ee1-36a6-427b-9d9e-4d77231fcb09")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_3g_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting downloading file from Internet.
        Initiate a MO voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.
        Note: file download will be suspended when call is initiated if voice
              is using voice channel and voice channel and data channel are
              on different RATs.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_voice_3g(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup 3G")
            return False
        return self._test_call_setup_in_active_data_transfer(
            GEN_3G,
            DIRECTION_MOBILE_ORIGINATED,
            allow_data_transfer_interruption=True)

    @test_tracker_info(uuid="b172bbb4-2d6e-4d83-a381-ebfdf23bc30e")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_3g_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting downloading file from Internet.
        Initiate a MT voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.
        Note: file download will be suspended when call is initiated if voice
              is using voice channel and voice channel and data channel are
              on different RATs.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_voice_3g(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup 3G")
            return False
        return self._test_call_setup_in_active_data_transfer(
            GEN_3G,
            DIRECTION_MOBILE_TERMINATED,
            allow_data_transfer_interruption=True)

    @test_tracker_info(uuid="f5d9bfd0-0996-4c18-b11e-c6113dc201e2")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_2g_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting downloading file from Internet.
        Initiate a MO voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.
        Note: file download will be suspended when call is initiated if voice
              is using voice channel and voice channel and data channel are
              on different RATs.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_voice_2g(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup voice in 2G")
            return False
        return self._test_call_setup_in_active_data_transfer(
            GEN_2G,
            DIRECTION_MOBILE_ORIGINATED,
            allow_data_transfer_interruption=True)

    @test_tracker_info(uuid="99cfd1be-b992-48bf-a50e-fc3eec8e5a67")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_2g_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting downloading file from Internet.
        Initiate a MT voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.
        Note: file download will be suspended when call is initiated if voice
              is using voice channel and voice channel and data channel are
              on different RATs.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_voice_2g(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup voice in 2G")
            return False
        return self._test_call_setup_in_active_data_transfer(
            GEN_2G,
            DIRECTION_MOBILE_TERMINATED,
            allow_data_transfer_interruption=True)

    @test_tracker_info(uuid="12677cf2-40d3-4bb1-8afa-91ebcbd0f862")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_wifi_wfc_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn off airplane mode, turn on wfc and wifi.
        Starting downloading file from Internet.
        Initiate a MO voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_iwlan(self.log, self.android_devices[0], False,
                                 WFC_MODE_WIFI_PREFERRED,
                                 self.wifi_network_ssid,
                                 self.wifi_network_pass):
            self.android_devices[0].log.error(
                "Failed to setup IWLAN with NON-APM WIFI WFC on")
            return False
        return self._test_call_setup_in_active_data_transfer(
            None, DIRECTION_MOBILE_ORIGINATED)

    @test_tracker_info(uuid="84adcc19-43bb-4ea3-9284-7322ab139aac")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_wifi_wfc_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn off airplane mode, turn on wfc and wifi.
        Starting downloading file from Internet.
        Initiate a MT voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_iwlan(self.log, self.android_devices[0], False,
                                 WFC_MODE_WIFI_PREFERRED,
                                 self.wifi_network_ssid,
                                 self.wifi_network_pass):
            self.android_devices[0].log.error(
                "Failed to setup iwlan with APM off and WIFI and WFC on")
            return False
        return self._test_call_setup_in_active_data_transfer(
            None, DIRECTION_MOBILE_TERMINATED)

    @test_tracker_info(uuid="42566255-c33f-406c-abab-932a0aaa01a8")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_apm_wifi_wfc_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn on wifi-calling, airplane mode and wifi.
        Starting downloading file from Internet.
        Initiate a MO voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_iwlan(self.log, self.android_devices[0], True,
                                 WFC_MODE_WIFI_PREFERRED,
                                 self.wifi_network_ssid,
                                 self.wifi_network_pass):
            self.android_devices[0].log.error(
                "Failed to setup iwlan with APM, WIFI and WFC on")
            return False
        return self._test_call_setup_in_active_data_transfer(
            None, DIRECTION_MOBILE_ORIGINATED)

    @test_tracker_info(uuid="fbf52f60-449b-46f2-9486-36d338a1b070")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_apm_wifi_wfc_in_active_data_transfer(self):
        """Test call can be established during active data connection.

        Turn on wifi-calling, airplane mode and wifi.
        Starting downloading file from Internet.
        Initiate a MT voice call. Verify call can be established.
        Hangup Voice Call, verify file is downloaded successfully.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_iwlan(self.log, self.android_devices[0], True,
                                 WFC_MODE_WIFI_PREFERRED,
                                 self.wifi_network_ssid,
                                 self.wifi_network_pass):
            self.android_devices[0].log.error(
                "Failed to setup iwlan with APM, WIFI and WFC on")
            return False
        return self._test_call_setup_in_active_data_transfer(
            None, DIRECTION_MOBILE_TERMINATED)

    def _test_call_setup_in_active_youtube_video(
            self,
            nw_gen=None,
            call_direction=DIRECTION_MOBILE_ORIGINATED,
            allow_data_transfer_interruption=False):
        """Test call can be established during active data connection.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting playing youtube video.
        Initiate a voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        if nw_gen:
            if not ensure_network_generation(
                    self.log, self.android_devices[0], nw_gen,
                    MAX_WAIT_TIME_NW_SELECTION, NETWORK_SERVICE_DATA):
                self.log.error("Device failed to reselect in %s.",
                               MAX_WAIT_TIME_NW_SELECTION)
                return False
        else:
            ensure_phones_default_state(self.log, self.android_devices)
        self.android_devices[0].droid.telephonyToggleDataConnection(True)
        if not wait_for_cell_data_connection(self.log, self.android_devices[0],
                                             True):
            self.log.error("Data connection is not on cell")
            return False

        if not verify_http_connection(self.log, self.android_devices[0]):
            self.log.error("HTTP connection is not available")
            return False

        if call_direction == DIRECTION_MOBILE_ORIGINATED:
            ad_caller = self.android_devices[0]
            ad_callee = self.android_devices[1]
        else:
            ad_caller = self.android_devices[1]
            ad_callee = self.android_devices[0]
        ad_download = self.android_devices[0]

        ad_download.log.info("Open an youtube video")
        ad_download.ensure_screen_on()
        ad_download.adb.shell('am start -a android.intent.action.VIEW -d '
                              '"https://www.youtube.com/watch?v=VHF-XK0Vg1s"')
        if wait_for_state(ad_download.droid.audioIsMusicActive, True, 15, 1):
            ad_download.log.info("Before call, audio is in MUSIC_state")
        else:
            ad_download.log.warning("Before call, audio is not in MUSIC state")

        if not call_setup_teardown(self.log, ad_caller, ad_callee, ad_caller,
                                   None, None, 30):
            self.log.error("Call setup failed in active youtube video")
            result = False
        else:
            self.log.info("Call setup succeed in active youtube video")
            result = True

        if wait_for_state(ad_download.droid.audioIsMusicActive, True, 15, 1):
            ad_download.log.info("After call hangup, audio is back to music")
        else:
            ad_download.log.warning(
                "After call hang up, audio is not back to music")
        ad_download.force_stop_apk("com.google.android.youtube")
        return result

    @test_tracker_info(uuid="1dc9f03f-1b6c-4c17-993b-3acafdc26ea3")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_general_in_active_youtube_video(self):
        """Test call can be established during active youtube video.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting an youtube video.
        Initiate a MO voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        return self._test_call_setup_in_active_youtube_video(
            None, DIRECTION_MOBILE_ORIGINATED)

    @test_tracker_info(uuid="32bc8fab-a0b9-4d47-8afb-940d1fdcde02")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_general_in_active_youtube_video(self):
        """Test call can be established during active youtube video.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting an youtube video.
        Initiate a MT voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        return self._test_call_setup_in_active_youtube_video(
            None, DIRECTION_MOBILE_TERMINATED)

    @test_tracker_info(uuid="72204212-e0c8-4447-be3f-ae23b2a63a1c")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_volte_in_active_youtube_video(self):
        """Test call can be established during active youtube video.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting an youtube video.
        Initiate a MO voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_volte(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup VoLTE")
            return False
        return self._test_call_setup_in_active_youtube_video(
            GEN_4G, DIRECTION_MOBILE_ORIGINATED)

    @test_tracker_info(uuid="84cd3ab9-a2b2-4ef9-b531-ee6201bec128")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_volte_in_active_youtube_video(self):
        """Test call can be established during active youtube video.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting an youtube video.
        Initiate a MT voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_volte(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup VoLTE")
            return False
        return self._test_call_setup_in_active_youtube_video(
            GEN_4G, DIRECTION_MOBILE_TERMINATED)

    @test_tracker_info(uuid="a8dca8d3-c44c-40a6-be56-931b4be5499b")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_csfb_in_active_youtube_video(self):
        """Test call can be established during active youbube video.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting an youtube video.
        Initiate a MO voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_csfb(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup VoLTE")
            return False
        return self._test_call_setup_in_active_youtube_video(
            GEN_4G,
            DIRECTION_MOBILE_ORIGINATED,
            allow_data_transfer_interruption=True)

    @test_tracker_info(uuid="d11f7263-f51d-4ea3-916a-0df4f52023ce")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_csfb_in_active_youtube_video(self):
        """Test call can be established during active youtube video.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting an youtube video.
        Initiate a MT voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_csfb(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup VoLTE")
            return False
        return self._test_call_setup_in_active_youtube_video(
            GEN_4G,
            DIRECTION_MOBILE_TERMINATED,
            allow_data_transfer_interruption=True)

    @test_tracker_info(uuid="676378b4-94b7-4ad7-8242-7ccd2bf1efba")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_3g_in_active_youtube_video(self):
        """Test call can be established during active youtube video.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting an youtube video.
        Initiate a MO voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_voice_3g(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup 3G")
            return False
        return self._test_call_setup_in_active_youtube_video(
            GEN_3G,
            DIRECTION_MOBILE_ORIGINATED,
            allow_data_transfer_interruption=True)

    @test_tracker_info(uuid="6216fc6d-2aa2-4eb9-90e2-5791cb31c12e")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_3g_in_active_youtube_video(self):
        """Test call can be established during active youtube video.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting youtube video.
        Initiate a MT voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_voice_3g(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup 3G")
            return False
        return self._test_call_setup_in_active_youtube_video(
            GEN_3G,
            DIRECTION_MOBILE_TERMINATED,
            allow_data_transfer_interruption=True)

    @test_tracker_info(uuid="58ec9783-6f8e-49f6-8dae-9dd33108b6f9")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_2g_in_active_youtube_video(self):
        """Test call can be established during active youtube video.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting youtube video.
        Initiate a MO voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_voice_2g(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup voice in 2G")
            return False
        return self._test_call_setup_in_active_youtube_video(
            GEN_2G,
            DIRECTION_MOBILE_ORIGINATED,
            allow_data_transfer_interruption=True)

    @test_tracker_info(uuid="e8ba7c0c-48a3-4fc6-aa34-a2e1c570521a")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_2g_in_active_youtube_video(self):
        """Test call can be established during active youtube video.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>.
        Starting an youtube video.
        Initiate a MT voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_voice_2g(self.log, self.android_devices[0]):
            self.android_devices[0].log.error("Failed to setup voice in 2G")
            return False
        return self._test_call_setup_in_active_youtube_video(
            GEN_2G,
            DIRECTION_MOBILE_TERMINATED,
            allow_data_transfer_interruption=True)

    @test_tracker_info(uuid="eb8971c1-b34a-430f-98df-0d4554c7ab12")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_wifi_wfc_in_active_youtube_video(self):
        """Test call can be established during active youtube video.

        Turn off airplane mode, turn on wfc and wifi.
        Starting youtube video.
        Initiate a MO voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_iwlan(self.log, self.android_devices[0], False,
                                 WFC_MODE_WIFI_PREFERRED,
                                 self.wifi_network_ssid,
                                 self.wifi_network_pass):
            self.android_devices[0].log.error(
                "Failed to setup IWLAN with NON-APM WIFI WFC on")
            return False
        return self._test_call_setup_in_active_youtube_video(
            None, DIRECTION_MOBILE_ORIGINATED)

    @test_tracker_info(uuid="275a93d6-1f39-40c8-893f-ff77afd09e54")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_wifi_wfc_in_active_youtube_video(self):
        """Test call can be established during active youtube_video.

        Turn off airplane mode, turn on wfc and wifi.
        Starting an youtube video.
        Initiate a MT voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_iwlan(self.log, self.android_devices[0], False,
                                 WFC_MODE_WIFI_PREFERRED,
                                 self.wifi_network_ssid,
                                 self.wifi_network_pass):
            self.android_devices[0].log.error(
                "Failed to setup iwlan with APM off and WIFI and WFC on")
            return False
        return self._test_call_setup_in_active_youtube_video(
            None, DIRECTION_MOBILE_TERMINATED)

    @test_tracker_info(uuid="ea087709-d4df-4223-b80c-1b33bacbd5a2")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_apm_wifi_wfc_in_active_youtube_video(self):
        """Test call can be established during active youtube video.

        Turn on wifi-calling, airplane mode and wifi.
        Starting an youtube video.
        Initiate a MO voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_iwlan(self.log, self.android_devices[0], True,
                                 WFC_MODE_WIFI_PREFERRED,
                                 self.wifi_network_ssid,
                                 self.wifi_network_pass):
            self.android_devices[0].log.error(
                "Failed to setup iwlan with APM, WIFI and WFC on")
            return False
        return self._test_call_setup_in_active_youtube_video(
            None, DIRECTION_MOBILE_ORIGINATED)

    @test_tracker_info(uuid="44cc14e0-60c7-4fdb-ad26-31fdc4e52aaf")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_apm_wifi_wfc_in_active_youtube_video(self):
        """Test call can be established during active youtube video.

        Turn on wifi-calling, airplane mode and wifi.
        Starting youtube video.
        Initiate a MT voice call. Verify call can be established.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_iwlan(self.log, self.android_devices[0], True,
                                 WFC_MODE_WIFI_PREFERRED,
                                 self.wifi_network_ssid,
                                 self.wifi_network_pass):
            self.android_devices[0].log.error(
                "Failed to setup iwlan with APM, WIFI and WFC on")
            return False
        return self._test_call_setup_in_active_youtube_video(
            None, DIRECTION_MOBILE_TERMINATED)

    @test_tracker_info(uuid="f367de12-1fd8-488d-816f-091deaacb791")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_wfc_wifi_preferred_after_mobile_data_usage_limit_reached(
            self):
        """ WiFi Preferred, WiFi calling test after data limit reached

        1. Set the data limit to the current usage
        2. Setup PhoneA WFC mode: WIFI_PREFERRED.
        3. Make Sure PhoneB is in 3G mode.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        5. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        try:
            subscriber_id = ads[0].droid.telephonyGetSubscriberId()
            data_usage = get_mobile_data_usage(ads[0], subscriber_id)
            set_mobile_data_usage_limit(ads[0], data_usage, subscriber_id)

            # Turn OFF WiFi for Phone B
            set_wifi_to_default(self.log, ads[1])
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_voice_3g, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                return False

            return two_phone_call_short_seq(
                self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                ads[1], phone_idle_3g, is_phone_in_call_3g, None)
        finally:
            remove_mobile_data_usage_limit(ads[0], subscriber_id)

    @test_tracker_info(uuid="af943c7f-2b42-408f-b8a3-2d360a7483f7")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_after_mobile_data_usage_limit_reached(self):
        """ VoLTE to VoLTE call test after mobile data usage limit reached

        1. Set the data limit to the current usage
        2. Make Sure PhoneA is in LTE mode (with VoLTE).
        3. Make Sure PhoneB is in LTE mode (with VoLTE).
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        5. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        try:
            subscriber_id = ads[0].droid.telephonyGetSubscriberId()
            data_usage = get_mobile_data_usage(ads[0], subscriber_id)
            set_mobile_data_usage_limit(ads[0], data_usage, subscriber_id)

            tasks = [(phone_setup_volte, (self.log, ads[0])),
                     (phone_setup_volte, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                return False

            return two_phone_call_short_seq(
                self.log, ads[0], phone_idle_volte, is_phone_in_call_volte,
                ads[1], phone_idle_volte, is_phone_in_call_volte, None,
                WAIT_TIME_IN_CALL_FOR_IMS)
        finally:
            remove_mobile_data_usage_limit(ads[0], subscriber_id)


""" Tests End """
