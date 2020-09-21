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

### Generic Constants Begin ###

bt_default_timeout = 15
default_rfcomm_timeout_ms = 10000
default_bluetooth_socket_timeout_ms = 10000
pan_connect_timeout = 5
bt_discovery_timeout = 3
small_timeout = 0.0001

# Time delay (in seconds) at the end of each LE CoC Test to give sufficient time
# for the ACL LE link to be disconnected. The ACL link stays connected after
# L2CAP disconnects.  An example of the timeout is L2CAP_LINK_INACTIVITY_TOUT.
# This delay must be greater than the maximum of these timeouts.
# TODO: Investigate the use of broadcast intent
# BluetoothDevice.ACTION_ACL_DISCONNECTED to replace this delay method.
l2cap_max_inactivity_delay_after_disconnect = 5

# LE specifications related constants
le_connection_interval_time_step_ms = 1.25
le_default_supervision_timeout = 2000
default_le_data_length = 23
default_le_connection_interval_ms = 30
le_connection_event_time_step_ms = 0.625

# Headers of LE L2CAP Connection-oriented Channels. See section 3.4, Vol 3, Part A, Version 5.0.
l2cap_header_size = 4
l2cap_coc_sdu_length_field_size = 2
l2cap_coc_header_size = l2cap_header_size + l2cap_coc_sdu_length_field_size

java_integer = {"min": -2147483648, "max": 2147483647}

btsnoop_log_path_on_device = "/data/misc/bluetooth/logs/btsnoop_hci.log"
btsnoop_last_log_path_on_device = \
    "/data/misc/bluetooth/logs/btsnoop_hci.log.last"
pairing_variant_passkey_confirmation = 2

# Callback strings
scan_result = "BleScan{}onScanResults"
scan_failed = "BleScan{}onScanFailed"
batch_scan_result = "BleScan{}onBatchScanResult"
adv_fail = "BleAdvertise{}onFailure"
adv_succ = "BleAdvertise{}onSuccess"
bluetooth_off = "BluetoothStateChangedOff"
bluetooth_on = "BluetoothStateChangedOn"
mtu_changed = "GattConnect{}onMtuChanged"
advertising_set_started = "AdvertisingSet{}onAdvertisingSetStarted"
advertising_set_stopped = "AdvertisingSet{}onAdvertisingSetStopped"
advertising_set_on_own_address_read = "AdvertisingSet{}onOwnAddressRead"
advertising_set_enabled = "AdvertisingSet{}onAdvertisingEnabled"
advertising_set_data_set = "AdvertisingSet{}onAdvertisingDataSet"
advertising_set_scan_response_set = "AdvertisingSet{}onScanResponseDataSet"
advertising_set_parameters_update = \
    "AdvertisingSet{}onAdvertisingParametersUpdated"
advertising_set_periodic_parameters_updated = \
    "AdvertisingSet{}onPeriodicAdvertisingParametersUpdated"
advertising_set_periodic_data_set = \
    "AdvertisingSet{}onPeriodicAdvertisingDataSet"
advertising_set_periodic_enable = "AdvertisingSet{}onPeriodicAdvertisingEnable"
bluetooth_profile_connection_state_changed = \
    "BluetoothProfileConnectionStateChanged"
bluetooth_le_on = "BleStateChangedOn"
bluetooth_le_off = "BleStateChangedOff"
# End Callback Strings

batch_scan_not_supported_list = [
    "Nexus 4",
    "Nexus 5",
    "Nexus 7",
]

### Generic Constants End ###

### Bluetooth Constants Begin ###

# rfcomm test uuids
rfcomm_secure_uuid = "fa87c0d0-afac-11de-8a39-0800200c9a66"
rfcomm_insecure_uuid = "8ce255c0-200a-11e0-ac64-0800200c9a66"

# bluetooth socket connection test uuid
bluetooth_socket_conn_test_uuid = "12345678-1234-5678-9abc-123456789abc"

