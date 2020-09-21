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

import logging

from acts.test_utils.bt.bt_test_utils import BtTestUtilsError
from acts.test_utils.bt.bt_test_utils import get_mac_address_of_generic_advertisement
from acts.test_utils.bt.bt_constants import gatt_cb_err
from acts.test_utils.bt.bt_constants import gatt_cb_strings
from acts.test_utils.bt.bt_constants import gatt_connection_state
from acts.test_utils.bt.bt_constants import gatt_characteristic
from acts.test_utils.bt.bt_constants import gatt_descriptor
from acts.test_utils.bt.bt_constants import gatt_phy_mask
from acts.test_utils.bt.bt_constants import gatt_service_types
from acts.test_utils.bt.bt_constants import gatt_transport
import pprint
from queue import Empty

default_timeout = 10
log = logging


class GattTestUtilsError(Exception):
    pass


def setup_gatt_connection(cen_ad,
                          mac_address,
                          autoconnect,
                          transport=gatt_transport['auto'],
                          opportunistic=False):
    gatt_callback = cen_ad.droid.gattCreateGattCallback()
    log.info("Gatt Connect to mac address {}.".format(mac_address))
    bluetooth_gatt = cen_ad.droid.gattClientConnectGatt(
        gatt_callback, mac_address, autoconnect, transport, opportunistic,
        gatt_phy_mask['1m_mask'])
    expected_event = gatt_cb_strings['gatt_conn_change'].format(gatt_callback)
    try:
        event = cen_ad.ed.pop_event(expected_event, default_timeout)
    except Empty:
        close_gatt_client(cen_ad, bluetooth_gatt)
        raise GattTestUtilsError(
            "Could not establish a connection to "
            "peripheral. Expected event: {}".format(expected_event))
    if event['data']['State'] != gatt_connection_state['connected']:
        close_gatt_client(cen_ad, bluetooth_gatt)
        try:
            cen_ad.droid.gattClientClose(bluetooth_gatt)
        except Exception:
            self.log.debug("Failed to close gatt client.")
        raise GattTestUtilsError("Could not establish a connection to "
                                 "peripheral. Event Details: {}".format(
                                     pprint.pformat(event)))
    return bluetooth_gatt, gatt_callback


def close_gatt_client(cen_ad, bluetooth_gatt):
    try:
        cen_ad.droid.gattClientClose(bluetooth_gatt)
    except Exception:
        log.debug("Failed to close gatt client.")


def disconnect_gatt_connection(cen_ad, bluetooth_gatt, gatt_callback):
    cen_ad.droid.gattClientDisconnect(bluetooth_gatt)
    wait_for_gatt_disconnect_event(cen_ad, gatt_callback)
    return


def wait_for_gatt_disconnect_event(cen_ad, gatt_callback):
    expected_event = gatt_cb_strings['gatt_conn_change'].format(gatt_callback)
    try:
        event = cen_ad.ed.pop_event(expected_event, default_timeout)
    except Empty:
        raise GattTestUtilsError(
            gatt_cb_err['gatt_conn_change_err'].format(expected_event))
    found_state = event['data']['State']
    expected_state = gatt_connection_state['disconnected']
    if found_state != expected_state:
        raise GattTestUtilsError(
            "GATT connection state change expected {}, found {}".format(
                expected_event, found_state))
    return


def orchestrate_gatt_connection(cen_ad,
                                per_ad,
                                transport=gatt_transport['le'],
                                mac_address=None,
                                autoconnect=False,
                                opportunistic=False):
    adv_callback = None
    if mac_address is None:
        if transport == gatt_transport['le']:
            try:
                mac_address, adv_callback, scan_callback = (
                    get_mac_address_of_generic_advertisement(cen_ad, per_ad))
            except BtTestUtilsError as err:
                raise GattTestUtilsError(
                    "Error in getting mac address: {}".format(err))
        else:
            mac_address = per_ad.droid.bluetoothGetLocalAddress()
            adv_callback = None
    bluetooth_gatt, gatt_callback = setup_gatt_connection(
        cen_ad, mac_address, autoconnect, transport, opportunistic)
    return bluetooth_gatt, gatt_callback, adv_callback


