#!/usr/bin/env python3.4
#
#   Copyright 2018 - Google
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
    Test Script for ViWiFi live call test
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
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
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
from acts.test_utils.tel.tel_video_utils import \
    is_phone_in_call_viwifi_bidirectional
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
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan

DEFAULT_LONG_DURATION_CALL_TOTAL_DURATION = 1 * 60 * 60  # default 1 hour


class TelWifiVideoTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)

        self.stress_test_number = self.get_stress_test_number()
        self.wifi_network_ssid = self.user_params.get("wifi_network_ssid")
        self.wifi_network_pass = self.user_params.get("wifi_network_pass")

        self.long_duration_call_total_duration = self.user_params.get(
            "long_duration_call_total_duration",
            DEFAULT_LONG_DURATION_CALL_TOTAL_DURATION)

    """ Tests Begin """

    @test_tracker_info(uuid="375e9b88-8d8e-45fe-8502-e4da4147682d")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_to_video_wifi_preferred(self):
        """ Test ViWifi<->ViWifi call functionality.

        Make Sure PhoneA is in iWLAN mode (with Video Calling).
        Make Sure PhoneB is in iWLAN mode (with Video Calling).
        Connect to Wifi
        Call from PhoneA to PhoneB as Bi-Directional Video,
        Accept on PhoneB as video call, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        tasks = [
            (phone_setup_iwlan,
             (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
              self.wifi_network_ssid, self.wifi_network_pass)),
            (phone_setup_iwlan,
             (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
              self.wifi_network_ssid, self.wifi_network_pass)),
        ]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ads[0],
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_viwifi_bidirectional,
                verify_callee_func=is_phone_in_call_viwifi_bidirectional):
            self.log.error("Failed to setup+teardown a call")
            return False

        return True

    @test_tracker_info(uuid="0c6782b4-fa81-4c18-a7bf-9f0f5cc05d6d")
    @TelephonyBaseTest.tel_test_wrap
    def test_call_video_to_video_wifi_preferred_apm(self):
        """ Test ViWifi<->ViWifi call functionality in APM Mode.

        Make Sure PhoneA is in iWLAN mode (with Video Calling).
        Make Sure PhoneB is in iWLAN mode (with Video Calling).
        Turn on APM Mode
        Connect to Wifi
        Call from PhoneA to PhoneB as Bi-Directional Video,
        Accept on PhoneB as video call, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        tasks = [
            (phone_setup_iwlan,
             (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
              self.wifi_network_ssid, self.wifi_network_pass)),
            (phone_setup_iwlan,
             (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
              self.wifi_network_ssid, self.wifi_network_pass)),
        ]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ads[0],
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_viwifi_bidirectional,
                verify_callee_func=is_phone_in_call_viwifi_bidirectional):
            self.log.error("Failed to setup+teardown a call")
            return False

        return True


""" Tests End """
