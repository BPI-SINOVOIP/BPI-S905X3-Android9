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
This test script for GATT longevity tests.
"""

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.GattConnectedBaseTest import GattConnectedBaseTest
from acts.test_utils.bt.bt_constants import gatt_characteristic
from acts.test_utils.bt.bt_constants import gatt_descriptor
from acts.test_utils.bt.bt_constants import gatt_event
from acts.test_utils.bt.bt_constants import gatt_cb_strings
from acts.test_utils.bt.bt_constants import gatt_connection_priority
from acts.test_utils.bt.bt_constants import gatt_characteristic_attr_length
from acts.test_utils.bt.GattEnum import MtuSize
from acts.test_utils.bt.bt_gatt_utils import setup_gatt_mtu


class GattLongevityTest(GattConnectedBaseTest):
    longevity_iterations = 1100000

    @test_tracker_info(uuid='d7d378f4-89d8-4330-bb80-0054b92020bb')
    def test_write_characteristic_no_resp_longevity(self):
        """Longevity test write characteristic value

        Longevity test to write characteristic value for
        self.longevity_iteration times. This is to test the
        integrity of written data and the robustness of central
        and peripheral mode of the Android devices under test.

        1. Central: write WRITABLE_CHAR_UUID characteristic with char_value
           using write command.
        2. Central: make sure write callback is called.
        3. Peripheral: receive the written data.
        4. Verify data written matches data received.
        5. Repeat steps 1-4 self.longevity_iterations times.

        Expected Result:
        Verify that write command is properly delivered.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, GATT, Characteristic, Longevity
        Priority: 0
        """
        self.cen_ad.droid.gattClientRequestConnectionPriority(
            self.bluetooth_gatt, gatt_connection_priority['high'])

        self.cen_ad.droid.gattClientCharacteristicSetWriteType(
            self.bluetooth_gatt, self.discovered_services_index,
            self.test_service_index, self.WRITABLE_CHAR_UUID,
            gatt_characteristic['write_type_no_response'])

        for i in range(self.longevity_iterations):
            self.log.debug("Iteration {} started.".format(i + 1))
            char_value = []
            for j in range(i, i + self.mtu - 3):
                char_value.append(j % 256)

            self.cen_ad.droid.gattClientCharacteristicSetValue(
                self.bluetooth_gatt, self.discovered_services_index,
                self.test_service_index, self.WRITABLE_CHAR_UUID, char_value)

            self.cen_ad.droid.gattClientWriteCharacteristic(
                self.bluetooth_gatt, self.discovered_services_index,
                self.test_service_index, self.WRITABLE_CHAR_UUID)

            # client shall not wait for server, get complete event right away
            event = self._client_wait(gatt_event['char_write'])
            if event["data"]["Status"] != 0:
                self.log.error("Write status should be 0")
                return False

            event = self._server_wait(gatt_event['char_write_req'])

            self.log.info("{} event found: {}".format(gatt_cb_strings[
                'char_write_req'].format(self.gatt_server_callback), event[
                    'data']['value']))
            request_id = event['data']['requestId']
            found_value = event['data']['value']
            if found_value != char_value:
                self.log.info("Values didn't match. Found: {}, "
                              "Expected: {}".format(found_value, char_value))
                return False

        return True