def run_continuous_write_descriptor(cen_droid,
                                    cen_ed,
                                    per_droid,
                                    per_ed,
                                    gatt_server,
                                    gatt_server_callback,
                                    bluetooth_gatt,
                                    services_count,
                                    discovered_services_index,
                                    number_of_iterations=100000):
    log.info("Starting continuous write")
    bt_device_id = 0
    status = 1
    offset = 1
    test_value = [1, 2, 3, 4, 5, 6, 7]
    test_value_return = [1, 2, 3]
    for _ in range(number_of_iterations):
        try:
            for i in range(services_count):
                characteristic_uuids = (
                    cen_droid.gattClientGetDiscoveredCharacteristicUuids(
                        discovered_services_index, i))
                log.info(characteristic_uuids)
                for characteristic in characteristic_uuids:
                    descriptor_uuids = (
                        cen_droid.gattClientGetDiscoveredDescriptorUuids(
                            discovered_services_index, i, characteristic))
                    log.info(descriptor_uuids)
                    for descriptor in descriptor_uuids:
                        cen_droid.gattClientDescriptorSetValue(
                            bluetooth_gatt, discovered_services_index, i,
                            characteristic, descriptor, test_value)
                        cen_droid.gattClientWriteDescriptor(
                            bluetooth_gatt, discovered_services_index, i,
                            characteristic, descriptor)
                        expected_event = gatt_cb_strings[
                            'desc_write_req'].format(gatt_server_callback)
                        try:
                            event = per_ed.pop_event(expected_event,
                                                     default_timeout)
                        except Empty:
                            log.error(gatt_cb_err['desc_write_req_err'].format(
                                expected_event))
                            return False
                        request_id = event['data']['requestId']
                        found_value = event['data']['value']
                        if found_value != test_value:
                            log.error(
                                "Values didn't match. Found: {}, Expected: "
                                "{}".format(found_value, test_value))
                        per_droid.gattServerSendResponse(
                            gatt_server, bt_device_id, request_id, status,
                            offset, test_value_return)
                        expected_event = gatt_cb_strings['desc_write'].format(
                            bluetooth_gatt)
                        try:
                            cen_ed.pop_event(expected_event, default_timeout)
                        except Empty:
                            log.error(gatt_cb_strings['desc_write_err'].format(
                                expected_event))
                            raise Exception("Thread ended prematurely.")
        except Exception as err:
            log.error("Continuing but found exception: {}".format(err))


def setup_characteristics_and_descriptors(droid):
    characteristic_input = [
        {
            'uuid':
            "aa7edd5a-4d1d-4f0e-883a-d145616a1630",
            'property':
            gatt_characteristic['property_write']
            | gatt_characteristic['property_write_no_response'],
            'permission':
            gatt_characteristic['permission_write']
        },
        {
            'uuid':
            "21c0a0bf-ad51-4a2d-8124-b74003e4e8c8",
            'property':
            gatt_characteristic['property_notify']
            | gatt_characteristic['property_read'],
            'permission':
            gatt_characteristic['permission_read']
        },
        {
            'uuid':
            "6774191f-6ec3-4aa2-b8a8-cf830e41fda6",
            'property':
            gatt_characteristic['property_notify']
            | gatt_characteristic['property_read'],
            'permission':
            gatt_characteristic['permission_read']
        },
    ]
    descriptor_input = [{
        'uuid':
        "aa7edd5a-4d1d-4f0e-883a-d145616a1630",
        'property':
        gatt_descriptor['permission_read']
        | gatt_descriptor['permission_write'],
    }, {
        'uuid':
        "76d5ed92-ca81-4edb-bb6b-9f019665fb32",
        'property':
        gatt_descriptor['permission_read']
        | gatt_characteristic['permission_write'],
    }]
    characteristic_list = setup_gatt_characteristics(droid,
                                                     characteristic_input)
    descriptor_list = setup_gatt_descriptors(droid, descriptor_input)
    return characteristic_list, descriptor_list


