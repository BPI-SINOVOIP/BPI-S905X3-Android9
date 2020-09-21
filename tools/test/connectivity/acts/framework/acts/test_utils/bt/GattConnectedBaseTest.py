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
This is base class for tests that exercises different GATT procedures between two connected devices.
Setup/Teardown methods take care of establishing connection, and doing GATT DB initialization/discovery.
"""

from queue import Empty

from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import gatt_characteristic
from acts.test_utils.bt.bt_constants import gatt_descriptor
from acts.test_utils.bt.bt_constants import gatt_service_types
from acts.test_utils.bt.bt_constants import gatt_event
from acts.test_utils.bt.bt_constants import gatt_cb_err
from acts.test_utils.bt.bt_constants import gatt_cb_strings
from acts.test_utils.bt.bt_constants import gatt_mtu_size
from acts.test_utils.bt.bt_gatt_utils import disconnect_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import orchestrate_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import setup_gatt_characteristics
from acts.test_utils.bt.bt_gatt_utils import setup_gatt_descriptors
from acts.test_utils.bt.bt_constants import gatt_char_desc_uuids
from acts.test_utils.bt.bt_constants import bt_default_timeout


class GattConnectedBaseTest(BluetoothBaseTest):

    TEST_SERVICE_UUID = "3846D7A0-69C8-11E4-BA00-0002A5D5C51B"
    READABLE_CHAR_UUID = "21c0a0bf-ad51-4a2d-8124-b74003e4e8c8"
    READABLE_DESC_UUID = "aa7edd5a-4d1d-4f0e-883a-d145616a1630"
    WRITABLE_CHAR_UUID = "aa7edd5a-4d1d-4f0e-883a-d145616a1630"
    WRITABLE_DESC_UUID = "76d5ed92-ca81-4edb-bb6b-9f019665fb32"
    NOTIFIABLE_CHAR_UUID = "b2c83efa-34ca-11e6-ac61-9e71128cae77"

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.cen_ad = self.android_devices[0]
        self.per_ad = self.android_devices[1]

    def setup_test(self):
        super(GattConnectedBaseTest, self).setup_test()

        self.gatt_server_callback, self.gatt_server = self._setup_multiple_services(
        )
        if not self.gatt_server_callback or not self.gatt_server:
            raise AssertionError('Service setup failed')

        self.bluetooth_gatt, self.gatt_callback, self.adv_callback = (
            orchestrate_gatt_connection(self.cen_ad, self.per_ad))
        self.per_ad.droid.bleStopBleAdvertising(self.adv_callback)

        self.mtu = gatt_mtu_size['min']

        if self.cen_ad.droid.gattClientDiscoverServices(self.bluetooth_gatt):
            event = self._client_wait(gatt_event['gatt_serv_disc'])
            self.discovered_services_index = event['data']['ServicesIndex']
        services_count = self.cen_ad.droid.gattClientGetDiscoveredServicesCount(
            self.discovered_services_index)
        self.test_service_index = None
        for i in range(services_count):
            disc_service_uuid = (
                self.cen_ad.droid.gattClientGetDiscoveredServiceUuid(
                    self.discovered_services_index, i).upper())
            if disc_service_uuid == self.TEST_SERVICE_UUID:
                self.test_service_index = i
                break

        if not self.test_service_index:
            print("Service not found")
            return False

        connected_device_list = self.per_ad.droid.gattServerGetConnectedDevices(
            self.gatt_server)
        if len(connected_device_list) == 0:
            self.log.info("No devices connected from peripheral.")
            return False

        return True

    def teardown_test(self):
        self.per_ad.droid.gattServerClearServices(self.gatt_server)
        self.per_ad.droid.gattServerClose(self.gatt_server)

        del self.gatt_server_callback
        del self.gatt_server

        self._orchestrate_gatt_disconnection(self.bluetooth_gatt,
                                             self.gatt_callback)

        return super(GattConnectedBaseTest, self).teardown_test()

    def _server_wait(self, gatt_event):
        return self._timed_pop(gatt_event, self.per_ad,
                               self.gatt_server_callback)

    def _client_wait(self, gatt_event):
        return self._timed_pop(gatt_event, self.cen_ad, self.gatt_callback)

    def _timed_pop(self, gatt_event, droid, gatt_callback):
        expected_event = gatt_event["evt"].format(gatt_callback)
        try:
            return droid.ed.pop_event(expected_event, bt_default_timeout)
        except Empty as emp:
            raise AssertionError(gatt_event["err"].format(expected_event))

    def _setup_characteristics_and_descriptors(self, droid):
        characteristic_input = [
            {
                'uuid': self.WRITABLE_CHAR_UUID,
                'property': gatt_characteristic['property_write'] |
                gatt_characteristic['property_write_no_response'],
                'permission': gatt_characteristic['permission_write']
            },
            {
                'uuid': self.READABLE_CHAR_UUID,
                'property': gatt_characteristic['property_read'],
                'permission': gatt_characteristic['permission_read']
            },
            {
                'uuid': self.NOTIFIABLE_CHAR_UUID,
                'property': gatt_characteristic['property_notify'] |
                gatt_characteristic['property_indicate'],
                'permission': gatt_characteristic['permission_read']
            },
        ]
        descriptor_input = [{
            'uuid': self.WRITABLE_DESC_UUID,
            'property': gatt_descriptor['permission_read'] |
            gatt_characteristic['permission_write'],
        }, {
            'uuid': self.READABLE_DESC_UUID,
            'property': gatt_descriptor['permission_read'] |
            gatt_descriptor['permission_write'],
        }, {
            'uuid': gatt_char_desc_uuids['client_char_cfg'],
            'property': gatt_descriptor['permission_read'] |
            gatt_descriptor['permission_write'],
        }]
        characteristic_list = setup_gatt_characteristics(droid,
                                                         characteristic_input)
        self.notifiable_char_index = characteristic_list[2]
        descriptor_list = setup_gatt_descriptors(droid, descriptor_input)
        return characteristic_list, descriptor_list

    def _orchestrate_gatt_disconnection(self, bluetooth_gatt, gatt_callback):
        self.log.info("Disconnecting from peripheral device.")
        try:
            disconnect_gatt_connection(self.cen_ad, bluetooth_gatt,
                                       gatt_callback)
        except GattTestUtilsError as err:
            log.error(err)
            return False
        self.cen_ad.droid.gattClientClose(bluetooth_gatt)
        return True

    def _find_service_added_event(self, gatt_server_callback, uuid):
        expected_event = gatt_cb_strings['serv_added'].format(
            gatt_server_callback)
        try:
            event = self.per_ad.ed.pop_event(expected_event,
                                             bt_default_timeout)
        except Empty:
            self.log.error(gatt_cb_err['serv_added_err'].format(
                expected_event))
            return False
        if event['data']['serviceUuid'].lower() != uuid.lower():
            self.log.error("Uuid mismatch. Found: {}, Expected {}.".format(
                event['data']['serviceUuid'], uuid))
            return False
        return True

    def _setup_multiple_services(self):
        gatt_server_callback = (
            self.per_ad.droid.gattServerCreateGattServerCallback())
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_callback)
        characteristic_list, descriptor_list = (
            self._setup_characteristics_and_descriptors(self.per_ad.droid))
        self.per_ad.droid.gattServerCharacteristicAddDescriptor(
            characteristic_list[0], descriptor_list[0])
        self.per_ad.droid.gattServerCharacteristicAddDescriptor(
            characteristic_list[1], descriptor_list[1])
        self.per_ad.droid.gattServerCharacteristicAddDescriptor(
            characteristic_list[2], descriptor_list[2])
        gatt_service3 = self.per_ad.droid.gattServerCreateService(
            self.TEST_SERVICE_UUID, gatt_service_types['primary'])
        for characteristic in characteristic_list:
            self.per_ad.droid.gattServerAddCharacteristicToService(
                gatt_service3, characteristic)
        self.per_ad.droid.gattServerAddService(gatt_server, gatt_service3)
        result = self._find_service_added_event(gatt_server_callback,
                                                self.TEST_SERVICE_UUID)
        if not result:
            return False, False
        return gatt_server_callback, gatt_server

    def assertEqual(self, first, second, msg=None):
        if not first == second:
            if not msg:
                raise AssertionError('%r != %r' % (first, second))
            else:
                raise AssertionError(msg + ' %r != %r' % (first, second))
