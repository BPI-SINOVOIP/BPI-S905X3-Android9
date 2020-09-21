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
from acts.test_utils.coex.coex_test_utils import multithread_func
from acts.test_utils.coex.coex_test_utils import initiate_disconnect_from_hf
from acts.test_utils.coex.coex_test_utils import pair_and_connect_headset
from acts.test_utils.coex.coex_test_utils import setup_tel_config
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import initiate_call


class WlanWithHfpPerformanceTest(CoexBaseTest):

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
            self.log.error("Failed to pair and connect to headset")
            return False

    def teardown_test(self):
        clear_bonded_devices(self.pri_ad)
        CoexBaseTest.teardown_test(self)
        self.audio_receiver.clean_up()

    def call_from_sec_ad_to_pri_ad(self):
        """Initiates the call from secondary device and accepts the call
        from HF connected to primary device.

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
        time.sleep(5)  # Wait until initiate call.
        if not self.audio_receiver.accept_call():
            self.log.error("Failed to answer call from HF.")
            return False
        time.sleep(self.iperf["duration"])
        if not hangup_call(self.log, self.pri_ad):
            self.log.error("Failed to hangup call.")
            return False
        return False

    def initiate_call_from_hf_with_iperf(self):
        """Wrapper function to start iperf and initiate call."""
        self.run_iperf_and_get_result()
        if not initiate_disconnect_from_hf(
                self.audio_receiver, self.pri_ad, self.sec_ad,
                self.iperf["duration"]):
            return False
        return self.teardown_result()

    def initiate_call_and_change_volume_with_iperf(self):
        """Wrapper function to start iperf and initiate call and check avrcp
        controls.
        """
        self.run_iperf_and_get_result()
        tasks = [(self.call_from_sec_ad_to_pri_ad, ()),
                 (self.change_volume, ())]
        if not multithread_func(self.log, tasks):
            return False
        return self.teardown_result()

    def test_performance_hfp_call_tcp_ul(self):
        """Test performance with hfp connection.

        This test is to start TCP-uplink traffic between host machine and
        android device and check throughput when hfp connection
        and call is active.

        Steps:.
        1. Start TCP-uplink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Kpi_017
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_performance_hfp_call_tcp_dl(self):
        """Test performance with hfp connection.

        This test is to start TCP-downlink traffic between host machine and
        android device and check throughput when hfp connection
        and call is active.

        Steps:.
        1. Start TCP-downlink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Kpi_018
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_performance_hfp_call_udp_ul(self):
        """Test performance with hfp connection.

        This test is to start UDP-uplink traffic between host machine and
        android device and check throughput when hfp connection
        and call is active.

        Steps:.
        1. Start UDP-uplink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Kpi_019
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_performance_hfp_call_udp_dl(self):
        """Test performance with hfp connection.

        This test is to start UDP-downlink traffic between host machine and
        android device and check throughput when hfp connection
        and call is active.

        Steps:.
        1. Start UDP-downlink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Kpi_020
        """
        if not self.initiate_call_from_hf_with_iperf():
            return False
        return True

    def test_performance_hfp_call_volume_check_tcp_ul(self):
        """Test performance with hfp connection and perform volume actions.

        This test is to start TCP-uplink traffic between host machine and
        android device and check throughput when hfp connection and perform
        volume change when call is active.

        Steps:.
        1. Start TCP-uplink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Kpi_037
        """
        if not self.initiate_call_and_change_volume_with_iperf():
            return False
        return True

    def test_performance_hfp_call_volume_check_tcp_dl(self):
        """Test performance with hfp connection and perform volume actions.

        This test is to start TCP-downlink traffic between host machine and
        android device and check throughput when hfp connection and perform
        volume change when call is active.

        Steps:.
        1. Start TCP-downlink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Kpi_038
        """
        if not self.initiate_call_and_change_volume_with_iperf():
            return False
        return True

    def test_performance_hfp_call_volume_check_udp_ul(self):
        """Test performance with hfp connection and perform volume actions.

        This test is to start UDP-uplink traffic between host machine and
        android device and check throughput when hfp connection and perform
        volume change when call is active.

        Steps:.
        1. Start UDP-uplink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Kpi_039
        """
        if not self.initiate_call_and_change_volume_with_iperf():
            return False
        return True

    def test_performance_hfp_call_volume_check_udp_dl(self):
        """Test performance with hfp connection and perform volume actions.

        This test is to start UDP-downlink traffic between host machine and
        android device and check throughput when hfp connection and perform
        volume change when call is active.

        Steps:.
        1. Start UDP-downlink traffic.
        2. Initiate call from HF and disconnect call from primary device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Kpi_040
        """
        if not self.initiate_call_and_change_volume_with_iperf():
            return False
        return True
