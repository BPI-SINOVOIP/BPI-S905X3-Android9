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
    Test Script for Telephony Stress Call Test
"""

import collections
import time
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_defines import VT_STATE_BIDIRECTIONAL
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import ensure_phone_subscription
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import set_wfc_mode
from acts.test_utils.tel.tel_test_utils import sms_send_receive_verify
from acts.test_utils.tel.tel_test_utils import start_qxdm_loggers
from acts.test_utils.tel.tel_test_utils import verify_incall_state
from acts.test_utils.tel.tel_test_utils import multithread_func
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_2g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_csfb
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_2g
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.tel.tel_voice_utils import phone_idle_iwlan
from acts.test_utils.tel.tel_video_utils import phone_setup_video
from acts.test_utils.tel.tel_video_utils import video_call_setup
from acts.test_utils.tel.tel_video_utils import \
    is_phone_in_call_video_bidirectional
from acts.logger import epoch_to_log_line_timestamp
from acts.utils import get_current_epoch_time
from acts.utils import rand_ascii_str


class TelLiveStressCallTest(TelephonyBaseTest):
    def setup_class(self):
        super(TelLiveStressCallTest, self).setup_class()
        self.caller = self.android_devices[0]
        self.callee = self.android_devices[1]
        self.number_of_devices = 2
        self.user_params["telephony_auto_rerun"] = False
        self.wifi_network_ssid = self.user_params.get(
            "wifi_network_ssid") or self.user_params.get("wifi_network_ssid_2g")
        self.wifi_network_pass = self.user_params.get(
            "wifi_network_pass") or self.user_params.get("wifi_network_pass_2g")
        self.phone_call_iteration = int(
            self.user_params.get("phone_call_iteration", 500))
        self.phone_call_duration = int(
            self.user_params.get("phone_call_duration", 60))
        self.sleep_time_between_test_iterations = int(
            self.user_params.get("sleep_time_between_test_iterations", 0))

        return True

    def on_fail(self, test_name, begin_time):
        pass

    def _setup_wfc(self):
        for ad in self.android_devices:
            if not ensure_wifi_connected(
                    ad.log,
                    ad,
                    self.wifi_network_ssid,
                    self.wifi_network_pass,
                    retries=3):
                ad.log.error("Phone Wifi connection fails.")
                return False
            ad.log.info("Phone WIFI is connected successfully.")
            if not set_wfc_mode(self.log, ad, WFC_MODE_WIFI_PREFERRED):
                ad.log.error("Phone failed to enable Wifi-Calling.")
                return False
            ad.log.info("Phone is set in Wifi-Calling successfully.")
            if not phone_idle_iwlan(self.log, ad):
                ad.log.error("Phone is not in WFC enabled state.")
                return False
            ad.log.info("Phone is in WFC enabled state.")
        return True

    def _setup_wfc_apm(self):
        for ad in self.android_devices:
            toggle_airplane_mode(ad.log, ad, True)
            if not ensure_wifi_connected(
                    ad.log,
                    ad,
                    self.wifi_network_ssid,
                    self.wifi_network_pass,
                    retries=3):
                ad.log.error("Phone Wifi connection fails.")
                return False
            ad.log.info("Phone WIFI is connected successfully.")
            if not set_wfc_mode(self.log, ad, WFC_MODE_WIFI_PREFERRED):
                ad.log.error("Phone failed to enable Wifi-Calling.")
                return False
            ad.log.info("Phone is set in Wifi-Calling successfully.")
            if not phone_idle_iwlan(self.log, ad):
                ad.log.error("Phone is not in WFC enabled state.")
                return False
            ad.log.info("Phone is in WFC enabled state.")
        return True

    def _setup_vt(self):
        ads = self.android_devices
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        return True

    def _setup_lte_volte_enabled(self):
        for ad in self.android_devices:
            if not phone_setup_volte(self.log, ad):
                ad.log.error("Phone failed to enable VoLTE.")
                return False
            ad.log.info("Phone VOLTE is enabled successfully.")
        return True

    def _setup_lte_volte_disabled(self):
        for ad in self.android_devices:
            if not phone_setup_csfb(self.log, ad):
                ad.log.error("Phone failed to setup CSFB.")
                return False
            ad.log.info("Phone VOLTE is disabled successfully.")
        return True

    def _setup_3g(self):
        for ad in self.android_devices:
            if not phone_setup_voice_3g(self.log, ad):
                ad.log.error("Phone failed to setup 3g.")
                return False
            ad.log.info("Phone RAT 3G is enabled successfully.")
        return True

    def _setup_2g(self):
        for ad in self.android_devices:
            if not phone_setup_voice_2g(self.log, ad):
                ad.log.error("Phone failed to setup 2g.")
                return False
            ad.log.info("RAT 2G is enabled successfully.")
        return True

    def _setup_phone_call(self, test_video):
        if test_video:
            if not video_call_setup(
                    self.log,
                    self.android_devices[0],
                    self.android_devices[1], ):
                self.log.error("Failed to setup Video call")
                return False
        else:
            if not call_setup_teardown(
                    self.log, self.caller, self.callee, ad_hangup=None):
                self.log.error("Setup Call failed.")
                return False
        self.log.info("Setup call successfully.")
        return True

    def _hangup_call(self):
        for ad in self.android_devices:
            hangup_call(self.log, ad)

    def stress_test(self,
                    setup_func=None,
                    network_check_func=None,
                    test_sms=False,
                    test_video=False):
        for ad in self.android_devices:
            #check for sim and service
            ensure_phone_subscription(self.log, ad)

        if setup_func and not setup_func():
            self.log.error("Test setup %s failed", setup_func.__name__)
            return False
        fail_count = collections.defaultdict(int)
        for i in range(1, self.phone_call_iteration + 1):
            msg = "Stress Call Test %s Iteration: <%s> / <%s>" % (
                self.test_name, i, self.phone_call_iteration)
            begin_time = get_current_epoch_time()
            self.log.info(msg)
            iteration_result = True
            ensure_phones_idle(self.log, self.android_devices)

            if not self._setup_phone_call(test_video):
                fail_count["dialing"] += 1
                iteration_result = False
                self.log.error("%s call dialing failure.", msg)
            else:
                if network_check_func and not network_check_func(
                        self.log, self.caller):
                    fail_count["caller_network_check"] += 1
                    reasons = self.caller.search_logcat(
                        "qcril_qmi_voice_map_qmi_to_ril_last_call_failure_cause",
                        begin_time)
                    if reasons:
                        self.caller.log.info(reasons[-1]["log_message"])
                    iteration_result = False
                    self.log.error("%s network check %s failure.", msg,
                                   network_check_func.__name__)

                if network_check_func and not network_check_func(
                        self.log, self.callee):
                    fail_count["callee_network_check"] += 1
                    reasons = self.callee.search_logcat(
                        "qcril_qmi_voice_map_qmi_to_ril_last_call_failure_cause",
                        begin_time)
                    if reasons:
                        self.callee.log.info(reasons[-1]["log_message"])
                    iteration_result = False
                    self.log.error("%s network check failure.", msg)

                time.sleep(self.phone_call_duration)

                if not verify_incall_state(self.log,
                                           [self.caller, self.callee], True):
                    self.log.error("%s call dropped.", msg)
                    iteration_result = False
                    fail_count["drop"] += 1

                self._hangup_call()

            if test_sms and not sms_send_receive_verify(
                    self.log, self.caller, self.callee, [rand_ascii_str(180)]):
                fail_count["sms"] += 1

            self.log.info("%s %s", msg, iteration_result)
            if not iteration_result:
                self._take_bug_report("%s_CallNo_%s" % (self.test_name, i),
                                      begin_time)
                start_qxdm_loggers(self.log, self.android_devices)

            if self.sleep_time_between_test_iterations:
                self.caller.droid.goToSleepNow()
                self.callee.droid.goToSleepNow()
                time.sleep(self.sleep_time_between_test_iterations)

        test_result = True
        for failure, count in fail_count.items():
            if count:
                self.log.error("%s: %s %s failures in %s iterations",
                               self.test_name, count, failure,
                               self.phone_call_iteration)
                test_result = False
        return test_result

    """ Tests Begin """

    @test_tracker_info(uuid="3c3daa08-e66a-451a-a772-634ec522c965")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_default_stress(self):
        """ Default state call stress test

        Steps:
        1. Make Sure PhoneA and PhoneB in default mode.
        2. Call from PhoneA to PhoneB, hang up on PhoneA.
        3, Repeat 2 around N times based on the config setup

        Expected Results:
        1, Verify phone is at IDLE state
        2, Verify the phone is at ACTIVE, if it is in dialing, then we retry
        3, Verify the phone is IDLE after hung up

        Returns:
            True if pass; False if fail.
        """
        return self.stress_test()

    @test_tracker_info(uuid="b7fd730a-d4c7-444c-9e36-12389679b430")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_and_sms_longevity(self):
        """ Default state call stress test

        Steps:
        1. Make Sure PhoneA and PhoneB in default mode.
        2. Call from PhoneA to PhoneB, hang up on PhoneA.
        3. Send a text message from PhoneA to PhoneB.
        4. Bring phone to sleep for x seconds based on the config setup.
        5, Repeat 2 around N times based on the config setup

        Expected Results:
        1. Phone calls and text messages are successfully made

        Returns:
            True if pass; False if fail.
        """
        return self.stress_test(test_sms=True)

    @test_tracker_info(uuid="3b711843-de27-4b0a-a163-8c439c901e24")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_stress(self):
        """ VoLTE call stress test

        Steps:
        1. Make Sure PhoneA and PhoneB in VoLTE mode.
        2. Call from PhoneA to PhoneB, hang up on PhoneA.
        3, Repeat 2 around N times based on the config setup

        Expected Results:
        1, Verify phone is at IDLE state
        2, Verify the phone is at ACTIVE, if it is in dialing, then we retry
        3, Verify the phone is IDLE after hung up

        Returns:
            True if pass; False if fail.
        """
        return self.stress_test(
            setup_func=self._setup_lte_volte_enabled,
            network_check_func=is_phone_in_call_volte)

    @test_tracker_info(uuid="518516ea-1c0a-494d-ad44-272f21075d39")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_csfb_stress(self):
        """ LTE CSFB call stress test

        Steps:
        1. Make Sure PhoneA in LTE CSFB mode.
        2. Call from PhoneA to PhoneB, hang up on PhoneA.
        3, Repeat 2 around N times based on the config setup

        Expected Results:
        1, Verify phone is at IDLE state
        2, Verify the phone is at ACTIVE, if it is in dialing, then we retry
        3, Verify the phone is IDLE after hung up

        Returns:
            True if pass; False if fail.
        """
        return self.stress_test(
            setup_func=self._setup_lte_volte_disabled,
            network_check_func=is_phone_in_call_csfb)

    @test_tracker_info(uuid="887608cb-e5c6-4b19-b02b-3461c1e78f2d")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_wifi_calling_stress(self):
        """ Wifi calling call stress test

        Steps:
        1. Make Sure PhoneA and PhoneB in WFC On + Wifi Connected
        2. Call from PhoneA to PhoneB, hang up on PhoneA.
        3, Repeat 2 around N times based on the config setup

        Expected Results:
        1, Verify phone is at IDLE state
        2, Verify the phone is at ACTIVE, if it is in dialing, then we retry
        3, Verify the phone is IDLE after hung up

        Returns:
            True if pass; False if fail.
        """
        return self.stress_test(
            setup_func=self._setup_wfc,
            network_check_func=is_phone_in_call_iwlan)

    @test_tracker_info(uuid="be45c620-b45b-4a06-8424-b17d744d0735")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_wifi_calling_stress_apm(self):
        """ Wifi calling in AirPlaneMode call stress test

        Steps:
        1. Make Sure PhoneA and PhoneB in WFC On + APM ON + Wifi Connected
        2. Call from PhoneA to PhoneB, hang up on PhoneA.
        3, Repeat 2 around N times based on the config setup

        Expected Results:
        1, Verify phone is at IDLE state
        2, Verify the phone is at ACTIVE, if it is in dialing, then we retry
        3, Verify the phone is IDLE after hung up

        Returns:
            True if pass; False if fail.
        """
        return self.stress_test(
            setup_func=self._setup_wfc_apm,
            network_check_func=is_phone_in_call_iwlan)

    @test_tracker_info(uuid="8af0454b-b4db-46d8-b5cc-e13ec5bc59ab")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_3g_stress(self):
        """ 3G call stress test

        Steps:
        1. Make Sure PhoneA and PhoneB in 3G mode.
        2. Call from PhoneA to PhoneB, hang up on PhoneA.
        3, Repeat 2 around N times based on the config setup

        Expected Results:
        1, Verify phone is at IDLE state
        2, Verify the phone is at ACTIVE, if it is in dialing, then we retry
        3, Verify the phone is IDLE after hung up

        Returns:
            True if pass; False if fail.
        """
        return self.stress_test(
            setup_func=self._setup_3g, network_check_func=is_phone_in_call_3g)

    @test_tracker_info(uuid="12380823-2e7f-4c41-95c0-5f8c483f9510")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_2g_stress(self):
        """ 2G call stress test

        Steps:
        1. Make Sure PhoneA and PhoneB in 3G mode.
        2. Call from PhoneA to PhoneB, hang up on PhoneA.
        3, Repeat 2 around N times based on the config setup

        Expected Results:
        1, Verify phone is at IDLE state
        2, Verify the phone is at ACTIVE, if it is in dialing, then we retry
        3, Verify the phone is IDLE after hung up

        Returns:
            True if pass; False if fail.
        """
        return self.stress_test(
            setup_func=self._setup_2g, network_check_func=is_phone_in_call_2g)

    @test_tracker_info(uuid="28a88b44-f239-4b77-b01f-e9068373d749")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_stress(self):
        """ VT call stress test

        Steps:
        1. Make Sure PhoneA and PhoneB in VoLTE mode (ViLTE provisioned).
        2. Call from PhoneA to PhoneB, hang up on PhoneA.
        3, Repeat 2 around N times based on the config setup

        Expected Results:
        1, Verify phone is at IDLE state
        2, Verify the phone is at ACTIVE, if it is in dialing, then we retry
        3, Verify the phone is IDLE after hung up

        Returns:
            True if pass; False if fail.
        """
        return self.stress_test(
            setup_func=self._setup_vt,
            network_check_func=is_phone_in_call_video_bidirectional,
            test_video=True)

    """ Tests End """
