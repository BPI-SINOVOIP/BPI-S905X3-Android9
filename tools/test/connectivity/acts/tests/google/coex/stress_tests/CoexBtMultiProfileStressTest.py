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
from acts.test_utils.coex.coex_test_utils import disconnect_headset_from_dev
from acts.test_utils.coex.coex_test_utils import pair_and_connect_headset


class CoexBtMultiProfileStressTest(CoexBaseTest):

    def __init__(self, controllers):
        CoexBaseTest.__init__(self, controllers)

    def setup_class(self):
        CoexBaseTest.setup_class(self)
        self.receiver = self.relay_devices[1]
        req_params = ["iterations"]
        self.unpack_userparams(req_params)

    def setup_test(self):
        CoexBaseTest.setup_test(self)
        self.audio_receiver.pairing_mode()
        self.receiver.setup()
        self.receiver.power_on()
        self.receiver.pairing_mode()

    def teardown_test(self):
        clear_bonded_devices(self.pri_ad)
        CoexBaseTest.teardown_test(self)
        self.audio_receiver.clean_up()
        self.receiver.clean_up()

    def initiate_classic_connection_to_multiple_devices(self):
        """Initiates multiple BR/EDR connections.

        Steps:
        1. Initiate A2DP Connection.
        2. Initiate HFP Connection.
        3. Disconnect A2DP Connection.
        4. Disconnect HFP Connection.
        5. Repeat step 1 to 4.

        Returns:
            True if successful, False otherwise.
        """
        for i in range(self.iterations):
            if not pair_and_connect_headset(
                    self.pri_ad, self.receiver.mac_address,
                    {BtEnum.BluetoothProfile.A2DP.value}):
                self.log.error("Failed to connect A2DP Profile.")
                return False
            time.sleep(2)

            if not pair_and_connect_headset(
                    self.pri_ad, self.audio_receiver.mac_address,
                    {BtEnum.BluetoothProfile.HEADSET.value}):
                self.log.error("Failed to connect HEADSET profile.")
                return False
            time.sleep(2)

            if not disconnect_headset_from_dev(
                    self.pri_ad, self.receiver.mac_address,
                    [BtEnum.BluetoothProfile.A2DP.value]):
                self.log.error("Could not disconnect {}".format(
                    self.receiver.mac_address))
                return False

            if not disconnect_headset_from_dev(
                    self.pri_ad, self.audio_receiver.mac_address,
                    [BtEnum.BluetoothProfile.HEADSET.value]):
                self.log.error("Could not disconnect {}".format(
                    self.audio_receiver.mac_address))
                return False
        return True

    def initiate_classic_connection_with_iperf(self):
        """Wrapper function to initiate bluetooth classic connection to
        multiple devices.
        """
        self.run_iperf_and_get_result()
        if not self.initiate_classic_connection_to_multiple_devices():
            return False
        return self.teardown_result()

    def test_stress_multiple_connection_with_tcp_ul(self):
        """ Connects multiple headsets with wlan traffic over TCP-uplink.

        This test is to perform connect and disconnect with A2DP and HFP
        profiles on two different bluetooth devices.

        Steps:
        1. Run wlan traffic over TCP-uplink.
        2. Initiate connect and disconnect to multiple profiles from primary
        device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_037
        """
        if not self.initiate_classic_connection_with_iperf():
            return False
        return True

    def test_stress_multiple_connection_with_tcp_dl(self):
        """ Connects multiple headsets with wlan traffic over TCP-downlink.

        This test is to perform connect and disconnect with A2DP and HFP
        profiles on two different bluetooth devices.

        Steps:
        1. Run wlan traffic over TCP-downlink.
        2. Initiate connect and disconnect to multiple profiles from primary
        device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_038
        """
        if not self.initiate_classic_connection_with_iperf():
            return False
        return True

    def test_stress_multiple_connection_with_udp_ul(self):
        """ Connects multiple headsets with wlan traffic over UDP-uplink.

        This test is to perform connect and disconnect with A2DP and HFP
        profiles on two different bluetooth devices.

        Steps:
        1. Run wlan traffic over UDP-uplink.
        2. Initiate connect and disconnect to multiple profiles from primary
        device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_039
        """
        if not self.initiate_classic_connection_with_iperf():
            return False
        return True

    def test_stress_multiple_connection_with_udp_dl(self):
        """ Connects multiple headsets with wlan traffic over UDP-downlink.

        This test is to perform connect and disconnect with A2DP and HFP
        profiles.

        Steps:
        1. Run wlan traffic over UDP-downlink.
        2. Initiate connect and disconnect to multiple profiles from primary
        device.

        Returns:
            True if successful, False otherwise.

        Test Id: Bt_CoEx_Stress_040
        """
        if not self.initiate_classic_connection_with_iperf():
            return False
        return True
