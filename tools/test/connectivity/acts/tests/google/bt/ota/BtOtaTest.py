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
Test script for Bluetooth OTA testing.
"""

from acts.libs.ota import ota_updater
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import pair_pri_to_sec
from acts import signals


class BtOtaTest(BluetoothBaseTest):
    def setup_class(self):
        super(BtOtaTest, self).setup_class()
        ota_updater.initialize(self.user_params, self.android_devices)
        self.dut = self.android_devices[0]
        self.pre_ota_name = self.dut.droid.bluetoothGetLocalName()
        self.pre_ota_address = self.dut.droid.bluetoothGetLocalAddress()
        self.sec_address = self.android_devices[
            1].droid.bluetoothGetLocalAddress()

        # Pairing devices
        if not pair_pri_to_sec(self.dut, self.android_devices[1]):
            raise signals.TestSkipClass(
                "Failed to bond devices prior to update")

        #Run OTA below, if ota fails then abort all tests
        try:
            ota_updater.update(self.dut)
        except Exception as err:
            raise signals.TestSkipClass(
                "Failed up apply OTA update. Aborting tests")

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='57545ef0-2c2e-463c-9dbf-28da73cc76df')
    def test_device_name_persists(self):
        """Test device name persists after OTA update

        Test device name persists after OTA update

        Steps:
        1. Verify pre OTA device name matches post OTA device name

        Expected Result:
        Bluetooth Device name persists

        Returns:
          Pass if True
          Fail if False

        TAGS: OTA
        Priority: 2
        """
        return self.pre_ota_name == self.dut.droid.bluetoothGetLocalName()

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1fd5e1a5-d930-499c-aebc-c1872ab49568')
    def test_device_address_persists(self):
        """Test device address persists after OTA update

        Test device address persists after OTA update

        Steps:
        1. Verify pre OTA device address matches post OTA device address

        Expected Result:
        Bluetooth Device address persists

        Returns:
          Pass if True
          Fail if False

        TAGS: OTA
        Priority: 2
        """
        return self.pre_ota_address == self.dut.droid.bluetoothGetLocalAddress(
        )

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2e6704e6-3df0-43fb-8425-41ff841d7473')
    def test_bluetooth_state_persists(self):
        """Test device Bluetooth state persists after OTA update

        Test device Bluetooth state persists after OTA update

        Steps:
        1. Verify post OTA Bluetooth state is on

        Expected Result:
        Bluetooth Device Bluetooth state is on

        Returns:
          Pass if True
          Fail if False

        TAGS: OTA
        Priority: 2
        """
        return self.dut.droid.bluetoothCheckState()

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='eb1c0a22-4b4e-4984-af17-ace3bcd203de')
    def test_bonded_devices_persist(self):
        """Test device bonded devices persists after OTA update

        Test device address persists after OTA update

        Steps:
        1. Verify pre OTA device bonded devices matches post OTA device
        bonded devices

        Expected Result:
        Bluetooth Device bonded devices persists

        Returns:
          Pass if True
          Fail if False

        TAGS: OTA
        Priority: 1
        """
        bonded_devices = self.dut.droid.bluetoothGetBondedDevices()
        for b in bonded_devices:
            if b['address'] == self.sec_address:
                return True
        return False
