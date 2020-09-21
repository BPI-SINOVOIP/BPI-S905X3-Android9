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
This test script exercises GATT notify/indicate procedures.
"""

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.GattConnectedBaseTest import GattConnectedBaseTest
from acts.test_utils.bt.bt_constants import gatt_characteristic
from acts.test_utils.bt.bt_constants import gatt_descriptor
from acts.test_utils.bt.bt_constants import gatt_event
from acts.test_utils.bt.bt_constants import gatt_cb_strings
from acts.test_utils.bt.bt_constants import gatt_char_desc_uuids
from math import ceil


class GattNotifyTest(GattConnectedBaseTest):
    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e0ba60af-c1f2-4516-a5d5-89e2def0c757')
    def test_notify_char(self):
        """Test notify characteristic value.

        Test GATT notify characteristic value.

        Steps:
        1. Central: write CCC - register for notifications.
        2. Peripheral: receive CCC modification.
        3. Peripheral: send characteristic notification.
        4. Central: receive notification, verify it's conent matches what was
           sent

        Expected Result:
        Verify that notification registration and delivery works.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, GATT, Characteristic
        Priority: 0
        """
        #write CCC descriptor to enable notifications
        self.cen_ad.droid.gattClientDescriptorSetValue(
            self.bluetooth_gatt, self.discovered_services_index,
            self.test_service_index, self.NOTIFIABLE_CHAR_UUID,
            gatt_char_desc_uuids['client_char_cfg'],
            gatt_descriptor['enable_notification_value'])

        self.cen_ad.droid.gattClientWriteDescriptor(
            self.bluetooth_gatt, self.discovered_services_index,
            self.test_service_index, self.NOTIFIABLE_CHAR_UUID,
            gatt_char_desc_uuids['client_char_cfg'])

        #enable notifications in app
        self.cen_ad.droid.gattClientSetCharacteristicNotification(
            self.bluetooth_gatt, self.discovered_services_index,
            self.test_service_index, self.NOTIFIABLE_CHAR_UUID, True)

        event = self._server_wait(gatt_event['desc_write_req'])

        request_id = event['data']['requestId']
        bt_device_id = 0
        status = 0
        #confirm notification registration was successful
        self.per_ad.droid.gattServerSendResponse(
            self.gatt_server, bt_device_id, request_id, status, 0, [])
        #wait for client to get response
        event = self._client_wait(gatt_event['desc_write'])

        #set notified value
        notified_value = [1, 5, 9, 7, 5, 3, 6, 4, 8, 2]
        self.per_ad.droid.gattServerCharacteristicSetValue(
            self.notifiable_char_index, notified_value)

        #send notification
        self.per_ad.droid.gattServerNotifyCharacteristicChanged(
            self.gatt_server, bt_device_id, self.notifiable_char_index, False)

        #wait for client to receive the notification
        event = self._client_wait(gatt_event['char_change'])
        self.assertEqual(notified_value, event["data"]["CharacteristicValue"])

        return True
