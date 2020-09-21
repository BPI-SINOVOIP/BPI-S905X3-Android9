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

from enum import Enum
from enum import IntEnum


class GattCbErr(Enum):
    CHAR_WRITE_REQ_ERR = "Characteristic Write Request event not found. Expected {}"
    CHAR_WRITE_ERR = "Characteristic Write event not found. Expected {}"
    DESC_WRITE_REQ_ERR = "Descriptor Write Request event not found. Expected {}"
    DESC_WRITE_ERR = "Descriptor Write event not found. Expected {}"
    CHAR_READ_ERR = "Characteristic Read event not found. Expected {}"
    CHAR_READ_REQ_ERR = "Characteristic Read Request not found. Expected {}"
    DESC_READ_ERR = "Descriptor Read event not found. Expected {}"
    DESC_READ_REQ_ERR = "Descriptor Read Request event not found. Expected {}"
    RD_REMOTE_RSSI_ERR = "Read Remote RSSI event not found. Expected {}"
    GATT_SERV_DISC_ERR = "GATT Services Discovered event not found. Expected {}"
    SERV_ADDED_ERR = "Service Added event not found. Expected {}"
    MTU_CHANGED_ERR = "MTU Changed event not found. Expected {}"
    MTU_SERV_CHANGED_ERR = "MTU Server Changed event not found. Expected {}"
    GATT_CONN_CHANGE_ERR = "GATT Connection Changed event not found. Expected {}"
    CHAR_CHANGE_ERR = "GATT Characteristic Changed event not fond. Expected {}"
    PHY_READ_ERR = "Phy Read event not fond. Expected {}"
    PHY_UPDATE_ERR = "Phy Update event not fond. Expected {}"
    EXEC_WRITE_ERR = "GATT Execute Write event not found. Expected {}"


class GattCbStrings(Enum):
    CHAR_WRITE_REQ = "GattServer{}onCharacteristicWriteRequest"
    EXEC_WRITE = "GattServer{}onExecuteWrite"
    CHAR_WRITE = "GattConnect{}onCharacteristicWrite"
    DESC_WRITE_REQ = "GattServer{}onDescriptorWriteRequest"
    DESC_WRITE = "GattConnect{}onDescriptorWrite"
    CHAR_READ = "GattConnect{}onCharacteristicRead"
    CHAR_READ_REQ = "GattServer{}onCharacteristicReadRequest"
    DESC_READ = "GattConnect{}onDescriptorRead"
    DESC_READ_REQ = "GattServer{}onDescriptorReadRequest"
    RD_REMOTE_RSSI = "GattConnect{}onReadRemoteRssi"
    GATT_SERV_DISC = "GattConnect{}onServicesDiscovered"
    SERV_ADDED = "GattServer{}onServiceAdded"
    MTU_CHANGED = "GattConnect{}onMtuChanged"
    MTU_SERV_CHANGED = "GattServer{}onMtuChanged"
    GATT_CONN_CHANGE = "GattConnect{}onConnectionStateChange"
    CHAR_CHANGE = "GattConnect{}onCharacteristicChanged"
    PHY_READ = "GattConnect{}onPhyRead"
    PHY_UPDATE = "GattConnect{}onPhyUpdate"
    SERV_PHY_READ = "GattServer{}onPhyRead"
    SERV_PHY_UPDATE = "GattServer{}onPhyUpdate"


