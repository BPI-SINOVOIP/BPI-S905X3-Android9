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

from acts.test_utils.bt.bt_constants import gatt_characteristic
from acts.test_utils.bt.bt_constants import gatt_descriptor
from acts.test_utils.bt.bt_constants import gatt_service_types
from acts.test_utils.bt.bt_constants import gatt_char_types
from acts.test_utils.bt.bt_constants import gatt_characteristic_value_format
from acts.test_utils.bt.bt_constants import gatt_char_desc_uuids

STRING_512BYTES = '''
11111222223333344444555556666677777888889999900000
11111222223333344444555556666677777888889999900000
11111222223333344444555556666677777888889999900000
11111222223333344444555556666677777888889999900000
11111222223333344444555556666677777888889999900000
11111222223333344444555556666677777888889999900000
11111222223333344444555556666677777888889999900000
11111222223333344444555556666677777888889999900000
11111222223333344444555556666677777888889999900000
11111222223333344444555556666677777888889999900000
111112222233
'''
STRING_50BYTES = '''
11111222223333344444555556666677777888889999900000
'''
STRING_25BYTES = '''
1111122222333334444455555
'''

INVALID_SMALL_DATABASE = {
    'services': [{
        'uuid': '00001800-0000-1000-8000-00805f9b34fb',
        'type': gatt_service_types['primary'],
        'characteristics': [{
            'uuid': gatt_char_types['device_name'],
            'properties': gatt_characteristic['property_read'],
            'permissions': gatt_characteristic['permission_read'],
            'instance_id': 0x0003,
            'value_type': gatt_characteristic_value_format['string'],
            'value': 'Test Database'
        }, {
            'uuid': gatt_char_types['appearance'],
            'properties': gatt_characteristic['property_read'],
            'permissions': gatt_characteristic['permission_read'],
            'instance_id': 0x0005,
            'value_type': gatt_characteristic_value_format['sint32'],
            'offset': 0,
            'value': 17
        }, {
            'uuid': gatt_char_types['peripheral_pref_conn'],
            'properties': gatt_characteristic['property_read'],
            'permissions': gatt_characteristic['permission_read'],
            'instance_id': 0x0007
        }]
    }, {
        'uuid': '00001801-0000-1000-8000-00805f9b34fb',
        'type': gatt_service_types['primary'],
        'characteristics': [{
            'uuid': gatt_char_types['service_changed'],
            'properties': gatt_characteristic['property_indicate'],
            'permissions': gatt_characteristic['permission_read'] |
            gatt_characteristic['permission_write'],
            'instance_id': 0x0012,
            'value_type': gatt_characteristic_value_format['byte'],
            'value': [0x0000],
            'descriptors': [{
                'uuid': gatt_char_desc_uuids['client_char_cfg'],
                'permissions': gatt_descriptor['permission_read'] |
                gatt_descriptor['permission_write'],
            }]
        }, {
            'uuid': '0000b004-0000-1000-8000-00805f9b34fb',
            'properties': gatt_characteristic['property_read'],
            'permissions': gatt_characteristic['permission_read'],
            'instance_id': 0x0015,
            'value_type': gatt_characteristic_value_format['byte'],
            'value': [0x04]
        }]
    }]
}

