# /usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
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
"""
Automated tests for the testing Connectivity of Avrcp/A2dp profile.
"""

import time

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt import bt_test_utils
from acts.test_utils.car import car_bt_utils
from acts.test_utils.bt import BtEnum
from acts.test_utils.bt.bt_test_utils import is_a2dp_connected


class BtCarMediaConnectionTest(BluetoothBaseTest):
    def setup_class(self):
        # AVRCP roles
        self.CT = self.android_devices[0]
        self.TG = self.android_devices[1]
        # A2DP roles for the same devices
        self.SNK = self.CT
        self.SRC = self.TG

        # Setup devices
        bt_test_utils.setup_multiple_devices_for_bt_test([self.CT, self.TG])

        self.btAddrCT = self.CT.droid.bluetoothGetLocalAddress()
        self.btAddrTG = self.TG.droid.bluetoothGetLocalAddress()

        # Additional time from the stack reset in setup.
        time.sleep(4)
        # Pair the devices.
        if not bt_test_utils.pair_pri_to_sec(
                self.CT, self.TG, attempts=4, auto_confirm=False):
            self.log.error("Failed to pair")
            return False

        # Disable all
        car_bt_utils.set_car_profile_priorities_off(self.SNK, self.SRC)

        # Enable A2DP
        bt_test_utils.set_profile_priority(
            self.SNK, self.SRC, [BtEnum.BluetoothProfile.A2DP_SINK],
            BtEnum.BluetoothPriorityLevel.PRIORITY_ON)

    @test_tracker_info(uuid='1934c0d5-3fa3-43e5-a91f-2c8a4424f5cd')
    @BluetoothBaseTest.bt_test_wrap
    def test_a2dp_connect_disconnect_from_src(self):
        """
        Test Connect/Disconnect on A2DP profile.

        Pre-Condition:
        1. Devices previously bonded and NOT connected on A2dp

        Steps:
        1. Initiate a connection on A2DP profile from SRC
        2. Check if they connected.
        3. Initiate a disconnect on A2DP profile from SRC
        4. Ensure they disconnected on A2dp alone

        Returns:
        True    if we connected/disconnected successfully
        False   if we did not connect/disconnect successfully

        Priority: 0
        """
        if (is_a2dp_connected(self.SNK, self.SRC)):
            self.log.info("Already Connected")
        else:
            if (not bt_test_utils.connect_pri_to_sec(
                    self.SRC, self.SNK,
                    set([BtEnum.BluetoothProfile.A2DP.value]))):
                return False
        # Delay to establish A2DP connection before disconnecting
        time.sleep(5)
        result = bt_test_utils.disconnect_pri_from_sec(
            self.SRC, self.SNK, [BtEnum.BluetoothProfile.A2DP.value])
        # Grace timeout to allow a2dp time to disconnect
        time.sleep(2)
        if not result:
            # Additional profile connection check for b/
            if bt_test_utils.is_a2dp_src_device_connected(
                    self.SRC, self.SNK.droid.bluetoothGetLocalAddress()):
                self.SRC.log.error("Failed to disconnect on A2dp")
                return False
        # Logging if we connected right back, since that happens sometimes
        # Not failing the test if it did though
        if (is_a2dp_connected(self.SNK, self.SRC)):
            self.SNK.log.error("Still connected after a disconnect")

        return True

    @test_tracker_info(uuid='70d30007-540a-4e86-bd75-ab218774350e')
    @BluetoothBaseTest.bt_test_wrap
    def test_a2dp_connect_disconnect_from_snk(self):
        """
        Test Connect/Disconnect on A2DP Sink profile.

        Pre-Condition:
        1. Devices previously bonded and NOT connected on A2dp

        Steps:
        1. Initiate a connection on A2DP Sink profile from SNK
        2. Check if they connected.
        3. Initiate a disconnect on A2DP Sink profile from SNK
        4. Ensure they disconnected on A2dp alone

        Returns:
        True    if we connected/disconnected successfully
        False   if we did not connect/disconnect successfully

        Priority: 0
        """
        # Connect
        if is_a2dp_connected(self.SNK, self.SRC):
            self.log.info("Already Connected")
        else:
            if (not bt_test_utils.connect_pri_to_sec(
                    self.SNK, self.SRC,
                    set([BtEnum.BluetoothProfile.A2DP_SINK.value]))):
                return False
        # Delay to establish A2DP connection before disconnecting
        time.sleep(5)
        # Disconnect
        result = bt_test_utils.disconnect_pri_from_sec(
            self.SNK, self.SRC, [BtEnum.BluetoothProfile.A2DP_SINK.value])
        # Grace timeout to allow a2dp time to disconnect
        time.sleep(2)
        if not result:
            # Additional profile connection check for b/
            if bt_test_utils.is_a2dp_snk_device_connected(
                    self.SNK, self.SRC.droid.bluetoothGetLocalAddress()):
                self.SNK.log.error("Failed to disconnect on A2dp Sink")
                return False
        # Logging if we connected right back, since that happens sometimes
        # Not failing the test if it did though
        if is_a2dp_connected(self.SNK, self.SRC):
            self.SNK.log.error("Still connected after a disconnect")
        return True
