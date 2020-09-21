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
GATT Client Libraries
"""

from acts.test_utils.bt.bt_constants import default_le_connection_interval_ms
from acts.test_utils.bt.bt_constants import default_bluetooth_socket_timeout_ms
from acts.test_utils.bt.bt_gatt_utils import disconnect_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import setup_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import setup_gatt_mtu
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_constants import gatt_cb_strings
from acts.test_utils.bt.bt_constants import gatt_char_desc_uuids
from acts.test_utils.bt.bt_constants import gatt_descriptor
from acts.test_utils.bt.bt_constants import gatt_transport
from acts.test_utils.bt.bt_constants import le_default_supervision_timeout
from acts.test_utils.bt.bt_constants import le_connection_interval_time_step_ms
from acts.test_utils.bt.bt_constants import scan_result
from acts.test_utils.bt.bt_gatt_utils import log_gatt_server_uuids

import time
import os


class GattClientLib():
    def __init__(self, log, dut, target_mac_addr=None):
        self.dut = dut
        self.log = log
        self.gatt_callback = None
        self.bluetooth_gatt = None
        self.discovered_services_index = None
        self.target_mac_addr = target_mac_addr
        self.generic_uuid = "0000{}-0000-1000-8000-00805f9b34fb"

    def set_target_mac_addr(self, mac_addr):
        self.target_mac_addr = mac_addr

    def connect_over_le_based_off_name(self, autoconnect, name):
        """Perform GATT connection over LE"""
        self.dut.droid.bleSetScanSettingsScanMode(
            ble_scan_settings_modes['low_latency'])
        filter_list = self.dut.droid.bleGenFilterList()
        scan_settings = self.dut.droid.bleBuildScanSetting()
        scan_callback = self.dut.droid.bleGenScanCallback()
        event_name = scan_result.format(scan_callback)
        self.dut.droid.bleSetScanFilterDeviceName("BLE Rect")
        self.dut.droid.bleBuildScanFilter(filter_list)
        self.dut.droid.bleStartBleScan(filter_list, scan_settings,
                                       scan_callback)

        try:
            event = self.dut.ed.pop_event(event_name, 10)
            self.log.info("Found scan result: {}".format(event))
        except Exception:
            self.log.info("Didn't find any scan results.")
        mac_addr = event['data']['Result']['deviceInfo']['address']
        self.bluetooth_gatt, self.gatt_callback = setup_gatt_connection(
            self.dut, mac_addr, autoconnect, transport=gatt_transport['le'])
        self.dut.droid.bleStopBleScan(scan_callback)
        self.discovered_services_index = None

    def connect_over_le(self, autoconnect):
        """Perform GATT connection over LE"""
        self.bluetooth_gatt, self.gatt_callback = setup_gatt_connection(
            self.dut,
            self.target_mac_addr,
            autoconnect,
            transport=gatt_transport['le'])
        self.discovered_services_index = None

    def connect_over_bredr(self):
        """Perform GATT connection over BREDR"""
        self.bluetooth_gatt, self.gatt_callback = setup_gatt_connection(
            self.dut,
            self.target_mac_addr,
            False,
            transport=gatt_transport['bredr'])

    def disconnect(self):
        """Perform GATT disconnect"""
        cmd = "Disconnect GATT connection"
        try:
            disconnect_gatt_connection(self.dut, self.bluetooth_gatt,
                                       self.gatt_callback)
        except Exception as err:
            self.log.info("Cmd {} failed with {}".format(cmd, err))
        try:
            self.dut.droid.gattClientClose(self.bluetooth_gatt)
        except Exception as err:
            self.log.info("Cmd failed with {}".format(err))

    def _setup_discovered_services_index(self):
        if not self.discovered_services_index:
            self.dut.droid.gattClientDiscoverServices(self.bluetooth_gatt)
            expected_event = gatt_cb_strings['gatt_serv_disc'].format(
                self.gatt_callback)
            event = self.dut.ed.pop_event(expected_event, 10)
            self.discovered_services_index = event['data']['ServicesIndex']

    def read_char_by_uuid(self, line):
        """GATT client read Characteristic by UUID."""
        uuid = line
        if len(line) == 4:
            uuid = self.generic_uuid.format(line)
        self.dut.droid.gattClientReadUsingCharacteristicUuid(
            self.bluetooth_gatt, uuid, 0x0001, 0xFFFF)

    def request_mtu(self, mtu):
        """Request MTU Change of input value"""
        setup_gatt_mtu(self.dut, self.bluetooth_gatt, self.gatt_callback,
                       int(mtu))

    def list_all_uuids(self):
        """From the GATT Client, discover services and list all services,
        chars and descriptors
        """
        self._setup_discovered_services_index()
        log_gatt_server_uuids(self.dut, self.discovered_services_index,
                              self.bluetooth_gatt)

    def discover_services(self):
        """GATT Client discover services of GATT Server"""
        self.dut.droid.gattClientDiscoverServices(self.bluetooth_gatt)

    def refresh(self):
        """Perform Gatt Client Refresh"""
        self.dut.droid.gattClientRefresh(self.bluetooth_gatt)

    def read_char_by_instance_id(self, id):
        """From the GATT Client, discover services and list all services,
        chars and descriptors
        """
        if not id:
            self.log.info("Invalid id")
            return
        self._setup_discovered_services_index()
        self.dut.droid.gattClientReadCharacteristicByInstanceId(
            self.bluetooth_gatt, self.discovered_services_index, int(id, 16))

    def write_char_by_instance_id(self, line):
        """GATT Client Write to Characteristic by instance ID"""
        args = line.split()
        if len(args) != 2:
            self.log.info("2 Arguments required: [InstanceId] [Size]")
            return
        instance_id = args[0]
        size = args[1]
        write_value = []
        for i in range(int(size)):
            write_value.append(i % 256)
        self._setup_discovered_services_index()
        self.dut.droid.gattClientWriteCharacteristicByInstanceId(
            self.bluetooth_gatt, self.discovered_services_index,
            int(instance_id, 16), write_value)

    def write_char_by_instance_id_value(self, line):
        """GATT Client Write to Characteristic by instance ID"""
        args = line.split()
        if len(args) != 2:
            self.log.info("2 Arguments required: [InstanceId] [Size]")
            return
        instance_id = args[0]
        write_value = args[1]
        self._setup_discovered_services_index()
        self.dut.droid.gattClientWriteCharacteristicByInstanceId(
            self.bluetooth_gatt, self.discovered_services_index,
            int(instance_id, 16), [int(write_value)])

    def mod_write_char_by_instance_id(self, line):
        """GATT Client Write to Char that doesn't have write permission"""
        args = line.split()
        if len(args) != 2:
            self.log.info("2 Arguments required: [InstanceId] [Size]")
            return
        instance_id = args[0]
        size = args[1]
        write_value = []
        for i in range(int(size)):
            write_value.append(i % 256)
        self._setup_discovered_services_index()
        self.dut.droid.gattClientModifyAccessAndWriteCharacteristicByInstanceId(
            self.bluetooth_gatt, self.discovered_services_index,
            int(instance_id, 16), write_value)

    def write_invalid_char_by_instance_id(self, line):
        """GATT Client Write to Char that doesn't exists"""
        args = line.split()
        if len(args) != 2:
            self.log.info("2 Arguments required: [InstanceId] [Size]")
            return
        instance_id = args[0]
        size = args[1]
        write_value = []
        for i in range(int(size)):
            write_value.append(i % 256)
        self._setup_discovered_services_index()
        self.dut.droid.gattClientWriteInvalidCharacteristicByInstanceId(
            self.bluetooth_gatt, self.discovered_services_index,
            int(instance_id, 16), write_value)

    def mod_read_char_by_instance_id(self, line):
        """GATT Client Read Char that doesn't have write permission"""
        instance_id = line
        self._setup_discovered_services_index()
        self.dut.droid.gattClientModifyAccessAndReadCharacteristicByInstanceId(
            self.bluetooth_gatt, self.discovered_services_index,
            int(instance_id, 16))

    def read_invalid_char_by_instance_id(self, line):
        """GATT Client Read Char that doesn't exists"""
        instance_id = line
        self.dut.droid.gattClientReadInvalidCharacteristicByInstanceId(
            self.bluetooth_gatt, self.discovered_services_index,
            int(instance_id, 16))

    def mod_write_desc_by_instance_id(self, line):
        """GATT Client Write to Desc that doesn't have write permission"""
        cmd = ""
        args = line.split()
        if len(args) != 2:
            self.log.info("2 Arguments required: [InstanceId] [Size]")
            return
        instance_id = args[0]
        size = args[1]
        write_value = []
        for i in range(int(size)):
            write_value.append(i % 256)
        self._setup_discovered_services_index()
        self.dut.droid.gattClientModifyAccessAndWriteDescriptorByInstanceId(
            self.bluetooth_gatt, self.discovered_services_index,
            int(instance_id, 16), write_value)

    def write_invalid_desc_by_instance_id(self, line):
        """GATT Client Write to Desc that doesn't exists"""
        args = line.split()
        if len(args) != 2:
            self.log.info("2 Arguments required: [InstanceId] [Size]")
            return
        instance_id = args[0]
        size = args[1]
        write_value = []
        for i in range(int(size)):
            write_value.append(i % 256)
        self._setup_discovered_services_index()
        self.dut.droid.gattClientWriteInvalidDescriptorByInstanceId(
            self.bluetooth_gatt, self.discovered_services_index,
            int(instance_id, 16), write_value)

    def mod_read_desc_by_instance_id(self, line):
        """GATT Client Read Desc that doesn't have write permission"""
        cmd = ""
        instance_id = line
        self._setup_discovered_services_index()
        self.dut.droid.gattClientModifyAccessAndReadDescriptorByInstanceId(
            self.bluetooth_gatt, self.discovered_services_index,
            int(instance_id, 16))

    def read_invalid_desc_by_instance_id(self, line):
        """GATT Client Read Desc that doesn't exists"""
        instance_id = line
        self._setup_discovered_services_index()
        self.dut.droid.gattClientReadInvalidDescriptorByInstanceId(
            self.bluetooth_gatt, self.discovered_services_index,
            int(instance_id, 16))

    def mod_read_char_by_uuid_and_instance_id(self, line):
        """GATT Client Read Char that doesn't have write permission"""
        args = line.split()
        if len(args) != 2:
            self.log.info("2 Arguments required: [uuid] [instance_id]")
            return
        uuid = args[0]
        instance_id = args[1]
        self._setup_discovered_services_index()
        self.dut.droid.gattClientModifyAccessAndReadCharacteristicByUuidAndInstanceId(
            self.bluetooth_gatt, self.discovered_services_index,
            int(instance_id, 16), self.generic_uuid.format(uuid))

    def read_invalid_char_by_uuid(self, line):
        """GATT Client Read Char that doesn't exists"""
        uuid = line
        self._setup_discovered_services_index()
        self.dut.droid.gattClientReadInvalidCharacteristicByUuid(
            self.bluetooth_gatt, self.discovered_services_index,
            self.generic_uuid.format(uuid))

    def write_desc_by_instance_id(self, line):
        """GATT Client Write to Descriptor by instance ID"""
        args = line.split()
        if len(args) != 2:
            self.log.info("2 Arguments required: [instanceID] [size]")
            return
        instance_id = args[0]
        size = args[1]
        write_value = []
        for i in range(int(size)):
            write_value.append(i % 256)
        self._setup_discovered_services_index()
        self.dut.droid.gattClientWriteDescriptorByInstanceId(
            self.bluetooth_gatt, self.discovered_services_index,
            int(instance_id, 16), write_value)

    def write_desc_notification_by_instance_id(self, line):
        """GATT Client Write to Descriptor by instance ID"""
        args = line.split()
        instance_id = args[0]
        switch = int(args[1])
        write_value = [0x00, 0x00]
        if switch == 2:
            write_value = [0x02, 0x00]
        self._setup_discovered_services_index()
        self.dut.droid.gattClientWriteDescriptorByInstanceId(
            self.bluetooth_gatt, self.discovered_services_index,
            int(instance_id, 16), write_value)

    def enable_notification_desc_by_instance_id(self, line):
        """GATT Client Enable Notification on Descriptor by instance ID"""
        instance_id = line
        self._setup_discovered_services_index()
        services_count = self.dut.droid.gattClientGetDiscoveredServicesCount(
            self.discovered_services_index)
        for i in range(services_count):
            characteristic_uuids = (
                self.dut.droid.gattClientGetDiscoveredCharacteristicUuids(
                    self.discovered_services_index, i))
            for j in range(len(characteristic_uuids)):
                descriptor_uuids = (
                    self.dut.droid.
                    gattClientGetDiscoveredDescriptorUuidsByIndex(
                        self.discovered_services_index, i, j))
                for k in range(len(descriptor_uuids)):
                    desc_inst_id = self.dut.droid.gattClientGetDescriptorInstanceId(
                        self.bluetooth_gatt, self.discovered_services_index, i,
                        j, k)
                    if desc_inst_id == int(instance_id, 16):
                        self.dut.droid.gattClientDescriptorSetValueByIndex(
                            self.bluetooth_gatt,
                            self.discovered_services_index, i, j, k,
                            gatt_descriptor['enable_notification_value'])
                        time.sleep(2)  #Necessary for PTS
                        self.dut.droid.gattClientWriteDescriptorByIndex(
                            self.bluetooth_gatt,
                            self.discovered_services_index, i, j, k)
                        time.sleep(2)  #Necessary for PTS
                        self.dut.droid.gattClientSetCharacteristicNotificationByIndex(
                            self.bluetooth_gatt,
                            self.discovered_services_index, i, j, True)

    def enable_indication_desc_by_instance_id(self, line):
        """GATT Client Enable indication on Descriptor by instance ID"""
        instance_id = line
        self._setup_discovered_services_index()
        services_count = self.dut.droid.gattClientGetDiscoveredServicesCount(
            self.discovered_services_index)
        for i in range(services_count):
            characteristic_uuids = (
                self.dut.droid.gattClientGetDiscoveredCharacteristicUuids(
                    self.discovered_services_index, i))
            for j in range(len(characteristic_uuids)):
                descriptor_uuids = (
                    self.dut.droid.
                    gattClientGetDiscoveredDescriptorUuidsByIndex(
                        self.discovered_services_index, i, j))
                for k in range(len(descriptor_uuids)):
                    desc_inst_id = self.dut.droid.gattClientGetDescriptorInstanceId(
                        self.bluetooth_gatt, self.discovered_services_index, i,
                        j, k)
                    if desc_inst_id == int(instance_id, 16):
                        self.dut.droid.gattClientDescriptorSetValueByIndex(
                            self.bluetooth_gatt,
                            self.discovered_services_index, i, j, k,
                            gatt_descriptor['enable_indication_value'])
                        time.sleep(2)  #Necessary for PTS
                        self.dut.droid.gattClientWriteDescriptorByIndex(
                            self.bluetooth_gatt,
                            self.discovered_services_index, i, j, k)
                        time.sleep(2)  #Necessary for PTS
                        self.dut.droid.gattClientSetCharacteristicNotificationByIndex(
                            self.bluetooth_gatt,
                            self.discovered_services_index, i, j, True)

    def char_enable_all_notifications(self):
        self._setup_discovered_services_index()
        services_count = self.dut.droid.gattClientGetDiscoveredServicesCount(
            self.discovered_services_index)
        for i in range(services_count):
            characteristic_uuids = (
                self.dut.droid.gattClientGetDiscoveredCharacteristicUuids(
                    self.discovered_services_index, i))
            for j in range(len(characteristic_uuids)):
                self.dut.droid.gattClientSetCharacteristicNotificationByIndex(
                    self.bluetooth_gatt, self.discovered_services_index, i, j,
                    True)

    def read_char_by_invalid_instance_id(self, line):
        self._setup_discovered_services_index()
        services_count = self.dut.droid.gattClientGetDiscoveredServicesCount(
            self.discovered_services_index)
        self.dut.droid.gattClientReadInvalidCharacteristicInstanceId(
            self.bluetooth_gatt, self.discovered_services_index, 0,
            int(line, 16))

    def begin_reliable_write(self):
        """Begin a reliable write on the Bluetooth Gatt Client"""
        self.dut.droid.gattClientBeginReliableWrite(self.bluetooth_gatt)

    def abort_reliable_write(self):
        """Abort a reliable write on the Bluetooth Gatt Client"""
        self.dut.droid.gattClientAbortReliableWrite(self.bluetooth_gatt)

    def execute_reliable_write(self):
        """Execute a reliable write on the Bluetooth Gatt Client"""
        self.dut.droid.gattExecuteReliableWrite(self.bluetooth_gatt)

    def read_all_char(self):
        """GATT Client read all Characteristic values"""
        self._setup_discovered_services_index()
        services_count = self.dut.droid.gattClientGetDiscoveredServicesCount(
            self.discovered_services_index)
        for i in range(services_count):
            characteristic_uuids = (
                self.dut.droid.gattClientGetDiscoveredCharacteristicUuids(
                    self.discovered_services_index, i))
            for j in range(len(characteristic_uuids)):
                char_inst_id = self.dut.droid.gattClientGetCharacteristicInstanceId(
                    self.bluetooth_gatt, self.discovered_services_index, i, j)
                self.log.info("Reading characteristic {} {}".format(
                    hex(char_inst_id), characteristic_uuids[j]))
                self.dut.droid.gattClientReadCharacteristicByIndex(
                    self.bluetooth_gatt, self.discovered_services_index, i, j)
                time.sleep(1)  # Necessary for PTS

    def read_all_desc(self):
        """GATT Client read all Descriptor values"""
        self._setup_discovered_services_index()
        services_count = self.dut.droid.gattClientGetDiscoveredServicesCount(
            self.discovered_services_index)
        for i in range(services_count):
            characteristic_uuids = (
                self.dut.droid.gattClientGetDiscoveredCharacteristicUuids(
                    self.discovered_services_index, i))
            for j in range(len(characteristic_uuids)):
                descriptor_uuids = (
                    self.dut.droid.
                    gattClientGetDiscoveredDescriptorUuidsByIndex(
                        self.discovered_services_index, i, j))
                for k in range(len(descriptor_uuids)):
                    time.sleep(1)
                    try:
                        self.log.info("Reading descriptor {}".format(
                            descriptor_uuids[k]))
                        self.dut.droid.gattClientReadDescriptorByIndex(
                            self.bluetooth_gatt,
                            self.discovered_services_index, i, j, k)
                    except Exception as err:
                        self.log.info(
                            "Failed to read to descriptor: {}".format(
                                descriptor_uuids[k]))

    def write_all_char(self, line):
        """Write to every Characteristic on the GATT server"""
        args = line.split()
        write_value = []
        for i in range(int(line)):
            write_value.append(i % 256)
        self._setup_discovered_services_index()
        services_count = self.dut.droid.gattClientGetDiscoveredServicesCount(
            self.discovered_services_index)
        for i in range(services_count):
            characteristic_uuids = (
                self.dut.droid.gattClientGetDiscoveredCharacteristicUuids(
                    self.discovered_services_index, i))
            for j in range(len(characteristic_uuids)):
                char_inst_id = self.dut.droid.gattClientGetCharacteristicInstanceId(
                    self.bluetooth_gatt, self.discovered_services_index, i, j)
                self.log.info("Writing to {} {}".format(
                    hex(char_inst_id), characteristic_uuids[j]))
                try:
                    self.dut.droid.gattClientCharacteristicSetValueByIndex(
                        self.bluetooth_gatt, self.discovered_services_index, i,
                        j, write_value)
                    self.dut.droid.gattClientWriteCharacteristicByIndex(
                        self.bluetooth_gatt, self.discovered_services_index, i,
                        j)
                    time.sleep(1)
                except Exception as err:
                    self.log.info(
                        "Failed to write to characteristic: {}".format(
                            characteristic_uuids[j]))

    def write_all_desc(self, line):
        """ Write to every Descriptor on the GATT server """
        args = line.split()
        write_value = []
        for i in range(int(line)):
            write_value.append(i % 256)
        self._setup_discovered_services_index()
        services_count = self.dut.droid.gattClientGetDiscoveredServicesCount(
            self.discovered_services_index)
        for i in range(services_count):
            characteristic_uuids = (
                self.dut.droid.gattClientGetDiscoveredCharacteristicUuids(
                    self.discovered_services_index, i))
            for j in range(len(characteristic_uuids)):
                descriptor_uuids = (
                    self.dut.droid.
                    gattClientGetDiscoveredDescriptorUuidsByIndex(
                        self.discovered_services_index, i, j))
                for k in range(len(descriptor_uuids)):
                    time.sleep(1)
                    desc_inst_id = self.dut.droid.gattClientGetDescriptorInstanceId(
                        self.bluetooth_gatt, self.discovered_services_index, i,
                        j, k)
                    self.log.info("Writing to {} {}".format(
                        hex(desc_inst_id), descriptor_uuids[k]))
                    try:
                        self.dut.droid.gattClientDescriptorSetValueByIndex(
                            self.bluetooth_gatt,
                            self.discovered_services_index, i, j, k,
                            write_value)
                        self.dut.droid.gattClientWriteDescriptorByIndex(
                            self.bluetooth_gatt,
                            self.discovered_services_index, i, j, k)
                    except Exception as err:
                        self.log.info(
                            "Failed to write to descriptor: {}".format(
                                descriptor_uuids[k]))

    def discover_service_by_uuid(self, line):
        """ Discover service by UUID """
        uuid = line
        if len(line) == 4:
            uuid = self.generic_uuid.format(line)
        self.dut.droid.gattClientDiscoverServiceByUuid(self.bluetooth_gatt,
                                                       uuid)

    def request_le_connection_parameters(self):
        le_min_ce_len = 0
        le_max_ce_len = 0
        le_connection_interval = 0
        minInterval = default_le_connection_interval_ms / le_connection_interval_time_step_ms
        maxInterval = default_le_connection_interval_ms / le_connection_interval_time_step_ms
        return_status = self.dut.droid.gattClientRequestLeConnectionParameters(
            self.bluetooth_gatt, minInterval, maxInterval, 0,
            le_default_supervision_timeout, le_min_ce_len, le_max_ce_len)
        self.log.info(
            "Result of request le connection param: {}".format(return_status))

    def socket_conn_begin_connect_thread_psm(self, line):
        args = line.split()
        is_ble = bool(int(args[0]))
        secured_conn = bool(int(args[1]))
        psm_value = int(args[2])  # 1
        self.dut.droid.bluetoothSocketConnBeginConnectThreadPsm(
            self.target_mac_addr, is_ble, psm_value, secured_conn)

    def socket_conn_begin_accept_thread_psm(self, line):
        accept_timeout_ms = default_bluetooth_socket_timeout_ms
        is_ble = True
        secured_conn = False
        self.dut.droid.bluetoothSocketConnBeginAcceptThreadPsm(
            accept_timeout_ms, is_ble, secured_conn)