class GattEvent(Enum):
    CHAR_WRITE_REQ = {
        "evt": GattCbStrings.CHAR_WRITE_REQ.value,
        "err": GattCbErr.CHAR_WRITE_REQ_ERR.value
    }
    EXEC_WRITE = {
        "evt": GattCbStrings.EXEC_WRITE.value,
        "err": GattCbErr.EXEC_WRITE_ERR.value
    }
    CHAR_WRITE = {
        "evt": GattCbStrings.CHAR_WRITE.value,
        "err": GattCbErr.CHAR_WRITE_ERR.value
    }
    DESC_WRITE_REQ = {
        "evt": GattCbStrings.DESC_WRITE_REQ.value,
        "err": GattCbErr.DESC_WRITE_REQ_ERR.value
    }
    DESC_WRITE = {
        "evt": GattCbStrings.DESC_WRITE.value,
        "err": GattCbErr.DESC_WRITE_ERR.value
    }
    CHAR_READ = {
        "evt": GattCbStrings.CHAR_READ.value,
        "err": GattCbErr.CHAR_READ_ERR.value
    }
    CHAR_READ_REQ = {
        "evt": GattCbStrings.CHAR_READ_REQ.value,
        "err": GattCbErr.CHAR_READ_REQ_ERR.value
    }
    DESC_READ = {
        "evt": GattCbStrings.DESC_READ.value,
        "err": GattCbErr.DESC_READ_ERR.value
    }
    DESC_READ_REQ = {
        "evt": GattCbStrings.DESC_READ_REQ.value,
        "err": GattCbErr.DESC_READ_REQ_ERR.value
    }
    RD_REMOTE_RSSI = {
        "evt": GattCbStrings.RD_REMOTE_RSSI.value,
        "err": GattCbErr.RD_REMOTE_RSSI_ERR.value
    }
    GATT_SERV_DISC = {
        "evt": GattCbStrings.GATT_SERV_DISC.value,
        "err": GattCbErr.GATT_SERV_DISC_ERR.value
    }
    SERV_ADDED = {
        "evt": GattCbStrings.SERV_ADDED.value,
        "err": GattCbErr.SERV_ADDED_ERR.value
    }
    MTU_CHANGED = {
        "evt": GattCbStrings.MTU_CHANGED.value,
        "err": GattCbErr.MTU_CHANGED_ERR.value
    }
    GATT_CONN_CHANGE = {
        "evt": GattCbStrings.GATT_CONN_CHANGE.value,
        "err": GattCbErr.GATT_CONN_CHANGE_ERR.value
    }
    CHAR_CHANGE = {
        "evt": GattCbStrings.CHAR_CHANGE.value,
        "err": GattCbErr.CHAR_CHANGE_ERR.value
    }
    PHY_READ = {
        "evt": GattCbStrings.PHY_READ.value,
        "err": GattCbErr.PHY_READ_ERR.value
    }
    PHY_UPDATE = {
        "evt": GattCbStrings.PHY_UPDATE.value,
        "err": GattCbErr.PHY_UPDATE_ERR.value
    }
    SERV_PHY_READ = {
        "evt": GattCbStrings.SERV_PHY_READ.value,
        "err": GattCbErr.PHY_READ_ERR.value
    }
    SERV_PHY_UPDATE = {
        "evt": GattCbStrings.SERV_PHY_UPDATE.value,
        "err": GattCbErr.PHY_UPDATE_ERR.value
    }


class GattConnectionState(IntEnum):
    STATE_DISCONNECTED = 0
    STATE_CONNECTING = 1
    STATE_CONNECTED = 2
    STATE_DISCONNECTING = 3


class GattCharacteristic(Enum):
    PROPERTY_BROADCAST = 0x01
    PROPERTY_READ = 0x02
    PROPERTY_WRITE_NO_RESPONSE = 0x04
    PROPERTY_WRITE = 0x08
    PROPERTY_NOTIFY = 0x10
    PROPERTY_INDICATE = 0x20
    PROPERTY_SIGNED_WRITE = 0x40
    PROPERTY_EXTENDED_PROPS = 0x80
    PERMISSION_READ = 0x01
    PERMISSION_READ_ENCRYPTED = 0x02
    PERMISSION_READ_ENCRYPTED_MITM = 0x04
    PERMISSION_WRITE = 0x10
    PERMISSION_WRITE_ENCRYPTED = 0x20
    PERMISSION_WRITE_ENCRYPTED_MITM = 0x40
    PERMISSION_WRITE_SIGNED = 0x80
    PERMISSION_WRITE_SIGNED_MITM = 0x100
    WRITE_TYPE_DEFAULT = 0x02
    WRITE_TYPE_NO_RESPONSE = 0x01
    WRITE_TYPE_SIGNED = 0x04
    FORMAT_UINT8 = 0x11
    FORMAT_UINT16 = 0x12
    FORMAT_UINT32 = 0x14
    FORMAT_SINT8 = 0x21
    FORMAT_SINT16 = 0x22
    FORMAT_SINT32 = 0x24
    FORMAT_SFLOAT = 0x32
    FORMAT_FLOAT = 0x34


class GattDescriptor(Enum):
    ENABLE_NOTIFICATION_VALUE = [0x01, 0x00]
    ENABLE_INDICATION_VALUE = [0x02, 0x00]
    DISABLE_NOTIFICATION_VALUE = [0x00, 0x00]
    PERMISSION_READ = 0x01
    PERMISSION_READ_ENCRYPTED = 0x02
    PERMISSION_READ_ENCRYPTED_MITM = 0x04
    PERMISSION_WRITE = 0x10
    PERMISSION_WRITE_ENCRYPTED = 0x20
    PERMISSION_WRITE_ENCRYPTED_MITM = 0x40
    PERMISSION_WRITE_SIGNED = 0x80
    PERMISSION_WRITE_SIGNED_MITM = 0x100


