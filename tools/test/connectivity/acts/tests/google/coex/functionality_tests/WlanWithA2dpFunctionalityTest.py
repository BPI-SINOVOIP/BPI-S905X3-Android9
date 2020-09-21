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
from acts.test_utils.coex.coex_test_utils import connect_ble
from acts.test_utils.coex.coex_test_utils import disconnect_headset_from_dev
from acts.test_utils.coex.coex_test_utils import multithread_func
from acts.test_utils.coex.coex_test_utils import music_play_and_check
from acts.test_utils.coex.coex_test_utils import pair_and_connect_headset
from acts.test_utils.coex.coex_test_utils import perform_classic_discovery
from acts.test_utils.coex.coex_test_utils import toggle_screen_state
from acts.test_utils.coex.coex_test_utils import start_fping

BLUETOOTH_WAIT_TIME = 2


class WlanWithA2dpFunctionalityTest(CoexBaseTest):

    def __init__(self, controllers):
        CoexBaseTest.__init__(self, controllers)

    def setup_class(self):
        CoexBaseTest.setup_class(self)
        req_params = ["iterations"]
        self.unpack_userparams(req_params)

    def setup_test(self):
        CoexBaseTest.setup_test(self)
        self.audio_receiver.power_on()
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

    def connect_disconnect_a2dp_headset(self):
        """Connects and disconnect a2dp profile on headset for multiple
        iterations.

        Steps:
        1.Connect a2dp profile on headset.
        2.Disconnect a2dp profile on headset.
        3.Repeat step 1 and 2 for N iterations.

        Returns:
            True if successful, False otherwise.
        """
        for i in range(0, self.iterations):
            self.log.info("A2DP connect/disconnect Iteration {}".format(i))
            if not connect_dev_to_headset(
                    self.pri_ad, self.audio_receiver.mac_address,
                    set([BtEnum.BluetoothProfile.A2DP.value])):
                self.log.error("Failed to connect headset.")
                return False

            if not disconnect_headset_from_dev(
                    self.pri_ad, self.audio_receiver.mac_address,
                    [BtEnum.BluetoothProfile.A2DP.value]):
                self.log.error("Failed to disconnect headset.")
                return False
        return True

    def connect_disconnect_headset(self):
        """Initiates connection to paired headset and disconnects headset.

        Returns:
            True if successful False otherwise.
        """
        for i in range(0, self.iterations):
            self.pri_ad.droid.bluetoothConnectBonded(
                self.audio_receiver.mac_address)
            time.sleep(BLUETOOTH_WAIT_TIME)
            if not self.pri_ad.droid.bluetoothIsDeviceConnected(
                    self.audio_receiver.mac_address):
                return False
            self.pri_ad.droid.bluetoothDisconnectConnected(
                self.audio_receiver.mac_address)
        return True

    def perform_classic_discovery_with_iperf(self):
        """Wrapper function to start iperf traffic and classic discovery"""
        self.run_iperf_and_get_result()
        if not perform_classic_discovery(self.pri_ad):
            return False
        return self.teardown_result()

    def connect_disconnect_a2dp_headset_with_iperf(self):
        """Wrapper function to start iperf traffic and connect/disconnect
        to headset for N iterations.
        """
        self.run_iperf_and_get_result()
        if not self.connect_disconnect_a2dp_headset():
            return False
        return self.teardown_result()

    def music_streaming_bluetooth_discovery_with_iperf(self):
        """Wrapper function to start iperf traffic, music streaming and
        classic discovery.
        """
        self.run_iperf_and_get_result()
        tasks = [(music_play_and_check,
                  (self.pri_ad, self.audio_receiver.mac_address,
                   self.music_file_to_play, self.iperf["duration"])),
                 (perform_classic_discovery, (self.pri_ad,))]
        if not multithread_func(self.log, tasks):
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

    def music_streaming_avrcp_controls_with_iperf(self):
        """Wrapper function to start iperf traffic, music streaming and avrcp
        controls.
        """
        self.run_iperf_and_get_result()
        tasks = [(music_play_and_check,
                  (self.pri_ad, self.audio_receiver.mac_address,
                   self.music_file_to_play, self.iperf["duration"])),
                 (self.avrcp_actions, ())]
        if not multithread_func(self.log, tasks):
            return False
        return self.teardown_result()

    def music_streaming_discovery_avrcp_controls_with_iperf(self):
        """Wrapper function to start iperf traffic, music streaming, bluetooth
        discovery and avrcp controls.
        """
        self.run_iperf_and_get_result()
        tasks = [(music_play_and_check,
                  (self.pri_ad, self.audio_receiver.mac_address,
                   self.music_file_to_play, self.iperf["duration"])),
                 (perform_classic_discovery, (self.pri_ad,)),
                 (self.avrcp_actions, ())]
        if not multithread_func(self.log, tasks):
            return False
        return self.teardown_result()

    def music_streaming_ble_connection_with_iperf(self):
        """Wrapper function to start iperf traffic, music streaming and ble
        connection.
        """
        self.run_iperf_and_get_result()
        tasks = [(music_play_and_check,
                  (self.pri_ad, self.audio_receiver.mac_address,
                   self.music_file_to_play, self.iperf["duration"])),
                 (connect_ble, (self.pri_ad, self.sec_ad))]
        if not multithread_func(self.log, tasks):
            return False
        return self.teardown_result()

    def test_inquiry_after_headset_connection_with_tcp_ul(self):
        """Starts TCP-uplink traffic, start inquiry after bluetooth connection.

        This test is to start TCP-uplink traffic between host machine and
        android device and test functional behaviour of bluetooth discovery
        after connecting to headset.

        Steps:
        1. Run TCP-uplink traffic.
        2. Start bluetooth discovery when headset is connected.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_009
        """
        if not self.perform_classic_discovery_with_iperf():
            return False
        return True

    def test_inquiry_after_headset_connection_with_tcp_dl(self):
        """Starts TCP-downlink traffic, start inquiry after bluetooth connection.

        This test is to start TCP-downlink traffic between host machine and
        android device and test functional behaviour of bluetooth discovery
        after connecting to headset.

        Steps:
        1. Run TCP-downlink traffic.
        2. Start bluetooth discovery when headset is connected.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_010
        """
        if not self.perform_classic_discovery_with_iperf():
            return False
        return True

    def test_inquiry_after_headset_connection_with_udp_ul(self):
        """Starts UDP-uplink traffic, start inquiry after bluetooth connection.

        This test is to start UDP-uplink traffic between host machine and
        android device and test functional behaviour of bluetooth discovery
        after connecting to headset.

        Steps:
        1. Run UDP-uplink traffic.
        2. Start bluetooth discovery when headset is connected.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_011
        """
        if not self.perform_classic_discovery_with_iperf():
            return False
        return True

    def test_inquiry_after_headset_connection_with_udp_dl(self):
        """Starts UDP-downlink traffic, start inquiry after bluetooth connection.

        This test is to start UDP-downlink traffic between host machine and
        android device and test functional behaviour of bluetooth discovery
        after connecting to headset.

        Steps:
        1. Run UDP-downlink traffic.
        2. Start bluetooth discovery when headset is connected.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_012
        """
        if not self.perform_classic_discovery_with_iperf():
            return False
        return True

    def test_connect_disconnect_a2dp_headset_with_tcp_ul(self):
        """Starts TCP-uplink traffic and connect/disconnect a2dp headset.

        This test is to start TCP-uplink traffic between host machine and
        android device and test functional behaviour of connection and
        disconnection to a2dp headset.

        Steps:
        1. Run TCP-uplink traffic.
        2. Connect and disconnect A2DP headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_013
        """
        if not self.connect_disconnect_a2dp_headset_with_iperf():
            return False
        return True

    def test_connect_disconnect_a2dp_headset_with_tcp_dl(self):
        """Starts TCP-downlink traffic and connect/disconnect a2dp headset.

        This test is to start TCP-downlink traffic between host machine and
        android device and test functional behaviour of connection and
        disconnection to a2dp headset.

        Steps:
        1. Run TCP-downlink traffic.
        2. Connect and disconnect A2DP headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_014
        """
        if not self.connect_disconnect_a2dp_headset_with_iperf():
            return False
        return True

    def test_connect_disconnect_a2dp_headset_with_udp_ul(self):
        """Starts UDP-uplink traffic and connect/disconnect a2dp headset.

        This test is to start UDP-uplink traffic between host machine and
        android device and test functional behaviour of connection and
        disconnection to a2dp headset.

        Steps:
        1. Run UDP-uplink traffic.
        2. Connect and disconnect A2DP headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_015
        """
        if not self.connect_disconnect_a2dp_headset_with_iperf():
            return False
        return True

    def test_connect_disconnect_a2dp_headset_with_udp_dl(self):
        """Starts UDP-downlink traffic and connect/disconnect a2dp headset.

        This test is to start UDP-downlink traffic between host machine and
        android device and test functional behaviour of connection and
        disconnection to a2dp headset.

        Steps:
        1. Run UDP-downlink traffic.
        2. Connect and disconnect A2DP headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_016
        """
        if not self.connect_disconnect_a2dp_headset_with_iperf():
            return False
        return True

    def test_a2dp_streaming_bluetooth_discovery_with_tcp_ul(self):
        """Starts TCP-uplink traffic, with music streaming to a2dp headset and
        bluetooth discovery.

        This test is to start TCP-uplink traffic between host machine and
        android device and test functional behaviour of a2dp music streaming
        and bluetooth discovery.

        Steps:
        1. Run TCP-uplink traffic.
        2. Start media streaming to a2dp headset.
        3. Start bluetooth discovery on android device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_017
        """
        if not self.music_streaming_bluetooth_discovery_with_iperf():
            return False
        return True

    def test_a2dp_streaming_bluetooth_discovery_with_tcp_dl(self):
        """Starts TCP-downlink traffic, with music streaming to a2dp headset
        and bluetooth discovery.

        This test is to start TCP-downlink traffic between host machine and
        android device and test functional behaviour of a2dp music streaming
        and bluetooth discovery.

        Steps:
        1. Run TCP-downlink traffic.
        2. Start media streaming to a2dp headset.
        3. Start bluetooth discovery on android device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_018
        """
        if not self.music_streaming_bluetooth_discovery_with_iperf():
            return False
        return True

    def test_a2dp_streaming_bluetooth_discovery_with_udp_ul(self):
        """Starts UDP-uplink traffic, with music streaming to a2dp headset and
        bluetooth discovery.

        This test is to start UDP-uplink traffic between host machine and
        android device and test functional behaviour of a2dp music streaming
        and bluetooth discovery.

        Steps:
        1. Run UDP-uplink traffic.
        2. Start media streaming to a2dp headset.
        3. Start bluetooth discovery on android device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_019
        """
        if not self.music_streaming_bluetooth_discovery_with_iperf():
            return False
        return True

    def test_a2dp_streaming_bluetooth_discovery_with_udp_dl(self):
        """Starts UDP-downlink traffic, with music streaming to a2dp headset
        and bluetooth discovery.

        This test is to start UDP-downlink traffic between host machine and
        android device and test functional behaviour of a2dp music streaming
        and bluetooth discovery.

        Steps:
        1. Run UDP-downlink traffic.
        2. Start media streaming to a2dp headset.
        3. Start bluetooth discovery on android device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_020
        """
        if not self.music_streaming_bluetooth_discovery_with_iperf():
            return False
        return True

    def test_a2dp_streaming_with_tcp_ul(self):
        """Starts TCP-uplink traffic with music streaming to a2dp headset.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming.

        Steps:
        1. Run TCP-uplink traffic.
        2. Start media streaming to a2dp headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_021
        """
        if not self.music_streaming_with_iperf():
            return False
        return True

    def test_a2dp_streaming_with_tcp_dl(self):
        """Starts TCP-downlink traffic with music streaming to a2dp headset.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming.

        Steps:
        1. Run TCP-downlink traffic.
        2. Start media streaming to a2dp headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_022
        """
        if not self.music_streaming_with_iperf():
            return False
        return True

    def test_a2dp_streaming_with_udp_ul(self):
        """Starts UDP-uplink traffic with music streaming to a2dp headset.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming.

        Steps:
        1. Run UDP-uplink traffic.
        2. Start media streaming to a2dp headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_023
        """
        if not self.music_streaming_with_iperf():
            return False
        return True

    def test_a2dp_streaming_with_udp_dl(self):
        """Starts UDP-downlink traffic with music streaming to a2dp headset.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming.

        Steps:
        1. Run UDP-downlink traffic.
        2. Start media streaming to a2dp headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_024
        """
        if not self.music_streaming_with_iperf():
            return False
        return True

    def test_a2dp_streaming_avrcp_controls_with_tcp_ul(self):
        """Starts TCP-uplink traffic with music streaming and avrcp controls.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming and avrcp controls.

        1. Run TCP-uplink traffic.
        2. Start media streaming to a2dp headset.
        3. Check all avrcp related controls.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_025
        """
        if not self.music_streaming_avrcp_controls_with_iperf():
            return False
        return True

    def test_a2dp_streaming_avrcp_controls_with_tcp_dl(self):
        """Starts TCP-downlink traffic with music streaming and avrcp controls.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming and avrcp controls.

        1. Run TCP-downlink traffic.
        2. Start media streaming to a2dp headset.
        3. Check all avrcp related controls.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_026
        """
        if not self.music_streaming_avrcp_controls_with_iperf():
            return False
        return True

    def test_a2dp_streaming_avrcp_controls_with_udp_ul(self):
        """Starts UDP-uplink traffic with music streaming and avrcp controls.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming and avrcp controls.

        1. Run UDP-uplink traffic.
        2. Start media streaming to a2dp headset.
        3. Check all avrcp related controls.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_027
        """
        if not self.music_streaming_avrcp_controls_with_iperf():
            return False
        return True

    def test_a2dp_streaming_avrcp_controls_with_udp_dl(self):
        """Starts UDP-downlink traffic with music streaming and avrcp controls.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming and avrcp controls.

        1. Run UDP-downlink traffic.
        2. Start media streaming to a2dp headset.
        3. Check all avrcp related controls.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_028
        """
        if not self.music_streaming_avrcp_controls_with_iperf():
            return False
        return True

    def test_a2dp_streaming_avrcp_controls_bluetooth_discovery_tcp_ul(self):
        """Starts TCP-uplink traffic with music streaming, avrcp controls and
        bluetooth discovery.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming, avrcp controls and bluetooth discovery.

        1. Run TCP-uplink traffic.
        2. Start media streaming to a2dp headset.
        3. Check all avrcp related controls.
        4. Start bluetooth discovery.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_029
        """
        if not self.music_streaming_discovery_avrcp_controls_with_iperf():
            return False
        return True

    def test_a2dp_streaming_avrcp_controls_bluetooth_discovery_tcp_dl(self):
        """Starts TCP-downlink traffic with music streaming, avrcp controls and
        bluetooth discovery.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming, avrcp controls and bluetooth discovery.

        1. Run TCP-downlink traffic.
        2. Start media streaming to a2dp headset.
        3. Check all avrcp related controls.
        4. Start bluetooth discovery.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_030
        """
        if not self.music_streaming_discovery_avrcp_controls_with_iperf():
            return False
        return True

    def test_a2dp_streaming_avrcp_controls_bluetooth_discovery_udp_ul(self):
        """Starts UDP-uplink traffic with music streaming, avrcp controls and
        bluetooth discovery.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming, avrcp controls and bluetooth discovery.

        1. Run UDP-uplink traffic.
        2. Start media streaming to a2dp headset.
        3. Check all avrcp related controls.
        4. Start bluetooth discovery.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_031
        """
        if not self.music_streaming_discovery_avrcp_controls_with_iperf():
            return False
        return True

    def test_a2dp_streaming_avrcp_controls_bluetooth_discovery_udp_dl(self):
        """Starts UDP-downlink traffic with music streaming, avrcp controls and
        bluetooth discovery.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming, avrcp controls and bluetooth discovery.

        1. Run UDP-downlink traffic.
        2. Start media streaming to a2dp headset.
        3. Check all avrcp related controls.
        4. Start bluetooth discovery.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_032
        """
        if not self.music_streaming_discovery_avrcp_controls_with_iperf():
            return False
        return True

    def test_connect_disconnect_headset_with_fping(self):
        """Starts fping, along with connection and disconnection of headset.

        This test is to start fping between host machine and android device
        with connection and disconnection of paired headset.

        Steps:
        1. Start fping.
        2. Enable bluetooth
        3. Connect bluetooth headset.
        4. Disconnect bluetooth headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_076
        """
        args = [lambda: start_fping(self.pri_ad, self.iperf["duration"])]
        self.run_thread(args)
        if not self.connect_disconnect_headset():
            return False
        return self.teardown_thread()

    def test_a2dp_streaming_with_fping(self):
        """Starts fping along with a2dp streaming.

        This test is to start fping between host machine and android device
        and test the functional behaviour of music streaming to a2dp headset.

        Steps:
        1. Start fping.
        1. Start media play on android device and check for music streaming.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_077
        """
        args = [lambda: start_fping(self.pri_ad, self.iperf["duration"])]
        self.run_thread(args)
        if not music_play_and_check(
                self.pri_ad, self.audio_receiver.mac_address,
                self.music_file_to_play, self.iperf["duration"]):
            return False
        return self.teardown_thread()

    def test_connect_disconnect_headset_toggle_screen_state_with_fping(self):
        """Starts fping along with connection and disconnection of the headset.

        This test is to start fping between host machine and android device
        and test the functional behaviour of connection and disconnection of
        the paired headset when screen is off and on.

        Steps:
        1. Start fping.
        2. Connect bluetooth headset.
        4. Disconnect bluetooth headset.
        5. Screen on/off.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_079
        """
        tasks = [(start_fping, (self.pri_ad, self.iperf["duration"])),
                 (self.connect_disconnect_headset, ()),
                 (toggle_screen_state, (self.pri_ad, self.iterations))]
        if not multithread_func(self.log, tasks):
            return False
        return self.teardown_thread()

    def test_a2dp_streaming_toggle_screen_state_with_fping(self):
        """Starts fping along with a2dp streaming.

        This test is to start fping with traffic between host machine and
        android device and test the functional behaviour of a2dp streaming when
        screen turned on or off.

        Steps:
        1. Start fping.
        2. Start media play on android device and check for music streaming.
        3. Start screen on/off of android device multiple times.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_080
        """
        tasks = [(start_fping, (self.pri_ad, self.iperf["duration"])),
                 (music_play_and_check,
                  (self.pri_ad, self.audio_receiver.mac_address,
                   self.music_file_to_play, self.iperf["duration"])),
                 (toggle_screen_state, (self.pri_ad, self.iterations))]
        if not multithread_func(self.log, tasks):
            return False
        return self.teardown_thread()

    def test_a2dp_streaming_ble_connection_with_tcp_ul(self):
        """Starts TCP-uplink traffic with a2dp streaming and ble connection.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the functional behaviour of  ble connection
        and a2dp streaming.

        Steps:
        1. Start TCP-uplink traffic.
        2. Start media play on android device and check for music streaming.
        3. Initiate ble connection to android device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_084
        """
        if not self.music_streaming_ble_connection_with_iperf():
            return False
        return True

    def test_a2dp_streaming_ble_connection_with_tcp_dl(self):
        """Starts TCP-downlink traffic with a2dp streaming and ble connection.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the functional behaviour of  ble connection
        and a2dp streaming.

        Steps:
        1. Start TCP-downlink traffic.
        2. Start media play on android device and check for music streaming.
        3. Initiate ble connection to android device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_085
        """
        if not self.music_streaming_ble_connection_with_iperf():
            return False
        return True