# Corresponds to the PTS defined LARGE_DB_1
LARGE_DB_1 = {
    'services': [
        {
            'uuid': '0000a00b-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'handles': 7,
            'characteristics': [{
                'uuid': '0000b008-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_read'] |
                gatt_characteristic['property_write'] |
                gatt_characteristic['property_extended_props'],
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x08],
                'descriptors': [{
                    'uuid': '0000b015-0000-1000-8000-00805f9b34fb',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                }, {
                    'uuid': '0000b016-0000-1000-8000-00805f9b34fb',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                }, {
                    'uuid': '0000b017-0000-1000-8000-00805f9b34fb',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'] |
                    gatt_characteristic['permission_read_encrypted_mitm'],
                }]
            }]
        },
        {
            'uuid': '0000a00d-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['secondary'],
            'handles': 6,
            'characteristics': [{
                'uuid': '0000b00c-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_extended_props'],
                'permissions': gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x0C],
            }, {
                'uuid': '0000b00b-0000-0000-0123-456789abcdef',
                'properties': gatt_characteristic['property_extended_props'],
                'permissions': gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x0B],
            }]
        },
        {
            'uuid': '0000a00a-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'handles': 10,
            'characteristics': [{
                'uuid': '0000b001-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_read'] |
                gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x01],
            }, {
                'uuid': '0000b002-0000-0000-0123-456789abcdef',
                'properties': gatt_characteristic['property_extended_props'],
                'permissions': gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            }, {
                'uuid': '0000b004-0000-0000-0123-456789abcdef',
                'properties': gatt_characteristic['property_read'],
                'permissions': gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            }, {
                'uuid': '0000b002-0000-0000-0123-456789abcdef',
                'properties': gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': '11111222223333344444555556666677777888889999900000',
            }, {
                'uuid': '0000b003-0000-0000-0123-456789abcdef',
                'properties': gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x03],
            }]
        },
        {
            'uuid': '0000a00b-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'handles': 3,
            'characteristics': [{
                'uuid': '0000b007-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x07],
            }]
        },
        {
            'uuid': '0000a00b-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'handles': 3,
            'characteristics': [{
                'uuid': '0000b006-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_read'] |
                gatt_characteristic['property_write'] |
                gatt_characteristic['property_write_no_response'] |
                gatt_characteristic['property_notify'] |
                gatt_characteristic['property_indicate'],
                'permissions': gatt_characteristic['permission_write'] |
                gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x06],
            }]
        },
        {
            'uuid': '0000a00b-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'handles': 12,
            'characteristics': [
                {
                    'uuid': '0000b004-0000-1000-8000-00805f9b34fb',
                    'properties': gatt_characteristic['property_read'] |
                    gatt_characteristic['property_write'],
                    'permissions': gatt_characteristic['permission_write'] |
                    gatt_characteristic['permission_read'],
                    'value_type': gatt_characteristic_value_format['byte'],
                    'value': [0x04],
                },
                {
                    'uuid': '0000b004-0000-1000-8000-00805f9b34fb',
                    'properties': gatt_characteristic['property_read'] |
                    gatt_characteristic['property_write'],
                    'permissions': gatt_characteristic['permission_write'] |
                    gatt_characteristic['permission_read'],
                    'value_type': gatt_characteristic_value_format['byte'],
                    'value': [0x04],
                    'descriptors': [{
                        'uuid': gatt_char_desc_uuids['server_char_cfg'],
                        'permissions': gatt_descriptor['permission_read'] |
                        gatt_descriptor['permission_write'],
                        'value': gatt_descriptor['disable_notification_value']
                    }]
                },
                {
                    'uuid': '0000b004-0000-1000-8000-00805f9b34fb',
                    'properties': 0x0,
                    'permissions': 0x0,
                    'value_type': gatt_characteristic_value_format['byte'],
                    'value': [0x04],
                    'descriptors': [{
                        'uuid': '0000b012-0000-1000-8000-00805f9b34fb',
                        'permissions': gatt_descriptor['permission_read'] |
                        gatt_descriptor['permission_write'],
                        'value': [
                            0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                            0x99, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                            0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44,
                            0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22,
                            0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                            0x11, 0x22, 0x33
                        ]
                    }]
                },
                {
                    'uuid': '0000b004-0000-1000-8000-00805f9b34fb',
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'],
                    'value_type': gatt_characteristic_value_format['byte'],
                    'value': [0x04],
                    'descriptors': [{
                        'uuid': '0000b012-0000-1000-8000-00805f9b34fb',
                        'permissions': gatt_descriptor['permission_read'],
                        'value': [
                            0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                            0x99, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                            0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44,
                            0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22,
                            0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                            0x11, 0x22, 0x33
                        ]
                    }]
                },
            ]
        },
        {
            'uuid': '0000a00b-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'handles': 7,
            'characteristics': [{
                'uuid': '0000b005-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_write'] |
                gatt_characteristic['property_extended_props'],
                'permissions': gatt_characteristic['permission_write'] |
                gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x05],
                'descriptors': [{
                    'uuid': gatt_char_desc_uuids['char_ext_props'],
                    'permissions': gatt_descriptor['permission_read'],
                    'value': [0x03, 0x00]
                }, {
                    'uuid': gatt_char_desc_uuids['char_user_desc'],
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                    'value': [
                        0x65, 0x66, 0x67, 0x68, 0x69, 0x70, 0x71, 0x72, 0x73,
                        0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x80, 0x81, 0x82,
                        0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x90
                    ]
                }, {
                    'uuid': gatt_char_desc_uuids['char_fmt_uuid'],
                    'permissions':
                    gatt_descriptor['permission_read_encrypted_mitm'],
                    'value': [0x00, 0x01, 0x30, 0x01, 0x11, 0x31]
                }, {
                    'uuid': '0000d5d4-0000-0000-0123-456789abcdef',
                    'permissions': gatt_descriptor['permission_read'],
                    'value': [0x44]
                }]
            }]
        },
        {
            'uuid': '0000a00c-0000-0000-0123-456789abcdef',
            'type': gatt_service_types['primary'],
            'handles': 7,
            'characteristics': [{
                'uuid': '0000b009-0000-0000-0123-456789abcdef',
                'properties': gatt_characteristic['property_write'] |
                gatt_characteristic['property_extended_props'] |
                gatt_characteristic['property_read'],
                'permissions': gatt_characteristic['permission_write'] |
                gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x09],
                'descriptors': [{
                    'uuid': gatt_char_desc_uuids['char_ext_props'],
                    'permissions': gatt_descriptor['permission_read'],
                    'value': gatt_descriptor['enable_notification_value']
                }, {
                    'uuid': '0000d9d2-0000-0000-0123-456789abcdef',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                    'value': [0x22]
                }, {
                    'uuid': '0000d9d3-0000-0000-0123-456789abcdef',
                    'permissions': gatt_descriptor['permission_write'],
                    'value': [0x33]
                }]
            }]
        },
        {
            'uuid': '0000a00f-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'handles': 18,
            'characteristics': [
                {
                    'uuid': '0000b00e-0000-1000-8000-00805f9b34fb',
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': "Length is ",
                    'descriptors': [{
                        'uuid': gatt_char_desc_uuids['char_fmt_uuid'],
                        'permissions': gatt_descriptor['permission_read'],
                        'value': [0x19, 0x00, 0x00, 0x30, 0x01, 0x00, 0x00]
                    }]
                },
                {
                    'uuid': '0000b00f-0000-1000-8000-00805f9b34fb',
                    'properties': gatt_characteristic['property_read'] |
                    gatt_characteristic['property_write'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['byte'],
                    'value': [0x65],
                    'descriptors': [{
                        'uuid': gatt_char_desc_uuids['char_fmt_uuid'],
                        'permissions': gatt_descriptor['permission_read'],
                        'value': [0x04, 0x00, 0x01, 0x27, 0x01, 0x01, 0x00]
                    }]
                },
                {
                    'uuid': '0000b006-0000-1000-8000-00805f9b34fb',
                    'properties': gatt_characteristic['property_read'] |
                    gatt_characteristic['property_write'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['byte'],
                    'value': [0x34, 0x12],
                    'descriptors': [{
                        'uuid': gatt_char_desc_uuids['char_fmt_uuid'],
                        'permissions': gatt_descriptor['permission_read'],
                        'value': [0x06, 0x00, 0x10, 0x27, 0x01, 0x02, 0x00]
                    }]
                },
                {
                    'uuid': '0000b007-0000-1000-8000-00805f9b34fb',
                    'properties': gatt_characteristic['property_read'] |
                    gatt_characteristic['property_write'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['byte'],
                    'value': [0x04, 0x03, 0x02, 0x01],
                    'descriptors': [{
                        'uuid': gatt_char_desc_uuids['char_fmt_uuid'],
                        'permissions': gatt_descriptor['permission_read'],
                        'value': [0x08, 0x00, 0x17, 0x27, 0x01, 0x03, 0x00]
                    }]
                },
                {
                    'uuid': '0000b010-0000-1000-8000-00805f9b34fb',
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'],
                    'value_type': gatt_characteristic_value_format['byte'],
                    'value': [0x65, 0x34, 0x12, 0x04, 0x03, 0x02, 0x01],
                    'descriptors': [{
                        'uuid': gatt_char_desc_uuids['char_agreg_fmt'],
                        'permissions': gatt_descriptor['permission_read'],
                        'value': [0xa6, 0x00, 0xa9, 0x00, 0xac, 0x00]
                    }]
                },
                {
                    'uuid': '0000b011-0000-1000-8000-00805f9b34fb',
                    'properties': gatt_characteristic['write_type_signed']
                    |  #for some reason 0x40 is not working...
                    gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['byte'],
                    'value': [0x12]
                }
            ]
        },
        {
            'uuid': '0000a00c-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'handles': 30,
            'characteristics': [{
                'uuid': '0000b00a-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_read'],
                'permissions': gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x0a],
            }, {
                'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_read'] |
                gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': "111112222233333444445",
                'descriptors': [{
                    'uuid': '0000b012-0000-1000-8000-00805f9b34fb',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                    'value': [
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
                        0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11
                    ]
                }]
            }, {
                'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_read'] |
                gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': "2222233333444445555566",
                'descriptors': [{
                    'uuid': '0000b013-0000-1000-8000-00805f9b34fb',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                    'value': [
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
                        0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22
                    ]
                }]
            }, {
                'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_read'] |
                gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': "33333444445555566666777",
                'descriptors': [{
                    'uuid': '0000b014-0000-1000-8000-00805f9b34fb',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                    'value': [
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
                        0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22, 0x33
                    ]
                }]
            }, {
                'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_read'] |
                gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x11, 0x22, 0x33
                ],
                'descriptors': [{
                    'uuid': '0000b012-0000-1000-8000-00805f9b34fb',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                    'value': [
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
                        0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56,
                        0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
                        0x56, 0x78, 0x90, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                        0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33
                    ]
                }]
            }, {
                'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_read'] |
                gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x11, 0x22, 0x33, 0x44
                ],
                'descriptors': [{
                    'uuid': '0000b013-0000-1000-8000-00805f9b34fb',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                    'value': [
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
                        0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56,
                        0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
                        0x56, 0x78, 0x90, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                        0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44
                    ]
                }]
            }, {
                'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_read'] |
                gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x11, 0x22, 0x33, 0x44, 0x55
                ],
                'descriptors': [{
                    'uuid': '0000b014-0000-1000-8000-00805f9b34fb',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                    'value': [
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
                        0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56,
                        0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
                        0x56, 0x78, 0x90, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                        0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55
                    ]
                }]
            }, {
                'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_read'] |
                gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': "1111122222333334444455555666667777788888999",
                'descriptors': [{
                    'uuid': '0000b012-0000-1000-8000-00805f9b34fb',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                    'value': [
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
                        0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56,
                        0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
                        0x56, 0x78, 0x90, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                        0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33
                    ]
                }]
            }, {
                'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_read'] |
                gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': "22222333334444455555666667777788888999990000",
                'descriptors': [{
                    'uuid': '0000b013-0000-1000-8000-00805f9b34fb',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                    'value': [
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
                        0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56,
                        0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
                        0x56, 0x78, 0x90, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                        0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44
                    ]
                }]
            }, {
                'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                'properties': gatt_characteristic['property_read'] |
                gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': "333334444455555666667777788888999990000011111",
                'descriptors': [{
                    'uuid': '0000b014-0000-1000-8000-00805f9b34fb',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                    'value': [
                        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
                        0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56,
                        0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
                        0x56, 0x78, 0x90, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                        0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55
                    ]
                }]
            }]
        },
    ]
}

