#/usr/bin/env python3.4
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
Test script to test connect and disconnect sequence between two devices which can run
SL4A. The script does the following:
  Setup:
    Clear up the bonded devices on both bluetooth adapters and bond the DUTs to each other.
  Test (NUM_TEST_RUNS times):
    1. Connect A2dpSink and HeadsetClient
      1.1. Check that devices are connected.
    2. Disconnect A2dpSink and HeadsetClient
      2.1 Check that devices are disconnected.
"""

import time

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.base_test import BaseTestClass
from acts.test_utils.bt import bt_test_utils
from acts.test_utils.bt import BtEnum
from acts import asserts


class BtCarPairedConnectDisconnectTest(BluetoothBaseTest):
    def setup_class(self):
        self.car = self.android_devices[0]
        self.ph = self.android_devices[1]
        self.car_bt_addr = self.car.droid.bluetoothGetLocalAddress()
        self.ph_bt_addr = self.ph.droid.bluetoothGetLocalAddress()

        bt_test_utils.setup_multiple_devices_for_bt_test([self.car, self.ph])

        # Pair the devices.
        # This call may block until some specified timeout in bt_test_utils.py.
        result = bt_test_utils.pair_pri_to_sec(
            self.car, self.ph, auto_confirm=False)

        asserts.assert_true(result, "pair_pri_to_sec returned false.")

        # Check for successful setup of test.
        devices = self.car.droid.bluetoothGetBondedDevices()
        asserts.assert_equal(
            len(devices), 1,
            "pair_pri_to_sec succeeded but no bonded devices.")

    @test_tracker_info(uuid='b0babf3b-8049-4b64-9125-408efb1bbcd2')
    @BluetoothBaseTest.bt_test_wrap
    def test_pairing(self):
        """
        Tests if we can connect two devices over A2dp and then disconnect

        Precondition:
        1. Devices are paired.

        Steps:
        1. Set the priority to OFF for all profiles.
        2. Initiate connection over A2dp Sink client profile.

        Returns:
          Pass if True
          Fail if False

        """
        # Set the priority to OFF for all profiles.
        self.car.droid.bluetoothHfpClientSetPriority(
            self.ph.droid.bluetoothGetLocalAddress(),
            BtEnum.BluetoothPriorityLevel.PRIORITY_OFF.value)
        self.ph.droid.bluetoothHspSetPriority(
            self.car.droid.bluetoothGetLocalAddress(),
            BtEnum.BluetoothPriorityLevel.PRIORITY_OFF.value)
        addr = self.ph.droid.bluetoothGetLocalAddress()
        if not bt_test_utils.connect_pri_to_sec(
                self.car, self.ph,
                set([BtEnum.BluetoothProfile.A2DP_SINK.value])):
            if not bt_test_utils.is_a2dp_snk_device_connected(self.car, addr):
                return False
        return True

    @test_tracker_info(uuid='a44f13e2-c012-4292-8dd5-9f32a023e297')
    @BluetoothBaseTest.bt_test_wrap
    def test_connect_disconnect_paired(self):
        """
        Tests if we can connect two devices over Headset, A2dp and then disconnect them with success

        Precondition:
        1. Devices are paired.

        Steps:
        1. Initiate connection over A2dp Sink and Headset client profiles.
        2. Check if the connection succeeded.

        Returns:
          Pass if True
          Fail if False

        Priority: 0
        """

        NUM_TEST_RUNS = 2
        failure = 0
        addr = self.ph.droid.bluetoothGetLocalAddress()
        for i in range(NUM_TEST_RUNS):
            self.log.info("Running test [" + str(i) + "/" + str(NUM_TEST_RUNS)
                          + "]")
            success = bt_test_utils.connect_pri_to_sec(
                self.car, self.ph,
                set([
                    BtEnum.BluetoothProfile.HEADSET_CLIENT.value,
                    BtEnum.BluetoothProfile.A2DP_SINK.value
                ]))

            # Check if we got connected.
            if not success:
                self.car.log.info("Not all profiles connected.")
                if (bt_test_utils.is_hfp_client_device_connected(self.car,
                                                                 addr) and
                        bt_test_utils.is_a2dp_snk_device_connected(self.car,
                                                                   addr)):
                    self.car.log.info(
                        "HFP Client or A2DP SRC connected successfully.")
                else:
                    failure = failure + 1
                continue

            # Disconnect the devices.
            success = bt_test_utils.disconnect_pri_from_sec(
                self.car, self.ph, [
                    BtEnum.BluetoothProfile.HEADSET_CLIENT.value,
                    BtEnum.BluetoothProfile.A2DP_SINK.value
                ])

            if success is False:
                self.car.log.info("Disconnect failed.")
                if (bt_test_utils.is_hfp_client_device_connected(self.car,
                                                                 addr) or
                        bt_test_utils.is_a2dp_snk_device_connected(self.car,
                                                                   addr)):
                    self.car.log.info(
                        "HFP Client or A2DP SRC failed to disconnect.")
                    failure = failure + 1
                continue

        self.log.info("Failure {} total tests {}".format(failure,
                                                         NUM_TEST_RUNS))
        if failure > 0:
            return False
        return True
