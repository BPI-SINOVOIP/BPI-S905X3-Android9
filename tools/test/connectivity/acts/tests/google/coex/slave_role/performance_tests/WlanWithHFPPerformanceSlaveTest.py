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
from acts.test_utils.coex.coex_constants import CALL_WAIT_TIME
from acts.test_utils.coex.coex_test_utils import multithread_func
from acts.test_utils.coex.coex_test_utils import setup_tel_config
from acts.test_utils.tel.tel_test_utils import initiate_call


class WlanWithHFPPerformanceSlaveTest(CoexBaseTest):

    def __init__(self, controllers):
        CoexBaseTest.__init__(self, controllers)

    def setup_class(self):
        CoexBaseTest.setup_class(self)
        req_params = ["sim_conf_file"]
        self.unpack_userparams(req_params)
        self.ag_phone_number, self.re_phone_number = setup_tel_config(
            self.pri_ad, self.sec_ad, self.sim_conf_file)
        self.device_id = str(self.pri_ad.droid.bluetoothGetLocalAddress())
        self.dbus = BluezUtils()

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
                bluetooth_profiles["HFP_AG"], bluetooth_profiles["A2DP_SRC"]):
            self.log.info("Connection failed")
            return False

    def teardown_test(self):
        clear_bonded_devices(self.pri_ad)
        CoexBaseTest.teardown_test(self)
        self.dbus.remove_bluetooth_device(self.device_id)

    def initiate_call_from_hf_with_iperf(self):
        """Wrapper function to start iperf and initiate call"""
        self.run_iperf_and_get_result()
        if not self.dbus.initiate_and_disconnect_call_from_hf(
                self.re_phone_number, self.iperf["duration"]):
            return False
        return self.teardown_result()

    def initiate_call_avrcp_controls_with_iperf(self):
        """Wrapper function to start iperf, initiate call from hf and answer
        call from secondary device.
        """
        initiate_call(self.log, self.sec_ad, self.ag_phone_number)
        time.sleep(CALL_WAIT_TIME)
        self.run_iperf_and_get_result()
        tasks = [(self.dbus.answer_call, (self.iperf["duration"],)),
                 (self.dbus.avrcp_actions, (self.device_id,))]
        if not multithread_func(self.log, tasks):
            return False
        return self.teardown_result()

    def test_performance_hfp_call_slave_role_tcp_ul(self):
        """Starts TCP-uplink traffic with hfp connection.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the performance hfp connection
        call and wifi throughput when device is in slave role.

        Steps:.
        1. Start TCP-uplink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_053
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_performance_hfp_call_slave_role_tcp_dl(self):
        """Starts TCP-downlink traffic with hfp connection.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the performance hfp connection
        call and wifi throughput when device is in slave role.

        Steps:.
        1. Start TCP-downlink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_054
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_performance_hfp_call_slave_role_udp_ul(self):
        """Starts UDP-uplink traffic with hfp connection.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the performance hfp connection
        call and wifi throughput when device is in slave role.

        Steps:.
        1. Start UDP-uplink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_055
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_performance_hfp_call_slave_role_udp_dl(self):
        """Starts UDP-downlink traffic with hfp connection.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the performance hfp connection
        call and wifi throughput when device is in slave role.

        Steps:
        1. Start UDP-downlink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_056
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_performance_hfp_call_avrcp_controls_slave_role_tcp_ul(self):
        """Starts TCP-uplink traffic with hfp connection and check volume.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the performance hfp connection,
        call, volume change and wifi throughput when device is in slave role.

        Steps:.
        1. Start TCP-uplink traffic.
        2. Initiate call from HF and disconnect call from primary device.
        3. Change call volume when device is in call.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_057
        """
        if not self.initiate_call_avrcp_controls_with_iperf():
            return False
        return True

    def test_performance_hfp_call_avrcp_controls_slave_role_tcp_dl(self):
        """Starts TCP-downlink traffic with hfp connection and check volume.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the performance hfp connection,
        call, volume change and wifi throughput when device is in slave role.

        Steps:.
        1. Start TCP-downlink traffic.
        2. Initiate call from HF and disconnect call from primary device.
        3. Change call volume when device is in call.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_058
        """
        if not self.initiate_call_avrcp_controls_with_iperf():
            return False
        return True

    def test_performance_hfp_call_avrcp_controls_slave_role_udp_ul(self):
        """Starts UDP-uplink traffic with hfp connection and check volume.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the performance hfp connection,
        call, volume change and wifi throughput when device is in slave role.

        Steps:.
        1. Start UDP-uplink traffic.
        2. Initiate call from HF and disconnect call from primary device.
        3. Change call volume when device is in call.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_059
        """
        if not self.initiate_call_avrcp_controls_with_iperf():
            return False
        return True

    def test_performance_hfp_call_avrcp_controls_slave_role_udp_dl(self):
        """Starts UDP-downlink traffic with hfp connection and check volume.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the performance hfp connection,
        call, volume change and wifi throughput when device is in slave role.

        Steps:.
        1. Start UDP-downlink traffic.
        2. Initiate call from HF and disconnect call from primary device.
        3. Change call volume when device is in call.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_060
        """
        if not self.initiate_call_avrcp_controls_with_iperf():
            return False
        return True