# Corresponds to the PTS defined LARGE_DB_2
LARGE_DB_2 = {
    'services': [
        {
            'uuid': '0000a00c-0000-0000-0123-456789abdcef',
            'type': gatt_service_types['primary'],
            'characteristics': [{
                'uuid': '0000b00a-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x0003,
                'properties': gatt_characteristic['property_read'],
                'permissions': gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x04],
            }, {
                'uuid': '0000b0002-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x0005,
                'properties': 0x0a,
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': '111112222233333444445',
            }, {
                'uuid': '0000b0002-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x0007,
                'properties': 0x0a,
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': '2222233333444445555566',
            }, {
                'uuid': '0000b0002-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x0009,
                'properties': 0x0a,
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': '33333444445555566666777',
            }, {
                'uuid': '0000b0002-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x000b,
                'properties': 0x0a0,
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': '1111122222333334444455555666667777788888999',
            }, {
                'uuid': '0000b0002-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x000d,
                'properties': 0x0a,
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': '22222333334444455555666667777788888999990000',
            }, {
                'uuid': '0000b0002-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x000f,
                'properties': 0x0a,
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': '333334444455555666667777788888999990000011111',
            }]
        },
        {
            'uuid': '0000a00c-0000-0000-0123-456789abcdef',
            'handles': 5,
            'type': gatt_service_types['primary'],
            'characteristics': [{
                'uuid': '0000b009-0000-0000-0123-456789abcdef',
                'instance_id': 0x0023,
                'properties': 0x8a,
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x09],
                'descriptors': [{
                    'uuid': '0000d9d2-0000-0000-0123-456789abcdef',
                    'permissions': gatt_descriptor['permission_read'] |
                    gatt_descriptor['permission_write'],
                    'value': [0x22]
                }, {
                    'uuid': '0000d9d3-0000-0000-0123-456789abcdef',
                    'permissions': gatt_descriptor['permission_write'],
                    'value': [0x33]
                }, {
                    'uuid': gatt_char_desc_uuids['char_ext_props'],
                    'permissions': gatt_descriptor['permission_write'],
                    'value': gatt_descriptor['enable_notification_value']
                }]
            }]
        },
        {
            'uuid': '0000a00b-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'characteristics': [{
                'uuid': '0000b007-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x0012,
                'properties': 0x0a,
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x04],
            }]
        },
    ]
}

