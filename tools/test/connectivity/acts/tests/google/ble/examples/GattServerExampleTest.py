# /usr/bin/env python3.4
#
# Copyright (C) 2018 The Android Open Source Project
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

from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import gatt_characteristic
from acts.test_utils.bt.bt_constants import gatt_descriptor
from acts.test_utils.bt.bt_constants import gatt_service_types
from acts.test_utils.bt.bt_constants import gatt_characteristic_value_format
from acts.test_utils.bt.bt_constants import gatt_char_desc_uuids
from acts.test_utils.bt.gatts_lib import GattServerLib

service_uuid = '0000a00a-0000-1000-8000-00805f9b34fb'
characteristic_uuid = 'aa7edd5a-4d1d-4f0e-883a-d145616a1630'
descriptor_uuid = gatt_char_desc_uuids['client_char_cfg']

gatt_server_read_descriptor_sample = {
    'services': [{
        'uuid':
        service_uuid,
        'type':
        gatt_service_types['primary'],
        'characteristics': [{
            'uuid':
            characteristic_uuid,
            'properties':
            gatt_characteristic['property_read'],
            'permissions':
            gatt_characteristic['permission_read'],
            'instance_id':
            0x002a,
            'value_type':
            gatt_characteristic_value_format['string'],
            'value':
            'Test Database',
            'descriptors': [{
                'uuid': descriptor_uuid,
                'permissions': gatt_descriptor['permission_read'],
            }]
        }]
    }]
}


class GattServerExampleTest(BluetoothBaseTest):
    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.dut = self.android_devices[0]

    @BluetoothBaseTest.bt_test_wrap
    def test_create_gatt_server_db_example(self):
        gatts = GattServerLib(log=self.log, dut=self.dut)
        gatts.setup_gatts_db(database=gatt_server_read_descriptor_sample)
        self.log.info(gatts.list_all_uuids())
        return True
