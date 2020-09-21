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
Test script to test pairing of an Android Device to a Fugu Remote
"""
import time

from acts.controllers.relay_lib.relay import SynchronizeRelays
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest

class AndroidFuguRemotePairingTest(BluetoothBaseTest):
    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.dut = self.android_devices[0]
        self.fugu_remote = self.relay_devices[0]

    def setup_test(self):
        super(BluetoothBaseTest, self).setup_test()
        self.fugu_remote.setup()
        return True

    def teardown_test(self):
        super(BluetoothBaseTest, self).teardown_test()
        self.fugu_remote.clean_up()
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_pairing(self):
        """Test pairing between a fugu device and a remote controller.

        Test the remote controller can be paired to fugu.

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

        TAGS: fugu
        Priority: 1
        """

        self.dut.droid.bluetoothStartPairingHelper(False)
        self.fugu_remote.enter_pairing_mode()
        self.dut.droid.bluetoothDiscoverAndBond(self.fugu_remote.mac_address)

        end_time = time.time() + 20
        self.dut.log.info("Verifying devices are bonded")
        while time.time() < end_time:
            bonded_devices = self.dut.droid.bluetoothGetBondedDevices()

            for d in bonded_devices:
                if d['address'] == self.fugu_remote.mac_address:
                    self.dut.log.info("Successfully bonded to device.")
                    self.log.info("Fugu Bonded devices:\n{}".format(
                        bonded_devices))
                    return True
        # Timed out trying to bond.
        self.dut.log.info("Failed to bond devices.")

        return False
