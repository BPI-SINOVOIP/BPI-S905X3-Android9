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

from acts.test_utils.bt import BtEnum
from acts.test_utils.bt.bt_test_utils import clear_bonded_devices
from acts.test_utils.coex.CoexBaseTest import CoexBaseTest
from acts.test_utils.coex.coex_test_utils import connect_ble
from acts.test_utils.coex.coex_test_utils import initiate_disconnect_from_hf
from acts.test_utils.coex.coex_test_utils import multithread_func
from acts.test_utils.coex.coex_test_utils import music_play_and_check_via_app
from acts.test_utils.coex.coex_test_utils import pair_and_connect_headset
from acts.test_utils.coex.coex_test_utils import setup_tel_config


class CoexBtMultiProfileFunctionalityTest(CoexBaseTest):

    def __init__(self, controllers):
        CoexBaseTest.__init__(self, controllers)

    def setup_class(self):
        CoexBaseTest.setup_class(self)
        req_params = ["sim_conf_file", "music_play_time"]
        self.unpack_userparams(req_params)
        self.ag_phone_number, self.re_phone_number = setup_tel_config(
            self.pri_ad, self.sec_ad, self.sim_conf_file)

    def setup_test(self):
        CoexBaseTest.setup_test(self)
        self.audio_receiver.pairing_mode()
        if not pair_and_connect_headset(
                self.pri_ad, self.audio_receiver.mac_address,
                set([BtEnum.BluetoothProfile.HEADSET.value]) and
                set([BtEnum.BluetoothProfile.A2DP.value])):
            self.log.error("Failed to pair and connect to headset")
            return False

    def teardown_test(self):
        clear_bonded_devices(self.pri_ad)
        CoexBaseTest.teardown_test(self)
        self.audio_receiver.clean_up()

    def start_media_streaming_initiate_hfp_call_with_iperf(self):
        """Start media streaming and initiate call from hf to check
        SCO connection along with iperf.

        Returns:
            True if successful, False otherwise.
        """
        self.run_iperf_and_get_result()
        if not music_play_and_check_via_app(
                self.pri_ad, self.audio_receiver.mac_address):
            self.log.error("Failed to stream music file")
            return False
        if not initiate_disconnect_from_hf(
                self.audio_receiver, self.pri_ad, self.sec_ad,
                self.iperf["duration"]):
            self.log.error("Failed to initiate/hung up call")
            return False
        return self.teardown_result()

    def ble_with_multiprofile_connection(self):
        """Wrapper function to check ble connection alongwith a2dp streaming
        and hfp call connection with iperf.
        """
        if not connect_ble(self.pri_ad, self.sec_ad):
            self.log.error("Failed to connect BLE device")
            return False
        if not music_play_and_check_via_app(
                self.pri_ad, self.audio_receiver.mac_address):
            self.log.error("Failed to stream music file")
            return False
        self.run_iperf_and_get_result()
        tasks = [(initiate_disconnect_from_hf,
                  (self.audio_receiver, self.pri_ad, self.sec_ad,
                   self.iperf["duration"]))]
        if not multithread_func(self.log, tasks):
            return False
        return self.teardown_result()

    def test_a2dp_streaming_hfp_call_with_tcp_ul(self):
        """Starts TCP-uplink traffic with media streaming and HFP call.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the functional behaviour of media streaming
        via A2DP and initiating a call when media streaming is ongoing to
        check HFP.

        Steps:
        1. Start TCP-uplink traffic.
        1. Enable bluetooth.
        2. Start media streaming to A2DP headset.
        4. Initiate a call from headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_066
        """
        if not self.start_media_streaming_initiate_hfp_call_with_iperf():
            return False
        return True

    def test_a2dp_streaming_hfp_call_with_tcp_dl(self):
        """Starts TCP-downlink traffic with media streaming and HFP call.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the functional behaviour of media streaming
        via A2DP and initiating a call when media streaming is ongoing to
        check HFP.

        Steps:
        1. Start TCP-downlink traffic.
        1. Enable bluetooth.
        2. Start media streaming to A2DP headset.
        4. Initiate a call from headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_067
        """
        if not self.start_media_streaming_initiate_hfp_call_with_iperf():
            return False
        return True

    def test_a2dp_streaming_hfp_call_with_udp_ul(self):
        """Starts UDP-uplink traffic with media streaming and HFP call.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the functional behaviour of media streaming
        via A2DP and initiating a call when media streaming is ongoing to
        check HFP.

        Steps:
        1. Start UDP-uplink traffic.
        1. Enable bluetooth.
        2. Start media streaming to A2DP headset.
        4. Initiate a call from headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_068
        """
        if not self.start_media_streaming_initiate_hfp_call_with_iperf():
            return False
        return True

    def test_a2dp_streaming_hfp_call_with_udp_dl(self):
        """Starts UDP-downlink traffic with media streaming and HFP call.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the functional behaviour of media streaming
        via A2DP and initiating a call when media streaming is ongoing to
        check HFP.

        Steps:
        1. Start UDP-downlink traffic.
        1. Enable bluetooth.
        2. Start media streaming to A2DP headset.
        4. Initiate a call from headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_069
        """
        if not self.start_media_streaming_initiate_hfp_call_with_iperf():
            return False
        return True

    def test_ble_connection_a2dp_streaming_hfp_call_with_tcp_ul(self):
        """Starts TCP-uplink traffic while connecting to BLE device,
        A2DP streaming and HFP call.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the functional behaviour of BLE connection,
        media streaming via A2DP and HFP call connection.

        Steps:
        1. Enable Bluetooth.
        2. Connect to BLE device.
        3. Start media streaming to A2DP headset.
        4. Start TCP-uplink traffic.
        5. Initiate HFP call.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_082
        """
        if not self.ble_with_multiprofile_connection():
            return True
        return False

    def test_ble_connection_a2dp_streaming_hfp_call_with_tcp_dl(self):
        """Starts TCP-downlink traffic while connecting to BLE device,
        A2DP streaming and HFP call.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the functional behaviour of BLE connection,
        media streaming via A2DP and HFP call connection.

        Steps:
        1. Enable Bluetooth.
        2. Connect to BLE device.
        3. Start media streaming to A2DP headset.
        4. Start TCP-uplink traffic.
        5. Initiate HFP call.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_083.
        """
        if not self.ble_with_multiprofile_connection():
            return True
        return False