def setup_multiple_services(per_ad):
    per_droid, per_ed = per_ad.droid, per_ad.ed
    gatt_server_callback = per_droid.gattServerCreateGattServerCallback()
    gatt_server = per_droid.gattServerOpenGattServer(gatt_server_callback)
    characteristic_list, descriptor_list = (
        setup_characteristics_and_descriptors(per_droid))
    per_droid.gattServerCharacteristicAddDescriptor(characteristic_list[1],
                                                    descriptor_list[0])
    per_droid.gattServerCharacteristicAddDescriptor(characteristic_list[2],
                                                    descriptor_list[1])
    gattService = per_droid.gattServerCreateService(
        "00000000-0000-1000-8000-00805f9b34fb", gatt_service_types['primary'])
    gattService2 = per_droid.gattServerCreateService(
        "FFFFFFFF-0000-1000-8000-00805f9b34fb", gatt_service_types['primary'])
    gattService3 = per_droid.gattServerCreateService(
        "3846D7A0-69C8-11E4-BA00-0002A5D5C51B", gatt_service_types['primary'])
    for characteristic in characteristic_list:
        per_droid.gattServerAddCharacteristicToService(gattService,
                                                       characteristic)
    per_droid.gattServerAddService(gatt_server, gattService)
    expected_event = gatt_cb_strings['serv_added'].format(gatt_server_callback)
    try:
        per_ed.pop_event(expected_event, default_timeout)
    except Empty:
        per_ad.droid.gattServerClose(gatt_server)
        raise GattTestUtilsError(
            gatt_cb_strings['serv_added_err'].format(expected_event))
    for characteristic in characteristic_list:
        per_droid.gattServerAddCharacteristicToService(gattService2,
                                                       characteristic)
    per_droid.gattServerAddService(gatt_server, gattService2)
    try:
        per_ed.pop_event(expected_event, default_timeout)
    except Empty:
        per_ad.droid.gattServerClose(gatt_server)
        raise GattTestUtilsError(
            gatt_cb_strings['serv_added_err'].format(expected_event))
    for characteristic in characteristic_list:
        per_droid.gattServerAddCharacteristicToService(gattService3,
                                                       characteristic)
    per_droid.gattServerAddService(gatt_server, gattService3)
    try:
        per_ed.pop_event(expected_event, default_timeout)
    except Empty:
        per_ad.droid.gattServerClose(gatt_server)
        raise GattTestUtilsError(
            gatt_cb_strings['serv_added_err'].format(expected_event))
    return gatt_server_callback, gatt_server


def setup_characteristics_and_descriptors(droid):
    characteristic_input = [
        {
            'uuid':
            "aa7edd5a-4d1d-4f0e-883a-d145616a1630",
            'property':
            gatt_characteristic['property_write']
            | gatt_characteristic['property_write_no_response'],
            'permission':
            gatt_characteristic['property_write']
        },
        {
            'uuid':
            "21c0a0bf-ad51-4a2d-8124-b74003e4e8c8",
            'property':
            gatt_characteristic['property_notify']
            | gatt_characteristic['property_read'],
            'permission':
            gatt_characteristic['permission_read']
        },
        {
            'uuid':
            "6774191f-6ec3-4aa2-b8a8-cf830e41fda6",
            'property':
            gatt_characteristic['property_notify']
            | gatt_characteristic['property_read'],
            'permission':
            gatt_characteristic['permission_read']
        },
    ]
    descriptor_input = [{
        'uuid':
        "aa7edd5a-4d1d-4f0e-883a-d145616a1630",
        'property':
        gatt_descriptor['permission_read']
        | gatt_descriptor['permission_write'],
    }, {
        'uuid':
        "76d5ed92-ca81-4edb-bb6b-9f019665fb32",
        'property':
        gatt_descriptor['permission_read']
        | gatt_characteristic['permission_write'],
    }]
    characteristic_list = setup_gatt_characteristics(droid,
                                                     characteristic_input)
    descriptor_list = setup_gatt_descriptors(droid, descriptor_input)
    return characteristic_list, descriptor_list


