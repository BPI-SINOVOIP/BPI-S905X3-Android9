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
This test script exercises different GATT read procedures.
"""

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.GattConnectedBaseTest import GattConnectedBaseTest
from acts.test_utils.bt.bt_constants import gatt_characteristic
from acts.test_utils.bt.bt_constants import gatt_descriptor
from acts.test_utils.bt.bt_constants import gatt_event
from acts.test_utils.bt.bt_constants import gatt_cb_strings
from math import ceil


class GattReadTest(GattConnectedBaseTest):
    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ed8523f4-0eb8-4d14-b558-d3c28902f8bb')
    def test_read_char(self):
        """Test read characteristic value.

        Test GATT read characteristic value.

        Steps:
        1. Central: send read request.
        2. Peripheral: receive read request .
        3. Peripheral: send read response with status 0 (success), and
           characteristic value.
        4. Central: receive read response, verify it's conent matches what was
           sent

        Expected Result:
        Verify that read request/response is properly delivered.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, GATT, Characteristic
        Priority: 0
        """
        self.cen_ad.droid.gattClientReadCharacteristic(
            self.bluetooth_gatt, self.discovered_services_index,
            self.test_service_index, self.READABLE_CHAR_UUID)

        event = self._server_wait(gatt_event['char_read_req'])

        request_id = event['data']['requestId']
        self.assertEqual(0, event['data']['offset'], "offset should be 0")

        bt_device_id = 0
        status = 0
        char_value = [1, 2, 3, 4, 5, 6, 7, 20]
        offset = 0
        self.per_ad.droid.gattServerSendResponse(self.gatt_server,
                                                 bt_device_id, request_id,
                                                 status, offset, char_value)

        event = self._client_wait(gatt_event['char_read'])
        self.assertEqual(status, event["data"]["Status"],
                         "Write status should be 0")
        self.assertEqual(char_value, event["data"]["CharacteristicValue"],
                         "Read value shall be equal to value sent from server")

        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5916a78d-3db8-4df2-9b96-b25e99096e0d')
    def test_read_long_char(self):
        """Test read long characteristic value.

        Test GATT read long characteristic value.

        Steps:
        1. Central: send read request.
        2. Peripheral: receive read request .
        3. Peripheral: send read response with status 0 (success), and
           characteristic content.
        5. Central: stack receives read response that was full, so stack sends
           read blob request with increased offset.
        6. Peripheral: receive read blob request, send read blob response with
           next piece of characteristic value.
        7. Central: stack receives read blob response, so stack sends read blob
           request with increased offset. No Java callbacks are called here
        8. Repeat steps 6 and 7 until whole characteristic is read.
        9. Central: verify onCharacteristicRead callback is called with whole
           characteristic content.

        Expected Result:
        Verify that read request/response is properly delivered, and that read
        blob reqest/response is properly sent internally by the stack.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, GATT, Characteristic
        Priority: 0
        """
        char_value = []
        for i in range(512):
            char_value.append(i % 256)

        self.cen_ad.droid.gattClientReadCharacteristic(
            self.bluetooth_gatt, self.discovered_services_index,
            self.test_service_index, self.READABLE_CHAR_UUID)

        # characteristic value is divided into packets, each contains MTU-1
        # bytes. Compute number of packets we expect to receive.
        num_packets = ceil((len(char_value) + 1) / (self.mtu - 1))

        for i in range(num_packets):
            startOffset = i * (self.mtu - 1)

            event = self._server_wait(gatt_event['char_read_req'])

            request_id = event['data']['requestId']
            self.assertEqual(startOffset, event['data']['offset'],
                             "offset should be 0")

            bt_device_id = 0
            status = 0
            offset = 0
            self.per_ad.droid.gattServerSendResponse(
                self.gatt_server, bt_device_id, request_id, status, offset,
                char_value[startOffset:startOffset + self.mtu - 1])

        event = self._client_wait(gatt_event['char_read'])

        self.assertEqual(status, event["data"]["Status"],
                         "Write status should be 0")
        self.assertEqual(char_value, event["data"]["CharacteristicValue"],
                         "Read value shall be equal to value sent from server")

        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='12498522-cac5-478b-a0cd-faa542832fa8')
    def test_read_using_char_uuid(self):
        """Test read using characteristic UUID.

        Test GATT read value using characteristic UUID.

        Steps:
        1. Central: send read by UUID request.
        2. Peripheral: receive read request .
        3. Peripheral: send read response with status 0 (success), and
           characteristic value.
        4. Central: receive read response, verify it's conent matches what was
           sent

        Expected Result:
        Verify that read request/response is properly delivered.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, GATT, Characteristic
        Priority: 0
        """
        self.cen_ad.droid.gattClientReadUsingCharacteristicUuid(
            self.bluetooth_gatt, self.READABLE_CHAR_UUID, 0x0001, 0xFFFF)

        event = self._server_wait(gatt_event['char_read_req'])

        request_id = event['data']['requestId']
        self.assertEqual(0, event['data']['offset'], "offset should be 0")

        bt_device_id = 0
        status = 0
        char_value = [1, 2, 3, 4, 5, 6, 7, 20]
        offset = 0
        self.per_ad.droid.gattServerSendResponse(self.gatt_server,
                                                 bt_device_id, request_id,
                                                 status, offset, char_value)

        event = self._client_wait(gatt_event['char_read'])
        self.assertEqual(status, event["data"]["Status"],
                         "Write status should be 0")
        self.assertEqual(char_value, event["data"]["CharacteristicValue"],
                         "Read value shall be equal to value sent from server")

        return True
