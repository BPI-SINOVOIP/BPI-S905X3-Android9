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

from acts.test_utils.bt.bt_test_utils import clear_bonded_devices
from acts.test_utils.coex.bluez_test_utils import BluezUtils
from acts.test_utils.coex.CoexBaseTest import CoexBaseTest
from acts.test_utils.coex.coex_constants import bluetooth_profiles
from acts.test_utils.coex.coex_constants import WAIT_TIME
from acts.test_utils.coex.coex_test_utils import music_play_and_check
from acts.test_utils.coex.coex_test_utils import connect_wlan_profile


class WlanWithA2dpFunctionalitySlaveTest(CoexBaseTest):

    def __init__(self, controllers):
        CoexBaseTest.__init__(self, controllers)

    def setup_class(self):
        CoexBaseTest.setup_class(self)
        req_params = ["iterations"]
        self.unpack_userparams(req_params)
        self.device_id = str(self.pri_ad.droid.bluetoothGetLocalAddress())
        self.dbus = BluezUtils()
        self.adapter_mac_address = self.dbus.get_bluetooth_adapter_address()

    def setup_test(self):
        CoexBaseTest.setup_test(self)
        self.pri_ad.droid.bluetoothMakeDiscoverable()
        if not self.dbus.find_device(self.device_id):
            self.log.info("Device is not discoverable")
            return False
        self.pri_ad.droid.bluetoothStartPairingHelper(True)
        if not self.dbus.pair_bluetooth_device():
            self.log.info("Pairing failed")
            return False
        if not self.dbus.connect_bluetooth_device(
                bluetooth_profiles["A2DP_SRC"]):
            self.log.info("Connection Failed")
            return False

    def teardown_test(self):
        clear_bonded_devices(self.pri_ad)
        CoexBaseTest.teardown_test(self)
        self.dbus.remove_bluetooth_device(self.device_id)

    def connect_disconnect_a2dp_headset(self):
        """Connect and disconnect a2dp profile from headset."""
        for i in range(self.iterations):
            if not self.dbus.disconnect_bluetooth_profile(
                    bluetooth_profiles["A2DP_SRC"], self.pri_ad):
                self.log.info("Disconnection Failed")
                return False
            time.sleep(WAIT_TIME)
            if not self.dbus.connect_bluetooth_device(
                    bluetooth_profiles["A2DP_SRC"]):
                self.log.info("Connection Failed")
                return False
        return True

    def connect_disconnect_a2dp_headset_with_iperf(self):
        """Wrapper function to start iperf traffic and connect/disconnect
        to headset for N iterations.
        """
        self.run_iperf_and_get_result()
        if not self.connect_disconnect_a2dp_headset():
            return False
        return self.teardown_result()

    def music_streaming_with_iperf(self):
        """Wrapper function to start iperf traffic, music streaming
        to headset and associate with access point for N iterations.
        """
        args = [
            lambda: music_play_and_check(
                self.pri_ad, self.audio_receiver.mac_address,
                self.music_file_to_play, self.iperf["duration"])
        ]
        self.run_thread(args)
        if not connect_wlan_profile(self.pri_ad, self.network):
            return False
        self.run_iperf_and_get_result()
        return self.teardown_result()

    def test_connect_disconnect_a2dp_headset_slave_role_with_tcp_ul(self):
        """Starts TCP-uplink traffic and connect/disconnect a2dp headset.

        This test is to start TCP-uplink traffic between host machine and
        android device and test functional behaviour of connection and
        disconnection to a2dp headset when android device as a slave.

        Steps:
        1. Run TCP-uplink traffic.
        2. Initiate connection from a2dp headset(bluez).
        2. Connect and disconnect A2DP headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_034
        """
        if not self.connect_disconnect_a2dp_headset_with_iperf():
            return False
        return True

    def test_connect_disconnect_a2dp_headset_slave_role_with_tcp_dl(self):
        """Starts TCP-downlink traffic and connect/disconnect a2dp headset.

        This test is to start TCP-downlink traffic between host machine and
        android device and test functional behaviour of connection and
        disconnection to a2dp headset when android device as a slave.

        Steps:
        1. Run TCP-downlink traffic.
        2. Initiate connection from a2dp headset(bluez).
        2. Connect and disconnect a2dp headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_035
        """
        if not self.connect_disconnect_a2dp_headset_with_iperf():
            return False
        return True

    def test_connect_disconnect_a2dp_headset_slave_role_with_udp_ul(self):
        """Starts UDP-uplink traffic and connect/disconnect a2dp headset.

        This test is to start UDP-uplink traffic between host machine and
        android device and test functional behaviour of connection and
        disconnection to a2dp headset when android device as a slave.

        Steps:
        1. Run UDP-uplink traffic.
        2. Initiate connection from a2dp headset(bluez).
        2. Connect and disconnect a2dp headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_036
        """
        if not self.connect_disconnect_a2dp_headset_with_iperf():
            return False
        return True

    def test_connect_disconnect_a2dp_headset_slave_role_with_udp_dl(self):
        """Starts UDP-downlink traffic and connect/disconnect a2dp headset.

        This test is to start UDP-downlink traffic between host machine and
        android device and test functional behaviour of connection and
        disconnection to a2dp headset when android device as a slave.

        Steps:
        1. Run UDP-downlink traffic.
        2. Initiate connection from a2dp headset(bluez).
        2. Connect and disconnect a2dp headset.
        3. Repeat step 2 for N iterations.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_037
        """
        if not self.connect_disconnect_a2dp_headset_with_iperf():
            return False
        return True

    def test_a2dp_streaming_slave_role_with_tcp_ul(self):
        """Starts TCP-uplink traffic with music streaming to a2dp headset.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming when device acts as a slave.

        Steps:
        1. Run TCP-uplink traffic.
        2. Start media streaming to a2dp headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_038
        """
        if not self.music_streaming_with_iperf():
            return False
        return True

    def test_a2dp_streaming_slave_role_with_tcp_dl(self):
        """Starts TCP-downlink traffic with music streaming to a2dp headset.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming when device acts as a slave.

        Steps:
        1. Run TCP-downlink traffic.
        2. Start media streaming to a2dp headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_039
        """
        if not self.music_streaming_with_iperf():
            return False
        return True

    def test_a2dp_streaming_slave_role_with_udp_ul(self):
        """Starts UDP-uplink traffic with music streaming to a2dp headset.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming when device acts as a slave.

        Steps:
        1. Run UDP-uplink traffic.
        2. Start media streaming to a2dp headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_040
        """
        if not self.music_streaming_with_iperf():
            return False
        return True

    def test_a2dp_streaming_slave_role_with_udp_dl(self):
        """Starts UDP-downlink traffic with music streaming to a2dp headset.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the functional behaviour of a2dp music
        streaming when device acts as a slave.

        Steps:
        1. Run UDP-downlink traffic.
        2. Start media streaming to a2dp headset.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_041
        """
        if not self.music_streaming_with_iperf():
            return False
        return True