def setup_gatt_characteristics(droid, input):
    characteristic_list = []
    for item in input:
        index = droid.gattServerCreateBluetoothGattCharacteristic(
            item['uuid'], item['property'], item['permission'])
        characteristic_list.append(index)
    return characteristic_list


def setup_gatt_descriptors(droid, input):
    descriptor_list = []
    for item in input:
        index = droid.gattServerCreateBluetoothGattDescriptor(
            item['uuid'],
            item['property'],
        )
        descriptor_list.append(index)
    log.info("setup descriptor list: {}".format(descriptor_list))
    return descriptor_list


def setup_gatt_mtu(cen_ad, bluetooth_gatt, gatt_callback, mtu):
    """utility function to set mtu for GATT connection.

    Steps:
    1. Request mtu change.
    2. Check if the mtu is changed to the new value

    Args:
        cen_ad: test device for client to scan.
        bluetooth_gatt: GATT object
        mtu: new mtu value to be set

    Returns:
        If success, return True.
        if fail, return False
    """
    cen_ad.droid.gattClientRequestMtu(bluetooth_gatt, mtu)
    expected_event = gatt_cb_strings['mtu_changed'].format(gatt_callback)
    try:
        mtu_event = cen_ad.ed.pop_event(expected_event, default_timeout)
        mtu_size_found = mtu_event['data']['MTU']
        if mtu_size_found != mtu:
            log.error("MTU size found: {}, expected: {}".format(
                mtu_size_found, mtu))
            return False
    except Empty:
        log.error(gatt_cb_err['mtu_changed_err'].format(expected_event))
        return False
    return True


def log_gatt_server_uuids(cen_ad,
                          discovered_services_index,
                          bluetooth_gatt=None):
    services_count = cen_ad.droid.gattClientGetDiscoveredServicesCount(
        discovered_services_index)
    for i in range(services_count):
        service = cen_ad.droid.gattClientGetDiscoveredServiceUuid(
            discovered_services_index, i)
        log.info("Discovered service uuid {}".format(service))
        characteristic_uuids = (
            cen_ad.droid.gattClientGetDiscoveredCharacteristicUuids(
                discovered_services_index, i))
        for j in range(len(characteristic_uuids)):
            descriptor_uuids = (
                cen_ad.droid.gattClientGetDiscoveredDescriptorUuidsByIndex(
                    discovered_services_index, i, j))
            if bluetooth_gatt:
                char_inst_id = cen_ad.droid.gattClientGetCharacteristicInstanceId(
                    bluetooth_gatt, discovered_services_index, i, j)
                log.info("Discovered characteristic handle uuid: {} {}".format(
                    hex(char_inst_id), characteristic_uuids[j]))
                for k in range(len(descriptor_uuids)):
                    desc_inst_id = cen_ad.droid.gattClientGetDescriptorInstanceId(
                        bluetooth_gatt, discovered_services_index, i, j, k)
                    log.info("Discovered descriptor handle uuid: {} {}".format(
                        hex(desc_inst_id), descriptor_uuids[k]))
            else:
                log.info("Discovered characteristic uuid: {}".format(
                    characteristic_uuids[j]))
                for k in range(len(descriptor_uuids)):
                    log.info("Discovered descriptor uuid {}".format(
                        descriptor_uuids[k]))