DB_TEST = {
    'services': [{
        'uuid': '0000a00b-0000-1000-8000-00805f9b34fb',
        'type': gatt_service_types['primary'],
        'characteristics': [{
            'uuid': '0000b004-0000-1000-8000-00805f9b34fb',
            'properties': 0x02 | 0x08,
            'permissions': 0x10 | 0x01,
            'value_type': gatt_characteristic_value_format['byte'],
            'value': [0x01],
            'descriptors': [{
                'uuid': '0000b004-0000-1000-8000-00805f9b34fb',
                'permissions': gatt_descriptor['permission_read'] |
                gatt_descriptor['permission_write'],
                'value': [0x01] * 30
            }]
        }, ]
    }]
}

PTS_TEST2 = {
    'services': [{
        'uuid': '0000a00b-0000-1000-8000-00805f9b34fb',
        'type': gatt_service_types['primary'],
        'characteristics': [
            {
                'uuid': '000018ba-0000-1000-8000-00805f9b34fb',
                'properties': 0x02,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000060aa-0000-0000-0123-456789abcdef',
                'properties': 0x02,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '00000af2-0000-1000-8000-00805f9b34fb',
                'properties': 0x20,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000000af2-0000-1000-8000-00805f9b34fb',
                'properties': 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000004d5e-0000-1000-8000-00805f9b34fb',
                'properties': 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000001b44-0000-1000-8000-00805f9b34fb',
                'properties': 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000006b98-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08 | 0x10 | 0x04,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '00000247f-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '00000247f-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '00000247f-0000-1000-8000-00805f9b34fb',
                'properties': 0x00,
                'permissions': 0x00,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '00000247f-0000-1000-8000-00805f9b34fb',
                'properties': 0x02,
                'permissions': 0x10,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000000d62-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08 | 0x80,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000002e85-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000004a64-0000-0000-0123-456789abcdef',
                'properties': 0x02 | 0x08 | 0x80,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000005b4a-0000-1000-8000-00805f9b34fb',
                'properties': 0x02,
                'permissions': 0x10,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000001c81-0000-1000-8000-00805f9b34fb',
                'properties': 0x02,
                'permissions': 0x10,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000006b98-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000001b44-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000000c55-0000-1000-8000-00805f9b34fb',
                'properties': 0x02,
                'permissions': 0x10,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '0000014dd-0000-1000-8000-00805f9b34fb',
                'properties': 0x02,
                'permissions': 0x10,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000000c55-0000-1000-8000-00805f9b34fb',
                'properties': 0x02,
                'permissions': 0x10,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000000c55-0000-1000-8000-00805f9b34fb',
                'properties': 0x02,
                'permissions': 0x10,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000000c55-0000-1000-8000-00805f9b34fb',
                'properties': 0x02,
                'permissions': 0x10,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000000c55-0000-1000-8000-00805f9b34fb',
                'properties': 0x02,
                'permissions': 0x10,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '00000008f-0000-1000-8000-00805f9b34fb',
                'properties': 0x02,
                'permissions': 0x10,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000000af2-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x32
                ],
            },
            {
                'uuid': '000000af2-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x32
                ],
            },
            {
                'uuid': '000000af2-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x32
                ],
            },
            {
                'uuid': '000000af2-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x32
                ],
            },
            {
                'uuid': '000000af2-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x32
                ],
            },
            {
                'uuid': '000000af2-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x32
                ],
            },
            {
                'uuid': '000000af2-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x32
                ],
            },
            {
                'uuid': '000002aad-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000002ab0-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
            {
                'uuid': '000002ab3-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_512BYTES,
            },
        ]
    }]
}

