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
Test script to test the pairing scenarios and setting priorities.
"""

import time

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.base_test import BaseTestClass
from acts.test_utils.bt import bt_test_utils
from acts.test_utils.car import car_bt_utils
from acts.test_utils.bt import BtEnum

# Timed wait between Bonding happens and Android actually gets the list of
# supported services (and subsequently updates the priorities)
BOND_TO_SDP_WAIT = 3
UNBOND_TIMEOUT = 3


class BtCarPairingTest(BluetoothBaseTest):
    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.car = self.android_devices[0]
        self.ph = self.android_devices[1]

    def teardown_test(self):
        for ad in self.android_devices:
            bt_test_utils.clear_bonded_devices(ad)
        # Give the stack time to unbond.
        time.sleep(UNBOND_TIMEOUT)

    @test_tracker_info(uuid='f56e915-eef7-45cd-b5a6-771f6ef72602')
    @BluetoothBaseTest.bt_test_wrap
    def test_simple_pairing(self):
        """
        Tests if after first pairing the remote device has the default
        priorities for A2DP and HFP.

        Steps:
        1. Pair the devices (do not connect)
        2. Check the priorities.

        Returns:
          Pass if True
          Fail if False

        Priority: 0
        """
        # Pair the devices.
        if not bt_test_utils.pair_pri_to_sec(
                self.car, self.ph, attempts=1, auto_confirm=False):
            self.log.error("cannot pair")
            return False

        # Sleep because priorities are not event driven.
        time.sleep(BOND_TO_SDP_WAIT)

        # Check that the default priority for HFP and A2DP is ON.
        ph_hfp_p = self.car.droid.bluetoothHfpClientGetPriority(
            self.ph.droid.bluetoothGetLocalAddress())
        if ph_hfp_p != BtEnum.BluetoothPriorityLevel.PRIORITY_ON.value:
            self.log.error("hfp {} priority {} expected {}".format(
                self.ph.serial, ph_hfp_p,
                BtEnum.BluetoothPriorityLevel.PRIORITY_ON.value))
            return False

        ph_a2dp_p = self.car.droid.bluetoothA2dpSinkGetPriority(
            self.ph.droid.bluetoothGetLocalAddress())
        if ph_a2dp_p != BtEnum.BluetoothPriorityLevel.PRIORITY_ON.value:
            self.log.error("a2dp {} priority {} expected {}".format(
                self.ph.serial, ph_a2dp_p,
                BtEnum.BluetoothPriorityLevel.PRIORITY_ON.value))
            return False

        ph_pbap_p = self.car.droid.bluetoothPbapClientGetPriority(
            self.ph.droid.bluetoothGetLocalAddress())
        if ph_pbap_p != BtEnum.BluetoothPriorityLevel.PRIORITY_ON.value:
            self.log.error("pbap {} priority {} expected {}".format(
                self.ph.serial, ph_pbap_p,
                BtEnum.BluetoothPriorityLevel.PRIORITY_ON.value))
            return False
        return True

    @test_tracker_info(uuid='be4db211-10a0-479a-8958-dff0ccadca1a')
    @BluetoothBaseTest.bt_test_wrap
    def test_repairing(self):
        """
        Tests that even if we modify the priorities, on unpair and pair
        we will reset the priorities.

        Steps:
        1. Pair the devices (do not connect)
        2. Unset the priorities for HFP and A2DP
        3. Pair again
        4. Check the priorities, they should be set to default.

        Returns:
          Pass if True
          Fail if False

        Priority: 0
        """
        # Pair the devices.
        self.log.info("Pairing the devices ...")
        if not bt_test_utils.pair_pri_to_sec(
                self.car, self.ph, attempts=1, auto_confirm=False):
            self.log.error("Failed to pair devices.")
            return False

        # Timed wait for the profile priorities to propagate.
        time.sleep(BOND_TO_SDP_WAIT)

        # Set the priority to OFF for ALL car profiles.
        self.car.log.info("Set priorities off ...")
        car_bt_utils.set_car_profile_priorities_off(self.car, self.ph)

        # Now unpair the devices.
        self.log.info("Resetting the devices ...")
        for ad in self.android_devices:
            bt_test_utils.clear_bonded_devices(ad)
        # Give the stack time to unbond.
        time.sleep(UNBOND_TIMEOUT)

        # Pair them again!
        self.log.info("Pairing them again ...")
        if not bt_test_utils.pair_pri_to_sec(
                self.car, self.ph, attempts=1, auto_confirm=False):
            self.log.error("Faild to pair devices.")
            return False

        # Timed wait for the profile priorities to propagate.
        time.sleep(BOND_TO_SDP_WAIT)

        # Check the default priorities.
        ph_hfp_p = self.car.droid.bluetoothHfpClientGetPriority(
            self.ph.droid.bluetoothGetLocalAddress())
        if ph_hfp_p != BtEnum.BluetoothPriorityLevel.PRIORITY_ON.value:
            self.hf.log.error("HFP priority found: {}, expected: {}.".format(
                ph_hfp_p, BtEnum.BluetoothPriorityLevel.PRIORITY_ON.value))
            return False

        ph_a2dp_p = self.car.droid.bluetoothA2dpSinkGetPriority(
            self.ph.droid.bluetoothGetLocalAddress())
        if ph_a2dp_p != BtEnum.BluetoothPriorityLevel.PRIORITY_ON.value:
            self.ph.log.error("A2DP priority found: {}, expected {}.".format(
                ph_a2dp_p, BtEnum.BluetoothPriorityLevel.PRIORITY_ON.value))
            return False

        return True
