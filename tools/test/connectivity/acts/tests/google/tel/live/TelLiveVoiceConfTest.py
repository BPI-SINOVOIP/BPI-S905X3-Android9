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
    Test Script for Live Network Telephony Conference Call
"""

import time
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import CALL_CAPABILITY_MANAGE_CONFERENCE
from acts.test_utils.tel.tel_defines import CALL_CAPABILITY_MERGE_CONFERENCE
from acts.test_utils.tel.tel_defines import CALL_CAPABILITY_SWAP_CONFERENCE
from acts.test_utils.tel.tel_defines import CALL_PROPERTY_CONFERENCE
from acts.test_utils.tel.tel_defines import CALL_STATE_ACTIVE
from acts.test_utils.tel.tel_defines import CALL_STATE_HOLDING
from acts.test_utils.tel.tel_defines import CARRIER_VZW
from acts.test_utils.tel.tel_defines import GEN_3G
from acts.test_utils.tel.tel_defines import RAT_3G
from acts.test_utils.tel.tel_defines import PHONE_TYPE_CDMA
from acts.test_utils.tel.tel_defines import PHONE_TYPE_GSM
from acts.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_ONLY
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_test_utils import call_reject
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import \
    ensure_network_generation_for_subscription
from acts.test_utils.tel.tel_test_utils import get_call_uri
from acts.test_utils.tel.tel_test_utils import get_phone_number
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import is_uri_equivalent
from acts.test_utils.tel.tel_test_utils import multithread_func
from acts.test_utils.tel.tel_test_utils import num_active_calls
from acts.test_utils.tel.tel_test_utils import set_call_state_listen_level
from acts.test_utils.tel.tel_test_utils import setup_sim
from acts.test_utils.tel.tel_test_utils import verify_incall_state
from acts.test_utils.tel.tel_test_utils import wait_and_answer_call
from acts.test_utils.tel.tel_voice_utils import get_cep_conference_call_id
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_1x
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_2g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_csfb
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_wcdma
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_2g
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_general
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.tel.tel_voice_utils import swap_calls


class TelLiveVoiceConfTest(TelephonyBaseTest):

    # Note: Currently Conference Call do not verify voice.
    # So even if test cases passed, does not necessarily means
    # conference call functionality is working.
    # Need to add code to check for voice.

    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)

        self.wifi_network_ssid = self.user_params["wifi_network_ssid"]

        try:
            self.wifi_network_pass = self.user_params["wifi_network_pass"]
        except KeyError:
            self.wifi_network_pass = None

    """ Private Test Utils """

    def _get_expected_call_state(self, ad):
        if ad.model in ("sailfish", "marlin") and "vzw" in [
                sub["operator"] for sub in ad.cfg["subscription"].values()
        ]:
            return CALL_STATE_ACTIVE
        return CALL_STATE_HOLDING

    def _hangup_call(self, ad, device_description='Device'):
        if not hangup_call(self.log, ad):
            ad.log.error("Failed to hang up on %s", device_description)
            return False
        return True

    def _three_phone_call_mo_add_mo(self, ads, phone_setups, verify_funcs):
        """Use 3 phones to make MO calls.

        Call from PhoneA to PhoneB, accept on PhoneB.
        Call from PhoneA to PhoneC, accept on PhoneC.

        Args:
            ads: list of ad object.
                The list should have three objects.
            phone_setups: list of phone setup functions.
                The list should have three objects.
            verify_funcs: list of phone call verify functions.
                The list should have three objects.

        Returns:
            If success, return 'call_AB' id in PhoneA.
            if fail, return None.
        """

        class _CallException(Exception):
            pass

        try:
            verify_func_a, verify_func_b, verify_func_c = verify_funcs
            tasks = []
            for ad, setup_func in zip(ads, phone_setups):
                if setup_func is not None:
                    tasks.append((setup_func, (self.log, ad)))
            if tasks != [] and not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                raise _CallException("Setup failed.")
            for ad in ads:
                ad.droid.telecomCallClearCallList()
                if num_active_calls(self.log, ad) != 0:
                    ad.log.error("Phone Call List is not empty.")
                    raise _CallException("Clear call list failed.")

            self.log.info("Step1: Call From PhoneA to PhoneB.")
            if not call_setup_teardown(
                    self.log,
                    ads[0],
                    ads[1],
                    ad_hangup=None,
                    verify_caller_func=verify_func_a,
                    verify_callee_func=verify_func_b):
                raise _CallException("PhoneA call PhoneB failed.")

            calls = ads[0].droid.telecomCallGetCallIds()
            ads[0].log.info("Calls in PhoneA %s", calls)
            if num_active_calls(self.log, ads[0]) != 1:
                raise _CallException("Call list verify failed.")
            call_ab_id = calls[0]

            self.log.info("Step2: Call From PhoneA to PhoneC.")
            if not call_setup_teardown(
                    self.log,
                    ads[0],
                    ads[2],
                    ad_hangup=None,
                    verify_caller_func=verify_func_a,
                    verify_callee_func=verify_func_c):
                raise _CallException("PhoneA call PhoneC failed.")
            if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]],
                                       True):
                raise _CallException("Not All phones are in-call.")

        except _CallException:
            return None

        return call_ab_id

    def _three_phone_call_mo_add_mt(self, ads, phone_setups, verify_funcs):
        """Use 3 phones to make MO call and MT call.

        Call from PhoneA to PhoneB, accept on PhoneB.
        Call from PhoneC to PhoneA, accept on PhoneA.

        Args:
            ads: list of ad object.
                The list should have three objects.
            phone_setups: list of phone setup functions.
                The list should have three objects.
            verify_funcs: list of phone call verify functions.
                The list should have three objects.

        Returns:
            If success, return 'call_AB' id in PhoneA.
            if fail, return None.
        """

        class _CallException(Exception):
            pass

        try:
            verify_func_a, verify_func_b, verify_func_c = verify_funcs
            tasks = []
            for ad, setup_func in zip(ads, phone_setups):
                if setup_func is not None:
                    tasks.append((setup_func, (self.log, ad)))
            if tasks != [] and not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                raise _CallException("Setup failed.")
            for ad in ads:
                ad.droid.telecomCallClearCallList()
                if num_active_calls(self.log, ad) != 0:
                    ad.log.error("Phone Call List is not empty.")
                    raise _CallException("Clear call list failed.")

            self.log.info("Step1: Call From PhoneA to PhoneB.")
            if not call_setup_teardown(
                    self.log,
                    ads[0],
                    ads[1],
                    ad_hangup=None,
                    verify_caller_func=verify_func_a,
                    verify_callee_func=verify_func_b):
                raise _CallException("PhoneA call PhoneB failed.")

            calls = ads[0].droid.telecomCallGetCallIds()
            ads[0].log.info("Calls in PhoneA %s", calls)
            if num_active_calls(self.log, ads[0]) != 1:
                raise _CallException("Call list verify failed.")
            call_ab_id = calls[0]

            self.log.info("Step2: Call From PhoneC to PhoneA.")
            if not call_setup_teardown(
                    self.log,
                    ads[2],
                    ads[0],
                    ad_hangup=None,
                    verify_caller_func=verify_func_c,
                    verify_callee_func=verify_func_a):
                raise _CallException("PhoneA call PhoneC failed.")
            if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]],
                                       True):
                raise _CallException("Not All phones are in-call.")

        except _CallException:
            return None

        return call_ab_id

    def _three_phone_call_mo_add_mt_reject(self, ads, verify_funcs, reject):
        """Use 3 phones to make MO call and MT call.

        Call from PhoneA to PhoneB, accept on PhoneB.
        Call from PhoneC to PhoneA. PhoneA receive incoming call.
            if reject is True, then reject the call on PhoneA.
            if reject if False, then just ignore the incoming call on PhoneA.

        Args:
            ads: list of ad object.
                The list should have three objects.
            verify_funcs: list of phone call verify functions for
                PhoneA and PhoneB. The list should have two objects.

        Returns:
            True if no error happened.
        """

        class _CallException(Exception):
            pass

        try:
            verify_func_a, verify_func_b = verify_funcs
            self.log.info("Step1: Call From PhoneA to PhoneB.")
            if not call_setup_teardown(
                    self.log,
                    ads[0],
                    ads[1],
                    ad_hangup=None,
                    verify_caller_func=verify_func_a,
                    verify_callee_func=verify_func_b):
                raise _CallException("PhoneA call PhoneB failed.")

            self.log.info("Step2: Call From PhoneC to PhoneA then decline.")
            if not call_reject(self.log, ads[2], ads[0], reject):
                raise _CallException("PhoneC call PhoneA then decline failed.")
            time.sleep(WAIT_TIME_IN_CALL)
            if not verify_incall_state(self.log, [ads[0], ads[1]], True):
                raise _CallException("PhoneA and PhoneB are not in call.")

        except _CallException:
            return False

        return True

    def _three_phone_call_mt_add_mt(self, ads, phone_setups, verify_funcs):
        """Use 3 phones to make MT call and MT call.

        Call from PhoneB to PhoneA, accept on PhoneA.
        Call from PhoneC to PhoneA, accept on PhoneA.

        Args:
            ads: list of ad object.
                The list should have three objects.
            phone_setups: list of phone setup functions.
                The list should have three objects.
            verify_funcs: list of phone call verify functions.
                The list should have three objects.

        Returns:
            If success, return 'call_AB' id in PhoneA.
            if fail, return None.
        """

        class _CallException(Exception):
            pass

        try:
            verify_func_a, verify_func_b, verify_func_c = verify_funcs
            tasks = []
            for ad, setup_func in zip(ads, phone_setups):
                if setup_func is not None:
                    tasks.append((setup_func, (self.log, ad)))
            if tasks != [] and not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                raise _CallException("Setup failed.")
            for ad in ads:
                ad.droid.telecomCallClearCallList()
                if num_active_calls(self.log, ad) != 0:
                    ad.log.error("Phone Call List is not empty.")
                    raise _CallException("Clear call list failed.")

            self.log.info("Step1: Call From PhoneB to PhoneA.")
            if not call_setup_teardown(
                    self.log,
                    ads[1],
                    ads[0],
                    ad_hangup=None,
                    verify_caller_func=verify_func_b,
                    verify_callee_func=verify_func_a):
                raise _CallException("PhoneB call PhoneA failed.")

            calls = ads[0].droid.telecomCallGetCallIds()
            ads[0].log.info("Calls in PhoneA %s", calls)
            if num_active_calls(self.log, ads[0]) != 1:
                raise _CallException("Call list verify failed.")
            call_ab_id = calls[0]

            self.log.info("Step2: Call From PhoneC to PhoneA.")
            if not call_setup_teardown(
                    self.log,
                    ads[2],
                    ads[0],
                    ad_hangup=None,
                    verify_caller_func=verify_func_c,
                    verify_callee_func=verify_func_a):
                raise _CallException("PhoneA call PhoneC failed.")
            if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]],
                                       True):
                raise _CallException("Not All phones are in-call.")

        except _CallException:
            return None

        return call_ab_id

    def _test_1x_mo_mo_add(self):
        """Test multi call feature in 1x call.

        PhoneA (1x) call PhoneB, accept on PhoneB.
        PhoneA (1x) call PhoneC, accept on PhoneC.

        Returns:
            call_ab_id, call_ac_id, call_conf_id if succeed;
            None, None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is CDMA phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
            ads[0].log.error("not CDMA phone, abort this 1x test.")
            return None, None, None

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]], [
                phone_setup_voice_3g, phone_setup_voice_general,
                phone_setup_voice_general
            ], [is_phone_in_call_1x, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 3:
            return None, None, None
        for call_id in calls:
            if (CALL_CAPABILITY_MERGE_CONFERENCE in ads[0]
                    .droid.telecomCallGetCapabilities(call_id)):
                call_conf_id = call_id
            elif call_id != call_ab_id:
                call_ac_id = call_id

        return call_ab_id, call_ac_id, call_conf_id

    def _test_1x_mo_mt_add_swap_x(self, num_swaps):
        """Test multi call feature in 1x call.

        PhoneA (1x) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (1x), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Returns:
            call_ab_id, call_ac_id, call_conf_id if succeed;
            None, None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is CDMA phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
            ads[0].log.error("not CDMA phone, abort this 1x test.")
            return None, None, None

        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]], [
                phone_setup_voice_3g, phone_setup_voice_general,
                phone_setup_voice_general
            ], [is_phone_in_call_1x, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None, None

        call_conf_id = None
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 3:
            return None, None, None
        for call_id in calls:
            if (CALL_CAPABILITY_SWAP_CONFERENCE in ads[0]
                    .droid.telecomCallGetCapabilities(call_id)):
                call_conf_id = call_id
            elif call_id != call_ab_id:
                call_ac_id = call_id

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(
                    self.log,
                    ads,
                    call_ab_id,
                    call_ac_id,
                    num_swaps,
                    check_call_status=False):
                self.log.error("Swap test failed.")
                return None, None, None

        return call_ab_id, call_ac_id, call_conf_id

    def _test_1x_mt_mt_add_swap_x(self, num_swaps):
        """Test multi call feature in 1x call.

        PhoneB call PhoneA (1x), accept on PhoneA.
        PhoneC call PhoneA (1x), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Returns:
            call_ab_id, call_ac_id, call_conf_id if succeed;
            None, None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is CDMA phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
            ads[0].log.error("not CDMA phone, abort this 1x test.")
            return None, None, None

        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]], [
                phone_setup_voice_3g, phone_setup_voice_general,
                phone_setup_voice_general
            ], [is_phone_in_call_1x, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None, None

        call_conf_id = None
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 3:
            return None, None, None
        for call_id in calls:
            if (CALL_CAPABILITY_SWAP_CONFERENCE in ads[0]
                    .droid.telecomCallGetCapabilities(call_id)):
                call_conf_id = call_id
            elif call_id != call_ab_id:
                call_ac_id = call_id

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(
                    self.log,
                    ads,
                    call_ab_id,
                    call_ac_id,
                    num_swaps,
                    check_call_status=False):
                self.log.error("Swap test failed.")
                return None, None, None

        return call_ab_id, call_ac_id, call_conf_id

    def _test_1x_multi_call_drop_from_participant(self, host, first_drop_ad,
                                                  second_drop_ad):
        """Test private function to drop call from participant in 1x multi call.

        Host(1x) is in multi call scenario with first_drop_ad and second_drop_ad.
        Drop call on first_drop_ad.
        Verify call continues between host and second_drop_ad.
        Drop call on second_drop_ad and verify host also ends.

        Args:
            host: android device object for multi-call/conference-call host.
            first_drop_ad: android device object for call participant, end call
                on this participant first.
            second_drop_ad: android device object for call participant, end call
                on this participant second.

        Returns:
            True if no error happened. Otherwise False.
        """
        self.log.info("Drop 1st call.")
        if not self._hangup_call(first_drop_ad):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        calls = host.droid.telecomCallGetCallIds()
        host.log.info("Calls list: %s", calls)
        if num_active_calls(self.log, host) != 3:
            return False
        if not verify_incall_state(self.log, [host, second_drop_ad], True):
            return False
        if not verify_incall_state(self.log, [first_drop_ad], False):
            return False

        self.log.info("Drop 2nd call.")
        if not self._hangup_call(second_drop_ad):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(
                self.log, [host, second_drop_ad, first_drop_ad], False):
            return False
        return True

    def _test_1x_multi_call_drop_from_host(self, host, active_participant_ad,
                                           held_participant_ad):
        """Test private function to drop call from host in 1x multi call.

        Host(1x) is in multi call scenario with first_drop_ad and second_drop_ad.
        Drop call on host. Then active_participant_ad should ends as well.
        Host should receive a call back from held_participant_ad. Answer on host.
        Drop call on host. Then verify held_participant_ad ends as well.

        Args:
            host: android device object for multi-call/conference-call host.
            active_participant_ad: android device object for the current active
                call participant.
            held_participant_ad: android device object for the current held
                call participant.

        Returns:
            True if no error happened. Otherwise False.
        """
        self.log.info("Drop current call on Host.")
        if not self._hangup_call(host, "Host"):
            return False
        if not wait_and_answer_call(self.log, host,
                                    get_phone_number(self.log,
                                                     held_participant_ad)):
            self.log.error("Did not receive call back.")
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [host, held_participant_ad],
                                   True):
            return False
        if not verify_incall_state(self.log, [active_participant_ad], False):
            return False

        self.log.info("Drop current call on Host.")
        if not self._hangup_call(host, "Host"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(
                self.log, [host, held_participant_ad, active_participant_ad],
                False):
            return False
        return True

    def _test_1x_conf_call_drop_from_host(self, host, participant_list):
        """Test private function to drop call from host in 1x conference call.

        Host(1x) is in conference call scenario with phones in participant_list.
        End call on host. Then all phones in participant_list should end call.

        Args:
            host: android device object for multi-call/conference-call host.
            participant_list: android device objects list for all other
                participants in multi-call/conference-call.

        Returns:
            True if no error happened. Otherwise False.
        """
        self.log.info("Drop conference call on Host.")
        if not self._hangup_call(host, "Host"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [host], False):
            return False
        if not verify_incall_state(self.log, participant_list, False):
            return False
        return True

    def _test_1x_merge_conference(self, host, participant_list, call_conf_id):
        """Test private function to merge to conference in 1x multi call scenario.

        Host(1x) is in multi call scenario with phones in participant_list.
        Merge to conference on host.
        Verify merge succeed.

        Args:
            host: android device object for multi-call/conference-call host.
            participant_list: android device objects list for all other
                participants in multi-call/conference-call.
            call_conf_id: conference call id in host android device object.

        Returns:
            True if no error happened. Otherwise False.
        """
        host.droid.telecomCallMergeToConf(call_conf_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = host.droid.telecomCallGetCallIds()
        host.log.info("Calls in Phone %s", calls)
        if num_active_calls(self.log, host) != 3:
            return False
        if not verify_incall_state(self.log, [host], True):
            return False
        if not verify_incall_state(self.log, participant_list, True):
            return False
        if (CALL_CAPABILITY_MERGE_CONFERENCE in
                host.droid.telecomCallGetCapabilities(call_conf_id)):
            self.log.error("Merge conference failed.")
            return False
        return True

    def _test_volte_mo_mo_add_volte_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_volte, phone_setup_volte], [
                is_phone_in_call_volte, is_phone_in_call_volte,
                is_phone_in_call_volte
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mo_mt_add_volte_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_volte, phone_setup_volte], [
                is_phone_in_call_volte, is_phone_in_call_volte,
                is_phone_in_call_volte
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mt_mt_add_volte_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_volte, phone_setup_volte], [
                is_phone_in_call_volte, is_phone_in_call_volte,
                is_phone_in_call_volte
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mo_mo_add_wcdma_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are GSM phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
                ad.log.error("not GSM phone, abort wcdma swap test.")
                return None, None

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_voice_3g, phone_setup_voice_3g], [
                is_phone_in_call_volte, is_phone_in_call_wcdma,
                is_phone_in_call_wcdma
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mo_mt_add_wcdma_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are GSM phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
                ad.log.error("not GSM phone, abort wcdma swap test.")
                return None, None

        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_voice_3g, phone_setup_voice_3g], [
                is_phone_in_call_volte, is_phone_in_call_wcdma,
                is_phone_in_call_wcdma
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mt_mt_add_wcdma_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are GSM phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
                ad.log.error("not GSM phone, abort wcdma swap test.")
                return None, None

        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_voice_3g, phone_setup_voice_3g], [
                is_phone_in_call_volte, is_phone_in_call_wcdma,
                is_phone_in_call_wcdma
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mo_mo_add_1x_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are CDMA phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
                ad.log.error("not CDMA phone, abort 1x swap test.")
                return None, None

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_voice_3g, phone_setup_voice_3g],
            [is_phone_in_call_volte, is_phone_in_call_1x, is_phone_in_call_1x])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mo_mt_add_1x_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are CDMA phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
                ad.log.error("not CDMA phone, abort 1x swap test.")
                return None, None

        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_voice_3g, phone_setup_voice_3g],
            [is_phone_in_call_volte, is_phone_in_call_1x, is_phone_in_call_1x])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mt_mt_add_1x_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are CDMA phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
                self.log.error("not CDMA phone, abort 1x swap test.")
                return None, None

        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_voice_3g, phone_setup_voice_3g],
            [is_phone_in_call_volte, is_phone_in_call_1x, is_phone_in_call_1x])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_wcdma_mo_mo_add_swap_x(self, num_swaps):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            ad.log.error("not GSM phone, abort wcdma swap test.")
            return None, None

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]], [
                phone_setup_voice_3g, phone_setup_voice_general,
                phone_setup_voice_general
            ], [is_phone_in_call_3g, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_wcdma_mt_mt_add_swap_x(self, num_swaps):
        """Test swap feature in WCDMA call.

        PhoneB call PhoneA (WCDMA), accept on PhoneA.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            ads[0].log.error("not GSM phone, abort wcdma swap test.")
            return None, None

        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]], [
                phone_setup_voice_3g, phone_setup_voice_general,
                phone_setup_voice_general
            ], [is_phone_in_call_3g, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_wcdma_mo_mt_add_swap_x(self, num_swaps):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            ads[0].log.error("not GSM phone, abort wcdma swap test.")
            return None, None

        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]], [
                phone_setup_voice_3g, phone_setup_voice_general,
                phone_setup_voice_general
            ], [is_phone_in_call_wcdma, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_csfb_wcdma_mo_mo_add_swap_x(self, num_swaps):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            ads[0].log.error("not GSM phone, abort wcdma swap test.")
            return None, None

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]], [
                phone_setup_csfb, phone_setup_voice_general,
                phone_setup_voice_general
            ], [is_phone_in_call_csfb, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_csfb_wcdma_mo_mt_add_swap_x(self, num_swaps):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            ads[0].log.error("not GSM phone, abort wcdma swap test.")
            return None, None

        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]], [
                phone_setup_csfb, phone_setup_voice_general,
                phone_setup_voice_general
            ], [is_phone_in_call_csfb, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_ims_conference_merge_drop_second_call_no_cep(
            self, call_ab_id, call_ac_id):
        """Test conference merge and drop in VoLTE call.

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 1:
            ads[0].log.error("Total number of call lists is not 1.")
            if get_cep_conference_call_id(ads[0]) is not None:
                self.log.error("CEP enabled.")
            else:
                self.log.error("Merge failed.")
            return False
        call_conf_id = None
        for call_id in calls:
            if call_id != call_ab_id and call_id != call_ac_id:
                call_conf_id = call_id
        if not call_conf_id:
            self.log.error("Merge call fail, no new conference call id.")
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        # Check if Conf Call is currently active
        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            ads[0].log.error(
                "Call_id:%s, state:%s, expected: STATE_ACTIVE", call_conf_id,
                ads[0].droid.telecomCallGetCallState(call_conf_id))
            return False

        self.log.info("Step5: End call on PhoneC and verify call continues.")
        if not self._hangup_call(ads[2], "PhoneC"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        # Because of b/18413009, VZW VoLTE conference host will not drop call
        # even if all participants drop. The reason is VZW network is not
        # providing such information to DUT.
        # So this test probably will fail on the last step for VZW.
        self.log.info("Step6: End call on PhoneB and verify PhoneA end.")
        if not self._hangup_call(ads[1], "PhoneB"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _merge_cep_conference_call(self, call_ab_id, call_ac_id):
        """Merge CEP conference call.

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA (CEP enabled IMS conference).

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            call_id for conference
        """
        ads = self.android_devices

        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)

        call_conf_id = get_cep_conference_call_id(ads[0])
        if call_conf_id is None:
            self.log.error(
                "No call with children. Probably CEP not enabled or merge failed."
            )
            return None
        calls.remove(call_conf_id)
        if (set(ads[0].droid.telecomCallGetCallChildren(call_conf_id)) !=
                set(calls)):
            ads[0].log.error(
                "Children list %s for conference call is not correct.",
                ads[0].droid.telecomCallGetCallChildren(call_conf_id))
            return None

        if (CALL_PROPERTY_CONFERENCE not in ads[0]
                .droid.telecomCallGetProperties(call_conf_id)):
            ads[0].log.error(
                "Conf call id % properties wrong: %s", call_conf_id,
                ads[0].droid.telecomCallGetProperties(call_conf_id))
            return None

        if (CALL_CAPABILITY_MANAGE_CONFERENCE not in ads[0]
                .droid.telecomCallGetCapabilities(call_conf_id)):
            ads[0].log.error(
                "Conf call id %s capabilities wrong: %s", call_conf_id,
                ads[0].droid.telecomCallGetCapabilities(call_conf_id))
            return None

        if (call_ab_id in calls) or (call_ac_id in calls):
            self.log.error(
                "Previous call ids should not in new call list after merge.")
            return None

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return None

        # Check if Conf Call is currently active
        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            ads[0].log.error(
                "Call_ID: %s, state: %s, expected: STATE_ACTIVE", call_conf_id,
                ads[0].droid.telecomCallGetCallState(call_conf_id))
            return None

        return call_conf_id

    def _test_ims_conference_merge_drop_second_call_from_participant_cep(
            self, call_ab_id, call_ac_id):
        """Test conference merge and drop in IMS (VoLTE or WiFi Calling) call.
        (CEP enabled).

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA (CEP enabled IMS conference).
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        call_conf_id = self._merge_cep_conference_call(call_ab_id, call_ac_id)
        if call_conf_id is None:
            return False

        self.log.info("Step5: End call on PhoneC and verify call continues.")
        if not self._hangup_call(ads[2], "PhoneC"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        self.log.info("Step6: End call on PhoneB and verify PhoneA end.")
        if not self._hangup_call(ads[1], "PhoneB"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _test_ims_conference_merge_drop_first_call_from_participant_cep(
            self, call_ab_id, call_ac_id):
        """Test conference merge and drop in IMS (VoLTE or WiFi Calling) call.
        (CEP enabled).

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA (CEP enabled IMS conference).
        Hangup on PhoneB, check call continues between AC.
        Hangup on PhoneC, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        call_conf_id = self._merge_cep_conference_call(call_ab_id, call_ac_id)
        if call_conf_id is None:
            return False

        self.log.info("Step5: End call on PhoneB and verify call continues.")
        if not self._hangup_call(ads[1], "PhoneB"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[2]], True):
            return False
        if not verify_incall_state(self.log, [ads[1]], False):
            return False

        self.log.info("Step6: End call on PhoneC and verify PhoneA end.")
        if not self._hangup_call(ads[2], "PhoneC"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _test_ims_conference_merge_drop_second_call_from_host_cep(
            self, call_ab_id, call_ac_id):
        """Test conference merge and drop in IMS (VoLTE or WiFi Calling) call.
        (CEP enabled).

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA (CEP enabled IMS conference).
        On PhoneA, disconnect call between A-C, verify PhoneA PhoneB still in call.
        On PhoneA, disconnect call between A-B, verify PhoneA PhoneB disconnected.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        call_ab_uri = get_call_uri(ads[0], call_ab_id)
        call_ac_uri = get_call_uri(ads[0], call_ac_id)

        call_conf_id = self._merge_cep_conference_call(call_ab_id, call_ac_id)
        if call_conf_id is None:
            return False

        calls = ads[0].droid.telecomCallGetCallIds()
        calls.remove(call_conf_id)

        self.log.info("Step5: Disconnect call A-C and verify call continues.")
        call_to_disconnect = None
        for call in calls:
            if is_uri_equivalent(call_ac_uri, get_call_uri(ads[0], call)):
                call_to_disconnect = call
                calls.remove(call_to_disconnect)
                break
        if call_to_disconnect is None:
            self.log.error("Can NOT find call on host represents A-C.")
            return False
        else:
            ads[0].droid.telecomCallDisconnect(call_to_disconnect)
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        self.log.info(
            "Step6: Disconnect call A-B and verify PhoneA PhoneB end.")
        call_to_disconnect = None
        for call in calls:
            if is_uri_equivalent(call_ab_uri, get_call_uri(ads[0], call)):
                call_to_disconnect = call
                calls.remove(call_to_disconnect)
                break
        if call_to_disconnect is None:
            self.log.error("Can NOT find call on host represents A-B.")
            return False
        else:
            ads[0].droid.telecomCallDisconnect(call_to_disconnect)
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _test_ims_conference_merge_drop_first_call_from_host_cep(
            self, call_ab_id, call_ac_id):
        """Test conference merge and drop in IMS (VoLTE or WiFi Calling) call.
        (CEP enabled).

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA (CEP enabled IMS conference).
        On PhoneA, disconnect call between A-B, verify PhoneA PhoneC still in call.
        On PhoneA, disconnect call between A-C, verify PhoneA PhoneC disconnected.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        call_ab_uri = get_call_uri(ads[0], call_ab_id)
        call_ac_uri = get_call_uri(ads[0], call_ac_id)

        call_conf_id = self._merge_cep_conference_call(call_ab_id, call_ac_id)
        if call_conf_id is None:
            return False

        calls = ads[0].droid.telecomCallGetCallIds()
        calls.remove(call_conf_id)

        self.log.info("Step5: Disconnect call A-B and verify call continues.")
        call_to_disconnect = None
        for call in calls:
            if is_uri_equivalent(call_ab_uri, get_call_uri(ads[0], call)):
                call_to_disconnect = call
                calls.remove(call_to_disconnect)
                break
        if call_to_disconnect is None:
            self.log.error("Can NOT find call on host represents A-B.")
            return False
        else:
            ads[0].droid.telecomCallDisconnect(call_to_disconnect)
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[2]], True):
            return False
        if not verify_incall_state(self.log, [ads[1]], False):
            return False

        self.log.info(
            "Step6: Disconnect call A-C and verify PhoneA PhoneC end.")
        call_to_disconnect = None
        for call in calls:
            if is_uri_equivalent(call_ac_uri, get_call_uri(ads[0], call)):
                call_to_disconnect = call
                calls.remove(call_to_disconnect)
                break
        if call_to_disconnect is None:
            self.log.error("Can NOT find call on host represents A-C.")
            return False
        else:
            ads[0].droid.telecomCallDisconnect(call_to_disconnect)
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _test_wcdma_conference_merge_drop(self, call_ab_id, call_ac_id):
        """Test conference merge and drop in WCDMA/CSFB_WCDMA call.

        PhoneA in WCDMA (or CSFB_WCDMA) call with PhoneB.
        PhoneA in WCDMA (or CSFB_WCDMA) call with PhoneC.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 3:
            ads[0].log.error("Total number of call ids is not 3.")
            return False
        call_conf_id = None
        for call_id in calls:
            if call_id != call_ab_id and call_id != call_ac_id:
                call_conf_id = call_id
        if not call_conf_id:
            self.log.error("Merge call fail, no new conference call id.")
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        # Check if Conf Call is currently active
        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            ads[0].log.error(
                "Call_id: %s, state: %s, expected: STATE_ACTIVE", call_conf_id,
                ads[0].droid.telecomCallGetCallState(call_conf_id))
            return False

        self.log.info("Step5: End call on PhoneC and verify call continues.")
        if not self._hangup_call(ads[2], "PhoneC"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 1:
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        self.log.info("Step6: End call on PhoneB and verify PhoneA end.")
        if not self._hangup_call(ads[1], "PhoneB"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _three_phone_hangup_call_verify_call_state(
            self, ad_hangup, ad_verify, call_id, call_state, ads_active):
        """Private Test utility for swap test.

        Hangup on 'ad_hangup'.
        Verify 'call_id' on 'ad_verify' is in expected 'call_state'
        Verify each ad in ads_active are 'in-call'.

        Args:
            ad_hangup: android object to hangup call.
            ad_verify: android object to verify call id state.
            call_id: call id in 'ad_verify'.
            call_state: expected state for 'call_id'.
                'call_state' is either CALL_STATE_HOLDING or CALL_STATE_ACTIVE.
            ads_active: list of android object.
                Each one of them should be 'in-call' after 'hangup' operation.

        Returns:
            True if no error happened. Otherwise False.

        """

        ad_hangup.log.info("Hangup, verify call continues.")
        if not self._hangup_call(ad_hangup):
            ad_hangup.log.error("Phone fails to hang up")
            return False
        time.sleep(WAIT_TIME_IN_CALL)

        if ad_verify.droid.telecomCallGetCallState(call_id) != call_state:
            ad_verify.log.error(
                "Call_id: %s, state: %s, expected: %s", call_id,
                ad_verify.droid.telecomCallGetCallState(call_id), call_state)
            return False
        ad_verify.log.info("Call in expected %s state", call_state)
        # TODO: b/26296375 add voice check.

        if not verify_incall_state(self.log, ads_active, True):
            ads_active.log.error("Phone not in call state")
            return False
        if not verify_incall_state(self.log, [ad_hangup], False):
            ad_hangup.log.error("Phone not in hangup state")
            return False

        return True

    def _test_epdg_mo_mo_add_epdg_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mo_add_epdg_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]], [None, None, None], [
                is_phone_in_call_iwlan, is_phone_in_call_iwlan,
                is_phone_in_call_iwlan
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mt_add_epdg_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mt_add_epdg_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None], [
                is_phone_in_call_iwlan, is_phone_in_call_iwlan,
                is_phone_in_call_iwlan
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mt_mt_add_epdg_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneB (epdg) call PhoneA (epdg), accept on PhoneA.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mt_mt_add_epdg_swap_x in test cases.
        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None], [
                is_phone_in_call_iwlan, is_phone_in_call_iwlan,
                is_phone_in_call_iwlan
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mo_add_volte_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (epdg) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mo_add_volte_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]], [None, None, None], [
                is_phone_in_call_iwlan, is_phone_in_call_volte,
                is_phone_in_call_volte
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA: %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mt_add_volte_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mt_add_volte_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None], [
                is_phone_in_call_iwlan, is_phone_in_call_volte,
                is_phone_in_call_volte
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA: %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mt_mt_add_volte_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneB (VoLTE) call PhoneA (epdg), accept on PhoneA.
        PhoneC (VoLTE) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mt_mt_add_volte_swap_x in test cases.
        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None], [
                is_phone_in_call_iwlan, is_phone_in_call_volte,
                is_phone_in_call_volte
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mo_add_wcdma_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (epdg) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are GSM phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
                ad.log.error("not GSM phone, abort wcdma swap test.")
                return None, None

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mo_add_wcdma_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]], [None, None, None], [
                is_phone_in_call_iwlan, is_phone_in_call_wcdma,
                is_phone_in_call_wcdma
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mt_add_wcdma_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are GSM phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
                ad.log.error("not GSM phone, abort wcdma swap test.")
                return None, None

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mt_add_wcdma_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None], [
                is_phone_in_call_iwlan, is_phone_in_call_wcdma,
                is_phone_in_call_wcdma
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mt_mt_add_wcdma_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneB (WCDMA) call PhoneA (epdg), accept on PhoneA.
        PhoneC (WCDMA) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are GSM phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
                ad.log.error("not GSM phone, abort wcdma swap test.")
                return None, None

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mt_add_wcdma_swap_x in test cases.
        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None], [
                is_phone_in_call_iwlan, is_phone_in_call_wcdma,
                is_phone_in_call_wcdma
            ])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mo_add_1x_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneA (epdg) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are CDMA phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
                ad.log.error("not CDMA phone, abort 1x swap test.")
                return None, None

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mo_add_1x_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_1x, is_phone_in_call_1x])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mt_add_1x_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are CDMA phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
                ad.log.error("not CDMA phone, abort 1x swap test.")
                return None, None

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mt_add_1x_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_1x, is_phone_in_call_1x])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mt_mt_add_1x_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneB (1x) call PhoneA (epdg), accept on PhoneA.
        PhoneC (1x) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are CDMA phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
                ad.log.error("not CDMA phone, abort 1x swap test.")
                return None, None

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mt_add_1x_swap_x in test cases.
        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_1x, is_phone_in_call_1x])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_conference_merge_drop(self, call_ab_id, call_ac_id):
        """Test conference merge and drop in epdg call.

        PhoneA in epdg call with PhoneB.
        PhoneA in epdg call with PhoneC.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """

        ads = self.android_devices

        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 1:
            ads[0].log.error("Total number of call ids is not 1.")
            return False
        call_conf_id = None
        for call_id in calls:
            if call_id != call_ab_id and call_id != call_ac_id:
                call_conf_id = call_id
        if not call_conf_id:
            self.log.error("Merge call fail, no new conference call id.")
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        # Check if Conf Call is currently active
        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            ads[0].log.error(
                "Call_id: %s, state: %s, expected: STATE_ACTIVE", call_conf_id,
                ads[0].droid.telecomCallGetCallState(call_conf_id))
            return False

        self.log.info("Step5: End call on PhoneC and verify call continues.")
        if not self._hangup_call(ads[2], "PhoneC"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        self.log.info("Step6: End call on PhoneB and verify PhoneA end.")
        if not self._hangup_call(ads[1], "PhoneB"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    """ Tests Begin """

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3cd45972-3862-4956-9504-7fefacdd5ca6")
    def test_wcdma_mo_mo_add_merge_drop(self):
        """ Test Conf Call among three phones.

        Call from PhoneA to PhoneB, accept on PhoneB.
        Call from PhoneA to PhoneC, accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c1158bd3-6327-4c91-96a7-400e69f68698")
    def test_wcdma_mt_mt_add_merge_drop(self):
        """ Test Conf Call among three phones.

        Call from PhoneB to PhoneA, accept on PhoneA.
        Call from PhoneC to PhoneA, accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_wcdma_mt_mt_add_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="804478b4-a826-48be-a9fa-9a0cec66ee54")
    def test_1x_mo_mo_add_merge_drop_from_participant(self):
        """ Test 1x Conf Call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from DUT to PhoneB, accept on PhoneB.
        3. Call from DUT to PhoneC, accept on PhoneC.
        4. On DUT, merge to conference call.
        5. End call PhoneC, verify call continues on DUT and PhoneB.
        6. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        4. Merge Call succeed on DUT.
        5. PhoneC drop call, DUT and PhoneB call continues.
        6. PhoneB drop call, call also end on DUT.

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mo_add()
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Merge to Conf Call and verify Conf Call.")
        if not self._test_1x_merge_conference(ads[0], [ads[1], ads[2]],
                                              call_conf_id):
            self.log.error("1x Conference merge failed.")

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(
            ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a36b02a6-480e-4cb6-9201-bd8bfa5ae8a4")
    def test_1x_mo_mo_add_merge_drop_from_host(self):
        """ Test 1x Conf Call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from DUT to PhoneB, accept on PhoneB.
        3. Call from DUT to PhoneC, accept on PhoneC.
        4. On DUT, merge to conference call.
        5. End call on DUT, make sure all participants drop.

        Expected Results:
        4. Merge Call succeed on DUT.
        5. Make sure DUT and all participants drop call.

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mo_add()
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Merge to Conf Call and verify Conf Call.")
        if not self._test_1x_merge_conference(ads[0], [ads[1], ads[2]],
                                              call_conf_id):
            self.log.error("1x Conference merge failed.")

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_conf_call_drop_from_host(ads[0], [ads[2], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="0c9e5da6-90db-4cb5-9b2c-4be3460b49d0")
    def test_1x_mo_mt_add_drop_active(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from DUT to PhoneB, accept on PhoneB.
        3. Call from PhoneC to DUT, accept on DUT.
        4. End call PhoneC, verify call continues on DUT and PhoneB.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        4. PhoneC drop call, DUT and PhoneB call continues.
        5. PhoneB drop call, call also end on DUT.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            0)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(
            ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="9dc16b45-3470-44c8-abf8-19cd5944a53c")
    def test_1x_mo_mt_add_swap_twice_drop_active(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. DUT MO call to PhoneB, answer on PhoneB.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Swap active call on DUT.
        6. Drop on PhoneC.
        7. Drop on PhoneB.

        Expected Results:
        4. Swap call succeed.
        5. Swap call succeed.
        6. Call between DUT and PhoneB continues.
        7. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            2)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(
            ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="dc7a3187-142e-4754-a914-d0241397a2b3")
    def test_1x_mo_mt_add_swap_once_drop_active(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. DUT MO call to PhoneB, answer on PhoneB.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Drop on PhoneB.
        6. Drop on PhoneC.

        Expected Results:
        4. Swap call succeed.
        5. Call between DUT and PhoneC continues.
        6. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            1)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneB, and end call on PhoneC.")
        return self._test_1x_multi_call_drop_from_participant(
            ads[0], ads[1], ads[2])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="24cd0ef0-1a69-4603-89c2-0f2b96715348")
    def test_1x_mo_mt_add_drop_held(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from DUT to PhoneB, accept on PhoneB.
        3. Call from PhoneC to DUT, accept on DUT.
        4. End call PhoneB, verify call continues on DUT and PhoneC.
        5. End call on PhoneC, verify call end on PhoneA.

        Expected Results:
        4. DUT drop call, PhoneC also end. Then DUT receive callback from PhoneB.
        5. DUT drop call, call also end on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            0)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneB, and end call on PhoneC.")
        return self._test_1x_multi_call_drop_from_participant(
            ads[0], ads[1], ads[2])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="1c5c1780-84c2-4547-9e57-eeadac6569d7")
    def test_1x_mo_mt_add_swap_twice_drop_held(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. DUT MO call to PhoneB, answer on PhoneB.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Swap active call on DUT.
        6. Drop on PhoneB.
        7. Drop on PhoneC.

        Expected Results:
        4. Swap call succeed.
        5. Swap call succeed.
        6. Call between DUT and PhoneC continues.
        7. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            2)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneB, and end call on PhoneC.")
        return self._test_1x_multi_call_drop_from_participant(
            ads[0], ads[1], ads[2])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="928a2b21-c4ca-4553-9acc-8d3db61ed6eb")
    def test_1x_mo_mt_add_swap_once_drop_held(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. DUT MO call to PhoneB, answer on PhoneB.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Drop on PhoneC.
        6. Drop on PhoneB.

        Expected Results:
        4. Swap call succeed.
        5. Call between DUT and PhoneB continues.
        6. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            1)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(
            ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="deb57627-a717-41f0-b8f4-f3ccf9ce2e15")
    def test_1x_mo_mt_add_drop_on_dut(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from DUT to PhoneB, accept on PhoneB.
        3. Call from PhoneC to DUT, accept on DUT.
        4. End call on DUT.
        5. End call on DUT.

        Expected Results:
        4. DUT drop call, PhoneC also end. Then DUT receive callback from PhoneB.
        5. DUT drop call, call also end on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            0)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on DUT, DUT should receive callback.")
        return self._test_1x_multi_call_drop_from_host(ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="9cdee9c2-98cf-40de-9396-516192e493a1")
    def test_1x_mo_mt_add_swap_twice_drop_on_dut(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. DUT MO call to PhoneB, answer on PhoneB.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Swap active call on DUT.
        6. Drop current call on DUT.
        7. Drop current call on DUT.

        Expected Results:
        4. Swap call succeed.
        5. Swap call succeed.
        6. DUT drop call, PhoneC also end. Then DUT receive callback from PhoneB.
        7. DUT drop call, call also end on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            2)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on DUT, DUT should receive callback.")
        return self._test_1x_multi_call_drop_from_host(ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="26187827-64c0-436e-9792-20c216aeb442")
    def test_1x_mo_mt_add_swap_once_drop_on_dut(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. DUT MO call to PhoneB, answer on PhoneB.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Drop current call on DUT.
        6. Drop current call on DUT.

        Expected Results:
        4. Swap call succeed.
        5. DUT drop call, PhoneB also end. Then DUT receive callback from PhoneC.
        6. DUT drop call, call also end on PhoneC.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            1)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on DUT, DUT should receive callback.")
        return self._test_1x_multi_call_drop_from_host(ads[0], ads[1], ads[2])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ce590b72-b4ab-4a27-9c01-f8e3b110419f")
    def test_1x_mt_mt_add_drop_active(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from PhoneB to DUT, accept on DUT.
        3. Call from PhoneC to DUT, accept on DUT.
        4. End call PhoneC, verify call continues on DUT and PhoneB.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        4. PhoneC drop call, DUT and PhoneB call continues.
        5. PhoneB drop call, call also end on DUT.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            0)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(
            ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="736aa74e-1d0b-4f85-b0f7-11840543cf54")
    def test_1x_mt_mt_add_swap_twice_drop_active(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. PhoneB call to DUT, answer on DUT.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Swap active call on DUT.
        6. Drop on PhoneC.
        7. Drop on PhoneB.

        Expected Results:
        4. Swap call succeed.
        5. Swap call succeed.
        6. Call between DUT and PhoneB continues.
        7. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            2)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(
            ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3eee6b6e-e1b1-43ec-82d5-d298b514fc07")
    def test_1x_mt_mt_add_swap_once_drop_active(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. PhoneB call to DUT, answer on DUT.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Drop on PhoneB.
        6. Drop on PhoneC.

        Expected Results:
        4. Swap call succeed.
        5. Call between DUT and PhoneC continues.
        6. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            1)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneB, and end call on PhoneC.")
        return self._test_1x_multi_call_drop_from_participant(
            ads[0], ads[1], ads[2])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="432549a9-e4bb-44d3-bd44-befffc1af02d")
    def test_1x_mt_mt_add_drop_held(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from PhoneB to DUT, accept on DUT.
        3. Call from PhoneC to DUT, accept on DUT.
        4. End call PhoneB, verify call continues on DUT and PhoneC.
        5. End call on PhoneC, verify call end on PhoneA.

        Expected Results:
        4. PhoneB drop call, DUT and PhoneC call continues.
        5. PhoneC drop call, call also end on DUT.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            0)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneB, and end call on PhoneC.")
        return self._test_1x_multi_call_drop_from_participant(
            ads[0], ads[1], ads[2])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c8f30fc1-8586-4eb0-854e-264989fd69b8")
    def test_1x_mt_mt_add_swap_twice_drop_held(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. PhoneB call to DUT, answer on DUT.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Swap active call on DUT.
        6. Drop on PhoneB.
        7. Drop on PhoneC.

        Expected Results:
        4. Swap call succeed.
        5. Swap call succeed.
        6. Call between DUT and PhoneC continues.
        7. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            2)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneB, and end call on PhoneC.")
        return self._test_1x_multi_call_drop_from_participant(
            ads[0], ads[1], ads[2])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="065ba51e-9843-4018-8009-7fdc6590011d")
    def test_1x_mt_mt_add_swap_once_drop_held(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. PhoneB call to DUT, answer on DUT.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Drop on PhoneC.
        6. Drop on PhoneB.

        Expected Results:
        4. Swap call succeed.
        5. Call between DUT and PhoneB continues.
        6. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            1)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(
            ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="69c69449-d430-4f00-ae19-c51242561ac9")
    def test_1x_mt_mt_add_drop_on_dut(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from PhoneB to DUT, accept on DUT.
        3. Call from PhoneC to DUT, accept on DUT.
        4. End call on DUT.
        5. End call on DUT.

        Expected Results:
        4. DUT drop call, PhoneC also end. Then DUT receive callback from PhoneB.
        5. DUT drop call, call also end on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            0)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on DUT, DUT should receive callback.")
        return self._test_1x_multi_call_drop_from_host(ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="282583a3-d455-4caf-a184-718f8bbccb91")
    def test_1x_mt_mt_add_swap_twice_drop_on_dut(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. PhoneB call to DUT, answer on DUT.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Swap active call on DUT.
        6. Drop current call on DUT.
        7. Drop current call on DUT.

        Expected Results:
        4. Swap call succeed.
        5. Swap call succeed.
        6. DUT drop call, PhoneC also end. Then DUT receive callback from PhoneB.
        7. DUT drop call, call also end on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            2)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on DUT, DUT should receive callback.")
        return self._test_1x_multi_call_drop_from_host(ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="1cf83159-d230-41a4-842c-064be5ef11e6")
    def test_1x_mt_mt_add_swap_once_drop_on_dut(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. PhoneB call to DUT, answer on DUT.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Drop current call on DUT.
        6. Drop current call on DUT.

        Expected Results:
        4. Swap call succeed.
        5. DUT drop call, PhoneB also end. Then DUT receive callback from PhoneC.
        6. DUT drop call, call also end on PhoneC.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            1)
        if ((call_ab_id is None) or (call_ac_id is None)
                or (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on DUT, DUT should receive callback.")
        return self._test_1x_multi_call_drop_from_host(ads[0], ads[1], ads[2])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="602db3cb-e02b-4e4c-9043-338e1231f51b")
    def test_volte_mo_mo_add_volte_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        Call from PhoneA (VoLTE) to PhoneC (VoLTE), accept on PhoneC.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="7c9ae738-9031-4a77-9ff7-356a186820a5")
    def test_volte_mo_mo_add_volte_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (VoLTE), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f50e7e94-0956-41c4-b02b-384a12668f10")
    def test_volte_mo_mo_add_volte_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (VoLTE), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a6c22e39-fd7e-4bed-982a-145065572281")
    def test_volte_mo_mo_add_volte_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (VoLTE), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="2188722a-31e3-4e46-8f74-6ea4cbc08476")
    def test_volte_mo_mo_add_volte_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (VoLTE), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ef5ec1c9-7771-4289-ad94-08a80145d680")
    def test_volte_mo_mt_add_volte_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="2111001d-c310-4eff-a6ef-201d199796ea")
    def test_volte_mo_mt_add_volte_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="eee3577b-5427-43ee-aff0-ed7f7846b41c")
    def test_volte_mo_mt_add_volte_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="86faf200-be78-452d-8662-85e7f42a2d3b")
    def test_volte_mo_mt_add_volte_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d0e18f3c-71a1-49c9-b3ad-b8c24f8a43ec")
    def test_volte_mo_mt_add_volte_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="b27d6b3d-b73b-4a20-a5ae-2990d73a07fe")
    def test_volte_mt_mt_add_volte_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneB (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f66e940c-30bd-48c7-b5e2-91147fa04ba2")
    def test_volte_mt_mt_add_volte_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ad313a8b-8bb0-43eb-a10e-e2c17f530ee4")
    def test_volte_mt_mt_add_volte_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="18b30c14-fef1-4055-8987-ee6137609b81")
    def test_volte_mt_mt_add_volte_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="7dc24162-f06e-453b-93e6-926d31e6d387")
    def test_volte_mt_mt_add_volte_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="eb90a56b-2085-4fde-a156-ada3620200df")
    def test_volte_mo_mo_add_wcdma_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        Call from PhoneA (VOLTE) to PhoneC (WCDMA), accept on PhoneC.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ae999260-7856-41cc-bf4c-67b26e18c9a3")
    def test_volte_mo_mo_add_wcdma_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (WCDMA), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="cd48de00-c1e5-4716-b232-3f1f98e89510")
    def test_volte_mo_mo_add_wcdma_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (WCDMA), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a84ea6e8-dabc-4bab-b6d1-700b0a0fb9e9")
    def test_volte_mo_mo_add_wcdma_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (WCDMA), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="7ac9a806-c608-42dd-a4fd-66b0ba535434")
    def test_volte_mo_mo_add_wcdma_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (WCDMA), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="35f9eb31-3a77-457c-aeb0-55a73c60dda1")
    def test_volte_mo_mt_add_wcdma_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f3314f74-e929-45ed-91cb-27c1c26e240f")
    def test_volte_mo_mt_add_wcdma_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="5e521ff1-505b-4d63-8b12-7b0187dea94b")
    def test_volte_mo_mt_add_wcdma_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d5732ea2-a657-40ea-bb30-151e53cf8058")
    def test_volte_mo_mt_add_wcdma_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="78e73444-3dde-465f-bf5e-dc48b40a93f3")
    def test_volte_mo_mt_add_wcdma_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f4efbf04-b117-4508-ba86-0ef37481cc3a")
    def test_volte_mt_mt_add_wcdma_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneB (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="064109cf-d166-448a-8655-81744ea37e05")
    def test_volte_mt_mt_add_wcdma_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="bedd0576-5bb6-4fef-9700-f638cf742201")
    def test_volte_mt_mt_add_wcdma_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="46178387-a0dc-4e77-8ca4-06f731e1104f")
    def test_volte_mt_mt_add_wcdma_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a1d13168-078b-47d8-89f0-0798b085502d")
    def test_volte_mt_mt_add_wcdma_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="08a26dc4-78e5-47cb-af75-9695453e82bb")
    def test_volte_mo_mo_add_1x_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        Call from PhoneA (VOLTE) to PhoneC (1x), accept on PhoneC.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="bde45028-b844-4192-89b1-8579941a03ed")
    def test_volte_mo_mo_add_1x_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (1x), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f38d3031-d7f1-4990-bce3-9c329beb5eeb")
    def test_volte_mo_mo_add_1x_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (1x), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d1391513-8592-4159-81b7-16cb10c406e8")
    def test_volte_mo_mo_add_1x_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (1x), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e05c261e-e99a-4ca7-a8db-9ad982e06913")
    def test_volte_mo_mo_add_1x_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (1x), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f4329201-a388-4070-9225-37d4c8045096")
    def test_volte_mo_mt_add_1x_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="fafa96ef-649a-4ff7-8fed-d4bfd6d88c2e")
    def test_volte_mo_mt_add_1x_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="66d79e0b-879d-461c-bf5d-b27495f73754")
    def test_volte_mo_mt_add_1x_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a5f2a3d0-9b00-4496-8316-ea626b1c978a")
    def test_volte_mo_mt_add_1x_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="98cfd8d8-200f-4820-94ed-1561df1ed152")
    def test_volte_mo_mt_add_1x_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c9ee6bb1-4aee-4fc9-95b0-f899d3d31d82")
    def test_volte_mt_mt_add_1x_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneB (1x) to PhoneA (VoLTE), accept on PhoneA.
        Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f4fb92a1-d4a0-4796-bdb4-f441b926c63c")
    def test_volte_mt_mt_add_1x_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (1x) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="8ad0e672-83cc-463a-aa12-d331faa5eb17")
    def test_volte_mt_mt_add_1x_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (1x) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3dad2fd1-d2c0-477c-a758-3c054df6e92a")
    def test_volte_mt_mt_add_1x_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (1x) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e382469b-8767-47fd-b3e6-8c81d8fb45ef")
    def test_volte_mt_mt_add_1x_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (1x) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="98db2430-07c2-4ec7-8644-2aa081a3eb22")
    def test_volte_mo_mo_add_volte_swap_twice_drop_held(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="7504958b-172b-4afc-97ea-9562b8429dfe")
    def test_volte_mo_mo_add_volte_swap_twice_drop_active(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ac02ce22-fd8c-48ba-9e68-6748d1e48c68")
    def test_volte_mo_mt_add_volte_swap_twice_drop_held(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="2fb2c4f6-1c14-4122-bc3e-a7a6416003a3")
    def test_volte_mo_mt_add_volte_swap_twice_drop_active(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="deed9c13-2e9d-464a-b8f7-62e91265451d")
    def test_volte_mo_mo_add_volte_swap_once_drop_held(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3324a4d3-68db-41a4-b0d0-3e8e82b84f46")
    def test_volte_mo_mo_add_volte_swap_once_drop_active(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="57c8c3f6-c690-41c3-aaed-98e5548cc4b6")
    def test_volte_mo_mt_add_volte_swap_once_drop_held(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False
        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="96376148-f069-41f9-b22f-f5240de427f7")
    def test_volte_mo_mt_add_volte_swap_once_drop_active(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="baac4ef4-198b-4a33-a09a-5507c6aa740d")
    def test_wcdma_mo_mo_add_swap_twice_drop_held(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="8c2588ff-3857-49eb-8db0-9e76d7c99c68")
    def test_wcdma_mo_mo_add_swap_twice_drop_active(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="16a6aa3c-fe68-41b4-af42-75847062d4ec")
    def test_wcdma_mo_mt_add_swap_twice_drop_held(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f38c4cd5-53b4-43d0-a5fa-2a008a96cedc")
    def test_wcdma_mo_mt_add_swap_twice_drop_active(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="734689e8-2acd-405f-be93-db62e2606252")
    def test_wcdma_mo_mo_add_swap_once_drop_held(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="126f294e-590a-4392-8fbd-1b826cd97214")
    def test_wcdma_mo_mo_add_swap_once_drop_active(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e5f36f28-ec2b-4958-8578-c0454ef2a8ad")
    def test_wcdma_mo_mt_add_swap_once_drop_held(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a2252ebe-3ee2-4b9e-b76b-6be68d6b2719")
    def test_wcdma_mo_mt_add_swap_once_drop_active(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="cba48f87-4026-422d-a760-f913d2763ee9")
    def test_csfb_wcdma_mo_mo_add_swap_twice_drop_held(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mo_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3e6bd083-ccae-4962-a3d7-4194ed685b64")
    def test_csfb_wcdma_mo_mo_add_swap_twice_drop_active(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mo_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a1972846-0bca-4583-a966-11ebf0670c04")
    def test_csfb_wcdma_mo_mt_add_swap_twice_drop_held(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="98609dfd-07fb-4414-bf6e-46144205fe70")
    def test_csfb_wcdma_mo_mt_add_swap_twice_drop_active(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="0c4c9b0b-ef7a-4e3d-9d31-dde721444820")
    def test_csfb_wcdma_mo_mo_add_swap_once_drop_held(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ee0f7772-9e58-4a00-8eb5-a03b3e5baf40")
    def test_csfb_wcdma_mo_mo_add_swap_once_drop_active(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="2f9e0120-c052-467b-be45-313ada433dce")
    def test_csfb_wcdma_mo_mt_add_swap_once_drop_held(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="9110ad34-04e4-4d0f-9ac0-183ad9e6fa8a")
    def test_csfb_wcdma_mo_mt_add_swap_once_drop_active(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="8cca4681-a5d4-472a-b0b5-46b79a6892a7")
    def test_volte_mo_mo_add_volte_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="9168698e-987a-42e3-8bc6-2a433fa4b5e3")
    def test_volte_mo_mo_add_volte_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="397bfab8-6651-4438-a603-22657fd69b84")
    def test_volte_mo_mo_add_volte_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ba766c4c-690f-407b-87f8-05a91783e224")
    def test_volte_mo_mo_add_volte_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="1e45c749-6d4c-4d30-b104-bfcc37e13f1d")
    def test_volte_mo_mo_add_volte_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d6648e19-8001-483a-a83b-a92b638a7650")
    def test_volte_mo_mo_add_volte_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="18d3c304-ad04-409f-bfd4-fd3f01f9a51e")
    def test_volte_mo_mo_add_volte_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f1e71550-3a45-45e4-88e4-7e4da919d58e")
    def test_volte_mo_mo_add_volte_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="60cc8bd1-4270-4b5c-9d29-9fa36a357503")
    def test_volte_mo_mo_add_volte_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="7d81c5f8-5ee1-4ad2-b08e-3e4c97748a66")
    def test_volte_mo_mo_add_volte_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="5a887fde-8f9b-436b-ae8d-5ff97a884fc9")
    def test_volte_mo_mt_add_volte_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="82574508-0d77-42cf-bf60-9fdddee24c42")
    def test_volte_mo_mt_add_volte_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="8f30c425-c689-43b8-84f3-f24f7d9216f6")
    def test_volte_mo_mt_add_volte_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="2a67845f-ccfd-4c06-aedc-9d6c1f022959")
    def test_volte_mo_mt_add_volte_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="0a5d1305-0890-40c4-8c7a-070876423e16")
    def test_volte_mo_mt_add_volte_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="77bbb5ec-fd16-4031-b316-7f6563b79a3d")
    def test_volte_mo_mt_add_volte_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="09cd41cd-821d-4d09-9b1e-7330dca77150")
    def test_volte_mo_mt_add_volte_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="bdb2791e-92d0-44a3-aa8f-bd83e8159cb7")
    def test_volte_mo_mt_add_volte_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="4f54e67d-7c7a-4952-ae09-f940094ec1ff")
    def test_volte_mo_mt_add_volte_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="4ca28f9f-098f-4f71-b89c-9b2793aa2f5f")
    def test_volte_mo_mt_add_volte_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f025c100-bb77-436e-b8ab-0c23a3d43318")
    def test_volte_mt_mt_add_volte_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="b6cd6b06-0984-4588-892e-939d332bf147")
    def test_volte_mt_mt_add_volte_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f7bda827-79bd-41cc-b720-140f49b381d9")
    def test_volte_mt_mt_add_volte_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="de8aae4e-c8fc-4996-9d0b-56c3904c9bb4")
    def test_volte_mt_mt_add_volte_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="35d72ebf-1a99-4571-8993-2899925f5489")
    def test_volte_mt_mt_add_volte_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="96c4c9d2-2e00-41a8-b4ce-3a9377262d36")
    def test_volte_mt_mt_add_volte_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e1e0bb6c-c2d5-4566-80b1-46bfb0b95544")
    def test_volte_mt_mt_add_volte_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="764b5930-ba96-4901-b629-e753bc5c8d8e")
    def test_volte_mt_mt_add_volte_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e03e52a1-7e7b-4a53-a496-3092a35153ae")
    def test_volte_mt_mt_add_volte_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="10a63692-56de-4563-b223-91e8814ddbc9")
    def test_volte_mt_mt_add_volte_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="cf4b1fe2-40d7-409b-a4ec-06bdb9a0d957")
    def test_volte_mo_mo_add_wcdma_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="fcc9dbd2-919d-4552-838c-2b672fb24b2b")
    def test_volte_mo_mo_add_wcdma_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="37021d31-4242-46a2-adbf-eb7b987e4a43")
    def test_volte_mo_mo_add_wcdma_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="cc362118-ccd1-4502-96f1-b3584b0c6bf3")
    def test_volte_mo_mo_add_wcdma_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="be57dd4f-020f-4491-9cd0-2461dcd0806b")
    def test_volte_mo_mo_add_wcdma_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="058425ca-7b25-4a85-a5c7-bfdcd7920f15")
    def test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="5f691165-d099-419d-a50e-1547c8677f6b")
    def test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="4fc8459e-23c6-408a-a061-e8f182086dd6")
    def test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3026802f-ddca-41e4-ae6f-db0c994613a7")
    def test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="5ce98774-66b4-4710-b0da-e43bb7fa1c0f")
    def test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="61766517-8762-4db3-9366-609c0f1a43b5")
    def test_volte_mo_mt_add_wcdma_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="9a809168-1ca4-4672-a5b3-934aeed0f06c")
    def test_volte_mo_mt_add_wcdma_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="4a146608-fbc7-4828-b2ee-77e08c19ddec")
    def test_volte_mo_mt_add_wcdma_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="4a3d4c0b-912a-4922-a4a3-f0bdf600c376")
    def test_volte_mo_mt_add_wcdma_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="614903e0-7091-4791-9af6-076799e43a97")
    def test_volte_mo_mt_add_wcdma_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="cea2ffc9-b8c8-46df-8f58-b606f3d352e2")
    def test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="4b0e040a-f199-4dc4-9f98-eb923f79814e")
    def test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="56d45561-8e9b-40d4-bacc-a4c5ac4c2af0")
    def test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="790ccd53-07e6-471b-a224-b7eeb3a83116")
    def test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="84083401-22af-4568-929a-4fd29e39db5c")
    def test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="11a3d9cd-6f05-4638-9683-3ee475e41fdb")
    def test_volte_mt_mt_add_wcdma_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ff03b64e-fccd-45e3-8245-f8c6cb328212")
    def test_volte_mt_mt_add_wcdma_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c1822404-df8e-4b0c-b5d2-f70bb8c3119d")
    def test_volte_mt_mt_add_wcdma_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3ebf7526-57f9-47f5-9196-b483384d6759")
    def test_volte_mt_mt_add_wcdma_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="8b8e664d-e872-493f-bff9-34a9500b2c25")
    def test_volte_mt_mt_add_wcdma_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="aa4079dd-b5c6-41a3-ae0f-99f17fd0bae0")
    def test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d9f32013-529c-46c3-9963-405ebbdbc537")
    def test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="93331954-4403-47ec-8af3-1a791ea2fc8b")
    def test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c5a47e45-166b-46db-8b12-734d5d062509")
    def test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="05bff9cf-492d-4511-ac0e-b54f1d5fa454")
    def test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f02add29-d61a-42f4-a1f0-271815e03c45")
    def test_volte_mo_mo_add_1x_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="776821e8-259b-441e-a126-45a990e6e14d")
    def test_volte_mo_mo_add_1x_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c542c999-d1c7-47f9-ba5c-50a582afa9e5")
    def test_volte_mo_mo_add_1x_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d40c681a-1bce-4012-a30c-774e6698ba3a")
    def test_volte_mo_mo_add_1x_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="2805f272-5ada-4dd4-916a-36070905aec4")
    def test_volte_mo_mo_add_1x_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="eb2d010c-ec32-409b-8272-5b950e0076f9")
    def test_volte_mo_mo_add_1x_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="03bb2fa3-8596-4c66-8adb-a3f9c9085420")
    def test_volte_mo_mo_add_1x_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="df3bb0e7-7d72-487a-8cb8-fa75b86662ef")
    def test_volte_mo_mo_add_1x_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3893ce25-0fad-4eb8-af35-f9ebf47c0615")
    def test_volte_mo_mo_add_1x_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="fcd0e656-4e98-40f2-b0ce-5ed90c7c4a81")
    def test_volte_mo_mo_add_1x_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="009b5b45-d863-4891-8403-06656b542f3d")
    def test_volte_mo_mt_add_1x_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="8cfe9354-70db-4dbb-8950-b02e65f9d6ba")
    def test_volte_mo_mt_add_1x_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d2502a9f-b35a-4390-b664-300b8310b55a")
    def test_volte_mo_mt_add_1x_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="1fbddcd1-2268-4c2c-a737-c0bcbdc842cb")
    def test_volte_mo_mt_add_1x_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="91a33ca8-508b-457e-a72f-6fabd00b2453")
    def test_volte_mo_mt_add_1x_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3642995f-4de0-4327-86d6-c9e37416c7e7")
    def test_volte_mo_mt_add_1x_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="dd11f3af-09af-4fa3-9c90-8497bbd8687e")
    def test_volte_mo_mt_add_1x_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="172360a1-47b2-430a-8a9c-cae6d29813b6")
    def test_volte_mo_mt_add_1x_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="05d63cef-45b6-47df-8999-aac2e6ecfc9f")
    def test_volte_mo_mt_add_1x_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e02dfbc5-ffa3-4bff-a45e-1b3f4147005e")
    def test_volte_mo_mt_add_1x_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="55bb85a0-bfbd-4b97-bd94-871e82276875")
    def test_volte_mt_mt_add_1x_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="31e71818-3522-4e04-8f26-ad26683f16d1")
    def test_volte_mt_mt_add_1x_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="0b51183d-3f92-4fe8-9487-64a72696a838")
    def test_volte_mt_mt_add_1x_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="2aee2db7-fcd0-4b0a-aaa0-b946a14e91bd")
    def test_volte_mt_mt_add_1x_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d78bbe8e-6d39-4843-9eaa-47f06b6e9a95")
    def test_volte_mt_mt_add_1x_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="bbf4dcd8-3e1d-4ad0-a4fd-bf875e409f15")
    def test_volte_mt_mt_add_1x_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c222e199-497b-4c34-886d-b35592ccd3b2")
    def test_volte_mt_mt_add_1x_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d916eef5-b942-48fe-9c63-be7e283b197d")
    def test_volte_mt_mt_add_1x_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e34acc9d-4389-4bc3-b34d-6b7e8173cadf")
    def test_volte_mt_mt_add_1x_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="31ddc496-03a5-407f-b925-8cea04dbdae0")
    def test_volte_mt_mt_add_1x_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="35b6fa0f-780f-4436-93a0-70f9b79bc71a")
    def test_csfb_wcdma_mo_mo_add_swap_once_merge_drop(self):
        """Test swap and merge feature in CSFB WCDMA call.

        PhoneA (CSFB_WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB_WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="9102ce81-0c17-4c7a-93df-f240245a07c5")
    def test_csfb_wcdma_mo_mo_add_swap_twice_merge_drop(self):
        """Test swap and merge feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mo_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="b85ff5a7-f512-4585-a6b1-d92c9e7c25de")
    def test_csfb_wcdma_mo_mt_add_swap_once_merge_drop(self):
        """Test swap and merge feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="83a74ce1-9028-4e67-a417-599616ab0b2c")
    def test_csfb_wcdma_mo_mt_add_swap_twice_merge_drop(self):
        """Test swap and merge feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="835fe3d6-4e44-451c-9c5f-277b8842bf10")
    def test_wcdma_mo_mo_add_swap_once_merge_drop(self):
        """Test swap and merge feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="627eb239-c6f6-47dc-b4cf-8d1796599417")
    def test_wcdma_mo_mo_add_swap_twice_merge_drop(self):
        """Test swap and merge feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="571efb21-0ed5-4871-a7d8-f86d94e0ef25")
    def test_wcdma_mo_mt_add_swap_once_merge_drop(self):
        """Test swap and merge feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_wcdma_mo_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c981a90a-19b8-4582-a07c-1e9447fbe9ae")
    def test_wcdma_mo_mt_add_swap_twice_merge_drop(self):
        """Test swap and merge feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_wcdma_mo_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="23843881-320b-4071-bbc8-93cd1ee08408")
    def test_wcdma_mt_mt_add_swap_once_merge_drop(self):
        """Test swap and merge feature in WCDMA call.

        PhoneB call PhoneA (WCDMA), accept on PhoneA.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_wcdma_mt_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e447ec45-016c-4723-a954-4bde2e342cb2")
    def test_wcdma_mt_mt_add_swap_twice_merge_drop(self):
        """Test swap and merge feature in WCDMA call.

        PhoneB call PhoneA (WCDMA), accept on PhoneA.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_wcdma_mt_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="876dc6b2-75c2-4fb5-92a0-35d3d29fbd42")
    def test_wcdma_mt_mt_add_merge_unmerge_swap_drop(self):
        """Test Conference Call Unmerge operation.

        Phones A, B, C are in WCDMA Conference call (MT-MT-Merge)
        Unmerge call with B on PhoneA
        Check the number of Call Ids to be 2 on PhoneA
        Check if call AB is active since 'B' was unmerged
        Swap call to C
        Check if call AC is active
        Tear down calls
        All Phones should be in Idle

        """
        call_ab_id, call_ac_id = self._test_wcdma_mt_mt_add_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            self.log.error("Either of Call AB ID or Call AC ID is None.")
            return False

        ads = self.android_devices

        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 3:
            ads[0].log.error("Total number of call ids is not 3.")
            return False
        call_conf_id = None
        for call_id in calls:
            if call_id != call_ab_id and call_id != call_ac_id:
                call_conf_id = call_id
        if not call_conf_id:
            self.log.error("Merge call fail, no new conference call id.")
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        # Check if Conf Call currently active
        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            ads[0].log.error(
                "Call_id: %s, state: %s, expected: STATE_ACTIVE", call_conf_id,
                ads[0].droid.telecomCallGetCallState(call_conf_id))
            return False

        # Unmerge
        self.log.info("Step5: UnMerge Conf Call into individual participants.")
        ads[0].droid.telecomCallSplitFromConf(call_ab_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)

        # Are there 2 calls?
        if num_active_calls(self.log, ads[0]) != 2:
            ads[0].log.error("Total number of call ids is not 2")
            return False

        # Unmerged calls not dropped?
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            self.log.error("Either Call_AB or Call_AC was dropped")
            return False

        # Unmerged call in call state ACTIVE?
        if ads[0].droid.telecomCallGetCallState(
                call_ab_id) != CALL_STATE_ACTIVE:
            ads[0].log.error("Call_id: %s, state:%s, expected: STATE_ACTIVE",
                             call_ab_id,
                             ads[0].droid.telecomCallGetCallState(call_ab_id))
            return False

        # Swap call
        self.log.info("Step6: Swap call and see if Call_AC is ACTIVE.")
        num_swaps = 1
        if not swap_calls(self.log, ads, call_ac_id, call_ab_id, num_swaps):
            self.log.error("Failed to swap calls.")
            return False

        # Other call in call state ACTIVE?
        if ads[0].droid.telecomCallGetCallState(
                call_ac_id) != CALL_STATE_ACTIVE:
            ads[0].log.error("Call_id: %s, state: %s, expected: STATE_ACTIVE",
                             call_ac_id,
                             ads[0].droid.telecomCallGetCallState(call_ac_id))
            return False

        # All calls still CONNECTED?
        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="63da439e-23f3-4c3c-9e7e-3af6500342c5")
    def test_epdg_mo_mo_add_epdg_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (epdg), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (epdg), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="89b9f228-97a6-4e5c-96b9-a7f87d847c22")
    def test_epdg_mo_mo_add_epdg_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (epdg), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (epdg), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="2c6dc281-59b0-4ea4-b811-e4c3a4d654ab")
    def test_epdg_mo_mt_add_epdg_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (epdg), accept on PhoneB.
        Call from PhoneC (epdg) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f6727241-b727-4eb8-8c0d-f61d3a14a635")
    def test_epdg_mo_mt_add_epdg_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (epdg), accept on PhoneB.
        Call from PhoneC (epdg) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c9e54db0-2b0b-428b-ba63-619ad0b8637b")
    def test_epdg_mt_mt_add_epdg_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneB (epdg) to PhoneA (epdg), accept on PhoneA.
        Call from PhoneC (epdg) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a478cc82-d95c-43fc-9735-d8333b8937e2")
    def test_epdg_mt_mt_add_epdg_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneB (epdg) to PhoneA (epdg), accept on PhoneA.
        Call from PhoneC (epdg) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="b1acb263-1481-44a5-b18e-58bdeff7bc1e")
    def test_epdg_mo_mo_add_volte_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (VoLTE), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (VoLTE), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="03b8a0d2-80dd-465a-ad14-5db94cdbcc53")
    def test_epdg_mo_mo_add_volte_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (VoLTE), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (VoLTE), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="9eb1a816-1e2c-41da-b083-2026163a3893")
    def test_epdg_mo_mt_add_volte_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (VoLTE), accept on PhoneB.
        Call from PhoneC (VoLTE) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3d66a1b6-916f-4221-bd99-21ff4d40ebb8")
    def test_epdg_mo_mt_add_volte_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (VoLTE), accept on PhoneB.
        Call from PhoneC (VoLTE) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="9d8e8b2f-e2b9-4607-8c54-6233b3096123")
    def test_epdg_mo_mo_add_wcdma_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (WCDMA), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (WCDMA), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="0884206b-2471-4a7e-95aa-228379416ff8")
    def test_epdg_mo_mo_add_wcdma_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (WCDMA), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (WCDMA), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c7706af6-dc77-4002-b295-66c60aeace6b")
    def test_epdg_mo_mt_add_wcdma_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (WCDMA), accept on PhoneB.
        Call from PhoneC (WCDMA) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="b079618f-e32b-4ba0-9009-06e013805c39")
    def test_epdg_mo_mt_add_wcdma_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (WCDMA), accept on PhoneB.
        Call from PhoneC (WCDMA) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="571fe98b-354f-4038-8441-0e4b1840eb7a")
    def test_epdg_mo_mo_add_1x_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (1x), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (1x), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="8f531f3c-493e-43d6-9d6d-f4990b5feba4")
    def test_epdg_mo_mo_add_1x_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (1x), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (1x), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="00e1194b-3c06-46c4-8764-0339c0aa9f9e")
    def test_epdg_mo_mt_add_1x_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (1x), accept on PhoneB.
        Call from PhoneC (1x) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c94f6444-d265-4277-9555-57041e3c4ff4")
    def test_epdg_mo_mt_add_1x_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (1x), accept on PhoneB.
        Call from PhoneC (1x) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="0980745e-dcdc-4c56-84e3-e2ee076059ee")
    def test_epdg_mo_mo_add_epdg_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="6bf0b152-fb1c-4edc-9525-b39e8640b967")
    def test_epdg_mo_mo_add_epdg_swap_once_merge_drop_wfc_wifi_preferred(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e3013df6-98ca-4318-85ba-04011ba0a24f")
    def test_epdg_mo_mo_add_epdg_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e88bf042-8799-44c7-bc50-66ac1e1fb2ac")
    def test_epdg_mo_mo_add_epdg_swap_twice_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a8302e73-82a4-4409-b038-5c604fb4c66c")
    def test_epdg_mo_mt_add_epdg_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="80f69baf-1649-4858-b35c-b25baf79b42c")
    def test_epdg_mo_mt_add_epdg_swap_once_merge_drop_wfc_wifi_preferred(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="1ac0c067-49fb-41d9-8649-cc709bdd8926")
    def test_epdg_mo_mt_add_epdg_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="b20b1a94-048c-4f10-9261-dde79e1edb00")
    def test_epdg_mo_mt_add_epdg_swap_twice_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="402b175f-1510-4e2a-97c2-7c9ea5ce40f6")
    def test_epdg_mo_mo_add_volte_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (epdg) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="7c710fbf-4b77-4b46-9719-e17b3d047cfc")
    def test_epdg_mo_mo_add_volte_swap_once_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (epdg) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="642afbac-30c1-4dbf-bf3e-758ab6c3a306")
    def test_epdg_mo_mo_add_volte_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (epdg) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a4ae1e39-ed6d-412e-b821-321c715a5d47")
    def test_epdg_mo_mo_add_volte_swap_twice_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (epdg) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="7b8431f2-8a49-4aa9-b84d-77f16a6a2c30")
    def test_epdg_mo_mt_add_volte_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="5b4d7444-32a1-4e82-8847-1c4ae002edca")
    def test_epdg_mo_mt_add_volte_swap_once_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="88c6c179-0b56-4d03-b5e4-76a147a40995")
    def test_epdg_mo_mt_add_volte_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="7f744ab3-f919-4a7a-83ce-e38487d619cc")
    def test_epdg_mo_mt_add_volte_swap_twice_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="5c861a99-a1b8-45fc-ba67-f8fde4575efc")
    def test_epdg_mo_mo_add_wcdma_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (epdg) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="fdb32a13-302c-4c1c-a77e-f78ed7e90911")
    def test_epdg_mo_mo_add_wcdma_swap_once_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (epdg) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a2cf3366-ae66-4e8f-a682-df506173f282")
    def test_epdg_mo_mo_add_wcdma_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (epdg) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="fc5f0f1c-9610-4d0f-adce-9c8db351e7da")
    def test_epdg_mo_mo_add_wcdma_swap_twice_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (epdg) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="05332b1e-c36b-4874-b13b-f8e49d0d9bca")
    def test_epdg_mo_mt_add_wcdma_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="2421d340-f9cb-47e7-ac3e-8581e141a6d0")
    def test_epdg_mo_mt_add_wcdma_swap_once_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="7c1f6008-cf59-4e63-9285-3cf1c26bc0aa")
    def test_epdg_mo_mt_add_wcdma_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="be153f3d-0707-45d0-9ddd-4aa696e0e536")
    def test_epdg_mo_mt_add_wcdma_swap_twice_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="7235c917-a2d4-4561-bda5-630171053f8f")
    def test_epdg_mo_mo_add_1x_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneA (epdg) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="8e52b9a4-c0d1-4dcd-9359-746354124763")
    def test_epdg_mo_mo_add_1x_swap_once_merge_drop_wfc_wifi_preferred(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneA (epdg) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="49a4440f-40a1-4518-810a-6ba9f1fbc243")
    def test_epdg_mo_mo_add_1x_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneA (epdg) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="9d05bde3-50ac-4a49-a0db-2181c9b5a10f")
    def test_epdg_mo_mo_add_1x_swap_twice_merge_drop_wfc_wifi_preferred(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneA (epdg) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="5c44eb64-b184-417a-97c9-8c22c48fb731")
    def test_epdg_mo_mt_add_1x_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e16e2e81-1b59-4b02-b601-bb27b62d6468")
    def test_epdg_mo_mt_add_1x_swap_once_merge_drop_wfc_wifi_preferred(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="fd4c3b72-ea2d-4cd4-af79-b93635eda8b8")
    def test_epdg_mo_mt_add_1x_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a33d7b2b-cc22-40c3-9689-a2a14642396d")
    def test_epdg_mo_mt_add_1x_swap_twice_merge_drop_wfc_wifi_preferred(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="7839c6a3-6797-4cd0-a918-c7d317881e3d")
    def test_epdg_mo_mo_add_epdg_swap_twice_drop_held_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3f4e761e-8fa2-4b85-bc11-8150532b7686")
    def test_epdg_mo_mo_add_epdg_swap_twice_drop_held_wfc_wifi_preferred(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="9fd4f171-9eea-4fc7-91f1-d3c7f08a5fad")
    def test_epdg_mo_mo_add_epdg_swap_twice_drop_active_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="8b97d6b9-253a-4ab7-8afb-126df71fee41")
    def test_epdg_mo_mo_add_epdg_swap_twice_drop_active_wfc_wifi_preferred(
            self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="6617e779-c987-41dd-acda-ff132662ccf0")
    def test_epdg_mo_mo_add_epdg_swap_twice_drop_active_apm_wifi_preferred(
            self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f8b61289-ccc5-4adf-b291-94c73925edb3")
    def test_epdg_mo_mt_add_epdg_swap_twice_drop_held_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="bb975203-7cee-4fbc-ad4a-da473413e410")
    def test_epdg_mo_mt_add_epdg_swap_twice_drop_held_wfc_wifi_preferred(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="593f6034-fd15-4b1d-a9fe-c4331e6a7f72")
    def test_epdg_mo_mt_add_epdg_swap_twice_drop_active_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="adc3fb00-543e-44ec-905b-0eea52790896")
    def test_epdg_mo_mt_add_epdg_swap_twice_drop_active_wfc_wifi_preferred(
            self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e96aac52-c536-4a08-9e6a-8bf598db9267")
    def test_epdg_mo_mo_add_epdg_swap_once_drop_held_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="74efa176-1ff2-4b06-9739-06f67009cb5d")
    def test_epdg_mo_mo_add_epdg_swap_once_drop_held_wfc_wifi_preferred(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="dfcdeebe-dada-4722-8880-5b3877d0809b")
    def test_epdg_mo_mo_add_epdg_swap_once_drop_active_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="b9658029-90da-4df8-bbb2-9c08eb3a3a8c")
    def test_epdg_mo_mo_add_epdg_swap_once_drop_active_wfc_wifi_preferred(
            self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3381c8e0-cdf1-47d1-8a17-58592f3cd6e6")
    def test_epdg_mo_mo_add_epdg_swap_once_drop_active_apm_wfc_wifi_preferred(
            self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="fb655f12-aabe-45bf-8020-61c21ada9440")
    def test_epdg_mo_mt_add_epdg_swap_once_drop_held_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False
        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="35e7b36d-e2d5-42bd-99d9-dbc7986ef93a")
    def test_epdg_mo_mt_add_epdg_swap_once_drop_held_wfc_wifi_preferred(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False
        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e959e668-8150-46c1-bf49-a1ab2a9f45a5")
    def test_epdg_mo_mt_add_epdg_swap_once_drop_held_apm_wifi_preferred(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False
        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="b9c47ccd-cc84-42cc-83f4-0a98c22c1d7a")
    def test_epdg_mo_mt_add_epdg_swap_once_drop_active_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="273d521f-d11c-4956-ae51-33f69de87663")
    def test_epdg_mo_mt_add_epdg_swap_once_drop_active_wfc_wifi_preferred(
            self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[2]])

    def _test_gsm_mo_mo_add_swap_x(self, num_swaps):
        """Test swap feature in GSM call.

        PhoneA (GSM) call PhoneB, accept on PhoneB.
        PhoneA (GSM) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            ads[0].log.error("not GSM phone, abort wcdma swap test.")
            return None, None

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]], [
                phone_setup_voice_2g, phone_setup_voice_general,
                phone_setup_voice_general
            ], [is_phone_in_call_2g, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_gsm_mt_mt_add_swap_x(self, num_swaps):
        """Test swap feature in GSM call.

        PhoneB call PhoneA (GSM), accept on PhoneA.
        PhoneC call PhoneA (GSM), accept on PhoneA.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            ads[0].log.error("not GSM phone, abort wcdma swap test.")
            return None, None

        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]], [
                phone_setup_voice_2g, phone_setup_voice_general,
                phone_setup_voice_general
            ], [is_phone_in_call_2g, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x%s test.", num_swaps)
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_gsm_conference_merge_drop(self, call_ab_id, call_ac_id):
        """Test conference merge and drop in GSM call.

        PhoneA in GSM call with PhoneB.
        PhoneA in GSM call with PhoneC.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 3:
            ads[0].log.error("Total number of call ids is not 3.")
            return False
        call_conf_id = None
        for call_id in calls:
            if call_id != call_ab_id and call_id != call_ac_id:
                call_conf_id = call_id
        if not call_conf_id:
            self.log.error("Merge call fail, no new conference call id.")
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        # Check if Conf Call is currently active
        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            ads[0].log.error(
                "Call_id: %s, state: %s, expected: STATE_ACTIVE", call_conf_id,
                ads[0].droid.telecomCallGetCallState(call_conf_id))
            return False

        self.log.info("Step5: End call on PhoneC and verify call continues.")
        if not self._hangup_call(ads[2], "PhoneC"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 1:
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        self.log.info("Step6: End call on PhoneB and verify PhoneA end.")
        if not self._hangup_call(ads[1], "PhoneB"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="03eb38b1-bd7f-457e-8b80-d58651d82741")
    def test_gsm_mo_mo_add_merge_drop(self):
        """ Test Conf Call among three phones.

        Call from PhoneA to PhoneB, accept on PhoneB.
        Call from PhoneA to PhoneC, accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_gsm_mo_mo_add_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_gsm_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3286306f-4d66-48c4-9303-f691c53bcfe0")
    def test_gsm_mo_mo_add_swap_once_drop_held(self):
        """ Test Conf Call among three phones.

        Call from PhoneA to PhoneB, accept on PhoneB.
        Call from PhoneA to PhoneC, accept on PhoneC.
        On PhoneA, swap active call.
        End call on PhoneB, verify call continues.
        End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        call_ab_id, call_ac_id = self._test_gsm_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="af4ff690-be2a-42d8-930a-8258fe81c77e")
    def test_gsm_mt_mt_add_merge_drop(self):
        """ Test Conf Call among three phones.

        Call from PhoneB to PhoneA, accept on PhoneA.
        Call from PhoneC to PhoneA, accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_gsm_mt_mt_add_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_gsm_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="6b70902e-ca59-4d92-9bfe-2116dcc91213")
    def test_gsm_mo_mo_add_swap_twice_drop_active(self):
        """Test swap feature in GSM call.

        PhoneA (GSM) call PhoneB, accept on PhoneB.
        PhoneA (GSM) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_gsm_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=self._get_expected_call_state(ads[0]),
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a9ddf5a7-8399-4a8c-abc9-b9235b0153b0")
    def test_epdg_mo_mo_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_no_cep(
            self):
        """ Test WFC Conference Call among three phones. No CEP.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneA (WFC APM WiFi Preferred) call PhoneC (WFC APM WiFi Preferred), accept on PhoneC.
        3. On PhoneA, merge to conference call (No CEP).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, No_CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="5aaff055-3329-4077-91e8-5707a0f6a309")
    def test_epdg_mo_mo_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneA (WFC APM WiFi Preferred) call PhoneC (WFC APM WiFi Preferred), accept on PhoneC.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d3624dd8-20bd-4f6b-8a81-0c8671987b84")
    def test_epdg_mo_mo_add_epdg_merge_drop_second_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneA (WFC APM WiFi Preferred) call PhoneC (WFC APM WiFi Preferred), accept on PhoneC.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ba43d88c-1347-4570-92d0-ebfa6404788f")
    def test_epdg_mo_mo_add_epdg_merge_drop_first_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneA (WFC APM WiFi Preferred) call PhoneC (WFC APM WiFi Preferred), accept on PhoneC.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-C continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="fbcd20ec-ebac-45c2-b228-30fdec42752f")
    def test_epdg_mo_mo_add_epdg_merge_drop_first_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneA (WFC APM WiFi Preferred) call PhoneC (WFC APM WiFi Preferred), accept on PhoneC.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-C continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="fc54b329-4ec6-45b2-8f91-0a5789542596")
    def test_epdg_mo_mt_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_no_cep(
            self):
        """ Test WFC Conference Call among three phones. No CEP.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (No CEP).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, No_CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="64096e42-1fb2-4eb4-9f60-3e22c7ad5c83")
    def test_epdg_mo_mt_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="24b6abb4-03c8-464c-a584-ca597bd67b46")
    def test_epdg_mo_mt_add_epdg_merge_drop_second_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f54776e4-84c0-43db-8724-f012ef551ebd")
    def test_epdg_mo_mt_add_epdg_merge_drop_first_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-C continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="6635aeff-f10a-4fb0-b658-4f1e7f2d9a68")
    def test_epdg_mo_mt_add_epdg_merge_drop_first_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-C continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c2534477-74ff-43ca-920a-48238928f344")
    def test_epdg_mt_mt_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_no_cep(
            self):
        """ Test WFC Conference Call among three phones. No CEP.

        Steps:
        1. PhoneB (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (No CEP).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, No_CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ef5ea03d-1c1b-4c9a-a72d-14b2ba7e87cb")
    def test_epdg_mt_mt_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps
        1. PhoneB (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="b0df507f-2adf-45fe-a174-44f62718296e")
    def test_epdg_mt_mt_add_epdg_merge_drop_second_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneB (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="278c6bec-7065-4f54-9834-33d8a6172f58")
    def test_epdg_mt_mt_add_epdg_merge_drop_first_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneB (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-C continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="1ffceadc-8bd1-489d-bb66-4b3081df3a64")
    def test_epdg_mt_mt_add_epdg_merge_drop_first_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneB (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-C continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="b3dfaa38-8e9b-45b7-8e4e-6e6ca10887bd")
    def test_epdg_mo_mt_add_epdg_swap_once_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_no_cep(
            self):
        """ Test swap and merge features in WFC call. No CEP.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (No CEP).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Swap operation succeeded.
        4. Conference merged successfully.
        5. Drop calls succeeded. Call between A-B continues.
        6. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, No_CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="eb82f1ac-e5a3-42bc-b9d9-806442263f79")
    def test_epdg_mo_mt_add_epdg_swap_once_merge_drop_second_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test swap and merge features in WFC call. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (WFC CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Expected Results:
        3. Swap operation succeeded.
        4. Conference merged successfully.
        5. Drop calls succeeded. Call between A-B continues.
        6. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="53ba057d-5c5c-4236-9ff9-829177e6f51e")
    def test_epdg_mo_mt_add_epdg_swap_once_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test swap and merge features in WFC call. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (WFC CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Swap operation succeeded.
        4. Conference merged successfully.
        5. Drop calls succeeded. Call between A-B continues.
        6. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a246cbd0-915d-4068-8d63-7e099d41fd43")
    def test_wcdma_add_mt_decline(self):
        ads = self.android_devices

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_wcdma, None], True):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="2789388a-c67c-4c37-a4ea-98c9083abcf9")
    def test_wcdma_add_mt_ignore(self):
        ads = self.android_devices

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_wcdma, None], False):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="8f5399b2-5075-45b2-b916-2d436d1c1c93")
    def test_1x_add_mt_decline(self):
        ads = self.android_devices

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_1x, None], True):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a7e6ea10-d4d4-4089-a012-31565314cf65")
    def test_1x_add_mt_ignore(self):
        ads = self.android_devices

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_1x, None], False):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="c7d878f6-2f1c-4029-bcf9-2aecf2d202e7")
    def test_volte_add_mt_decline(self):
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_volte, None], True):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="4f03240f-88a7-4d39-9d90-6327e835d5e2")
    def test_volte_add_mt_ignore(self):
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_volte, None], False):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ce51844a-4879-470e-9a22-4eafe25f8e2a")
    def test_wfc_lte_add_mt_decline(self):
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_volte, None], True):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="b5058cd0-4073-4018-9683-335fd27ab529")
    def test_wfc_lte_add_mt_ignore(self):
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_volte, None], False):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="456e0a04-7d82-4387-89bb-80613732412e")
    def test_wfc_apm_add_mt_decline(self):
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_volte, None], True):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ee71fc98-9e52-406f-8d8a-6d9c62cbe6f4")
    def test_wfc_apm_add_mt_ignore(self):
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_volte, None], False):
            return False
        return True

    """ Tests End """
