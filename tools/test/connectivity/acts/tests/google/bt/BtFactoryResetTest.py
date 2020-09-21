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
Test script to test BluetoothAdapter's resetFactory method.
"""
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import pair_pri_to_sec


class BtFactoryResetTest(BluetoothBaseTest):
    default_timeout = 10
    grace_timeout = 4

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.pri_dut = self.android_devices[0]
        self.sec_dut = self.android_devices[1]

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='800bce6a-87ef-4fd1-82a5-4c60c4133be1')
    def test_factory_reset_bluetooth(self):
        """Test that BluetoothAdapter.factoryReset removes bonded devices

        After having bonded devices, call the factoryReset method
        in BluetoothAdapter to clear all bonded devices. Verify that
        no bonded devices exist after calling function.

        Steps:
        1. Two Android devices should bond successfully.
        2. Factory Reset Bluetooth on dut.
        3. Verify that there are no bonded devices on dut.

        Expected Result:
        BluetoothAdapter should not have any bonded devices.

        Returns:
          Pass if True
          Fail if False

        TAGS: Bluetooth, Factory Reset
        Priority: 2
        """
        if not pair_pri_to_sec(self.pri_dut, self.sec_dut, attempts=1):
            self.log.error("Failed to bond devices.")
            return False
        if not self.pri_dut.droid.bluetoothFactoryReset():
            self.log.error("BluetoothAdapter failed to factory reset.")
        self.log.info("Verifying There are no bonded devices.")
        if len(self.pri_dut.droid.bluetoothGetBondedDevices()) > 0:
            print(self.pri_dut.droid.bluetoothGetBondedDevices())
            self.log.error("Bonded devices still exist.")
            return False
        return True