PTS_TEST = {
    'services': [{
        'uuid': '0000a00b-0000-1000-8000-00805f9b34fb',
        'type': gatt_service_types['primary'],
        'characteristics': [
            {
                'uuid': '000018ba-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_25BYTES,
            },
            {
                'uuid': '000060aa-0000-1000-8000-00805f9b34fb',
                'properties': 0x02 | 0x08,
                'permissions': 0x10 | 0x01,
                'value_type': gatt_characteristic_value_format['string'],
                'value': STRING_25BYTES,
            },
        ]
    }]
}

# Corresponds to the PTS defined LARGE_DB_3
LARGE_DB_3 = {
    'services': [
        {
            'uuid': '0000a00b-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'characteristics': [
                {
                    'uuid': '0000b004-0000-1000-8000-00805f9b34fb',
                    'instance_id': 0x0003,
                    'properties': 0x0a,
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['byte'],
                    'value': [0x04],
                },
                {
                    'uuid': '0000b004-0000-1000-8000-00805f9b34fb',
                    'instance_id': 0x0013,
                    'properties': 0x10,
                    'permissions': 0x17,
                    'value_type': gatt_characteristic_value_format['byte'],
                    'value': [0x04],
                    'descriptors': [
                        {
                            'uuid': gatt_char_desc_uuids['char_ext_props'],
                            'permissions': gatt_descriptor['permission_read'] |
                            gatt_descriptor['permission_write'],
                            'value': [0x09]
                        },
                        {
                            'uuid': gatt_char_desc_uuids['char_user_desc'],
                            'permissions': gatt_descriptor['permission_read'] |
                            gatt_descriptor['permission_write'],
                            'value': [0x22]
                        },
                        {
                            'uuid': gatt_char_desc_uuids['client_char_cfg'],
                            'permissions': gatt_descriptor['permission_read'] |
                            gatt_descriptor['permission_write'],
                            'value': [0x01, 0x00]
                        },
                        {
                            'uuid': gatt_char_desc_uuids['server_char_cfg'],
                            'permissions': gatt_descriptor['permission_read'] |
                            gatt_descriptor['permission_write'],
                            'value': [0x22]
                        },
                        {
                            'uuid': gatt_char_desc_uuids['char_fmt_uuid'],
                            'permissions': gatt_descriptor['permission_read'] |
                            gatt_descriptor['permission_write'],
                            'value': [0x22]
                        },
                        {
                            'uuid': gatt_char_desc_uuids['char_agreg_fmt'],
                            'permissions': gatt_descriptor['permission_read'] |
                            gatt_descriptor['permission_write'],
                            'value': [0x22]
                        },
                        {
                            'uuid': gatt_char_desc_uuids['char_valid_range'],
                            'permissions': gatt_descriptor['permission_read'] |
                            gatt_descriptor['permission_write'],
                            'value': [0x22]
                        },
                        {
                            'uuid':
                            gatt_char_desc_uuids['external_report_reference'],
                            'permissions': gatt_descriptor['permission_read'] |
                            gatt_descriptor['permission_write'],
                            'value': [0x22]
                        },
                        {
                            'uuid': gatt_char_desc_uuids['report_reference'],
                            'permissions': gatt_descriptor['permission_read'] |
                            gatt_descriptor['permission_write'],
                            'value': [0x22]
                        },
                    ]
                },
                {
                    'uuid': gatt_char_types['service_changed'],
                    'instance_id': 0x0023,
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '333334444455555666667777788888999990000011111',
                },
                {
                    'uuid': gatt_char_types['appearance'],
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '333334444455555666667777788888999990000011111',
                },
                {
                    'uuid': gatt_char_types['peripheral_priv_flag'],
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '333334444455555666667777788888999990000011111',
                },
                {
                    'uuid': gatt_char_types['reconnection_address'],
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '333334444455555666667777788888999990000011111',
                },
                {
                    'uuid': gatt_char_types['system_id'],
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '333334444455555666667777788888999990000011111',
                },
                {
                    'uuid': gatt_char_types['model_number_string'],
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '333334444455555666667777788888999990000011111',
                },
                {
                    'uuid': gatt_char_types['serial_number_string'],
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '333334444455555666667777788888999990000011111',
                },
                {
                    'uuid': gatt_char_types['firmware_revision_string'],
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '333334444455555666667777788888999990000011111',
                },
                {
                    'uuid': gatt_char_types['hardware_revision_string'],
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '333334444455555666667777788888999990000011111',
                },
                {
                    'uuid': gatt_char_types['software_revision_string'],
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '333334444455555666667777788888999990000011111',
                },
                {
                    'uuid': gatt_char_types['manufacturer_name_string'],
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '333334444455555666667777788888999990000011111',
                },
                {
                    'uuid': gatt_char_types['pnp_id'],
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '333334444455555666667777788888999990000011111',
                },
            ]
        },
        {
            'uuid': '0000a00d-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['secondary'],
            'handles': 5,
            'characteristics': [{
                'uuid': '0000b00c-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x0023,
                'properties': gatt_characteristic['property_read'],
                'permissions': gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x0c],
            }, {
                'uuid': '0000b00b-0000-0000-0123-456789abcdef',
                'instance_id': 0x0025,
                'properties': gatt_characteristic['property_read'],
                'permissions': gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x0b],
            }]
        },
        {
            'uuid': '0000a00b-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'characteristics': [{
                'uuid': '0000b008-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x0032,
                'properties': gatt_characteristic['property_read'],
                'permissions': gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x08],
            }]
        },
        {
            'uuid': '0000a00b-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'characteristics': [{
                'uuid': '0000b007-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x0042,
                'properties': gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x07],
            }]
        },
        {
            'uuid': '0000a00b-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'characteristics': [{
                'uuid': '0000b006-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x0052,
                'properties': 0x3e,
                'permissions': gatt_characteristic['permission_write'] |
                gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x06],
            }]
        },
        {
            'uuid': '0000a00a-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'handles': 10,
            'characteristics': [{
                'uuid': '0000b001-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x0074,
                'properties': gatt_characteristic['property_read'],
                'permissions': gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x01],
            }, {
                'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x0076,
                'properties': 0x0a,
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['string'],
                'value': '11111222223333344444555556666677777888889999900000',
            }, {
                'uuid': '0000b003-0000-1000-8000-00805f9b34fb',
                'instance_id': 0x0078,
                'properties': gatt_characteristic['property_write'],
                'permissions': gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x03],
            }]
        },
        {
            'uuid': '0000a00c-0000-0000-0123-456789abcdef',
            'type': gatt_service_types['primary'],
            'handles': 10,
            'characteristics': [{
                'uuid': '0000b009-0000-0000-0123-456789abcdef',
                'instance_id': 0x0082,
                'properties': 0x8a,
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x09],
                'descriptors': [
                    {
                        'uuid': '0000b009-0000-0000-0123-456789abcdef',
                        'permissions': gatt_descriptor['permission_read'] |
                        gatt_descriptor['permission_write'],
                        'value': [0x09]
                    },
                    {
                        'uuid': '0000d9d2-0000-0000-0123-456789abcdef',
                        'permissions': gatt_descriptor['permission_read'] |
                        gatt_descriptor['permission_write'],
                        'value': [0x22]
                    },
                    {
                        'uuid': gatt_char_desc_uuids['char_ext_props'],
                        'permissions': gatt_descriptor['permission_read'],
                        'value': [0x01, 0x00]
                    },
                    {
                        'uuid': '0000d9d3-0000-0000-0123-456789abcdef',
                        'permissions': gatt_descriptor['permission_write'],
                        'value': [0x22]
                    },
                ]
            }]
        },
        {
            'uuid': '0000a00b-0000-0000-0123-456789abcdef',
            'type': gatt_service_types['primary'],
            'characteristics': [{
                'uuid': '0000b009-0000-0000-0123-456789abcdef',
                'instance_id': 0x0092,
                'properties': 0x8a,
                'permissions': gatt_characteristic['permission_read'] |
                gatt_characteristic['permission_write'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x05],
                'descriptors': [
                    {
                        'uuid': gatt_char_desc_uuids['char_user_desc'],
                        'permissions': gatt_descriptor['permission_read'] |
                        gatt_descriptor['permission_write'],
                        'value': [0] * 26
                    },
                    {
                        'uuid': gatt_char_desc_uuids['char_ext_props'],
                        'permissions': gatt_descriptor['permission_read'],
                        'value': [0x03, 0x00]
                    },
                    {
                        'uuid': '0000d5d4-0000-0000-0123-456789abcdef',
                        'permissions': gatt_descriptor['permission_read'],
                        'value': [0x44]
                    },
                    {
                        'uuid': gatt_char_desc_uuids['char_fmt_uuid'],
                        'permissions': gatt_descriptor['permission_read'],
                        'value': [0x04, 0x00, 0x01, 0x30, 0x01, 0x11, 0x31]
                    },
                ]
            }]
        },
        {
            'uuid': '0000a00c-0000-0000-0123-456789abcdef',
            'type': gatt_service_types['primary'],
            'characteristics': [
                {
                    'uuid': '0000b00a-0000-1000-8000-00805f9b34fb',
                    'instance_id': 0x00a2,
                    'properties': gatt_characteristic['property_read'],
                    'permissions': gatt_characteristic['permission_read'],
                    'value_type': gatt_characteristic_value_format['byte'],
                    'value': [0x0a],
                },
                {
                    'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                    'instance_id': 0x00a4,
                    'properties': 0x0a,
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '111112222233333444445',
                },
                {
                    'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                    'instance_id': 0x00a6,
                    'properties': 0x0a,
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '2222233333444445555566',
                },
                {
                    'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                    'instance_id': 0x00a8,
                    'properties': 0x0a,
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '33333444445555566666777',
                },
                {
                    'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                    'instance_id': 0x00aa,
                    'properties': 0x0a,
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '1111122222333334444455555666667777788888999',
                },
                {
                    'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                    'instance_id': 0x00ac,
                    'properties': 0x0a,
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '22222333334444455555666667777788888999990000',
                },
                {
                    'uuid': '0000b002-0000-1000-8000-00805f9b34fb',
                    'instance_id': 0x00ae,
                    'properties': 0x0a,
                    'permissions': gatt_characteristic['permission_read'] |
                    gatt_characteristic['permission_write'],
                    'value_type': gatt_characteristic_value_format['string'],
                    'value': '333334444455555666667777788888999990000011111',
                },
            ]
        },
        {
            'uuid': '0000a00e-0000-1000-8000-00805f9b34fb',
            'type': gatt_service_types['primary'],
            'characteristics': [{
                'uuid': '0000b00d-0000-1000-8000-00805f9b34fb',
                'instance_id': 0xffff,
                'properties': gatt_characteristic['property_read'],
                'permissions': gatt_characteristic['permission_read'],
                'value_type': gatt_characteristic_value_format['byte'],
                'value': [0x0d],
            }]
        },
    ]
}

