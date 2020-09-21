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
    Test Script for VT live call test
"""

import time
from queue import Empty
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import AUDIO_ROUTE_EARPIECE
from acts.test_utils.tel.tel_defines import AUDIO_ROUTE_SPEAKER
from acts.test_utils.tel.tel_defines import CALL_STATE_ACTIVE
from acts.test_utils.tel.tel_defines import CALL_STATE_HOLDING
from acts.test_utils.tel.tel_defines import CALL_CAPABILITY_MANAGE_CONFERENCE
from acts.test_utils.tel.tel_defines import CALL_CAPABILITY_MERGE_CONFERENCE
from acts.test_utils.tel.tel_defines import CALL_CAPABILITY_SWAP_CONFERENCE
from acts.test_utils.tel.tel_defines import CALL_PROPERTY_CONFERENCE
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_VIDEO_SESSION_EVENT
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_VOLTE_ENABLED
from acts.test_utils.tel.tel_defines import VT_STATE_AUDIO_ONLY
from acts.test_utils.tel.tel_defines import VT_STATE_BIDIRECTIONAL
from acts.test_utils.tel.tel_defines import VT_STATE_BIDIRECTIONAL_PAUSED
from acts.test_utils.tel.tel_defines import VT_VIDEO_QUALITY_DEFAULT
from acts.test_utils.tel.tel_defines import VT_STATE_RX_ENABLED
from acts.test_utils.tel.tel_defines import VT_STATE_TX_ENABLED
from acts.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_defines import EVENT_VIDEO_SESSION_EVENT
from acts.test_utils.tel.tel_defines import EventTelecomVideoCallSessionEvent
from acts.test_utils.tel.tel_defines import SESSION_EVENT_RX_PAUSE
from acts.test_utils.tel.tel_defines import SESSION_EVENT_RX_RESUME
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import disconnect_call_by_id
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import multithread_func
from acts.test_utils.tel.tel_test_utils import num_active_calls
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_test_utils import verify_incall_state
from acts.test_utils.tel.tel_test_utils import wait_for_video_enabled
from acts.test_utils.tel.tel_video_utils import get_call_id_in_video_state
from acts.test_utils.tel.tel_video_utils import \
    is_phone_in_call_video_bidirectional
from acts.test_utils.tel.tel_video_utils import is_phone_in_call_voice_hd
from acts.test_utils.tel.tel_video_utils import phone_setup_video
from acts.test_utils.tel.tel_video_utils import \
    verify_video_call_in_expected_state
from acts.test_utils.tel.tel_video_utils import video_call_downgrade
from acts.test_utils.tel.tel_video_utils import video_call_modify_video
from acts.test_utils.tel.tel_video_utils import video_call_setup_teardown
from acts.test_utils.tel.tel_voice_utils import get_audio_route
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.tel.tel_voice_utils import set_audio_route
from acts.test_utils.tel.tel_voice_utils import get_cep_conference_call_id

DEFAULT_LONG_DURATION_CALL_TOTAL_DURATION = 1 * 60 * 60  # default 1 hour


class TelLiveVideoTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)

        self.stress_test_number = self.get_stress_test_number()
        self.wifi_network_ssid = self.user_params.get("wifi_network_ssid")
        self.wifi_network_pass = self.user_params.get("wifi_network_pass")

        self.long_duration_call_total_duration = self.user_params.get(
            "long_duration_call_total_duration",
            DEFAULT_LONG_DURATION_CALL_TOTAL_DURATION)

    """ Tests Begin """

    @test_tracker_info(uuid="9f0b7c98-b010-4f9b-bd80-9925fe1cb5f8")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_to_video(self):
        """ Test VT<->VT call functionality.

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Call from PhoneA to PhoneB as Bi-Directional Video,
        Accept on PhoneB as video call, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        self.number_of_devices = 2
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ads[0],
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup+teardown a call")
            return False

        return True

    @test_tracker_info(uuid="8abebda7-6646-4180-a37d-2f0acca63b64")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_to_video_long(self):
        """ Test VT<->VT call functionality.

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Call from PhoneA to PhoneB as Bi-Directional Video,
        Accept on PhoneB as video call.
        Keep the VT call ON for 60 mins.
        Hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        self.number_of_devices = 2
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ads[0],
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional,
                wait_time_in_call=self.long_duration_call_total_duration):
            self.log.error("Failed to setup+teardown long call")
            return False

        return True

    @test_tracker_info(uuid="6eaef46f-dd73-4835-be9d-c9529fc0ad3d")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_accept_as_voice(self):
        """ Test VT<->VT call functionality.

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Call from PhoneA to PhoneB as Bi-Directional Video,
        Accept on PhoneB as audio only, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        self.number_of_devices = 2
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ads[0],
                video_state=VT_STATE_AUDIO_ONLY,
                verify_caller_func=is_phone_in_call_voice_hd,
                verify_callee_func=is_phone_in_call_voice_hd):
            self.log.error("Failed to setup+teardown a call")
            return False
        return True

    @test_tracker_info(uuid="dcd43fd5-4c92-4f09-90f8-04ccce66d396")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_to_video_mo_disable_camera(self):
        """ Test VT<->VT call functionality.

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Call from PhoneA to PhoneB as Bi-Directional Video,
        Accept on PhoneB as video call.
        On PhoneA disabled video transmission.
        Verify PhoneA as RX_ENABLED and PhoneB as TX_ENABLED.
        Hangup on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        self.number_of_devices = 2
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False

        self.log.info("Disable video on PhoneA:{}".format(ads[0].serial))
        if not video_call_downgrade(
                self.log, ads[0],
                get_call_id_in_video_state(self.log, ads[0],
                                           VT_STATE_BIDIRECTIONAL), ads[1],
                get_call_id_in_video_state(self.log, ads[1],
                                           VT_STATE_BIDIRECTIONAL)):
            self.log.error("Failed to disable video on PhoneA.")
            return False
        return hangup_call(self.log, ads[0])

    @test_tracker_info(uuid="088c0590-ffd0-4337-9576-569f27c4c527")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_to_video_mt_disable_camera(self):
        """ Test VT<->VT call functionality.

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Call from PhoneA to PhoneB as Bi-Directional Video,
        Accept on PhoneB as video call.
        On PhoneB disabled video transmission.
        Verify PhoneB as RX_ENABLED and PhoneA as TX_ENABLED.
        Hangup on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        self.number_of_devices = 2
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False

        self.log.info("Disable video on PhoneB:{}".format(ads[1].serial))
        if not video_call_downgrade(
                self.log, ads[1],
                get_call_id_in_video_state(self.log, ads[1],
                                           VT_STATE_BIDIRECTIONAL), ads[0],
                get_call_id_in_video_state(self.log, ads[0],
                                           VT_STATE_BIDIRECTIONAL)):
            self.log.error("Failed to disable video on PhoneB.")
            return False
        return hangup_call(self.log, ads[0])

    @test_tracker_info(uuid="879579ac-7106-4c4b-a8d0-64695108f6f7")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_to_video_mo_mt_disable_camera(self):
        """ Test VT<->VT call functionality.

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Call from PhoneA to PhoneB as Bi-Directional Video,
        Accept on PhoneB as video call.
        On PhoneA disabled video transmission.
        Verify PhoneA as RX_ENABLED and PhoneB as TX_ENABLED.
        On PhoneB disabled video transmission.
        Verify PhoneA as AUDIO_ONLY and PhoneB as AUDIO_ONLY.
        Hangup on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        self.number_of_devices = 2
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False

        self.log.info("Disable video on PhoneA:{}".format(ads[0].serial))
        if not video_call_downgrade(
                self.log, ads[0],
                get_call_id_in_video_state(self.log, ads[0],
                                           VT_STATE_BIDIRECTIONAL), ads[1],
                get_call_id_in_video_state(self.log, ads[1],
                                           VT_STATE_BIDIRECTIONAL)):
            self.log.error("Failed to disable video on PhoneA.")
            return False

        self.log.info("Disable video on PhoneB:{}".format(ads[1].serial))
        if not video_call_downgrade(
                self.log, ads[1],
                get_call_id_in_video_state(self.log, ads[1],
                                           VT_STATE_TX_ENABLED), ads[0],
                get_call_id_in_video_state(self.log, ads[0],
                                           VT_STATE_RX_ENABLED)):
            self.log.error("Failed to disable video on PhoneB.")
            return False
        return hangup_call(self.log, ads[0])

    @test_tracker_info(uuid="13ff7df6-bf13-4f60-80a1-d9cbeae8e1df")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_to_video_mt_mo_disable_camera(self):
        """ Test VT<->VT call functionality.

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Call from PhoneA to PhoneB as Bi-Directional Video,
        Accept on PhoneB as video call.
        On PhoneB disabled video transmission.
        Verify PhoneB as RX_ENABLED and PhoneA as TX_ENABLED.
        On PhoneA disabled video transmission.
        Verify PhoneA as AUDIO_ONLY and PhoneB as AUDIO_ONLY.
        Hangup on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        self.number_of_devices = 2
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False

        self.log.info("Disable video on PhoneB:{}".format(ads[1].serial))
        if not video_call_downgrade(
                self.log, ads[1],
                get_call_id_in_video_state(self.log, ads[1],
                                           VT_STATE_BIDIRECTIONAL), ads[0],
                get_call_id_in_video_state(self.log, ads[0],
                                           VT_STATE_BIDIRECTIONAL)):
            self.log.error("Failed to disable video on PhoneB.")
            return False

        self.log.info("Disable video on PhoneA:{}".format(ads[0].serial))
        if not video_call_downgrade(
                self.log, ads[0],
                get_call_id_in_video_state(self.log, ads[0],
                                           VT_STATE_TX_ENABLED), ads[1],
                get_call_id_in_video_state(self.log, ads[1],
                                           VT_STATE_RX_ENABLED)):
            self.log.error("Failed to disable video on PhoneB.")
            return False
        return hangup_call(self.log, ads[0])

    def _mo_upgrade_bidirectional(self, ads):
        """Send + accept an upgrade request from Phone A to B.

        Returns:
            True if pass; False if fail.
        """
        call_id_requester = get_call_id_in_video_state(self.log, ads[0],
                                                       VT_STATE_AUDIO_ONLY)

        call_id_responder = get_call_id_in_video_state(self.log, ads[1],
                                                       VT_STATE_AUDIO_ONLY)

        if not call_id_requester or not call_id_responder:
            self.log.error("Couldn't find a candidate call id {}:{}, {}:{}"
                           .format(ads[0].serial, call_id_requester, ads[1]
                                   .serial, call_id_responder))
            return False

        if not video_call_modify_video(self.log, ads[0], call_id_requester,
                                       ads[1], call_id_responder,
                                       VT_STATE_BIDIRECTIONAL):
            self.log.error("Failed to upgrade video call!")
            return False

        #Wait for a completed upgrade and ensure the call is stable
        time.sleep(WAIT_TIME_IN_CALL)

        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            self.log.error("_mo_upgrade_bidirectional: Call Drop!")
            return False

        if (get_call_id_in_video_state(self.log, ads[0],
                                       VT_STATE_BIDIRECTIONAL) !=
                call_id_requester):
            self.log.error("Caller not in correct state: {}".format(
                VT_STATE_BIDIRECTIONAL))
            return False

        if (get_call_id_in_video_state(self.log, ads[1],
                                       VT_STATE_BIDIRECTIONAL) !=
                call_id_responder):
            self.log.error("Callee not in correct state: {}".format(
                VT_STATE_BIDIRECTIONAL))
            return False

        return hangup_call(self.log, ads[0])

    @test_tracker_info(uuid="e56eea96-467c-49ce-a135-f82f12302369")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_accept_as_voice_mo_upgrade_bidirectional(self):
        """ Test Upgrading from VoLTE to Bi-Directional VT.

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Call from PhoneA to PhoneB as Video, accept on PhoneB as audio only.
        Send + accept an upgrade request from Phone A to B.

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices
        self.number_of_devices = 2
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_AUDIO_ONLY,
                verify_caller_func=is_phone_in_call_volte,
                verify_callee_func=is_phone_in_call_volte):
            self.log.error("Failed to setup a call")
            return False

        return self._mo_upgrade_bidirectional(ads)

    @test_tracker_info(uuid="c1f58f4a-28aa-4cd0-9835-f294cdcff854")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_mo_upgrade_bidirectional(self):
        """ Test Upgrading from VoLTE to Bi-Directional VT.

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Call from PhoneA to PhoneB as VoLTE, accept on PhoneB.
        Send + accept an upgrade request from Phone A to B.

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices
        self.number_of_devices = 2
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_setup_teardown(self.log, ads[0], ads[1], None,
                                   is_phone_in_call_volte,
                                   is_phone_in_call_volte):
            self.log.error("Failed to setup a call")
            return False

        return self._mo_upgrade_bidirectional(ads)

    def _mo_upgrade_reject(self, ads):
        """Send + reject an upgrade request from Phone A to B.

        Returns:
            True if pass; False if fail.
        """
        call_id_requester = get_call_id_in_video_state(self.log, ads[0],
                                                       VT_STATE_AUDIO_ONLY)

        call_id_responder = get_call_id_in_video_state(self.log, ads[1],
                                                       VT_STATE_AUDIO_ONLY)

        if not call_id_requester or not call_id_responder:
            self.log.error("Couldn't find a candidate call id {}:{}, {}:{}"
                           .format(ads[0].serial, call_id_requester, ads[1]
                                   .serial, call_id_responder))
            return False

        if not video_call_modify_video(
                self.log, ads[0], call_id_requester, ads[1], call_id_responder,
                VT_STATE_BIDIRECTIONAL, VT_VIDEO_QUALITY_DEFAULT,
                VT_STATE_AUDIO_ONLY, VT_VIDEO_QUALITY_DEFAULT):
            self.log.error("Failed to upgrade video call!")
            return False

        time.sleep(WAIT_TIME_IN_CALL)

        if not is_phone_in_call_voice_hd(self.log, ads[0]):
            self.log.error("PhoneA not in correct state.")
            return False
        if not is_phone_in_call_voice_hd(self.log, ads[1]):
            self.log.error("PhoneB not in correct state.")
            return False

        return hangup_call(self.log, ads[0])

    @test_tracker_info(uuid="427b0906-f082-4f6d-9d94-4f9c4d5005a5")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_mo_upgrade_reject(self):
        """ Test Upgrading from VoLTE to Bi-Directional VT and reject.

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Call from PhoneA to PhoneB as VoLTE, accept on PhoneB.
        Send an upgrade request from Phone A to PhoneB.
        Reject on PhoneB. Verify PhoneA and PhoneB ad AUDIO_ONLY.
        Verify call continues.
        Hangup on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_setup_teardown(self.log, ads[0], ads[1], None,
                                   is_phone_in_call_volte,
                                   is_phone_in_call_volte):
            self.log.error("Failed to setup a call")
            return False

        return self._mo_upgrade_reject(ads)

    @test_tracker_info(uuid="f733f694-c0c2-4da0-b3c2-ff21df026426")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_accept_as_voice_mo_upgrade_reject(self):
        """ Test Upgrading from VoLTE to Bi-Directional VT and reject.

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Call from PhoneA to PhoneB as Video, accept on PhoneB as audio only.
        Send an upgrade request from Phone A to PhoneB.
        Reject on PhoneB. Verify PhoneA and PhoneB ad AUDIO_ONLY.
        Verify call continues.
        Hangup on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_AUDIO_ONLY,
                verify_caller_func=is_phone_in_call_volte,
                verify_callee_func=is_phone_in_call_volte):
            self.log.error("Failed to setup a call")
            return False

        return self._mo_upgrade_reject(ads)

    def _test_put_call_to_backgroundpause_and_foregroundresume(
            self, ad_requester, ad_responder):
        call_id_requester = get_call_id_in_video_state(self.log, ad_requester,
                                                       VT_STATE_BIDIRECTIONAL)
        call_id_responder = get_call_id_in_video_state(self.log, ad_responder,
                                                       VT_STATE_BIDIRECTIONAL)
        ad_requester.droid.telecomCallVideoStartListeningForEvent(
            call_id_requester, EVENT_VIDEO_SESSION_EVENT)
        ad_responder.droid.telecomCallVideoStartListeningForEvent(
            call_id_responder, EVENT_VIDEO_SESSION_EVENT)
        self.log.info("Put In-Call UI on {} to background.".format(
            ad_requester.serial))
        ad_requester.droid.showHomeScreen()
        try:
            event_on_responder = ad_responder.ed.pop_event(
                EventTelecomVideoCallSessionEvent,
                MAX_WAIT_TIME_VIDEO_SESSION_EVENT)
            event_on_requester = ad_requester.ed.pop_event(
                EventTelecomVideoCallSessionEvent,
                MAX_WAIT_TIME_VIDEO_SESSION_EVENT)
            if event_on_responder['data']['Event'] != SESSION_EVENT_RX_PAUSE:
                self.log.error(
                    "Event not correct. event_on_responder: {}. Expected :{}".
                    format(event_on_responder, SESSION_EVENT_RX_PAUSE))
                return False
            if event_on_requester['data']['Event'] != SESSION_EVENT_RX_PAUSE:
                self.log.error(
                    "Event not correct. event_on_requester: {}. Expected :{}".
                    format(event_on_requester, SESSION_EVENT_RX_PAUSE))
                return False
        except Empty:
            self.log.error("Expected event not received.")
            return False
        finally:
            ad_requester.droid.telecomCallVideoStopListeningForEvent(
                call_id_requester, EVENT_VIDEO_SESSION_EVENT)
            ad_responder.droid.telecomCallVideoStopListeningForEvent(
                call_id_responder, EVENT_VIDEO_SESSION_EVENT)
        time.sleep(WAIT_TIME_IN_CALL)

        if not verify_video_call_in_expected_state(
                self.log, ad_requester, call_id_requester,
                VT_STATE_BIDIRECTIONAL_PAUSED, CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ad_responder, call_id_responder,
                VT_STATE_BIDIRECTIONAL_PAUSED, CALL_STATE_ACTIVE):
            return False

        self.log.info("Put In-Call UI on {} to foreground.".format(
            ad_requester.serial))
        ad_requester.droid.telecomCallVideoStartListeningForEvent(
            call_id_requester, EVENT_VIDEO_SESSION_EVENT)
        ad_responder.droid.telecomCallVideoStartListeningForEvent(
            call_id_responder, EVENT_VIDEO_SESSION_EVENT)
        ad_requester.droid.telecomShowInCallScreen()
        try:
            event_on_responder = ad_responder.ed.pop_event(
                EventTelecomVideoCallSessionEvent,
                MAX_WAIT_TIME_VIDEO_SESSION_EVENT)
            event_on_requester = ad_requester.ed.pop_event(
                EventTelecomVideoCallSessionEvent,
                MAX_WAIT_TIME_VIDEO_SESSION_EVENT)
            if event_on_responder['data']['Event'] != SESSION_EVENT_RX_RESUME:
                self.log.error(
                    "Event not correct. event_on_responder: {}. Expected :{}".
                    format(event_on_responder, SESSION_EVENT_RX_RESUME))
                return False
            if event_on_requester['data']['Event'] != SESSION_EVENT_RX_RESUME:
                self.log.error(
                    "Event not correct. event_on_requester: {}. Expected :{}".
                    format(event_on_requester, SESSION_EVENT_RX_RESUME))
                return False
        except Empty:
            self.log.error("Expected event not received.")
            return False
        finally:
            ad_requester.droid.telecomCallVideoStopListeningForEvent(
                call_id_requester, EVENT_VIDEO_SESSION_EVENT)
            ad_responder.droid.telecomCallVideoStopListeningForEvent(
                call_id_responder, EVENT_VIDEO_SESSION_EVENT)
        time.sleep(WAIT_TIME_IN_CALL)
        self.log.info("Verify both calls are in bi-directional/active state.")
        if not verify_video_call_in_expected_state(
                self.log, ad_requester, call_id_requester,
                VT_STATE_BIDIRECTIONAL, CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ad_responder, call_id_responder,
                VT_STATE_BIDIRECTIONAL, CALL_STATE_ACTIVE):
            return False

        return True

    @test_tracker_info(uuid="f78b40a4-3be7-46f2-882f-0333f733e334")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_to_video_mo_to_backgroundpause_foregroundresume(self):
        ads = self.android_devices
        self.number_of_devices = 2
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False

        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        return self._test_put_call_to_backgroundpause_and_foregroundresume(
            ads[0], ads[1])

    @test_tracker_info(uuid="9aafdf6a-6535-4137-a801-4fbb67fdb281")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_to_video_mt_to_backgroundpause_foregroundresume(self):
        ads = self.android_devices
        self.number_of_devices = 2
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False

        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        return self._test_put_call_to_backgroundpause_and_foregroundresume(
            ads[1], ads[0])

    def _vt_test_multi_call_hangup(self, ads):
        """private function to hangup calls for VT tests.

        Hangup on PhoneB.
        Verify PhoneA and PhoneC still in call.
        Hangup on PhoneC.
        Verify all phones not in call.
        """
        self.number_of_devices = 3
        if not hangup_call(self.log, ads[1]):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[2]], True):
            return False
        if not hangup_call(self.log, ads[2]):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    @test_tracker_info(uuid="cde91e7d-dbc5-40f5-937d-36840c77667e")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_add_mo_voice(self):
        """
        From Phone_A, Initiate a Bi-Directional Video Call to Phone_B
        Accept the call on Phone_B as Bi-Directional Video
        From Phone_A, add a voice call to Phone_C
        Accept the call on Phone_C
        Verify both calls remain active.
        """
        self.number_of_devices = 3
        # This test case is not supported by VZW.
        ads = self.android_devices
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_video, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate Video Call PhoneA->PhoneB.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False
        call_id_video = get_call_id_in_video_state(self.log, ads[0],
                                                   VT_STATE_BIDIRECTIONAL)
        if call_id_video is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info("Step2: Initiate Voice Call PhoneA->PhoneC.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[2],
                None,
                verify_caller_func=None,
                verify_callee_func=is_phone_in_call_volte):
            self.log.error("Failed to setup a call")
            return False

        self.log.info(
            "Step3: Verify PhoneA's video/voice call in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False
        for call in calls:
            if call != call_id_video:
                call_id_voice = call

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_HOLDING):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice, VT_STATE_AUDIO_ONLY,
                CALL_STATE_ACTIVE):
            return False

        return self._vt_test_multi_call_hangup(ads)

    @test_tracker_info(uuid="60511b22-7004-4539-9164-1331220e4d18")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_add_mt_voice(self):
        """
        From Phone_A, Initiate a Bi-Directional Video Call to Phone_B
        Accept the call on Phone_B as Bi-Directional Video
        From Phone_C, add a voice call to Phone_A
        Accept the call on Phone_A
        Verify both calls remain active.
        """
        self.number_of_devices = 3
        ads = self.android_devices
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_video, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate Video Call PhoneA->PhoneB.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False
        call_id_video = get_call_id_in_video_state(self.log, ads[0],
                                                   VT_STATE_BIDIRECTIONAL)
        if call_id_video is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info("Step2: Initiate Voice Call PhoneC->PhoneA.")
        if not call_setup_teardown(
                self.log,
                ads[2],
                ads[0],
                None,
                verify_caller_func=is_phone_in_call_volte,
                verify_callee_func=None):
            self.log.error("Failed to setup a call")
            return False

        self.log.info(
            "Step3: Verify PhoneA's video/voice call in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False
        for call in calls:
            if call != call_id_video:
                call_id_voice = call

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video, VT_STATE_BIDIRECTIONAL_PAUSED,
                CALL_STATE_HOLDING):
            return False

        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice, VT_STATE_AUDIO_ONLY,
                CALL_STATE_ACTIVE):
            return False

        return self._vt_test_multi_call_hangup(ads)

    @test_tracker_info(uuid="782847f4-8eab-42db-a036-ebf8de28eb23")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_add_mo_video(self):
        """
        From Phone_A, Initiate a VoLTE Call to Phone_B
        Accept the call on Phone_B
        From Phone_A, add a Video call to Phone_C
        Accept the call on Phone_C as Video
        Verify both calls remain active.
        """
        # This test case is not supported by VZW.
        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_video,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate VoLTE Call PhoneA->PhoneB.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                verify_caller_func=is_phone_in_call_volte,
                verify_callee_func=is_phone_in_call_volte):
            self.log.error("Failed to setup a call")
            return False
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 1:
            self.log.error("Active call numbers in PhoneA is not 1.")
            return False
        call_id_voice = calls[0]

        self.log.info("Step2: Initiate Video Call PhoneA->PhoneC.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[2],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False
        call_id_video = get_call_id_in_video_state(self.log, ads[0],
                                                   VT_STATE_BIDIRECTIONAL)
        if call_id_video is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info(
            "Step3: Verify PhoneA's video/voice call in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False

        return self._vt_test_multi_call_hangup(ads)

    @test_tracker_info(uuid="bc3ac5b0-4bf7-4068-9bd0-2f8301c2ad05")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_add_mt_video(self):
        """
        From Phone_A, Initiate a VoLTE Call to Phone_B
        Accept the call on Phone_B
        From Phone_C, add a Video call to Phone_A
        Accept the call on Phone_A as Video
        Verify both calls remain active.
        """
        # TODO (b/21437650):
        # Test will fail. After established 2nd call ~15s, Phone C will drop call.
        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_video,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate VoLTE Call PhoneA->PhoneB.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                verify_caller_func=is_phone_in_call_volte,
                verify_callee_func=is_phone_in_call_volte):
            self.log.error("Failed to setup a call")
            return False
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 1:
            self.log.error("Active call numbers in PhoneA is not 1.")
            return False
        call_id_voice = calls[0]

        self.log.info("Step2: Initiate Video Call PhoneC->PhoneA.")
        if not video_call_setup_teardown(
                self.log,
                ads[2],
                ads[0],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False

        call_id_video = get_call_id_in_video_state(self.log, ads[0],
                                                   VT_STATE_BIDIRECTIONAL)
        if call_id_video is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info(
            "Step3: Verify PhoneA's video/voice call in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False

        return self._vt_test_multi_call_hangup(ads)

    @test_tracker_info(uuid="97c7f5c3-c994-477b-839e-cea1d450d4e7")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_add_mt_voice_swap_once_local_drop(self):
        """
        From Phone_A, Initiate a Bi-Directional Video Call to Phone_B
        Accept the call on Phone_B as Bi-Directional Video
        From Phone_C, add a voice call to Phone_A
        Accept the call on Phone_A
        Verify both calls remain active.
        Swap calls on PhoneA.
        End Video call on PhoneA.
        End Voice call on PhoneA.
        """
        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_video, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate Video Call PhoneA->PhoneB.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False
        call_id_video = get_call_id_in_video_state(self.log, ads[0],
                                                   VT_STATE_BIDIRECTIONAL)
        if call_id_video is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info("Step2: Initiate Voice Call PhoneC->PhoneA.")
        if not call_setup_teardown(
                self.log,
                ads[2],
                ads[0],
                None,
                verify_caller_func=is_phone_in_call_volte,
                verify_callee_func=None):
            self.log.error("Failed to setup a call")
            return False

        self.log.info(
            "Step3: Verify PhoneA's video/voice call in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False
        for call in calls:
            if call != call_id_video:
                call_id_voice = call

        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video, VT_STATE_BIDIRECTIONAL_PAUSED,
                CALL_STATE_HOLDING):
            return False

        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice, VT_STATE_AUDIO_ONLY,
                CALL_STATE_ACTIVE):
            return False
        self.log.info("Step4: Verify all phones remain in-call.")
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info(
            "Step5: Swap calls on PhoneA and verify call state correct.")
        ads[0].droid.telecomCallHold(call_id_voice)
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        for ad in [ads[0], ads[1]]:
            if get_audio_route(self.log, ad) != AUDIO_ROUTE_SPEAKER:
                self.log.error("{} Audio is not on speaker.".format(ad.serial))
                # TODO: b/26337892 Define expected audio route behavior.

            set_audio_route(self.log, ad, AUDIO_ROUTE_EARPIECE)

        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False

        self.log.info("Step6: Drop Video Call on PhoneA.")
        disconnect_call_by_id(self.log, ads[0], call_id_video)
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[2]], True):
            return False
        disconnect_call_by_id(self.log, ads[0], call_id_voice)
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    @test_tracker_info(uuid="6b2c8701-eb65-47cd-a190-a074bc60ebfa")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_add_mt_voice_swap_twice_remote_drop_voice_unhold_video(
            self):
        """
        From Phone_A, Initiate a Bi-Directional Video Call to Phone_B
        Accept the call on Phone_B as Bi-Directional Video
        From Phone_C, add a voice call to Phone_A
        Accept the call on Phone_A
        Verify both calls remain active.
        Swap calls on PhoneA.
        Swap calls on PhoneA.
        End Voice call on PhoneC.
        Unhold Video call on PhoneA.
        End Video call on PhoneA.
        """

        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_video, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate Video Call PhoneA->PhoneB.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False
        call_id_video = get_call_id_in_video_state(self.log, ads[0],
                                                   VT_STATE_BIDIRECTIONAL)
        if call_id_video is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info("Step2: Initiate Voice Call PhoneC->PhoneA.")
        if not call_setup_teardown(
                self.log,
                ads[2],
                ads[0],
                None,
                verify_caller_func=is_phone_in_call_volte,
                verify_callee_func=None):
            self.log.error("Failed to setup a call")
            return False

        self.log.info(
            "Step3: Verify PhoneA's video/voice call in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False
        for call in calls:
            if call != call_id_video:
                call_id_voice = call

        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video, VT_STATE_BIDIRECTIONAL_PAUSED,
                CALL_STATE_HOLDING):
            return False

        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice, VT_STATE_AUDIO_ONLY,
                CALL_STATE_ACTIVE):
            return False
        self.log.info("Step4: Verify all phones remain in-call.")
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info(
            "Step5: Swap calls on PhoneA and verify call state correct.")
        ads[0].droid.telecomCallHold(call_id_voice)
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        for ad in [ads[0], ads[1]]:
            if get_audio_route(self.log, ad) != AUDIO_ROUTE_SPEAKER:
                self.log.error("{} Audio is not on speaker.".format(ad.serial))
                # TODO: b/26337892 Define expected audio route behavior.
            set_audio_route(self.log, ad, AUDIO_ROUTE_EARPIECE)

        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False

        self.log.info(
            "Step6: Swap calls on PhoneA and verify call state correct.")
        ads[0].droid.telecomCallHold(call_id_video)
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        # Audio will goto earpiece in here
        for ad in [ads[0], ads[1]]:
            if get_audio_route(self.log, ad) != AUDIO_ROUTE_EARPIECE:
                self.log.error("{} Audio is not on EARPIECE.".format(
                    ad.serial))
                # TODO: b/26337892 Define expected audio route behavior.

        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_HOLDING):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice, VT_STATE_AUDIO_ONLY,
                CALL_STATE_ACTIVE):
            return False

        self.log.info("Step7: Drop Voice Call on PhoneC.")
        hangup_call(self.log, ads[2])
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False

        self.log.info(
            "Step8: Unhold Video call on PhoneA and verify call state.")
        ads[0].droid.telecomCallUnhold(call_id_video)
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        # Audio will goto earpiece in here
        for ad in [ads[0], ads[1]]:
            if get_audio_route(self.log, ad) != AUDIO_ROUTE_EARPIECE:
                self.log.error("{} Audio is not on EARPIECE.".format(
                    ad.serial))
                # TODO: b/26337892 Define expected audio route behavior.

        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_ACTIVE):
            return False

        self.log.info("Step9: Drop Video Call on PhoneA.")
        disconnect_call_by_id(self.log, ads[0], call_id_video)
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    @test_tracker_info(uuid="9d897505-efed-4b04-b5c8-3f9ba9d26861")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_add_mo_video(self):
        """
        From Phone_A, Initiate a Bi-Directional Video Call to Phone_B
        Accept the call on Phone_B as Bi-Directional Video
        From Phone_A, add a Bi-Directional Video Call to Phone_C
        Accept the call on Phone_C
        Verify both calls remain active.
        """
        # This test case is not supported by VZW.
        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_video, (self.log, ads[1])), (phone_setup_video,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate Video Call PhoneA->PhoneB.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False
        call_id_video_ab = get_call_id_in_video_state(self.log, ads[0],
                                                      VT_STATE_BIDIRECTIONAL)
        if call_id_video_ab is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info("Step2: Initiate Video Call PhoneA->PhoneC.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[2],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False

        self.log.info("Step3: Verify PhoneA's video calls in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False
        for call in calls:
            if call != call_id_video_ab:
                call_id_video_ac = call

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ab, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_HOLDING):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ac, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_ACTIVE):
            return False

        return self._vt_test_multi_call_hangup(ads)

    @test_tracker_info(uuid="d501a744-fda7-4a0c-a25d-a1ed4e7a356e")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_add_mt_video(self):
        """
        From Phone_A, Initiate a Bi-Directional Video Call to Phone_B
        Accept the call on Phone_B as Bi-Directional Video
        From Phone_C, add a Bi-Directional Video Call to Phone_A
        Accept the call on Phone_A
        Verify both calls remain active.
        Hang up on PhoneC.
        Hang up on PhoneA.
        """
        # TODO: b/21437650 Test will fail. After established 2nd call ~15s,
        # Phone C will drop call.
        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_video, (self.log, ads[1])), (phone_setup_video,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate Video Call PhoneA->PhoneB.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False
        call_id_video_ab = get_call_id_in_video_state(self.log, ads[0],
                                                      VT_STATE_BIDIRECTIONAL)
        if call_id_video_ab is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info("Step2: Initiate Video Call PhoneC->PhoneA.")
        if not video_call_setup_teardown(
                self.log,
                ads[2],
                ads[0],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False

        self.log.info("Step3: Verify PhoneA's video calls in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False
        for call in calls:
            if call != call_id_video_ab:
                call_id_video_ac = call

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ab,
                VT_STATE_BIDIRECTIONAL_PAUSED, CALL_STATE_HOLDING):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ac, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_ACTIVE):
            return False

        self.log.info("Step4: Hangup on PhoneC.")
        if not hangup_call(self.log, ads[2]):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        self.log.info("Step4: Hangup on PhoneA.")
        if not hangup_call(self.log, ads[0]):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    @test_tracker_info(uuid="26c9c6f6-b68e-492a-b188-ce8109a4ba34")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_video_add_mt_video(self):
        """
        From Phone_B, Initiate a Bi-Directional Video Call to Phone_A
        Accept the call on Phone_A as Bi-Directional Video
        From Phone_C, add a Bi-Directional Video Call to Phone_A
        Accept the call on Phone_A
        Verify both calls remain active.
        Hang up on PhoneC.
        Hang up on PhoneA.
        """
        # TODO: b/21437650 Test will fail. After established 2nd call ~15s,
        # Phone C will drop call.
        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_video, (self.log, ads[1])), (phone_setup_video,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate Video Call PhoneB->PhoneA.")
        if not video_call_setup_teardown(
                self.log,
                ads[1],
                ads[0],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False
        call_id_video_ab = get_call_id_in_video_state(self.log, ads[0],
                                                      VT_STATE_BIDIRECTIONAL)
        if call_id_video_ab is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info("Step2: Initiate Video Call PhoneC->PhoneA.")
        if not video_call_setup_teardown(
                self.log,
                ads[2],
                ads[0],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False

        self.log.info("Step3: Verify PhoneA's video calls in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False
        for call in calls:
            if call != call_id_video_ab:
                call_id_video_ac = call

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ab,
                VT_STATE_BIDIRECTIONAL_PAUSED, CALL_STATE_HOLDING):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ac, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_ACTIVE):
            return False

        self.log.info("Step4: Hangup on PhoneC.")
        if not hangup_call(self.log, ads[2]):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        self.log.info("Step4: Hangup on PhoneA.")
        if not hangup_call(self.log, ads[0]):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    @test_tracker_info(uuid="5ceb6eb2-c128-405e-8ba4-69a4646842a0")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_video_add_mo_video(self):
        """
        From Phone_B, Initiate a Bi-Directional Video Call to Phone_A
        Accept the call on Phone_A as Bi-Directional Video
        From Phone_A, add a Bi-Directional Video Call to Phone_C
        Accept the call on Phone_C
        Verify both calls remain active.
        """
        # This test case is not supported by VZW.
        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_video, (self.log, ads[1])), (phone_setup_video,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate Video Call PhoneB->PhoneA.")
        if not video_call_setup_teardown(
                self.log,
                ads[1],
                ads[0],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False
        call_id_video_ab = get_call_id_in_video_state(self.log, ads[0],
                                                      VT_STATE_BIDIRECTIONAL)
        if call_id_video_ab is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info("Step2: Initiate Video Call PhoneA->PhoneC.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[2],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False

        self.log.info("Step3: Verify PhoneA's video calls in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False
        for call in calls:
            if call != call_id_video_ab:
                call_id_video_ac = call

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ab, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_HOLDING):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ac, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_ACTIVE):
            return False

        return self._vt_test_multi_call_hangup(ads)

    def _test_vt_conference_merge_drop(self, ads, call_ab_id, call_ac_id):
        """Test conference merge and drop for VT call test.

        PhoneA in call with PhoneB.
        PhoneA in call with PhoneC.
        Merge calls to conference on PhoneA.
        Hangup on PhoneB, check call continues between AC.
        Hangup on PhoneC.
        Hangup on PhoneA.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        self.number_of_devices = 3
        self.log.info(
            "Merge - Step1: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 1:
            self.log.error("Total number of call ids in {} is not 1.".format(
                ads[0].serial))
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
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_conf_id,
                    ads[0].droid.telecomCallGetCallState(call_conf_id)))
            return False

        self.log.info(
            "Merge - Step2: End call on PhoneB and verify call continues.")
        if not hangup_call(self.log, ads[1]):
            self.log.error("Failed to end the call on PhoneB")
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if not verify_incall_state(self.log, [ads[0], ads[2]], True):
            return False
        if not verify_incall_state(self.log, [ads[1]], False):
            return False

        if not (hangup_call(self.log, ads[2])
                and hangup_call(self.log, ads[0])):
            self.log.error("Failed to clean up remaining calls")
            return False
        return True

    def _test_vt_conference_merge_drop_cep(self, ads, call_ab_id, call_ac_id):
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

        self.number_of_devices = 2
        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))

        call_conf_id = get_cep_conference_call_id(ads[0])
        if call_conf_id is None:
            self.log.error(
                "No call with children. Probably CEP not enabled or merge failed."
            )
            return False
        calls.remove(call_conf_id)
        if (set(ads[0].droid.telecomCallGetCallChildren(call_conf_id)) !=
                set(calls)):
            self.log.error(
                "Children list<{}> for conference call is not correct.".format(
                    ads[0].droid.telecomCallGetCallChildren(call_conf_id)))
            return False

        if (CALL_PROPERTY_CONFERENCE not in ads[0]
                .droid.telecomCallGetProperties(call_conf_id)):
            self.log.error("Conf call id properties wrong: {}".format(
                ads[0].droid.telecomCallGetProperties(call_conf_id)))
            return False

        if (CALL_CAPABILITY_MANAGE_CONFERENCE not in ads[0]
                .droid.telecomCallGetCapabilities(call_conf_id)):
            self.log.error("Conf call id capabilities wrong: {}".format(
                ads[0].droid.telecomCallGetCapabilities(call_conf_id)))
            return False

        if (call_ab_id in calls) or (call_ac_id in calls):
            self.log.error(
                "Previous call ids should not in new call list after merge.")
            return False

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        # Check if Conf Call is currently active
        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_conf_id,
                    ads[0].droid.telecomCallGetCallState(call_conf_id)))
            return False

        if not hangup_call(self.log, ads[1]):
            self.log.error("Failed to end call on PhoneB")
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if not verify_incall_state(self.log, [ads[0], ads[2]], True):
            return False
        if not verify_incall_state(self.log, [ads[1]], False):
            return False

        if not (hangup_call(self.log, ads[2])
                and hangup_call(self.log, ads[0])):
            self.log.error("Failed to clean up remaining calls")
            return False

        return True

    @test_tracker_info(uuid="51731afc-e278-4f72-a5e1-590d49ba348d")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_add_mo_video_accept_as_voice_merge_drop(self):
        """Conference call

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with VoLTE).
        Make Sure PhoneC is in LTE mode (with Video Calling).
        PhoneA VoLTE call to PhoneB. Accept on PhoneB.
        PhoneA add a Bi-Directional Video call to PhoneC.
        PhoneC accept as voice.
        Merge call on PhoneA.
        Hang up on PhoneB.
        Hang up on PhoneC.
        """
        return self._test_call_volte_add_mo_video_accept_as_voice_merge_drop(
            False)

    @test_tracker_info(uuid="23e3a071-5453-48da-8439-bd75bc79547f")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_add_mo_video_accept_as_voice_merge_drop_cep(self):
        """Conference call

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with VoLTE).
        Make Sure PhoneC is in LTE mode (with Video Calling).
        PhoneA VoLTE call to PhoneB. Accept on PhoneB.
        PhoneA add a Bi-Directional Video call to PhoneC.
        PhoneC accept as voice.
        Merge call on PhoneA.
        Hang up on PhoneB.
        Hang up on PhoneC.
        """
        return self._test_call_volte_add_mo_video_accept_as_voice_merge_drop(
            True)

    def _test_call_volte_add_mo_video_accept_as_voice_merge_drop(
            self, use_cep=False):
        # This test case is not supported by VZW.
        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_video,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        self.log.info("Step1: Initiate VoLTE Call PhoneA->PhoneB.")
        if not call_setup_teardown(self.log, ads[0], ads[1], None,
                                   is_phone_in_call_volte,
                                   is_phone_in_call_volte):
            self.log.error("Failed to setup a call")
            return False
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 1:
            self.log.error("Active call numbers in PhoneA is not 1.")
            return False
        call_ab_id = calls[0]

        self.log.info(
            "Step2: Initiate Video Call PhoneA->PhoneC and accept as voice.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[2],
                None,
                video_state=VT_STATE_AUDIO_ONLY,
                verify_caller_func=is_phone_in_call_voice_hd,
                verify_callee_func=is_phone_in_call_voice_hd):
            self.log.error("Failed to setup a call")
            return False
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False
        for call in calls:
            if call != call_ab_id:
                call_ac_id = call

        self.log.info("Step3: Verify calls in correct state.")
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_ab_id, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_ac_id, VT_STATE_AUDIO_ONLY,
                CALL_STATE_ACTIVE):
            return False

        return {
            False: self._test_vt_conference_merge_drop,
            True: self._test_vt_conference_merge_drop_cep
        }[use_cep](ads, call_ab_id, call_ac_id)

    @test_tracker_info(uuid="5ac216a6-4ce3-4bd6-a132-8b1294e461a7")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_add_mt_video_accept_as_voice_merge_drop(self):
        """Conference call

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with VoLTE).
        Make Sure PhoneC is in LTE mode (with Video Calling).
        PhoneA VoLTE call to PhoneB. Accept on PhoneB.
        PhoneC add a Bi-Directional Video call to PhoneA.
        PhoneA accept as voice.
        Merge call on PhoneA.
        Hang up on PhoneB.
        Hang up on PhoneC.
        """
        return self._test_call_volte_add_mt_video_accept_as_voice_merge_drop(
            False)

    @test_tracker_info(uuid="be1d2337-ed0d-4293-afe8-0fa677b6bee1")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_add_mt_video_accept_as_voice_merge_drop_cep(self):
        """Conference call

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with VoLTE).
        Make Sure PhoneC is in LTE mode (with Video Calling).
        PhoneA VoLTE call to PhoneB. Accept on PhoneB.
        PhoneC add a Bi-Directional Video call to PhoneA.
        PhoneA accept as voice.
        Merge call on PhoneA.
        Hang up on PhoneB.
        Hang up on PhoneC.
        """
        return self._test_call_volte_add_mt_video_accept_as_voice_merge_drop(
            True)

    def _test_call_volte_add_mt_video_accept_as_voice_merge_drop(
            self, use_cep=False):
        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_video,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        self.log.info("Step1: Initiate VoLTE Call PhoneA->PhoneB.")
        if not call_setup_teardown(self.log, ads[0], ads[1], None,
                                   is_phone_in_call_volte,
                                   is_phone_in_call_volte):
            self.log.error("Failed to setup a call")
            return False
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 1:
            self.log.error("Active call numbers in PhoneA is not 1.")
            return False
        call_ab_id = calls[0]

        self.log.info(
            "Step2: Initiate Video Call PhoneC->PhoneA and accept as voice.")
        if not video_call_setup_teardown(
                self.log,
                ads[2],
                ads[0],
                None,
                video_state=VT_STATE_AUDIO_ONLY,
                verify_caller_func=is_phone_in_call_voice_hd,
                verify_callee_func=is_phone_in_call_voice_hd):
            self.log.error("Failed to setup a call")
            return False
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False
        for call in calls:
            if call != call_ab_id:
                call_ac_id = call

        self.log.info("Step3: Verify calls in correct state.")
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_ab_id, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_ac_id, VT_STATE_AUDIO_ONLY,
                CALL_STATE_ACTIVE):
            return False

        return {
            False: self._test_vt_conference_merge_drop,
            True: self._test_vt_conference_merge_drop_cep
        }[use_cep](ads, call_ab_id, call_ac_id)

    @test_tracker_info(uuid="ead8b199-2703-4e6c-b55b-57c5280b52e8")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_add_mo_voice_swap_downgrade_merge_drop(self):
        """Conference call

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Make Sure PhoneC is in LTE mode (with VoLTE).
        PhoneA add a Bi-Directional Video call to PhoneB.
        PhoneB accept as Video.
        PhoneA VoLTE call to PhoneC. Accept on PhoneC.
        Swap Active call on PhoneA.
        Downgrade Video call on PhoneA and PhoneB to audio only.
        Merge call on PhoneA.
        Hang up on PhoneB.
        Hang up on PhoneC.
        """
        return self._test_call_video_add_mo_voice_swap_downgrade_merge_drop(
            False)

    @test_tracker_info(uuid="fb52dc54-0b11-46d4-a619-412eda2df390")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_add_mo_voice_swap_downgrade_merge_drop_cep(self):
        """Conference call

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Make Sure PhoneC is in LTE mode (with VoLTE).
        PhoneA add a Bi-Directional Video call to PhoneB.
        PhoneB accept as Video.
        PhoneA VoLTE call to PhoneC. Accept on PhoneC.
        Swap Active call on PhoneA.
        Downgrade Video call on PhoneA and PhoneB to audio only.
        Merge call on PhoneA.
        Hang up on PhoneB.
        Hang up on PhoneC.
        """
        return self._test_call_video_add_mo_voice_swap_downgrade_merge_drop(
            True)

    def _test_call_video_add_mo_voice_swap_downgrade_merge_drop(self, use_cep):
        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_video, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate Video Call PhoneA->PhoneB.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False
        call_id_video_ab = get_call_id_in_video_state(self.log, ads[0],
                                                      VT_STATE_BIDIRECTIONAL)
        if call_id_video_ab is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info("Step2: Initiate Voice Call PhoneA->PhoneC.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[2],
                None,
                verify_caller_func=None,
                verify_callee_func=is_phone_in_call_volte):
            self.log.error("Failed to setup a call")
            return False

        self.log.info(
            "Step3: Verify PhoneA's video/voice call in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False
        for call in calls:
            if call != call_id_video_ab:
                call_id_voice_ac = call

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ab, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_HOLDING):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice_ac, VT_STATE_AUDIO_ONLY,
                CALL_STATE_ACTIVE):
            return False

        self.log.info(
            "Step4: Swap calls on PhoneA and verify call state correct.")
        ads[0].droid.telecomCallHold(call_id_voice_ac)
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        for ad in [ads[0], ads[1]]:
            self.log.info("{} audio: {}".format(ad.serial,
                                                get_audio_route(self.log, ad)))
            set_audio_route(self.log, ad, AUDIO_ROUTE_EARPIECE)

        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ab, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice_ac, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False

        self.log.info("Step5: Disable camera on PhoneA and PhoneB.")
        if not video_call_downgrade(self.log, ads[0], call_id_video_ab, ads[1],
                                    get_call_id_in_video_state(
                                        self.log, ads[1],
                                        VT_STATE_BIDIRECTIONAL)):
            self.log.error("Failed to disable video on PhoneA.")
            return False
        if not video_call_downgrade(self.log, ads[1],
                                    get_call_id_in_video_state(
                                        self.log, ads[1], VT_STATE_TX_ENABLED),
                                    ads[0], call_id_video_ab):
            self.log.error("Failed to disable video on PhoneB.")
            return False

        self.log.info("Step6: Verify calls in correct state.")
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ab, VT_STATE_AUDIO_ONLY,
                CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice_ac, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False

        return {
            False: self._test_vt_conference_merge_drop,
            True: self._test_vt_conference_merge_drop_cep
        }[use_cep](ads, call_id_video_ab, call_id_voice_ac)

    @test_tracker_info(uuid="cd051485-80be-47a3-b249-e09bf786a012")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_add_mt_voice_swap_downgrade_merge_drop(self):
        """Conference call

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Make Sure PhoneC is in LTE mode (with VoLTE).
        PhoneA add a Bi-Directional Video call to PhoneB.
        PhoneB accept as Video.
        PhoneC VoLTE call to PhoneA. Accept on PhoneA.
        Swap Active call on PhoneA.
        Downgrade Video call on PhoneA and PhoneB to audio only.
        Merge call on PhoneA.
        Hang up on PhoneB.
        Hang up on PhoneC.
        """
        return self._test_call_video_add_mt_voice_swap_downgrade_merge_drop(
            False)

    @test_tracker_info(uuid="3e171185-7bfc-4db3-8b0b-f0f1a1b79698")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_add_mt_voice_swap_downgrade_merge_drop_cep(self):
        """Conference call

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Make Sure PhoneC is in LTE mode (with VoLTE).
        PhoneA add a Bi-Directional Video call to PhoneB.
        PhoneB accept as Video.
        PhoneC VoLTE call to PhoneA. Accept on PhoneA.
        Swap Active call on PhoneA.
        Downgrade Video call on PhoneA and PhoneB to audio only.
        Merge call on PhoneA.
        Hang up on PhoneB.
        Hang up on PhoneC.
        """
        return self._test_call_video_add_mt_voice_swap_downgrade_merge_drop(
            True)

    def _test_call_video_add_mt_voice_swap_downgrade_merge_drop(
            self, use_cep=False):
        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_video, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate Video Call PhoneA->PhoneB.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False
        call_id_video_ab = get_call_id_in_video_state(self.log, ads[0],
                                                      VT_STATE_BIDIRECTIONAL)
        if call_id_video_ab is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info("Step2: Initiate Voice Call PhoneC->PhoneA.")
        if not call_setup_teardown(
                self.log,
                ads[2],
                ads[0],
                None,
                verify_caller_func=is_phone_in_call_volte,
                verify_callee_func=None):
            self.log.error("Failed to setup a call")
            return False

        self.log.info(
            "Step3: Verify PhoneA's video/voice call in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False
        for call in calls:
            if call != call_id_video_ab:
                call_id_voice_ac = call

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ab,
                VT_STATE_BIDIRECTIONAL_PAUSED, CALL_STATE_HOLDING):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice_ac, VT_STATE_AUDIO_ONLY,
                CALL_STATE_ACTIVE):
            return False

        self.log.info(
            "Step4: Swap calls on PhoneA and verify call state correct.")
        ads[0].droid.telecomCallHold(call_id_voice_ac)
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        for ad in [ads[0], ads[1]]:
            if get_audio_route(self.log, ad) != AUDIO_ROUTE_SPEAKER:
                self.log.error("{} Audio is not on speaker.".format(ad.serial))
                # TODO: b/26337892 Define expected audio route behavior.
            set_audio_route(self.log, ad, AUDIO_ROUTE_EARPIECE)

        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ab, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice_ac, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False

        self.log.info("Step5: Disable camera on PhoneA and PhoneB.")
        if not video_call_downgrade(self.log, ads[0], call_id_video_ab, ads[1],
                                    get_call_id_in_video_state(
                                        self.log, ads[1],
                                        VT_STATE_BIDIRECTIONAL)):
            self.log.error("Failed to disable video on PhoneA.")
            return False
        if not video_call_downgrade(self.log, ads[1],
                                    get_call_id_in_video_state(
                                        self.log, ads[1], VT_STATE_TX_ENABLED),
                                    ads[0], call_id_video_ab):
            self.log.error("Failed to disable video on PhoneB.")
            return False

        self.log.info("Step6: Verify calls in correct state.")
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ab, VT_STATE_AUDIO_ONLY,
                CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice_ac, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False

        return {
            False: self._test_vt_conference_merge_drop,
            True: self._test_vt_conference_merge_drop_cep
        }[use_cep](ads, call_id_video_ab, call_id_voice_ac)

    @test_tracker_info(uuid="3dd68dee-87ca-4e9b-8de8-dba6dc8f5725")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_add_mo_video_downgrade_merge_drop(self):
        """Conference call

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with VoLTE).
        Make Sure PhoneC is in LTE mode (with Video Calling).
        PhoneA VoLTE call to PhoneB. Accept on PhoneB.
        PhoneA add a Bi-Directional Video call to PhoneC.
        PhoneC accept as Video.
        Downgrade Video call on PhoneA and PhoneC to audio only.
        Merge call on PhoneA.
        Hang up on PhoneB.
        Hang up on PhoneC.
        """
        return self._test_call_volte_add_mo_video_downgrade_merge_drop(False)

    @test_tracker_info(uuid="823f9b6a-7812-4f17-9534-e784a623e7e2")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_add_mo_video_downgrade_merge_drop_cep(self):
        """Conference call

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with VoLTE).
        Make Sure PhoneC is in LTE mode (with Video Calling).
        PhoneA VoLTE call to PhoneB. Accept on PhoneB.
        PhoneA add a Bi-Directional Video call to PhoneC.
        PhoneC accept as Video.
        Downgrade Video call on PhoneA and PhoneC to audio only.
        Merge call on PhoneA.
        Hang up on PhoneB.
        Hang up on PhoneC.
        """
        return self._test_call_volte_add_mo_video_downgrade_merge_drop(True)

    def _test_call_volte_add_mo_video_downgrade_merge_drop(self, use_cep):
        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_video,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate VoLTE Call PhoneA->PhoneB.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                verify_caller_func=is_phone_in_call_volte,
                verify_callee_func=is_phone_in_call_volte):
            self.log.error("Failed to setup a call")
            return False
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 1:
            self.log.error("Active call numbers in PhoneA is not 1.")
            return False
        call_id_voice_ab = calls[0]

        self.log.info("Step2: Initiate Video Call PhoneA->PhoneC.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[2],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False
        call_id_video_ac = get_call_id_in_video_state(self.log, ads[0],
                                                      VT_STATE_BIDIRECTIONAL)
        if call_id_video_ac is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info(
            "Step3: Verify PhoneA's video/voice call in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ac, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice_ab, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False

        self.log.info("Step4: Disable camera on PhoneA and PhoneC.")
        if not video_call_downgrade(self.log, ads[0], call_id_video_ac, ads[2],
                                    get_call_id_in_video_state(
                                        self.log, ads[2],
                                        VT_STATE_BIDIRECTIONAL)):
            self.log.error("Failed to disable video on PhoneA.")
            return False
        if not video_call_downgrade(self.log, ads[2],
                                    get_call_id_in_video_state(
                                        self.log, ads[2], VT_STATE_TX_ENABLED),
                                    ads[0], call_id_video_ac):
            self.log.error("Failed to disable video on PhoneB.")
            return False

        self.log.info("Step6: Verify calls in correct state.")
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ac, VT_STATE_AUDIO_ONLY,
                CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice_ab, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False

        return {
            False: self._test_vt_conference_merge_drop,
            True: self._test_vt_conference_merge_drop_cep
        }[use_cep](ads, call_id_video_ac, call_id_voice_ab)

    @test_tracker_info(uuid="9926fb63-8230-461a-8c8c-1a9556fbb2a9")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_add_mt_video_downgrade_merge_drop(self):
        """Conference call

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with VoLTE).
        Make Sure PhoneC is in LTE mode (with Video Calling).
        PhoneA VoLTE call to PhoneB. Accept on PhoneB.
        PhoneC add a Bi-Directional Video call to PhoneA.
        PhoneA accept as Video.
        Downgrade Video call on PhoneA and PhoneC to audio only.
        Merge call on PhoneA.
        Hang up on PhoneB.
        Hang up on PhoneC.
        """
        return self._test_call_volte_add_mt_video_downgrade_merge_drop(False)

    @test_tracker_info(uuid="26b72fda-1a25-47fb-8acb-6f750be8237a")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_add_mt_video_downgrade_merge_drop_cep(self):
        """Conference call

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with VoLTE).
        Make Sure PhoneC is in LTE mode (with Video Calling).
        PhoneA VoLTE call to PhoneB. Accept on PhoneB.
        PhoneC add a Bi-Directional Video call to PhoneA.
        PhoneA accept as Video.
        Downgrade Video call on PhoneA and PhoneC to audio only.
        Merge call on PhoneA.
        Hang up on PhoneB.
        Hang up on PhoneC.
        """
        return self._test_call_volte_add_mt_video_downgrade_merge_drop(True)

    def _test_call_volte_add_mt_video_downgrade_merge_drop(self, use_cep):
        # TODO: b/21437650 Test will fail. After established 2nd call ~15s,
        # Phone C will drop call.
        ads = self.android_devices
        self.number_of_devices = 3
        tasks = [(phone_setup_video, (self.log, ads[0])),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_video,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Initiate VoLTE Call PhoneA->PhoneB.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                verify_caller_func=is_phone_in_call_volte,
                verify_callee_func=is_phone_in_call_volte):
            self.log.error("Failed to setup a call")
            return False
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 1:
            self.log.error("Active call numbers in PhoneA is not 1.")
            return False
        call_id_voice_ab = calls[0]

        self.log.info("Step2: Initiate Video Call PhoneC->PhoneA.")
        if not video_call_setup_teardown(
                self.log,
                ads[2],
                ads[0],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False
        call_id_video_ac = get_call_id_in_video_state(self.log, ads[0],
                                                      VT_STATE_BIDIRECTIONAL)
        if call_id_video_ac is None:
            self.log.error("No active video call in PhoneA.")
            return False

        self.log.info(
            "Step3: Verify PhoneA's video/voice call in correct state.")
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Active call numbers in PhoneA is not 2.")
            return False

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ac, VT_STATE_BIDIRECTIONAL,
                CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice_ab, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False

        self.log.info("Step4: Disable camera on PhoneA and PhoneC.")
        if not video_call_downgrade(self.log, ads[0], call_id_video_ac, ads[2],
                                    get_call_id_in_video_state(
                                        self.log, ads[2],
                                        VT_STATE_BIDIRECTIONAL)):
            self.log.error("Failed to disable video on PhoneA.")
            return False
        if not video_call_downgrade(self.log, ads[2],
                                    get_call_id_in_video_state(
                                        self.log, ads[2], VT_STATE_TX_ENABLED),
                                    ads[0], call_id_video_ac):
            self.log.error("Failed to disable video on PhoneB.")
            return False

        self.log.info("Step6: Verify calls in correct state.")
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_video_ac, VT_STATE_AUDIO_ONLY,
                CALL_STATE_ACTIVE):
            return False
        if not verify_video_call_in_expected_state(
                self.log, ads[0], call_id_voice_ab, VT_STATE_AUDIO_ONLY,
                CALL_STATE_HOLDING):
            return False

        return {
            False: self._test_vt_conference_merge_drop,
            True: self._test_vt_conference_merge_drop_cep
        }[use_cep](ads, call_id_video_ac, call_id_voice_ab)

    @test_tracker_info(uuid="4031040c-d077-4bf1-8a86-82f484693e64")
    @TelephonyBaseTest.tel_test_wrap
    def test_disable_data_vt_unavailable(self):
        """Disable Data, phone should no be able to make VT call.

        Make sure PhoneA and PhoneB can make VT call.
        Disable Data on PhoneA.
        Make sure phoneA report vt_enabled as false.
        Attempt to make a VT call from PhoneA to PhoneB,
        Verify the call succeed as Voice call.
        """

        self.log.info("Step1 Make sure Phones are able make VT call")
        ads = self.android_devices
        ads[0], ads[1] = ads[1], ads[0]
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        try:
            self.log.info("Step2 Turn off data and verify not connected.")
            ads[0].droid.telephonyToggleDataConnection(False)
            if verify_http_connection(self.log, ads[0]):
                self.log.error("Internet Accessible when Disabled")
                return False

            self.log.info("Step3 Verify vt_enabled return false.")
            if wait_for_video_enabled(self.log, ads[0],
                                      MAX_WAIT_TIME_VOLTE_ENABLED):
                self.log.error(
                    "{} failed to <report vt enabled false> for {}s.".format(
                        ads[0].serial, MAX_WAIT_TIME_VOLTE_ENABLED))
                return False
            self.log.info(
                "Step4 Attempt to make VT call, verify call is AUDIO_ONLY.")
            if not video_call_setup_teardown(
                    self.log,
                    ads[0],
                    ads[1],
                    ads[0],
                    video_state=VT_STATE_BIDIRECTIONAL,
                    verify_caller_func=is_phone_in_call_voice_hd,
                    verify_callee_func=is_phone_in_call_voice_hd):
                self.log.error("Call failed or is not AUDIO_ONLY")
                return False

        finally:
            ads[0].droid.telephonyToggleDataConnection(True)

        return True


""" Tests End """
