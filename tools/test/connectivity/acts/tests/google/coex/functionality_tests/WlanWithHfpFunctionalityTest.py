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
from acts.test_utils.coex.coex_test_utils import connect_dev_to_headset
from acts.test_utils.coex.coex_test_utils import initiate_disconnect_from_hf
from acts.test_utils.coex.coex_test_utils import initiate_disconnect_call_dut
from acts.test_utils.coex.coex_test_utils import multithread_func
from acts.test_utils.coex.coex_test_utils import pair_and_connect_headset
from acts.test_utils.coex.coex_test_utils import perform_classic_discovery
from acts.test_utils.coex.coex_test_utils import connect_wlan_profile
from acts.test_utils.coex.coex_test_utils import toggle_screen_state
from acts.test_utils.coex.coex_test_utils import setup_tel_config
from acts.test_utils.coex.coex_test_utils import start_fping
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import initiate_call

BLUETOOTH_WAIT_TIME = 2


class WlanWithHfpFunctionalityTest(CoexBaseTest):

    def __init__(self, controllers):
        CoexBaseTest.__init__(self, controllers)

    def setup_class(self):
        CoexBaseTest.setup_class(self)
        req_params = ["sim_conf_file"]
        self.unpack_userparams(req_params)
        self.ag_phone_number, self.re_phone_number = setup_tel_config(
            self.pri_ad, self.sec_ad, self.sim_conf_file)

    def setup_test(self):
        CoexBaseTest.setup_test(self)
        self.audio_receiver.pairing_mode()
        if not pair_and_connect_headset(
                self.pri_ad, self.audio_receiver.mac_address,
                set([BtEnum.BluetoothProfile.HEADSET.value])):
            self.log.error("Failed to pair and connect to headset.")
            return False

    def teardown_test(self):
        clear_bonded_devices(self.pri_ad)
        CoexBaseTest.teardown_test(self)
        self.audio_receiver.clean_up()

    def call_from_sec_ad_to_pri_ad(self):
        """Initiates the call from secondary device and accepts the call
        from HF.

        Steps:
        1. Initiate call from secondary device to primary device.
        2. Accept the call from HF.
        3. Hangup the call from primary device.

        Returns:
            True if successful, False otherwise.
        """
        if not initiate_call(self.log, self.sec_ad, self.ag_phone_number):
            self.log.error("Failed to initiate call")
            return False
        if not self.audio_receiver.accept_call():
            self.log.error("Failed to answer call from HF.")
            return False
        if not hangup_call(self.log, self.pri_ad):
            self.log.error("Failed to hangup call.")
            return False
        return False

    def connect_to_headset_when_turned_off_with_iperf(self):
        """Wrapper function to start iperf and test connection to headset
        when it is turned off.

        Returns:
            True if successful, False otherwise.
        """
        self.run_iperf_and_get_result()
        self.audio_receiver.clean_up()
        if not connect_dev_to_headset(
                self.pri_ad, self.audio_receiver.mac_address,
                set([BtEnum.BluetoothProfile.HEADSET.value])):
            self.log.error("Failed to connect to headset.")
            return True
        return False

    def check_headset_reconnection_with_iperf(self):
        """Wrapper function to start iperf and check behaviour of hfp
        reconnection."""
        self.run_iperf_and_get_result()
        self.audio_receiver.clean_up()
        self.audio_receiver.power_on()
        if not self.pri_ad.droid.bluetoothIsDeviceConnected(
                self.audio_receiver.mac_address):
            self.log.error("Device not found in connected list")
            return False
        return self.teardown_result()

    def initiate_call_from_hf_with_iperf(self):
        """Wrapper function to start iperf and initiate call"""
        self.run_iperf_and_get_result()
        if not initiate_disconnect_from_hf(
                self.audio_receiver, self.pri_ad, self.sec_ad,
                self.iperf["duration"]):
            return False
        return self.teardown_result()

    def initiate_call_from_hf_bt_discovery_with_iperf(self):
        """Wrapper function to start iperf, initiate call and perform classic
        discovery.
        """
        self.run_iperf_and_get_result()
        tasks = [(initiate_disconnect_from_hf, (
                self.audio_receiver, self.pri_ad, self.sec_ad,
                self.iperf["duration"])),
                 (perform_classic_discovery, (self.pri_ad,))]
        if not multithread_func(self.log, tasks):
            return False
        return self.teardown_result()

    def initiate_call_associate_ap_with_iperf(self):
        """Wrapper function to initiate call from primary device and associate
        with access point and start iperf traffic."""
        args = [
            lambda: initiate_disconnect_call_dut(
                self.pri_ad, self.sec_ad, self.iperf["duration"],
                self.re_phone_number)
        ]
        self.run_thread(args)
        if not connect_wlan_profile(self.pri_ad, self.network):
            return False
        self.run_iperf_and_get_result()
        return self.teardown_result()

    def test_hfp_call_with_tcp_ul(self):
        """Starts TCP-uplink traffic with hfp connection.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the functional behaviour of hfp connection
        and call.

        Steps:.
        1. Start TCP-uplink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_042
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_hfp_call_with_tcp_dl(self):
        """Starts TCP-downlink traffic with hfp connection.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the functional behaviour of hfp connection
        and call.

        Steps:.
        1. Start TCP-downlink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_043
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_hfp_call_with_udp_ul(self):
        """Starts UDP-uplink traffic with hfp connection.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the functional behaviour of hfp connection
        and call.

        Steps:.
        1. Start UDP-uplink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_044
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_hfp_call_with_udp_dl(self):
        """Starts UDP-downlink traffic with hfp connection.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the functional behaviour of hfp connection
        and call.

        Steps:.
        1. Start UDP-downlink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_045
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_hfp_call_bluetooth_discovery_with_tcp_ul(self):
        """Starts TCP-uplink traffic with hfp connection and bluetooth
        discovery.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the functional behaviour of hfp connection
        and call and bluetooth discovery.

        Steps:.
        1. Start TCP-uplink traffic.
        2. Initiate call from HF and disconnect call from primary device.
        3. Start bluetooth discovery.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_046
        """
        if not self.initiate_call_from_hf_bt_discovery_with_iperf():
            return False
        return True

    def test_hfp_call_bluetooth_discovery_with_tcp_dl(self):
        """Starts TCP-downlink traffic with hfp connection and bluetooth
        discovery.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the functional behaviour of hfp connection
        and call and bluetooth discovery.

        Steps:.
        1. Start TCP-downlink traffic.
        2. Initiate call from HF and disconnect call from primary device.
        3. Start bluetooth discovery.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_047
        """
        if not self.initiate_call_from_hf_bt_discovery_with_iperf():
            return False
        return True

    def test_hfp_call_bluetooth_discovery_with_udp_ul(self):
        """Starts UDP-uplink traffic with hfp connection and bluetooth
        discovery.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the functional behaviour of hfp connection
        and call and bluetooth discovery.

        Steps:.
        1. Start UDP-uplink traffic.
        2. Initiate call from HF and disconnect call from primary device.
        3. Start bluetooth discovery.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_048
        """
        if not self.initiate_call_from_hf_bt_discovery_with_iperf():
            return False
        return True

    def test_hfp_call_bluetooth_discovery_with_udp_dl(self):
        """Starts UDP-downlink traffic with hfp connection and bluetooth
        discovery.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the functional behaviour of hfp connection
        and call and bluetooth discovery.

        Steps:.
        1. Start UDP-downlink traffic.
        2. Initiate call from HF and disconnect call from primary device.
        3. Start bluetooth discovery.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_049
        """
        if not self.initiate_call_from_hf_bt_discovery_with_iperf():
            return False
        return True

    def test_hfp_call_and_associate_ap_with_tcp_ul(self):
        """Starts TCP-uplink traffic with hfp call.

        This test is to start TCP-uplink traffic between host machine and
        android device and test functional behaviour of hfp call connection
        while associating with AP.

        Steps:
        1. Initiate call from HF and disconnect call from primary device.
        2. Associate with AP.
        3. Start TCP-uplink traffic.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_050
        """
        if not self.initiate_call_associate_ap_with_iperf():
            return False
        return True

    def test_hfp_call_and_associate_ap_with_tcp_dl(self):
        """Starts TCP-downlink traffic with hfp call.

        This test is to start TCP-downlink traffic between host machine and
        android device and test functional behaviour of hfp call connection
        while associating with AP.

        Steps:
        1. Initiate call from HF and disconnect call from primary device.
        2. Associate with AP.
        3. Start TCP-downlink traffic.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_051
        """
        if not self.initiate_call_associate_ap_with_iperf():
            return False
        return True

    def test_hfp_call_and_associate_ap_with_udp_ul(self):
        """Starts UDP-uplink traffic with hfp call.

        This test is to start UDP-uplink traffic between host machine and
        android device and test functional behaviour of hfp call connection
        while associating with AP.

        Steps:
        1. Initiate call from HF and disconnect call from primary device.
        2. Associate with AP.
        3. Start UDP-uplink traffic.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_052
        """
        if not self.initiate_call_associate_ap_with_iperf():
            return False
        return True

    def test_hfp_call_and_associate_ap_with_udp_dl(self):
        """Starts UDP-downlink traffic with hfp call.

        This test is to start UDP-downlink traffic between host machine and
        android device and test functional behaviour of hfp call connection
        while associating with AP.

        Steps:
        1. Initiate call from HF and disconnect call from primary device.
        2. Associate with AP.
        3. Start UDP-downlink traffic.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_053
        """
        if not self.initiate_call_associate_ap_with_iperf():
            return False
        return True

    def test_hfp_redial_with_tcp_ul(self):
        """Starts TCP-uplink traffic with hfp connection.

        This test is to start TCP-uplink traffic between host machine and
        android device with hfp connection.

        Steps:
        1. Start TCP-uplink traffic.
        2. Initiate call from HF(last dialed number) and disconnect call
        from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_054
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_hfp_redial_with_tcp_dl(self):
        """Starts TCP-downlink traffic with hfp connection.

        This test is to start TCP-downlink traffic between host machine and
        android device with hfp connection.

        Steps:
        1. Start TCP-downlink traffic.
        2. Initiate call from HF(last dialed number) and disconnect call
        from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_055
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_hfp_redial_with_udp_ul(self):
        """Starts UDP-uplink traffic with hfp connection.

        This test is to start UDP-uplink traffic between host machine and
        android device with hfp connection.

        Steps:
        1. Start UDP-uplink traffic.
        2. Initiate call from HF(last dialed number) and disconnect call
        from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_056
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_hfp_redial_with_udp_dl(self):
        """Starts UDP-downlink traffic with hfp connection.

        This test is to start TCP-downlink traffic between host machine and
        android device with hfp connection.

        Steps:
        1. Start UDP-downlink traffic.
        2. Initiate call from HF(last dialed number) and disconnect call
        from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_057
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_hfp_reconnection_with_tcp_ul(self):
        """Starts TCP-uplink traffic with hfp reconnection.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the functional behaviour of hfp reconnection.

        Steps:.
        1. Start TCP-uplink traffic.
        2. Connect HF to DUT.
        3. Disconnect HF from DUT.
        4. Switch off the headset and turn ON HF to reconnect.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_062
        """
        if not self.check_headset_reconnection_with_iperf():
            return False
        return True

    def test_hfp_reconnection_with_tcp_dl(self):
        """Starts TCP-downlink traffic with hfp reconnection.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the functional behaviour of hfp reconnection.

        Steps:.
        1. Start TCP-downlink traffic.
        2. Connect HF to DUT.
        3. Disconnect HF from DUT.
        4. Switch off the headset and turn ON HF to reconnect.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_063
        """
        if not self.check_headset_reconnection_with_iperf():
            return False
        return True

    def test_hfp_reconnection_with_udp_ul(self):
        """Starts UDP-uplink traffic with hfp reconnection.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the functional behaviour of hfp reconnection.

        Steps:.
        1. Start UDP-uplink traffic.
        2. Connect HF to DUT.
        3. Disconnect HF from DUT.
        4. Switch off the headset and turn ON HF to reconnect.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_064
        """
        if not self.check_headset_reconnection_with_iperf():
            return False
        return True

    def test_hfp_reconnection_with_udp_dl(self):
        """Starts UDP-downlink traffic with hfp reconnection.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the functional behaviour of hfp reconnection.

        Steps:.
        1. Start UDP-downlink traffic.
        2. Connect HF to DUT.
        3. Disconnect HF from DUT.
        4. Switch off the headset and turn ON HF to reconnect.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_065
        """
        if not self.check_headset_reconnection_with_iperf():
            return False
        return True

    def test_hfp_connection_when_hf_turned_off_with_tcp_ul(self):
        """Starts TCP-uplink traffic with hfp connection.

        This test is to start TCP-Uplink traffic between host machine and
        android device and test the functional behaviour of hfp connection
        when device is off.

        Steps:
        1. Start TCP-uplink traffic.
        2. Make sure headset is turned off.
        3. Initiate hfp connection to headset from DUT.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_072
        """
        if not self.connect_to_headset_when_turned_off_with_iperf():
            return False
        return self.teardown_result()

    def test_hfp_connection_when_hf_turned_off_with_tcp_dl(self):
        """Starts TCP-downlink traffic with hfp connection.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the functional behaviour of hfp connection
        when device is off.

        Steps:
        1. Start TCP-downlink traffic.
        2. Make sure headset is turned off.
        3. Initiate hfp connection to headset from DUT.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_073
        """
        if not self.connect_to_headset_when_turned_off_with_iperf():
            return False
        return self.teardown_result()

    def test_hfp_connection_when_hf_turned_off_with_udp_ul(self):
        """Starts UDP-uplink traffic with hfp connection.

        This test is to start UDP-Uplink traffic between host machine and
        android device and test the functional behaviour of hfp connection
        when device is off.

        Steps:
        1. Start UDP-uplink traffic.
        2. Make sure headset is turned off.
        3. Initiate hfp connection to headset from DUT.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_074
        """
        if not self.connect_to_headset_when_turned_off_with_iperf():
            return False
        return self.teardown_result()

    def test_hfp_connection_when_hf_turned_off_with_udp_dl(self):
        """Starts UDP-downlink traffic with hfp connection.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the functional behaviour of hfp connection
        when device is off.

        Steps:
        1. Start UDP-downlink traffic.
        2. Make sure headset is turned off.
        3. Initiate hfp connection to headset from DUT.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_075
        """
        if not self.connect_to_headset_when_turned_off_with_iperf():
            return False
        return self.teardown_result()

    def test_hfp_call_with_fping(self):
        """Starts fping with hfp call connection.

        This test is to start fping between host machine and android device
        and test the functional behaviour of hfp call.

        Steps:
        1. Start fping from AP backend to android device.
        1. Initiate call from headset to secondary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_078
        """
        args = [lambda: start_fping(self.pri_ad, self.iperf["duration"])]
        self.run_thread(args)
        if not initiate_disconnect_from_hf(
                self.audio_receiver,self.pri_ad, self.sec_ad,
                self.iperf["duration"]):
            return False
        return self.teardown_thread()

    def test_hfp_call_toggle_screen_state_with_fping(self):
        """Starts fping with hfp call connection.

        This test is to start fping between host machine and android device
        and test the functional behaviour of hfp call when toggling the
        screen state.

        Steps:
        1. Start fping from AP backend.
        1. Initiate call from primary device headset to secondary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_081
        """
        tasks = [(start_fping, (self.pri_ad, self.iperf["duration"])),
                 (initiate_disconnect_from_hf, (
                     self.audio_receiver, self.pri_ad, self.sec_ad,
                     self.iperf["duration"])),
                 (toggle_screen_state, (self.pri_ad, self.iterations))]
        if not multithread_func(self.log, tasks):
            return False
        return True