TEST_DB_1 = {
    'services': [{
        'uuid': '0000180d-0000-1000-8000-00805f9b34fb',
        'type': gatt_service_types['primary'],
        'handles': 4,
        'characteristics': [{
            'uuid': '00002a29-0000-1000-8000-00805f9b34fb',
            'properties': gatt_characteristic['property_read'] |
            gatt_characteristic['property_write'],
            'permissions': gatt_characteristic['permission_read'] |
            gatt_characteristic['permission_write'],
            'value_type': gatt_characteristic_value_format['string'],
            'value': 'test',
            'instance_id': 0x002a,
            'descriptors': [{
                'uuid': gatt_char_desc_uuids['char_user_desc'],
                'permissions': gatt_descriptor['permission_read'],
                'value': [0x01]
            }]
        }]
    }]
}

TEST_DB_2 = {
    'services': [{
        'uuid': '0000180d-0000-1000-8000-00805f9b34fb',
        'type': gatt_service_types['primary'],
        'handles': 4,
        'characteristics': [{
            'uuid': '00002a29-0000-1000-8000-00805f9b34fb',
            'properties': gatt_characteristic['property_read'],
            'permissions':
            gatt_characteristic['permission_read_encrypted_mitm'],
            'value_type': gatt_characteristic_value_format['string'],
            'value': 'test',
            'instance_id': 0x002a,
        }, {
            'uuid': '00002a30-0000-1000-8000-00805f9b34fb',
            'properties': gatt_characteristic['property_read'],
            'permissions':
            gatt_characteristic['permission_read_encrypted_mitm'],
            'value_type': gatt_characteristic_value_format['string'],
            'value': 'test',
            'instance_id': 0x002b,
        }]
    }]
}