# Bluetooth Adapter Scan Mode Types
bt_scan_mode_types = {
    "state_off": -1,
    "none": 0,
    "connectable": 1,
    "connectable_discoverable": 3
}

# Bluetooth Adapter State Constants
bt_adapter_states = {
    "off": 10,
    "turning_on": 11,
    "on": 12,
    "turning_off": 13,
    "ble_turning_on": 14,
    "ble_on": 15,
    "ble_turning_off": 16
}

# Should be kept in sync with BluetoothProfile.java
bt_profile_constants = {
    "headset": 1,
    "a2dp": 2,
    "health": 3,
    "input_device": 4,
    "pan": 5,
    "pbap_server": 6,
    "gatt": 7,
    "gatt_server": 8,
    "map": 9,
    "sap": 10,
    "a2dp_sink": 11,
    "avrcp_controller": 12,
    "headset_client": 16,
    "pbap_client": 17,
    "map_mce": 18
}

# Bluetooth RFCOMM UUIDs as defined by the SIG
bt_rfcomm_uuids = {
    "default_uuid": "457807c0-4897-11df-9879-0800200c9a66",
    "base_uuid": "00000000-0000-1000-8000-00805F9B34FB",
    "sdp": "00000001-0000-1000-8000-00805F9B34FB",
    "udp": "00000002-0000-1000-8000-00805F9B34FB",
    "rfcomm": "00000003-0000-1000-8000-00805F9B34FB",
    "tcp": "00000004-0000-1000-8000-00805F9B34FB",
    "tcs_bin": "00000005-0000-1000-8000-00805F9B34FB",
    "tcs_at": "00000006-0000-1000-8000-00805F9B34FB",
    "att": "00000007-0000-1000-8000-00805F9B34FB",
    "obex": "00000008-0000-1000-8000-00805F9B34FB",
    "ip": "00000009-0000-1000-8000-00805F9B34FB",
    "ftp": "0000000A-0000-1000-8000-00805F9B34FB",
    "http": "0000000C-0000-1000-8000-00805F9B34FB",
    "wsp": "0000000E-0000-1000-8000-00805F9B34FB",
    "bnep": "0000000F-0000-1000-8000-00805F9B34FB",
    "upnp": "00000010-0000-1000-8000-00805F9B34FB",
    "hidp": "00000011-0000-1000-8000-00805F9B34FB",
    "hardcopy_control_channel": "00000012-0000-1000-8000-00805F9B34FB",
    "hardcopy_data_channel": "00000014-0000-1000-8000-00805F9B34FB",
    "hardcopy_notification": "00000016-0000-1000-8000-00805F9B34FB",
    "avctp": "00000017-0000-1000-8000-00805F9B34FB",
    "avdtp": "00000019-0000-1000-8000-00805F9B34FB",
    "cmtp": "0000001B-0000-1000-8000-00805F9B34FB",
    "mcap_control_channel": "0000001E-0000-1000-8000-00805F9B34FB",
    "mcap_data_channel": "0000001F-0000-1000-8000-00805F9B34FB",
    "l2cap": "00000100-0000-1000-8000-00805F9B34FB"
}

# Should be kept in sync with BluetoothProfile#STATE_* constants.
bt_profile_states = {
    "disconnected": 0,
    "connecting": 1,
    "connected": 2,
    "disconnecting": 3
}

# Access Levels from BluetoothDevice.
bt_access_levels = {"access_allowed": 1, "access_denied": 2}

# Priority levels as defined in BluetoothProfile.java.
bt_priority_levels = {
    "auto_connect": 1000,
    "on": 100,
    "off": 0,
    "undefined": -1
}

# Bluetooth HID constants.
hid_connection_timeout = 5

# Bluetooth HID EventFacade constants.
hid_on_set_report_event = "onSetReport"
hid_on_get_report_event = "onGetReport"
hid_on_set_protocol_event = "onSetProtocol"
hid_on_intr_data_event = "onInterruptData"
hid_on_virtual_cable_unplug_event = "onVirtualCableUnplug"
hid_id_keyboard = 1
hid_id_mouse = 2
hid_default_event_timeout = 15
hid_default_set_report_payload = "Haha"

