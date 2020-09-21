#/usr/bin/env python3.4
#
# Copyright (C) 2017 The Android Open Source Project
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
Test script to test various ThreeButtonDongle devices
"""
import time

from acts.controllers.relay_lib.relay import SynchronizeRelays
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import clear_bonded_devices


class ThreeButtonDongleTest(BluetoothBaseTest):
    iterations = 10

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.dut = self.android_devices[0]
        self.dongle = self.relay_devices[0]
        self.log.info("Target dongle is {}".format(self.dongle.name))

    def setup_test(self):
        super(BluetoothBaseTest, self).setup_test()
        self.dongle.setup()
        return True

    def teardown_test(self):
        super(BluetoothBaseTest, self).teardown_test()
        self.dongle.clean_up()
        clear_bonded_devices(self.dut)
        return True

    def _pair_devices(self):
        self.dut.droid.bluetoothStartPairingHelper(False)
        self.dongle.enter_pairing_mode()

        self.dut.droid.bluetoothBond(self.dongle.mac_address)

        end_time = time.time() + 20
        self.dut.log.info("Verifying devices are bonded")
        while time.time() < end_time:
            bonded_devices = self.dut.droid.bluetoothGetBondedDevices()

            for d in bonded_devices:
                if d['address'] == self.dongle.mac_address:
                    self.dut.log.info("Successfully bonded to device.")
                    self.log.info("Bonded devices:\n{}".format(bonded_devices))
                return True
        self.dut.log.info("Failed to bond devices.")
        return False

    @BluetoothBaseTest.bt_test_wrap
    def test_pairing(self):
        """Test pairing between a three button dongle and an Android device.

        Test the dongle can be paired to Android device.

        Steps:
        1. Find the MAC address of remote controller from relay config file.
        2. Start the device paring process.
        3. Enable remote controller in pairing mode.
        4. Verify the remote is paired.

        Expected Result:
          Remote controller is paired.

        Returns:
          Pass if True
          Fail if False

        TAGS: Bluetooth, bonding, relay
        Priority: 3
        """
        if not self._pair_devices():
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_pairing_multiple_iterations(self):
        """Test pairing between a three button dongle and an Android device.

        Test the dongle can be paired to Android device.

        Steps:
        1. Find the MAC address of remote controller from relay config file.
        2. Start the device paring process.
        3. Enable remote controller in pairing mode.
        4. Verify the remote is paired.

        Expected Result:
          Remote controller is paired.

        Returns:
          Pass if True
          Fail if False

        TAGS: Bluetooth, bonding, relay
        Priority: 3
        """
        for i in range(self.iterations):
            self.log.info("Testing iteration {}.".format(i))
            if not self._pair_devices():
                return False
            self.log.info("Unbonding devices.")
            self.dut.droid.bluetoothUnbond(self.dongle.mac_address)
            # Sleep for relax time for the relay
            time.sleep(2)
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_next_multiple_iterations(self):
        """Test pairing for multiple iterations.

        Test the dongle can be paired to Android device.

        Steps:
        1. Pair devices
        2. Press the next button on dongle for pre-definied iterations.

        Expected Result:
          Test is successful

        Returns:
          Pass if True
          Fail if False

        TAGS: Bluetooth, bonding, relay
        Priority: 3
        """
        if not self._pair_devices():
            return False
        for _ in range(self.iterations):
            self.dongle.press_next()
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_play_pause_multiple_iterations(self):
        """Test play/pause button on a three button dongle.

        Test the dongle can be paired to Android device.

        Steps:
        1. Pair devices
        2. Press the next button on dongle for pre-definied iterations.

        Expected Result:
          Test is successful

        Returns:
          Pass if True
          Fail if False

        TAGS: Bluetooth, bonding, relay
        Priority: 3
        """
        if not self._pair_devices():
            return False
        for _ in range(self.iterations):
            self.dongle.press_play_pause()
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_previous_mulitple_iterations(self):
        """Test previous button on a three button dongle.

        Test the dongle can be paired to Android device.

        Steps:
        1. Pair devices
        2. Press the next button on dongle for pre-definied iterations.

        Expected Result:
          Test is successful

        Returns:
          Pass if True
          Fail if False

        TAGS: Bluetooth, bonding, relay
        Priority: 3
        """
        if not self._pair_devices():
            return False
        for _ in range(100):
            self.dongle.press_previous()
        return True
