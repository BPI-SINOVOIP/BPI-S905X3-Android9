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


class BluetoothScanModeType(IntEnum):
    STATE_OFF = -1
    SCAN_MODE_NONE = 0
    SCAN_MODE_CONNECTABLE = 1
    SCAN_MODE_CONNECTABLE_DISCOVERABLE = 3


class BluetoothAdapterState(IntEnum):
    STATE_OFF = 10
    STATE_TURNING_ON = 11
    STATE_ON = 12
    STATE_TURNING_OFF = 13
    STATE_BLE_TURNING_ON = 14
    STATE_BLE_ON = 15
    STATE_BLE_TURNING_OFF = 16


class BluetoothProfile(IntEnum):
    # Should be kept in sync with BluetoothProfile.java
    HEADSET = 1
    A2DP = 2
    HEALTH = 3
    INPUT_DEVICE = 4
    PAN = 5
    PBAP_SERVER = 6
    GATT = 7
    GATT_SERVER = 8
    MAP = 9
    SAP = 10
    A2DP_SINK = 11
    AVRCP_CONTROLLER = 12
    HEADSET_CLIENT = 16
    PBAP_CLIENT = 17
    MAP_MCE = 18


class RfcommUuid(Enum):
    DEFAULT_UUID = "457807c0-4897-11df-9879-0800200c9a66"
    BASE_UUID = "00000000-0000-1000-8000-00805F9B34FB"
    SDP = "00000001-0000-1000-8000-00805F9B34FB"
    UDP = "00000002-0000-1000-8000-00805F9B34FB"
    RFCOMM = "00000003-0000-1000-8000-00805F9B34FB"
    TCP = "00000004-0000-1000-8000-00805F9B34FB"
    TCS_BIN = "00000005-0000-1000-8000-00805F9B34FB"
    TCS_AT = "00000006-0000-1000-8000-00805F9B34FB"
    ATT = "00000007-0000-1000-8000-00805F9B34FB"
    OBEX = "00000008-0000-1000-8000-00805F9B34FB"
    IP = "00000009-0000-1000-8000-00805F9B34FB"
    FTP = "0000000A-0000-1000-8000-00805F9B34FB"
    HTTP = "0000000C-0000-1000-8000-00805F9B34FB"
    WSP = "0000000E-0000-1000-8000-00805F9B34FB"
    BNEP = "0000000F-0000-1000-8000-00805F9B34FB"
    UPNP = "00000010-0000-1000-8000-00805F9B34FB"
    HIDP = "00000011-0000-1000-8000-00805F9B34FB"
    HARDCOPY_CONTROL_CHANNEL = "00000012-0000-1000-8000-00805F9B34FB"
    HARDCOPY_DATA_CHANNEL = "00000014-0000-1000-8000-00805F9B34FB"
    HARDCOPY_NOTIFICATION = "00000016-0000-1000-8000-00805F9B34FB"
    AVCTP = "00000017-0000-1000-8000-00805F9B34FB"
    AVDTP = "00000019-0000-1000-8000-00805F9B34FB"
    CMTP = "0000001B-0000-1000-8000-00805F9B34FB"
    MCAP_CONTROL_CHANNEL = "0000001E-0000-1000-8000-00805F9B34FB"
    MCAP_DATA_CHANNEL = "0000001F-0000-1000-8000-00805F9B34FB"
    L2CAP = "00000100-0000-1000-8000-00805F9B34FB"


class BluetoothProfileState(Enum):
    # Should be kept in sync with BluetoothProfile#STATE_* constants.
    STATE_DISCONNECTED = 0
    STATE_CONNECTING = 1
    STATE_CONNECTED = 2
    STATE_DISCONNECTING = 3


class BluetoothAccessLevel(Enum):
    # Access Levels from BluetoothDevice.
    ACCESS_ALLOWED = 1
    ACCESS_DENIED = 2


class BluetoothPriorityLevel(Enum):
    # Priority levels as defined in BluetoothProfile.java.
    PRIORITY_AUTO_CONNECT = 1000
    PRIORITY_ON = 100
    PRIORITY_OFF = 0
    PRIORITY_UNDEFINED = -1