### Bluetooth Constants End ###

### Bluetooth Low Energy Constants Begin ###

# Bluetooth Low Energy scan callback types
ble_scan_settings_callback_types = {
    "all_matches": 1,
    "first_match": 2,
    "match_lost": 4,
    "found_and_lost": 6
}

# Bluetooth Low Energy scan settings match mode
ble_scan_settings_match_modes = {"aggresive": 1, "sticky": 2}

# Bluetooth Low Energy scan settings match nums
ble_scan_settings_match_nums = {"one": 1, "few": 2, "max": 3}

# Bluetooth Low Energy scan settings result types
ble_scan_settings_result_types = {"full": 0, "abbreviated": 1}

# Bluetooth Low Energy scan settings mode
ble_scan_settings_modes = {
    "opportunistic": -1,
    "low_power": 0,
    "balanced": 1,
    "low_latency": 2
}

# Bluetooth Low Energy scan settings report delay millis
ble_scan_settings_report_delay_milli_seconds = {
    "min": 0,
    "max": 9223372036854775807
}

# Bluetooth Low Energy scan settings phy
ble_scan_settings_phys = {"1m": 1, "coded": 3, "all_supported": 255}

# Bluetooth Low Energy advertise settings types
ble_advertise_settings_types = {"non_connectable": 0, "connectable": 1}

# Bluetooth Low Energy advertise settings modes
ble_advertise_settings_modes = {
    "low_power": 0,
    "balanced": 1,
    "low_latency": 2
}

# Bluetooth Low Energy advertise settings tx power
ble_advertise_settings_tx_powers = {
    "ultra_low": 0,
    "low": 1,
    "medium": 2,
    "high": 3
}

# Bluetooth Low Energy service uuids for specific devices
ble_uuids = {
    "p_service": "0000feef-0000-1000-8000-00805f9b34fb",
    "hr_service": "0000180d-0000-1000-8000-00805f9b34fb"
}

# Bluetooth Low Energy advertising error codes
ble_advertise_error_code = {
    "data_too_large": 1,
    "too_many_advertisers": 2,
    "advertisement_already_started": 3,
    "bluetooth_internal_failure": 4,
    "feature_not_supported": 5
}

### Bluetooth Low Energy Constants End ###

### Bluetooth GATT Constants Begin ###

# Gatt Callback error messages
gatt_cb_err = {
    "char_write_req_err":
    "Characteristic Write Request event not found. Expected {}",
    "char_write_err":
    "Characteristic Write event not found. Expected {}",
    "desc_write_req_err":
    "Descriptor Write Request event not found. Expected {}",
    "desc_write_err":
    "Descriptor Write event not found. Expected {}",
    "char_read_err":
    "Characteristic Read event not found. Expected {}",
    "char_read_req_err":
    "Characteristic Read Request not found. Expected {}",
    "desc_read_err":
    "Descriptor Read event not found. Expected {}",
    "desc_read_req_err":
    "Descriptor Read Request event not found. Expected {}",
    "rd_remote_rssi_err":
    "Read Remote RSSI event not found. Expected {}",
    "gatt_serv_disc_err":
    "GATT Services Discovered event not found. Expected {}",
    "serv_added_err":
    "Service Added event not found. Expected {}",
    "mtu_changed_err":
    "MTU Changed event not found. Expected {}",
    "mtu_serv_changed_err":
    "MTU Server Changed event not found. Expected {}",
    "gatt_conn_changed_err":
    "GATT Connection Changed event not found. Expected {}",
    "char_change_err":
    "GATT Characteristic Changed event not fond. Expected {}",
    "phy_read_err":
    "Phy Read event not fond. Expected {}",
    "phy_update_err":
    "Phy Update event not fond. Expected {}",
    "exec_write_err":
    "GATT Execute Write event not found. Expected {}"
}