TEST_DB_3 = {
    'services': [{
        'uuid': '0000180d-0000-1000-8000-00805f9b34fb',
        'type': gatt_service_types['primary'],
        'handles': 4,
        'characteristics': [{
            'uuid': '00002a29-0000-1000-8000-00805f9b34fb',
            'properties': gatt_characteristic['property_read'] |
            gatt_characteristic['property_write'],
            'permissions': gatt_characteristic['permission_read'] |
            gatt_characteristic['permission_write'],
            'value_type': gatt_characteristic_value_format['string'],
            'value': 'test',
            'instance_id': 0x002a,
            'descriptors': [{
                'uuid': gatt_char_desc_uuids['char_user_desc'],
                'permissions': gatt_descriptor['permission_read'],
                'value': [0x01]
            }, {
                'uuid': '00002a20-0000-1000-8000-00805f9b34fb',
                'permissions': gatt_descriptor['permission_read'] |
                gatt_descriptor['permission_write'],
                'instance_id': 0x002c,
                'value': [0x01]
            }]
        }, {
            'uuid': '00002a30-0000-1000-8000-00805f9b34fb',
            'properties': gatt_characteristic['property_read'] |
            gatt_characteristic['property_write'],
            'permissions': gatt_characteristic['permission_read'] |
            gatt_characteristic['permission_write'],
            'value_type': gatt_characteristic_value_format['string'],
            'value': 'test',
            'instance_id': 0x002b,
        }]
    }]
}