class GattCharDesc(Enum):
    GATT_CHARAC_EXT_PROPER_UUID = '00002900-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_USER_DESC_UUID = '00002901-0000-1000-8000-00805f9b34fb'
    GATT_CLIENT_CHARAC_CFG_UUID = '00002902-0000-1000-8000-00805f9b34fb'
    GATT_SERVER_CHARAC_CFG_UUID = '00002903-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_FMT_UUID = '00002904-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_AGREG_FMT_UUID = '00002905-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_VALID_RANGE_UUID = '00002906-0000-1000-8000-00805f9b34fb'
    GATT_EXTERNAL_REPORT_REFERENCE = '00002907-0000-1000-8000-00805f9b34fb'
    GATT_REPORT_REFERENCE = '00002908-0000-1000-8000-00805f9b34fb'


class GattCharTypes(Enum):
    GATT_CHARAC_DEVICE_NAME = '00002a00-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_APPEARANCE = '00002a01-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_PERIPHERAL_PRIV_FLAG = '00002a02-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_RECONNECTION_ADDRESS = '00002a03-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_PERIPHERAL_PREF_CONN = '00002a04-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_SERVICE_CHANGED = '00002a05-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_SYSTEM_ID = '00002a23-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_MODEL_NUMBER_STRING = '00002a24-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_SERIAL_NUMBER_STRING = '00002a25-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_FIRMWARE_REVISION_STRING = '00002a26-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_HARDWARE_REVISION_STRING = '00002a27-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_SOFTWARE_REVISION_STRING = '00002a28-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_MANUFACTURER_NAME_STRING = '00002a29-0000-1000-8000-00805f9b34fb'
    GATT_CHARAC_PNP_ID = '00002a50-0000-1000-8000-00805f9b34fb'


class GattCharacteristicAttrLength(Enum):
    MTU_ATTR_1 = 1
    MTU_ATTR_2 = 3
    MTU_ATTR_3 = 15


class CharacteristicValueFormat(Enum):
    STRING = 0x1
    BYTE = 0x2
    FORMAT_SINT8 = 0x21
    FORMAT_UINT8 = 0x11
    FORMAT_SINT16 = 0x22
    FORMAT_UINT16 = 0x12
    FORMAT_SINT32 = 0x24
    FORMAT_UINT32 = 0x14


class GattService(IntEnum):
    SERVICE_TYPE_PRIMARY = 0
    SERVICE_TYPE_SECONDARY = 1


class GattConnectionPriority(IntEnum):
    CONNECTION_PRIORITY_BALANCED = 0
    CONNECTION_PRIORITY_HIGH = 1
    CONNECTION_PRIORITY_LOW_POWER = 2


class MtuSize(IntEnum):
    MIN = 23
    MAX = 217


class GattCharacteristicAttrLength(IntEnum):
    MTU_ATTR_1 = 1
    MTU_ATTR_2 = 3
    MTU_ATTR_3 = 15


class BluetoothGatt(Enum):
    GATT_SUCCESS = 0
    GATT_FAILURE = 0x101


class GattTransport(IntEnum):
    TRANSPORT_AUTO = 0x00
    TRANSPORT_BREDR = 0x01
    TRANSPORT_LE = 0x02


class GattPhy(IntEnum):
    PHY_LE_1M = 1
    PHY_LE_2M = 2
    PHY_LE_CODED = 3


class GattPhyMask(IntEnum):
    PHY_LE_1M_MASK = 1
    PHY_LE_2M_MASK = 2
    PHY_LE_CODED_MASK = 4


# TODO Decide whether to continue with Enums or move to dictionaries
GattServerResponses = {
    "GATT_SUCCESS": 0x0,
    "GATT_FAILURE": 0x1,
    "GATT_READ_NOT_PERMITTED": 0x2,
    "GATT_WRITE_NOT_PERMITTED": 0x3,
    "GATT_INVALID_PDU": 0x4,
    "GATT_INSUFFICIENT_AUTHENTICATION": 0x5,
    "GATT_REQUEST_NOT_SUPPORTED": 0x6,
    "GATT_INVALID_OFFSET": 0x7,
    "GATT_INSUFFICIENT_AUTHORIZATION": 0x8,
    "GATT_INVALID_ATTRIBUTE_LENGTH": 0xD,
    "GATT_INSUFFICIENT_ENCRYPTION": 0xF,
    "GATT_CONNECTION_CONGESTED": 0x8F,
    "GATT_13_ERR": 0x13,
    "GATT_12_ERR": 0x12,
    "GATT_0C_ERR": 0x0C,
    "GATT_16": 0x16
}