# GATT callback strings as defined in GattClientFacade.java and
# GattServerFacade.java implemented callbacks.
gatt_cb_strings = {
    "char_write_req": "GattServer{}onCharacteristicWriteRequest",
    "exec_write": "GattServer{}onExecuteWrite",
    "char_write": "GattConnect{}onCharacteristicWrite",
    "desc_write_req": "GattServer{}onDescriptorWriteRequest",
    "desc_write": "GattConnect{}onDescriptorWrite",
    "char_read": "GattConnect{}onCharacteristicRead",
    "char_read_req": "GattServer{}onCharacteristicReadRequest",
    "desc_read": "GattConnect{}onDescriptorRead",
    "desc_read_req": "GattServer{}onDescriptorReadRequest",
    "rd_remote_rssi": "GattConnect{}onReadRemoteRssi",
    "gatt_serv_disc": "GattConnect{}onServicesDiscovered",
    "serv_added": "GattServer{}onServiceAdded",
    "mtu_changed": "GattConnect{}onMtuChanged",
    "mtu_serv_changed": "GattServer{}onMtuChanged",
    "gatt_conn_change": "GattConnect{}onConnectionStateChange",
    "char_change": "GattConnect{}onCharacteristicChanged",
    "phy_read": "GattConnect{}onPhyRead",
    "phy_update": "GattConnect{}onPhyUpdate",
    "serv_phy_read": "GattServer{}onPhyRead",
    "serv_phy_update": "GattServer{}onPhyUpdate",
}

# GATT event dictionary of expected callbacks and errors.
gatt_event = {
    "char_write_req": {
        "evt": gatt_cb_strings["char_write_req"],
        "err": gatt_cb_err["char_write_req_err"]
    },
    "exec_write": {
        "evt": gatt_cb_strings["exec_write"],
        "err": gatt_cb_err["exec_write_err"]
    },
    "char_write": {
        "evt": gatt_cb_strings["char_write"],
        "err": gatt_cb_err["char_write_err"]
    },
    "desc_write_req": {
        "evt": gatt_cb_strings["desc_write_req"],
        "err": gatt_cb_err["desc_write_req_err"]
    },
    "desc_write": {
        "evt": gatt_cb_strings["desc_write"],
        "err": gatt_cb_err["desc_write_err"]
    },
    "char_read": {
        "evt": gatt_cb_strings["char_read"],
        "err": gatt_cb_err["char_read_err"]
    },
    "char_read_req": {
        "evt": gatt_cb_strings["char_read_req"],
        "err": gatt_cb_err["char_read_req_err"]
    },
    "desc_read": {
        "evt": gatt_cb_strings["desc_read"],
        "err": gatt_cb_err["desc_read_err"]
    },
    "desc_read_req": {
        "evt": gatt_cb_strings["desc_read_req"],
        "err": gatt_cb_err["desc_read_req_err"]
    },
    "rd_remote_rssi": {
        "evt": gatt_cb_strings["rd_remote_rssi"],
        "err": gatt_cb_err["rd_remote_rssi_err"]
    },
    "gatt_serv_disc": {
        "evt": gatt_cb_strings["gatt_serv_disc"],
        "err": gatt_cb_err["gatt_serv_disc_err"]
    },
    "serv_added": {
        "evt": gatt_cb_strings["serv_added"],
        "err": gatt_cb_err["serv_added_err"]
    },
    "mtu_changed": {
        "evt": gatt_cb_strings["mtu_changed"],
        "err": gatt_cb_err["mtu_changed_err"]
    },
    "gatt_conn_change": {
        "evt": gatt_cb_strings["gatt_conn_change"],
        "err": gatt_cb_err["gatt_conn_changed_err"]
    },
    "char_change": {
        "evt": gatt_cb_strings["char_change"],
        "err": gatt_cb_err["char_change_err"]
    },
    "phy_read": {
        "evt": gatt_cb_strings["phy_read"],
        "err": gatt_cb_err["phy_read_err"]
    },
    "phy_update": {
        "evt": gatt_cb_strings["phy_update"],
        "err": gatt_cb_err["phy_update_err"]
    },
    "serv_phy_read": {
        "evt": gatt_cb_strings["serv_phy_read"],
        "err": gatt_cb_err["phy_read_err"]
    },
    "serv_phy_update": {
        "evt": gatt_cb_strings["serv_phy_update"],
        "err": gatt_cb_err["phy_update_err"]
    }
}

