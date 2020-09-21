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

import time
import os

from acts.keys import Config
from acts.utils import rand_ascii_str
from acts.test_utils.bt.bt_constants import gatt_cb_strings
from acts.test_utils.bt.bt_constants import gatt_characteristic
from acts.test_utils.bt.bt_constants import gatt_characteristic_value_format
from acts.test_utils.bt.bt_constants import gatt_cb_err
from acts.test_utils.bt.bt_constants import gatt_transport
from acts.test_utils.bt.bt_constants import gatt_event
from acts.test_utils.bt.bt_constants import gatt_server_responses
from acts.test_utils.bt.bt_constants import gatt_service_types
from acts.test_utils.bt.bt_constants import small_timeout
from acts.test_utils.bt.gatt_test_database import STRING_512BYTES

from acts.utils import exe_cmd
from math import ceil


class GattServerLib():

    characteristic_list = []
    default_timeout = 10
    descriptor_list = []
    dut = None
    gatt_server = None
    gatt_server_callback = None
    gatt_server_list = []
    log = None
    service_list = []
    write_mapping = {}

    def __init__(self, log, dut):
        self.dut = dut
        self.log = log

    def list_all_uuids(self):
        """From the GATT Client, discover services and list all services,
        chars and descriptors.
        """
        self.log.info("Service List:")
        for service in self.dut.droid.gattGetServiceUuidList(self.gatt_server):
            self.dut.log.info("GATT Server service uuid: {}".format(service))
        self.log.info("Characteristics List:")
        for characteristic in self.characteristic_list:
            instance_id = self.dut.droid.gattServerGetCharacteristicInstanceId(
                characteristic)
            uuid = self.dut.droid.gattServerGetCharacteristicUuid(
                characteristic)
            self.dut.log.info(
                "GATT Server characteristic handle uuid: {} {}".format(
                    hex(instance_id), uuid))
        # TODO: add getting insance ids and uuids from each descriptor.

    def open(self):
        """Open an empty GATT Server instance"""
        self.gatt_server_callback = self.dut.droid.gattServerCreateGattServerCallback(
        )
        self.gatt_server = self.dut.droid.gattServerOpenGattServer(
            self.gatt_server_callback)
        self.gatt_server_list.append(self.gatt_server)

    def clear_services(self):
        """Clear BluetoothGattServices from BluetoothGattServer"""
        self.dut.droid.gattServerClearServices(self.gatt_server)

    def close_bluetooth_gatt_servers(self):
        """Close Bluetooth Gatt Servers"""
        try:
            for btgs in self.gatt_server_list:
                self.dut.droid.gattServerClose(btgs)
        except Exception as err:
            self.log.error(
                "Failed to close Bluetooth GATT Servers: {}".format(err))
        self.characteristic_list = []
        self.descriptor_list = []
        self.gatt_server_list = []
        self.service_list = []

    def characteristic_set_value_by_instance_id(self, instance_id, value):
        """Set Characteristic value by instance id"""
        self.dut.droid.gattServerCharacteristicSetValueByInstanceId(
            int(instance_id, 16), value)

    def notify_characteristic_changed(self, instance_id, confirm):
        """ Notify characteristic changed """
        self.dut.droid.gattServerNotifyCharacteristicChangedByInstanceId(
            self.gatt_server, 0, int(instance_id, 16), confirm)

    def send_response(self, user_input):
        """Send a single response to the GATT Client"""
        args = user_input.split()
        mtu = 23
        if len(args) == 2:
            user_input = args[0]
            mtu = int(args[1])
        desc_read = gatt_event['desc_read_req']['evt'].format(
            self.gatt_server_callback)
        desc_write = gatt_event['desc_write_req']['evt'].format(
            self.gatt_server_callback)
        char_read = gatt_event['char_read_req']['evt'].format(
            self.gatt_server_callback)
        char_write_req = gatt_event['char_write_req']['evt'].format(
            self.gatt_server_callback)
        char_write = gatt_event['char_write']['evt'].format(
            self.gatt_server_callback)
        execute_write = gatt_event['exec_write']['evt'].format(
            self.gatt_server_callback)
        regex = "({}|{}|{}|{}|{}|{})".format(desc_read, desc_write, char_read,
                                             char_write, execute_write,
                                             char_write_req)
        events = self.dut.ed.pop_events(regex, 5, small_timeout)
        status = 0
        if user_input:
            status = gatt_server_responses.get(user_input)
        for event in events:
            self.log.debug("Found event: {}.".format(event))
            request_id = event['data']['requestId']
            if event['name'] == execute_write:
                if ('execute' in event['data']
                        and event['data']['execute'] == True):
                    for key in self.write_mapping:
                        value = self.write_mapping[key]
                        self.log.info("Writing key, value: {}, {}".format(
                            key, value))
                        self.dut.droid.gattServerSetByteArrayValueByInstanceId(
                            key, value)
                else:
                    self.log.info("Execute result is false")
                self.write_mapping = {}
                self.dut.droid.gattServerSendResponse(
                    self.gatt_server, 0, request_id, status, 0, [])
                continue
            offset = event['data']['offset']
            instance_id = event['data']['instanceId']
            if (event['name'] == desc_write or event['name'] == char_write
                    or event['name'] == char_write_req):
                if ('preparedWrite' in event['data']
                        and event['data']['preparedWrite'] == True):
                    value = event['data']['value']
                    if instance_id in self.write_mapping.keys():
                        self.write_mapping[
                            instance_id] = self.write_mapping[instance_id] + value
                        self.log.info(
                            "New Prepared Write Value for {}: {}".format(
                                instance_id, self.write_mapping[instance_id]))
                    else:
                        self.log.info("write mapping key, value {}, {}".format(
                            instance_id, value))
                        self.write_mapping[instance_id] = value
                        self.log.info("current value {}, {}".format(
                            instance_id, value))
                    self.dut.droid.gattServerSendResponse(
                        self.gatt_server, 0, request_id, status, 0, value)
                    continue
                else:
                    self.dut.droid.gattServerSetByteArrayValueByInstanceId(
                        event['data']['instanceId'], event['data']['value'])

            try:
                data = self.dut.droid.gattServerGetReadValueByInstanceId(
                    int(event['data']['instanceId']))
            except Exception as err:
                self.log.error(err)
            if not data:
                data = [1]
            self.log.info(
                "GATT Server Send Response [request_id, status, offset, data]" \
                " [{}, {}, {}, {}]".
                format(request_id, status, offset, data))
            data = data[offset:offset + mtu - 1]
            self.dut.droid.gattServerSendResponse(
                self.gatt_server, 0, request_id, status, offset, data)

    def _setup_service(self, serv):
        service = self.dut.droid.gattServerCreateService(
            serv['uuid'], serv['type'])
        if 'handles' in serv:
            self.dut.droid.gattServerServiceSetHandlesToReserve(
                service, serv['handles'])
        return service

    def _setup_characteristic(self, char):
        characteristic = \
            self.dut.droid.gattServerCreateBluetoothGattCharacteristic(
                char['uuid'], char['properties'], char['permissions'])
        if 'instance_id' in char:
            self.dut.droid.gattServerCharacteristicSetInstanceId(
                characteristic, char['instance_id'])
            set_id = self.dut.droid.gattServerCharacteristicGetInstanceId(
                characteristic)
            if set_id != char['instance_id']:
                self.log.error(
                    "Instance ID did not match up. Found {} Expected {}".
                    format(set_id, char['instance_id']))
        if 'value_type' in char:
            value_type = char['value_type']
            value = char['value']
            if value_type == gatt_characteristic_value_format['string']:
                self.log.info("Set String value result: {}".format(
                    self.dut.droid.gattServerCharacteristicSetStringValue(
                        characteristic, value)))
            elif value_type == gatt_characteristic_value_format['byte']:
                self.log.info("Set Byte Array value result: {}".format(
                    self.dut.droid.gattServerCharacteristicSetByteValue(
                        characteristic, value)))
            else:
                self.log.info("Set Int value result: {}".format(
                    self.dut.droid.gattServerCharacteristicSetIntValue(
                        characteristic, value, value_type, char['offset'])))
        return characteristic

    def _setup_descriptor(self, desc):
        descriptor = self.dut.droid.gattServerCreateBluetoothGattDescriptor(
            desc['uuid'], desc['permissions'])
        if 'value' in desc:
            self.dut.droid.gattServerDescriptorSetByteValue(
                descriptor, desc['value'])
        if 'instance_id' in desc:
            self.dut.droid.gattServerDescriptorSetInstanceId(
                descriptor, desc['instance_id'])
        self.descriptor_list.append(descriptor)
        return descriptor

    def setup_gatts_db(self, database):
        """Setup GATT Server database"""
        self.gatt_server_callback = \
            self.dut.droid.gattServerCreateGattServerCallback()
        self.gatt_server = self.dut.droid.gattServerOpenGattServer(
            self.gatt_server_callback)
        self.gatt_server_list.append(self.gatt_server)
        for serv in database['services']:
            service = self._setup_service(serv)
            self.service_list.append(service)
            if 'characteristics' in serv:
                for char in serv['characteristics']:
                    characteristic = self._setup_characteristic(char)
                    if 'descriptors' in char:
                        for desc in char['descriptors']:
                            descriptor = self._setup_descriptor(desc)
                            self.dut.droid.gattServerCharacteristicAddDescriptor(
                                characteristic, descriptor)
                    self.characteristic_list.append(characteristic)
                    self.dut.droid.gattServerAddCharacteristicToService(
                        service, characteristic)
            self.dut.droid.gattServerAddService(self.gatt_server, service)
            expected_event = gatt_cb_strings['serv_added'].format(
                self.gatt_server_callback)
            self.dut.ed.pop_event(expected_event, 10)
        return self.gatt_server, self.gatt_server_callback

    def send_continuous_response(self, user_input):
        """Send the same response"""
        desc_read = gatt_event['desc_read_req']['evt'].format(
            self.gatt_server_callback)
        desc_write = gatt_event['desc_write_req']['evt'].format(
            self.gatt_server_callback)
        char_read = gatt_event['char_read_req']['evt'].format(
            self.gatt_server_callback)
        char_write = gatt_event['char_write']['evt'].format(
            self.gatt_server_callback)
        execute_write = gatt_event['exec_write']['evt'].format(
            self.gatt_server_callback)
        regex = "({}|{}|{}|{}|{})".format(desc_read, desc_write, char_read,
                                          char_write, execute_write)
        offset = 0
        status = 0
        mtu = 23
        char_value = []
        for i in range(512):
            char_value.append(i % 256)
        len_min = 470
        end_time = time.time() + 180
        i = 0
        num_packets = ceil((len(char_value) + 1) / (mtu - 1))
        while time.time() < end_time:
            events = self.dut.ed.pop_events(regex, 10, small_timeout)
            for event in events:
                start_offset = i * (mtu - 1)
                i += 1
                self.log.debug("Found event: {}.".format(event))
                request_id = event['data']['requestId']
                data = char_value[start_offset:start_offset + mtu - 1]
                if not data:
                    data = [1]
                self.log.debug(
                    "GATT Server Send Response [request_id, status, offset, " \
                    "data] [{}, {}, {}, {}]".format(request_id, status, offset,
                        data))
                self.dut.droid.gattServerSendResponse(
                    self.gatt_server, 0, request_id, status, offset, data)

    def send_continuous_response_data(self, user_input):
        """Send the same response with data"""
        desc_read = gatt_event['desc_read_req']['evt'].format(
            self.gatt_server_callback)
        desc_write = gatt_event['desc_write_req']['evt'].format(
            self.gatt_server_callback)
        char_read = gatt_event['char_read_req']['evt'].format(
            self.gatt_server_callback)
        char_write = gatt_event['char_write']['evt'].format(
            self.gatt_server_callback)
        execute_write = gatt_event['exec_write']['evt'].format(
            self.gatt_server_callback)
        regex = "({}|{}|{}|{}|{})".format(desc_read, desc_write, char_read,
                                          char_write, execute_write)
        offset = 0
        status = 0
        mtu = 11
        char_value = []
        len_min = 470
        end_time = time.time() + 180
        i = 0
        num_packets = ceil((len(char_value) + 1) / (mtu - 1))
        while time.time() < end_time:
            events = self.dut.ed.pop_events(regex, 10, small_timeout)
            for event in events:
                self.log.info(event)
                request_id = event['data']['requestId']
                if event['name'] == execute_write:
                    if ('execute' in event['data']
                            and event['data']['execute'] == True):
                        for key in self.write_mapping:
                            value = self.write_mapping[key]
                            self.log.debug("Writing key, value: {}, {}".format(
                                key, value))
                            self.dut.droid.gattServerSetByteArrayValueByInstanceId(
                                key, value)
                        self.write_mapping = {}
                    self.dut.droid.gattServerSendResponse(
                        self.gatt_server, 0, request_id, status, 0, [1])
                    continue
                offset = event['data']['offset']
                instance_id = event['data']['instanceId']
                if (event['name'] == desc_write
                        or event['name'] == char_write):
                    if ('preparedWrite' in event['data']
                            and event['data']['preparedWrite'] == True):
                        value = event['data']['value']
                        if instance_id in self.write_mapping:
                            self.write_mapping[
                                instance_id] = self.write_mapping[instance_id] + value
                        else:
                            self.write_mapping[instance_id] = value
                    else:
                        self.dut.droid.gattServerSetByteArrayValueByInstanceId(
                            event['data']['instanceId'],
                            event['data']['value'])
                try:
                    data = self.dut.droid.gattServerGetReadValueByInstanceId(
                        int(event['data']['instanceId']))
                except Exception as err:
                    self.log.error(err)
                if not data:
                    self.dut.droid.gattServerSendResponse(
                        self.gatt_server, 0, request_id, status, offset, [1])
                else:
                    self.dut.droid.gattServerSendResponse(
                        self.gatt_server, 0, request_id, status, offset,
                        data[offset:offset + 17])