TEST_DB_4 = {
    'services': [{
        'uuid': '0000180d-0000-1000-8000-00805f9b34fb',
        'type': gatt_service_types['primary'],
        'handles': 4,
        'characteristics': [{
            'uuid': '00002a29-0000-1000-8000-00805f9b34fb',
            'properties': gatt_characteristic['property_write_no_response'],
            'permissions': gatt_characteristic['permission_read'],
            'value_type': gatt_characteristic_value_format['string'],
            'value': "test",
            'instance_id': 0x002a,
            'descriptors': [{
                'uuid': gatt_char_desc_uuids['char_user_desc'],
                'permissions':
                gatt_descriptor['permission_read_encrypted_mitm'],
                'value': [0] * 512
            }]
        }]
    }]
}

TEST_DB_5 = {
    'services': [{
        'uuid': '0000180d-0000-1000-8000-00805f9b34fb',
        'type': gatt_service_types['primary'],
        'characteristics': [{
            'uuid': 'b2c83efa-34ca-11e6-ac61-9e71128cae77',
            'properties': gatt_characteristic['property_write'] |
            gatt_characteristic['property_read'] |
            gatt_characteristic['property_notify'],
            'permissions': gatt_characteristic['permission_read'] |
            gatt_characteristic['permission_write'],
            'value_type': gatt_characteristic_value_format['byte'],
            'value': [0x1],
            'instance_id': 0x002c,
            'descriptors': [{
                'uuid': '00002902-0000-1000-8000-00805f9b34fb',
                'permissions': gatt_descriptor['permission_read'] |
                gatt_descriptor['permission_write'],
            }]
        }]
    }]
}

TEST_DB_6 = {
    'services': [{
        'uuid': '0000180d-0000-1000-8000-00805f9b34fb',
        'type': gatt_service_types['primary'],
        'handles': 4,
        'characteristics': [{
            'uuid': '00002a29-0000-1000-8000-00805f9b34fb',
            'properties': gatt_characteristic['property_read'],
            'permissions': gatt_characteristic['permission_read'],
            'value_type': gatt_characteristic_value_format['string'],
            'value': 'test',
            'instance_id': 0x002a,
            'descriptors': [{
                'uuid': '00002a19-0000-1000-8000-00805f9b34fb',
                'permissions': gatt_descriptor['permission_read'],
                'value': [0x01] * 30
            }]
        }]
    }]
}

SIMPLE_READ_DESCRIPTOR = {
    'services': [{
        'uuid': '0000a00a-0000-1000-8000-00805f9b34fb',
        'type': gatt_service_types['primary'],
        'characteristics': [{
            'uuid': 'aa7edd5a-4d1d-4f0e-883a-d145616a1630',
            'properties': gatt_characteristic['property_read'],
            'permissions': gatt_characteristic['permission_read'],
            'instance_id': 0x002a,
            'value_type': gatt_characteristic_value_format['string'],
            'value': 'Test Database',
            'descriptors': [{
                'uuid': gatt_char_desc_uuids['client_char_cfg'],
                'permissions': gatt_descriptor['permission_read'],
            }]
        }]
    }]
}

CHARACTERISTIC_PROPERTY_WRITE_NO_RESPONSE = {
    'services': [{
        'uuid': '0000a00a-0000-1000-8000-00805f9b34fb',
        'type': gatt_service_types['primary'],
        'characteristics': [{
            'uuid': 'aa7edd5a-4d1d-4f0e-883a-d145616a1630',
            'properties': gatt_characteristic['property_write_no_response'],
            'permissions': gatt_characteristic['permission_write'] |
            gatt_characteristic['permission_read'],
            'instance_id': 0x0042,
            'value_type': gatt_characteristic_value_format['string'],
            'value': 'Test Database'
        }, {
            'uuid': 'aa7edd6a-4d1d-4f0e-883a-d145616a1630',
            'properties': gatt_characteristic['property_write_no_response'],
            'permissions': gatt_characteristic['permission_write'] |
            gatt_characteristic['permission_read'],
            'instance_id': 0x004d,
            'value_type': gatt_characteristic_value_format['string'],
            'value': 'Test Database'
        }]
    }]
}

GATT_SERVER_DB_MAPPING = {
    'LARGE_DB_1': LARGE_DB_1,
    'LARGE_DB_3': LARGE_DB_3,
    'INVALID_SMALL_DATABASE': INVALID_SMALL_DATABASE,
    'SIMPLE_READ_DESCRIPTOR': SIMPLE_READ_DESCRIPTOR,
    'CHARACTERISTIC_PROPERTY_WRITE_NO_RESPONSE':
    CHARACTERISTIC_PROPERTY_WRITE_NO_RESPONSE,
    'TEST_DB_1': TEST_DB_1,
    'TEST_DB_2': TEST_DB_2,
    'TEST_DB_3': TEST_DB_3,
    'TEST_DB_4': TEST_DB_4,
    'TEST_DB_5': TEST_DB_5,
    'LARGE_DB_3_PLUS': LARGE_DB_3,
    'DB_TEST': DB_TEST,
    'PTS_TEST': PTS_TEST,
    'PTS_TEST2': PTS_TEST2,
    'TEST_DB_6': TEST_DB_6,
}