# Matches constants of connection states defined in BluetoothGatt.java
gatt_connection_state = {
    "disconnected": 0,
    "connecting": 1,
    "connected": 2,
    "disconnecting": 3,
    "closed": 4
}

# Matches constants of Bluetooth GATT Characteristic values as defined
# in BluetoothGattCharacteristic.java
gatt_characteristic = {
    "property_broadcast": 0x01,
    "property_read": 0x02,
    "property_write_no_response": 0x04,
    "property_write": 0x08,
    "property_notify": 0x10,
    "property_indicate": 0x20,
    "property_signed_write": 0x40,
    "property_extended_props": 0x80,
    "permission_read": 0x01,
    "permission_read_encrypted": 0x02,
    "permission_read_encrypted_mitm": 0x04,
    "permission_write": 0x10,
    "permission_write_encrypted": 0x20,
    "permission_write_encrypted_mitm": 0x40,
    "permission_write_signed": 0x80,
    "permission_write_signed_mitm": 0x100,
    "write_type_default": 0x02,
    "write_type_no_response": 0x01,
    "write_type_signed": 0x04,
}

# Matches constants of Bluetooth GATT Characteristic values as defined
# in BluetoothGattDescriptor.java
gatt_descriptor = {
    "enable_notification_value": [0x01, 0x00],
    "enable_indication_value": [0x02, 0x00],
    "disable_notification_value": [0x00, 0x00],
    "permission_read": 0x01,
    "permission_read_encrypted": 0x02,
    "permission_read_encrypted_mitm": 0x04,
    "permission_write": 0x10,
    "permission_write_encrypted": 0x20,
    "permission_write_encrypted_mitm": 0x40,
    "permission_write_signed": 0x80,
    "permission_write_signed_mitm": 0x100
}

# https://www.bluetooth.com/specifications/gatt/descriptors
gatt_char_desc_uuids = {
    "char_ext_props": '00002900-0000-1000-8000-00805f9b34fb',
    "char_user_desc": '00002901-0000-1000-8000-00805f9b34fb',
    "client_char_cfg": '00002902-0000-1000-8000-00805f9b34fb',
    "server_char_cfg": '00002903-0000-1000-8000-00805f9b34fb',
    "char_fmt_uuid": '00002904-0000-1000-8000-00805f9b34fb',
    "char_agreg_fmt": '00002905-0000-1000-8000-00805f9b34fb',
    "char_valid_range": '00002906-0000-1000-8000-00805f9b34fb',
    "external_report_reference": '00002907-0000-1000-8000-00805f9b34fb',
    "report_reference": '00002908-0000-1000-8000-00805f9b34fb'
}

# https://www.bluetooth.com/specifications/gatt/characteristics
gatt_char_types = {
    "device_name": '00002a00-0000-1000-8000-00805f9b34fb',
    "appearance": '00002a01-0000-1000-8000-00805f9b34fb',
    "peripheral_priv_flag": '00002a02-0000-1000-8000-00805f9b34fb',
    "reconnection_address": '00002a03-0000-1000-8000-00805f9b34fb',
    "peripheral_pref_conn": '00002a04-0000-1000-8000-00805f9b34fb',
    "service_changed": '00002a05-0000-1000-8000-00805f9b34fb',
    "system_id": '00002a23-0000-1000-8000-00805f9b34fb',
    "model_number_string": '00002a24-0000-1000-8000-00805f9b34fb',
    "serial_number_string": '00002a25-0000-1000-8000-00805f9b34fb',
    "firmware_revision_string": '00002a26-0000-1000-8000-00805f9b34fb',
    "hardware_revision_string": '00002a27-0000-1000-8000-00805f9b34fb',
    "software_revision_string": '00002a28-0000-1000-8000-00805f9b34fb',
    "manufacturer_name_string": '00002a29-0000-1000-8000-00805f9b34fb',
    "pnp_id": '00002a50-0000-1000-8000-00805f9b34fb',
}

