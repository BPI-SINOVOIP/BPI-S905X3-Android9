# /usr/bin/env python3.4
#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

import time

from acts.test_utils.bt import BtEnum
from acts.test_utils.bt.bt_test_utils import clear_bonded_devices
from acts.test_utils.coex.CoexBaseTest import CoexBaseTest
from acts.test_utils.coex.coex_test_utils import connect_dev_to_headset
from acts.test_utils.coex.coex_test_utils import disconnect_headset_from_dev
from acts.test_utils.coex.coex_constants import AUDIO_ROUTE_BLUETOOTH
from acts.test_utils.coex.coex_constants import AUDIO_ROUTE_SPEAKER
from acts.test_utils.coex.coex_test_utils import initiate_disconnect_from_hf
from acts.test_utils.coex.coex_test_utils import pair_and_connect_headset
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_voice_utils import set_audio_route


class CoexHfpStressTest(CoexBaseTest):

    def __init__(self, controllers):
        CoexBaseTest.__init__(self, controllers)

    def setup_class(self):
        CoexBaseTest.setup_class(self)
        req_params = ["iterations"]
        self.unpack_userparams(req_params)

    def setup_test(self):
        CoexBaseTest.setup_test(self)
        self.audio_receiver.pairing_mode()
        if not pair_and_connect_headset(
                self.pri_ad, self.audio_receiver.mac_address,
                set([BtEnum.BluetoothProfile.HEADSET.value])):
            self.log.error("Failed to pair and connect to headset")
            return False

    def teardown_test(self):
        clear_bonded_devices(self.pri_ad)
        CoexBaseTest.teardown_test(self)
        self.audio_receiver.clean_up()

    def connect_disconnect_hfp_headset(self):
        """Connect and disconnect hfp profile on headset for multiple
         iterations.

        Steps:
        1.Connect hfp profile on headset.
        2.Disconnect hfp profile on headset.
        3.Repeat step 1 and 2 for N iterations.

        Returns:
            True if successful, False otherwise.
        """
        for i in range(self.iterations):
            if not connect_dev_to_headset(
                    self.pri_ad, self.audio_receiver.mac_address,
                    {BtEnum.BluetoothProfile.HEADSET.value}):
                self.log.error("Failure to connect HFP headset.")
                return False

            if not disconnect_headset_from_dev(
                    self.pri_ad, self.audio_receiver.mac_address,
                    {BtEnum.BluetoothProfile.HEADSET.value}):
                self.log.error("Could not disconnect {}".format(
                    self.audio_receiver.mac_address))
                return False
        return True

    def initiate_call_from_hf_disconnect_from_ag(self):
        """Initiates call from HF and disconnect call from ag for multiple
        iterations.

        Returns:
            True if successful, False otherwise.
        """
        for i in range(self.iterations):
            if not self.audio_receiver.initiate_call_from_hf():
                self.log.error("Failed to initiate call.")
                return False
            time.sleep(5)
            if not hangup_call(self.log, self.pri_ad):
                self.log.error("Failed to hang up the call.")
                return False
        return True

    def route_audio_from_hf_to_speaker(self):
        """Route audio from HF to primary device inbuilt speakers and
        vice_versa.

        Steps:
        1. Initiate call from HF.
        2. Toggle audio from HF to speaker and vice-versa from N iterations.
        3. Hangup call from primary device.

        Returns:
            True if successful, False otherwise.
        """
        if not self.audio_receiver.initiate_call_from_hf():
            self.log.error("Failed to initiate call.")
            return False
        for i in range(self.iterations):
            self.log.info("DUT speaker iteration = {}".format(i))
            if not set_audio_route(self.log, self.pri_ad, AUDIO_ROUTE_SPEAKER):
                self.log.error("Failed switching to primary device speaker.")
                hangup_call(self.log, self.pri_ad)
                return False
            time.sleep(2)
            if not set_audio_route(self.log, self.pri_ad,
                                   AUDIO_ROUTE_BLUETOOTH):
                self.log.error("Failed switching to bluetooth headset.")
                hangup_call(self.log, self.pri_ad)
                return False
        if not hangup_call(self.log, self.pri_ad):
            self.log.error("Failed to hang up the call.")
            return False
        return True

    def connect_disconnect_hfp_headset_with_iperf(self):
        """Wrapper function to start iperf traffic and connect/disconnect
        to a2dp headset for N iterations.
        """
        self.run_iperf_and_get_result()
        if not self.connect_disconnect_hfp_headset():
            return False
        return self.teardown_result()

    def hfp_long_duration_with_iperf(self):
        """Wrapper function to start iperf traffic and initiate hfp call."""
        self.run_iperf_and_get_result()
        if not initiate_disconnect_from_hf(
                self.audio_receiver, self.pri_ad, self.sec_ad,
                self.iperf["duration"]):
            return False
        return self.teardown_result()

    def initiate_call_multiple_times_with_iperf(self):
        """Wrapper function to start iperf traffic and initiate call and
        disconnect call simultaneously.
        """
        self.run_iperf_and_get_result()
        if not self.initiate_call_from_hf_disconnect_from_ag():
            return False
        return self.teardown_result()

    def route_audio_from_hf_to_speaker_with_iperf(self):
        """Wrapper function to start iperf traffic and route audio from
        headset to speaker.
        """
        self.run_iperf_and_get_result()
        if not self.route_audio_from_hf_to_speaker():
            return False
        return self.teardown_result()

    def test_stress_hfp_long_duration_with_tcp_ul(self):
        """Stress test on hfp call continuously for 12 hours.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the integrity of hfp connection for 12 hours.

        Steps:
        1. Start TCP-uplink traffic.
        2. Initiate call.
        3. Verify call status.
        4. Disconnect call.
        5. Repeat steps 2 to 4 for N iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_021
        """
        if not self.hfp_long_duration_with_iperf():
            return False
        return True

    def test_stress_hfp_long_duration_with_tcp_dl(self):
        """Stress test on hfp call continuously for 12 hours.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the integrity of hfp connection for 12 hours.

        Steps:
        1. Start TCP-downlink traffic.
        2. Initiate call.
        3. Verify call status.
        4. Disconnect call.
        5. Repeat steps 2 to 4 for N iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_022
        """
        if not self.hfp_long_duration_with_iperf():
            return False
        return True

    def test_stress_hfp_long_duration_with_udp_ul(self):
        """Stress test on hfp call continuously for 12 hours.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the integrity of hfp connection for 12 hours.

        Steps:
        1. Start UDP-uplink traffic.
        2. Initiate call.
        3. Verify call status.
        4. Disconnect call.
        5. Repeat steps 2 to 4 for N iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_023
        """
        if not self.hfp_long_duration_with_iperf():
            return False
        return True

    def test_stress_hfp_long_duration_with_udp_dl(self):
        """Stress test on hfp call continuously for 12 hours.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the integrity of hfp connection for 12 hours.

        Steps:
        1. Start UDP-downlink traffic.
        2. Initiate call.
        3. Verify call status.
        4. Disconnect call.
        5. Repeat steps 2 to 4 for N iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_024
        """
        if not self.hfp_long_duration_with_iperf():
            return False
        return True

    def test_stress_hfp_call_multiple_times_with_tcp_ul(self):
        """Stress test for initiate and disconnect hfp call.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the integrity of hfp call.

        Steps:
        1. Start TCP-uplink traffic.
        2. Initiate call from HF
        3. Verify status of call
        4. Disconnect from AG.
        5. Repeat steps 2 to 4 for N iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_025
        """
        if not self.initiate_call_multiple_times_with_iperf():
            return False
        return True

    def test_stress_hfp_call_multiple_times_with_tcp_dl(self):
        """Stress test for initiate and disconnect hfp call.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the integrity of hfp call.

        Steps:
        1. Start TCP-downlink traffic.
        2. Initiate call from HF
        3. Verify status of call
        4. Disconnect from AG.
        5. Repeat steps 2 to 4 for N iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_026
        """
        if not self.initiate_call_multiple_times_with_iperf():
            return False
        return True

    def test_stress_hfp_call_multiple_times_with_udp_ul(self):
        """Stress test for initiate and disconnect hfp call.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the integrity of hfp call.

        Steps:
        1. Start UDP-uplink traffic.
        2. Initiate call from HF
        3. Verify status of call
        4. Disconnect from AG.
        5. Repeat steps 2 to 4 for N iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_027
        """
        if not self.initiate_call_multiple_times_with_iperf():
            return False
        return True

    def test_stress_hfp_call_multiple_times_with_udp_dl(self):
        """Stress test for initiate and disconnect hfp call.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the integrity of hfp call.

        Steps:
        1. Start UDP-downlink traffic.
        2. Initiate call from HF
        3. Verify status of call
        4. Disconnect from AG.
        5. Repeat steps 2 to 4 for N iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_028
        """
        if not self.initiate_call_multiple_times_with_iperf():
            return False
        return True

    def test_stress_connect_disconnect_hfp_profile_with_tcp_ul(self):
        """Stress test for connect/disconnect hfp headset.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset with hfp profile.

        Steps:
        1. Run TCP-uplink traffic.
        2. Connect and disconnect headset with hfp profile.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_033
        """
        if not self.connect_disconnect_hfp_headset():
            return False
        return True

    def test_stress_connect_disconnect_hfp_profile_with_tcp_dl(self):
        """Stress test for connect/disconnect hfp headset.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset with hfp profile.

        Steps:
        1. Run TCP-downlink traffic.
        2. Connect and disconnect headset with hfp profile.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_034
        """
        if not self.connect_disconnect_hfp_headset():
            return False
        return True

    def test_stress_connect_disconnect_hfp_profile_with_udp_ul(self):
        """Stress test for connect/disconnect hfp headset.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset with hfp profile.

        Steps:
        1. Run UDP-uplink traffic.
        2. Connect and disconnect headset with hfp profile.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_035
        """
        if not self.connect_disconnect_hfp_headset():
            return False
        return True

    def test_stress_connect_disconnect_hfp_profile_with_udp_dl(self):
        """Stress test for connect/disconnect hfp headset.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset with hfp profile.

        Steps:
        1. Run UDP-downlink traffic.
        2. Connect and disconnect headset with hfp profile.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_036
        """
        if not self.connect_disconnect_hfp_headset():
            return False
        return True

    def test_stress_audio_routing_with_tcp_ul(self):
        """Stress to route audio from HF to primary device speaker.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the integrity of audio routing between
        bluetooth headset and android device inbuilt speaker.

        Steps:
        1. Starts TCP-uplink traffic.
        2. Route audio from hf to speaker and vice-versa.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_037
        """
        if not self.route_audio_from_hf_to_speaker_with_iperf():
            return False
        return True

    def test_stress_audio_routing_with_tcp_dl(self):
        """Stress to route audio from HF to primary device speaker.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the integrity of audio routing between
        bluetooth headset and android device inbuilt speaker.

        Steps:
        1. Starts TCP-downlink traffic.
        2. Route audio from hf to speaker and vice-versa.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_038
        """
        if not self.route_audio_from_hf_to_speaker_with_iperf():
            return False
        return True

    def test_stress_audio_routing_with_udp_ul(self):
        """Stress to route audio from HF to primary device speaker.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the integrity of audio routing between
        bluetooth headset and android device inbuilt speaker.

        Steps:
        1. Starts UDP-uplink traffic.
        2. Route audio from hf to speaker and vice-versa.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_039
        """
        if not self.route_audio_from_hf_to_speaker_with_iperf():
            return False
        return True

    def test_stress_audio_routing_with_udp_dl(self):
        """Stress to route audio from HF to primary device speaker.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the integrity of audio routing between
        bluetooth headset and android device inbuilt speaker.

        Steps:
        1. Starts UDP-downlink traffic.
        2. Route audio from hf to speaker and vice-versa.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_040
        """
        if not self.route_audio_from_hf_to_speaker_with_iperf():
            return False
        return True

    def test_stress_connect_disconnect_hfp_with_tcp_bidirectional(self):
        """Stress test for connect/disconnect headset.

        This test is to start TCP-bidirectional traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset with hfp profile.

        Steps:
        1. Run TCP-bidirectional traffic.
        2. Connect and disconnect headset with hfp profile.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_067
        """
        if not self.connect_disconnect_hfp_headset():
            return False
        return True

    def test_stress_connect_disconnect_hfp_with_udp_bidirectional(self):
        """Stress test for connect/disconnect headset.

        This test is to start UDP-bidirectional traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset with hfp profile.

        Steps:
        1. Run UDP-bidirectional traffic.
        2. Connect and disconnect headset with hfp profile.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_068
        """
        if not self.connect_disconnect_hfp_headset():
            return False
        return True

    def test_stress_hfp_long_duration_with_tcp_bidirectional(self):
        """Stress test on hfp call continuously for 12 hours.

        This test is to start TCP-bidirectional traffic between host machine and
        android device and test the integrity of hfp connection for 12 hours.

        Steps:
        1. Start TCP-bidirectional traffic.
        2. Initiate call.
        3. Verify call status.
        4. Disconnect call.
        5. Repeat steps 2 to 4 for N iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_069
        """
        if not self.hfp_long_duration_with_iperf():
            return False
        return True

    def test_stress_hfp_long_duration_with_udp_bidirectional(self):
        """Stress test on hfp call continuously for 12 hours.

        This test is to start UDP-bidirectional traffic between host machine and
        android device and test the integrity of hfp connection for 12 hours.

        Steps:
        1. Start UDP-bidirectional traffic.
        2. Initiate call.
        3. Verify call status.
        4. Disconnect call.
        5. Repeat steps 2 to 4 for N iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_070
        """
        if not self.hfp_long_duration_with_iperf():
            return False
        return True

    def test_stress_hfp_call_multiple_times_with_tcp_bidirectional(self):
        """Stress test for initiate and disconnect hfp call.

        This test is to start TCP-bidirectional traffic between host machine and
        android device and test the integrity of hfp call.

        Steps:
        1. Start TCP-bidirectional traffic.
        2. Initiate call from HF
        3. Verify status of call
        4. Disconnect from AG.
        5. Repeat steps 2 to 4 for N iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_071
        """
        if not self.initiate_call_multiple_times_with_iperf():
            return False
        return True

    def test_stress_hfp_call_multiple_times_with_udp_bidirectional(self):
        """Stress test for initiate and disconnect hfp call.

        This test is to start UDP-bidirectional traffic between host machine and
        android device and test the integrity of hfp call.

        Steps:
        1. Start UDP-bidirectional traffic.
        2. Initiate call from HF
        3. Verify status of call
        4. Disconnect from AG.
        5. Repeat steps 2 to 4 for N iterations

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_072
        """
        if not self.initiate_call_multiple_times_with_iperf():
            return False
        return True
