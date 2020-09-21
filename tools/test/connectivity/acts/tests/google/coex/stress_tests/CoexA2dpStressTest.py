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
from acts.test_utils.coex.coex_test_utils import music_play_and_check
from acts.test_utils.coex.coex_test_utils import pair_and_connect_headset


class CoexA2dpStressTest(CoexBaseTest):

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
                set([BtEnum.BluetoothProfile.A2DP.value])):
            self.log.error("Failed to pair and connect to headset")
            return False

    def teardown_test(self):
        clear_bonded_devices(self.pri_ad)
        CoexBaseTest.teardown_test(self)
        self.audio_receiver.clean_up()

    def connect_disconnect_headset(self):
        """Initiates connection to paired headset and disconnects headset.

        Returns:
            True if successful False otherwise.
        """
        for i in range(self.iterations):
            self.log.info("Headset connect/disconnect iteration={}".format(i))
            self.pri_ad.droid.bluetoothConnectBonded(
                self.audio_receiver.mac_address)
            time.sleep(2)
            self.pri_ad.droid.bluetoothDisconnectConnected(
                self.audio_receiver.mac_address)
        return True

    def connect_disconnect_a2dp_headset(self):
        """Connect and disconnect a2dp profile on headset for multiple
         iterations.

        Steps:
        1.Connect a2dp profile on headset.
        2.Disconnect a2dp profile on headset.
        3.Repeat step 1 and 2 for N iterations.

        Returns:
            True if successful, False otherwise.
        """
        for i in range(self.iterations):
            if not connect_dev_to_headset(
                    self.pri_ad, self.audio_receiver.mac_address,
                    {BtEnum.BluetoothProfile.A2DP.value}):
                self.log.error("Failure to connect A2dp headset.")
                return False

            if not disconnect_headset_from_dev(
                    self.pri_ad, self.audio_receiver.mac_address,
                    [BtEnum.BluetoothProfile.A2DP.value]):
                self.log.error("Could not disconnect {}".format(
                    self.audio_receiver.mac_address))
                return False
        return True

    def connect_disconnect_headset_with_iperf(self):
        """Wrapper function to start iperf traffic and connect/disconnect
        to headset for N iterations.
        """
        self.run_iperf_and_get_result()
        if not self.connect_disconnect_headset():
            return False
        return self.teardown_result()

    def connect_disconnect_a2dp_headset_with_iperf(self):
        """Wrapper function to start iperf traffic and connect/disconnect
        to a2dp headset for N iterations.
        """
        self.run_iperf_and_get_result()
        if not self.connect_disconnect_a2dp_headset():
            return False
        return self.teardown_result()

    def music_streaming_with_iperf(self):
        """Wrapper function to start iperf traffic and music streaming."""
        self.run_iperf_and_get_result()
        if not music_play_and_check(
                self.pri_ad, self.audio_receiver.mac_address,
                self.music_file_to_play, self.iperf["duration"]):
            return False
        return self.teardown_result()

    def test_stress_connect_disconnect_headset_with_tcp_ul(self):
        """Stress test for connect/disconnect headset.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset.

        Steps:
        1. Run TCP-uplink traffic.
        2. Connect and disconnect headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_013
        """
        if not self.connect_disconnect_headset_with_iperf():
            return False
        return True

    def test_stress_connect_disconnect_headset_with_tcp_dl(self):
        """Stress test for connect/disconnect headset.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset.

        Steps:
        1. Run TCP-downlink traffic.
        2. Connect and disconnect headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_014
        """
        if not self.connect_disconnect_headset_with_iperf():
            return False
        return True

    def test_stress_connect_disconnect_headset_with_udp_ul(self):
        """Stress test for connect/disconnect headset.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset.

        Steps:
        1. Run UDP-uplink traffic.
        2. Connect and disconnect headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_015
        """
        if not self.connect_disconnect_headset_with_iperf():
            return False
        return True

    def test_stress_connect_disconnect_headset_with_udp_dl(self):
        """Stress test for connect/disconnect headset.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset.

        Steps:
        1. Run UDP-downlink traffic.
        2. Connect and disconnect headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_016
        """
        if not self.connect_disconnect_headset_with_iperf():
            return False
        return True

    def test_stress_a2dp_long_duration_with_tcp_ul(self):
        """Stress test to stream music to headset continuously for 12 hours.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the integrity of audio streaming for 12 hours.

        Steps:
        1. Start TCP uplink traffic.
        2. Start music streaming to headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_017
        """
        if not self.music_streaming_with_iperf():
            return False
        return True

    def test_stress_a2dp_long_duration_with_tcp_dl(self):
        """Stress test to stream music to headset continuously for 12 hours.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the integrity of audio streaming for 12 hours.

        Steps:
        1. Start TCP downlink traffic.
        2. Start music streaming to headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_018
        """
        if not self.music_streaming_with_iperf():
            return False
        return True

    def test_stress_a2dp_long_duration_with_udp_ul(self):
        """Stress test to stream music to headset continuously for 12 hours.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the integrity of audio streaming for 12 hours.

        Steps:
        1. Start UDP uplink traffic.
        2. Start music streaming to headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_019
        """
        if not self.music_streaming_with_iperf():
            return False
        return True

    def test_stress_a2dp_long_duration_with_udp_dl(self):
        """Stress test to stream music to headset continuously for 12 hours.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the integrity of audio streaming for 12 hours.

        Steps:
        1. Start UDP downlink traffic.
        2. Start music streaming to headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_020
        """
        if not self.music_streaming_with_iperf():
            return False
        return True

    def test_stress_connect_disconnect_a2dp_profile_with_tcp_ul(self):
        """Stress test for connect/disconnect a2dp headset.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset with a2dp profile.

        Steps:
        1. Run TCP-uplink traffic.
        2. Connect and disconnect headset with a2dp profile.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_029
        """
        if not self.connect_disconnect_a2dp_headset_with_iperf():
            return False
        return True

    def test_stress_connect_disconnect_a2dp_profile_with_tcp_dl(self):
        """Stress test for connect/disconnect a2dp headset.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset with a2dp profile.

        Steps:
        1. Run TCP-downlink traffic.
        2. Connect and disconnect headset with a2dp profile.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_030
        """
        if not self.connect_disconnect_a2dp_headset_with_iperf():
            return False
        return True

    def test_stress_connect_disconnect_a2dp_profile_with_udp_ul(self):
        """Stress test for connect/disconnect a2dp headset.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset with a2dp profile.

        Steps:
        1. Run UDP-uplink traffic.
        2. Connect and disconnect headset with a2dp profile.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_031
        """
        if not self.connect_disconnect_a2dp_headset_with_iperf():
            return False
        return True

    def test_stress_connect_disconnect_a2dp_profile_with_udp_dl(self):
        """Stress test for connect/disconnect a2dp headset.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset with a2dp profile.

        Steps:
        1. Run UDP-downlink traffic.
        2. Connect and disconnect headset with a2dp profile.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_032
        """
        if not self.connect_disconnect_a2dp_headset_with_iperf():
            return False
        return True

    def test_stress_connect_disconnect_headset_with_tcp_bidirectional(self):
        """Stress test for connect/disconnect headset.

        This test starts TCP-bidirectional traffic between host machine and
        android device and test the integrity of connection and disconnection
        to headset.

        Steps:
        1. Run TCP-bidirectional traffic.
        2. Connect and disconnect headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_057
        """
        if not self.connect_disconnect_headset_with_iperf():
            return False
        return True

    def test_stress_connect_disconnect_headset_with_udp_bidirectional(self):
        """Stress test for connect/disconnect headset.

        This test starts UDP-bidirectional traffic between host machin and
        android device and test the integrity of connection and disconnection
        to headset.

        Steps:
        1. Run UDP-bidirectional traffic.
        2. Connect and disconnect headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_058
        """
        if not self.connect_disconnect_headset_with_iperf():
            return False
        return True

    def test_stress_a2dp_long_duration_with_tcp_bidirectional(self):
        """Stress test to stream music to headset continuously for 12 hours.

        This test starts TCP-bidirectional traffic between host machin and
        android device and test the integrity of audio streaming for 12 hours.

        Steps:
        1. Start TCP bidirectional traffic.
        2. Start music streaming to headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_065
        """
        if not self.music_streaming_with_iperf():
            return False
        return True

    def test_stress_a2dp_long_duration_with_udp_bidirectional(self):
        """Stress test to stream music to headset continuously for 12 hours.

        This test starts UDP-bidirectional traffic between host machin and
        android device and test the integrity of audio streaming for 12 hours.

        Steps:
        1. Start UDP bidirectional traffic.
        2. Start music streaming to headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_066
        """
        if not self.music_streaming_with_iperf():
            return False
        return True