# Matches constants of Bluetooth GATT Characteristic values as defined
# in BluetoothGattCharacteristic.java
gatt_characteristic_value_format = {
    "string": 0x1,
    "byte": 0x2,
    "sint8": 0x21,
    "uint8": 0x11,
    "sint16": 0x22,
    "unit16": 0x12,
    "sint32": 0x24,
    "uint32": 0x14
}

# Matches constants of Bluetooth Gatt Service types as defined in
# BluetoothGattService.java
gatt_service_types = {"primary": 0, "secondary": 1}

# Matches constants of Bluetooth Gatt Connection Priority values as defined in
# BluetoothGatt.java
gatt_connection_priority = {"balanced": 0, "high": 1, "low_power": 2}

# Min and max MTU values
gatt_mtu_size = {"min": 23, "max": 217}

# Gatt Characteristic attribute lengths
gatt_characteristic_attr_length = {"attr_1": 1, "attr_2": 3, "attr_3": 15}

# Matches constants of Bluetooth Gatt operations status as defined in
# BluetoothGatt.java
gatt_status = {"success": 0, "failure": 0x101}

# Matches constants of Bluetooth transport values as defined in
# BluetoothDevice.java
gatt_transport = {"auto": 0x00, "bredr": 0x01, "le": 0x02}

# Matches constants of Bluetooth physical channeling values as defined in
# BluetoothDevice.java
gatt_phy = {"1m": 1, "2m": 2, "le_coded": 3}

# Matches constants of Bluetooth physical channeling bitmask values as defined
# in BluetoothDevice.java
gatt_phy_mask = {"1m_mask": 1, "2m_mask": 2, "coded_mask": 4}

# Values as defiend in the Bluetooth GATT specification
gatt_server_responses = {
    "GATT_SUCCESS": 0x0,
    "GATT_FAILURE": 0x1,
    "GATT_READ_NOT_PERMITTED": 0x2,
    "GATT_WRITE_NOT_PERMITTED": 0x3,
    "GATT_INVALID_PDU": 0x4,
    "GATT_INSUFFICIENT_AUTHENTICATION": 0x5,
    "GATT_REQUEST_NOT_SUPPORTED": 0x6,
    "GATT_INVALID_OFFSET": 0x7,
    "GATT_INSUFFICIENT_AUTHORIZATION": 0x8,
    "GATT_INVALID_ATTRIBUTE_LENGTH": 0xd,
    "GATT_INSUFFICIENT_ENCRYPTION": 0xf,
    "GATT_CONNECTION_CONGESTED": 0x8f,
    "GATT_13_ERR": 0x13,
    "GATT_12_ERR": 0x12,
    "GATT_0C_ERR": 0x0C,
    "GATT_16": 0x16
}

### Bluetooth GATT Constants End ###

### Chameleon Constants Begin ###

# Chameleon audio bits per sample.
audio_bits_per_sample_16 = 16
audio_bits_per_sample_24 = 24
audio_bits_per_sample_32 = 32

# Chameleon audio sample rates.
audio_sample_rate_44100 = 44100
audio_sample_rate_48000 = 48000
audio_sample_rate_88200 = 88200
audio_sample_rate_96000 = 96000

# Chameleon audio channel modes.
audio_channel_mode_mono = 1
audio_channel_mode_stereo = 2
audio_channel_mode_8 = 8

# Chameleon time delays.
delay_after_binding_seconds = 0.5
delay_before_record_seconds = 0.5
silence_wait_seconds = 5

# Chameleon bus endpoints.
fpga_linein_bus_endpoint = 'Chameleon FPGA line-in'
headphone_bus_endpoint = 'Cros device headphone'

### Chameleon Constants End ###

### Begin logcat strings dict"""
logcat_strings = {
    "media_playback_vol_changed": "onRouteVolumeChanged",
}

### End logcat strings dict"""
