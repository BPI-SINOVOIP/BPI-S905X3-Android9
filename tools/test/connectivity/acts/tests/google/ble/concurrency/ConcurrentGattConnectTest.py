#/usr/bin/env python3.4
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
"""
Test script for concurrent Gatt connections.
Testbed assumes 6 Android devices. One will be the central and the rest
peripherals.
"""

from queue import Empty
import concurrent.futures
import threading
import time
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_constants import ble_advertise_settings_modes
from acts.test_utils.bt.bt_constants import bt_profile_constants
from acts.test_utils.bt.bt_constants import gatt_characteristic
from acts.test_utils.bt.bt_constants import gatt_characteristic_value_format
from acts.test_utils.bt.bt_constants import gatt_char_desc_uuids
from acts.test_utils.bt.bt_constants import gatt_descriptor
from acts.test_utils.bt.bt_constants import gatt_service_types
from acts.test_utils.bt.bt_constants import scan_result
from acts.test_utils.bt.bt_gatt_utils import run_continuous_write_descriptor
from acts.test_utils.bt.bt_gatt_utils import setup_gatt_connection
from acts.test_utils.bt.gatts_lib import GattServerLib
from acts.test_decorators import test_tracker_info

service_uuid = '0000a00a-0000-1000-8000-00805f9b34fb'
characteristic_uuid = 'aa7edd5a-4d1d-4f0e-883a-d145616a1630'
descriptor_uuid = "00000003-0000-1000-8000-00805f9b34fb"

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
            gatt_characteristic['property_write'],
            'permissions':
            gatt_characteristic['permission_write'],
            'instance_id':
            0x002a,
            'value_type':
            gatt_characteristic_value_format['string'],
            'value':
            'Test Database',
            'descriptors': [{
                'uuid': descriptor_uuid,
                'permissions': gatt_descriptor['permission_write'],
            }]
        }]
    }]
}


class ConcurrentGattConnectTest(BluetoothBaseTest):
    bt_default_timeout = 10
    max_connections = 5
    # List of tuples (android_device, advertise_callback)
    advertise_callbacks = []
    # List of tuples (android_device, advertisement_name)
    advertisement_names = []
    list_of_arguments_list = []

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.pri_dut = self.android_devices[0]

    def setup_class(self):
        super(BluetoothBaseTest, self).setup_class()

        # Create 5 advertisements from different android devices
        for i in range(1, self.max_connections + 1):
            # Set device name
            ad = self.android_devices[i]
            name = "test_adv_{}".format(i)
            self.advertisement_names.append((ad, name))
            ad.droid.bluetoothSetLocalName(name)

            # Setup and start advertisements
            ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
            ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
                ble_advertise_settings_modes['low_latency'])
            advertise_data = ad.droid.bleBuildAdvertiseData()
            advertise_settings = ad.droid.bleBuildAdvertiseSettings()
            advertise_callback = ad.droid.bleGenBleAdvertiseCallback()
            ad.droid.bleStartBleAdvertising(advertise_callback, advertise_data,
                                            advertise_settings)
            self.advertise_callbacks.append((ad, advertise_callback))

    def obtain_address_list_from_scan(self):
        """Returns the address list of all devices that match the scan filter.

        Returns:
          A list if all devices are found; None is any devices are not found.
        """
        # From central device, scan for all appropriate addresses by name.
        filter_list = self.pri_dut.droid.bleGenFilterList()
        self.pri_dut.droid.bleSetScanSettingsScanMode(
            ble_scan_settings_modes['low_latency'])
        scan_settings = self.pri_dut.droid.bleBuildScanSetting()
        scan_callback = self.pri_dut.droid.bleGenScanCallback()
        for android_device, name in self.advertisement_names:
            self.pri_dut.droid.bleSetScanFilterDeviceName(name)
            self.pri_dut.droid.bleBuildScanFilter(filter_list)
        self.pri_dut.droid.bleStartBleScan(filter_list, scan_settings,
                                           scan_callback)
        address_list = []
        devices_found = []
        # Set the scan time out to 20 sec to provide enough time to discover the
        # devices in busy environment
        scan_timeout = 20
        end_time = time.time() + scan_timeout
        while time.time() < end_time and len(address_list) < len(
                self.advertisement_names):
            try:
                event = self.pri_dut.ed.pop_event(
                    "BleScan{}onScanResults".format(scan_callback),
                    self.bt_default_timeout)

                adv_name = event['data']['Result']['deviceInfo']['name']
                mac_address = event['data']['Result']['deviceInfo']['address']
                # Look up the android device handle based on event name
                device = [
                    item for item in self.advertisement_names
                    if adv_name in item
                ]
                devices_found.append(device[0][0].serial)
                if len(device) is not 0:
                    address_list_tuple = (device[0][0], mac_address)
                else:
                    continue
                result = [item for item in address_list if mac_address in item]
                # if length of result is 0, it indicates that we have discovered
                # new mac address.
                if len(result) is 0:
                    self.log.info("Found new mac address: {}".format(
                        address_list_tuple[1]))
                    address_list.append(address_list_tuple)
            except Empty as err:
                self.log.error("Failed to find any scan results.")
                return None
        if len(address_list) < self.max_connections:
            self.log.info("Only found these devices: {}".format(devices_found))
            self.log.error("Could not find all necessary advertisements.")
            return None
        return address_list

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6638282c-69b5-4237-9f0d-18e131424a9f')
    def test_concurrent_gatt_connections(self):
        """Test max concurrent GATT connections

        Connect to all peripherals.

        Steps:
        1. Scan
        2. Save addresses
        3. Connect all addresses of the peripherals

        Expected Result:
        All connections successful.

        Returns:
          Pass if True
          Fail if False

        TAGS: Bluetooth, GATT
        Priority: 2
        """

        address_list = self.obtain_address_list_from_scan()
        if address_list is None:
            return False

        # Connect to all addresses
        for address_tuple in address_list:
            address = address_tuple[1]
            try:
                autoconnect = False
                bluetooth_gatt, gatt_callback = setup_gatt_connection(
                    self.pri_dut, address, autoconnect)
                self.log.info("Successfully connected to {}".format(address))
            except Exception as err:
                self.log.error(
                    "Failed to establish connection to {}".format(address))
                return False
        if (len(
                self.pri_dut.droid.bluetoothGetConnectedLeDevices(
                    bt_profile_constants['gatt_server'])) !=
                self.max_connections):
            self.log.error("Did not reach max connection count.")
            return False

        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='660bf05e-a8e5-45f3-b42b-b66b4ac0d85f')
    def test_data_transfer_to_concurrent_gatt_connections(self):
        """Test writing GATT descriptors concurrently to many peripherals.

        Connect to all peripherals and write gatt descriptors concurrently.


        Steps:
        1. Scan the addresses by names
        2. Save mac addresses of the peripherals
        3. Connect all addresses of the peripherals and write gatt descriptors


        Expected Result:
        All connections and data transfers are successful.

        Returns:
          Pass if True
          Fail if False

        TAGS: Bluetooth, GATT
        Priority: 2
        """

        address_list = self.obtain_address_list_from_scan()
        if address_list is None:
            return False

        # Connect to all addresses
        executor = concurrent.futures.ThreadPoolExecutor(max_workers=10)

        for address_tuple in address_list:
            ad, address = address_tuple

            gatts = GattServerLib(log=self.log, dut=ad)
            gatt_server, gatt_server_callback = gatts.setup_gatts_db(
                database=gatt_server_read_descriptor_sample)

            try:
                bluetooth_gatt, gatt_callback = setup_gatt_connection(
                    self.pri_dut, address, autoconnect=False)
                self.log.info("Successfully connected to {}".format(address))

            except Exception as err:
                self.log.error(
                    "Failed to establish connection to {}".format(address))
                return False

            if self.pri_dut.droid.gattClientDiscoverServices(bluetooth_gatt):
                event = self.pri_dut.ed.pop_event(
                    "GattConnect{}onServicesDiscovered".format(bluetooth_gatt),
                    self.bt_default_timeout)
                discovered_services_index = event['data']['ServicesIndex']
            else:
                self.log.info("Failed to discover services.")
                return False
            services_count = self.pri_dut.droid.gattClientGetDiscoveredServicesCount(
                discovered_services_index)

            arguments_list = [
                self.pri_dut.droid, self.pri_dut.ed, ad.droid, ad.ed,
                gatt_server, gatt_server_callback, bluetooth_gatt,
                services_count, discovered_services_index, 100
            ]
            self.list_of_arguments_list.append(arguments_list)

        for arguments_list in self.list_of_arguments_list:
            executor.submit(run_continuous_write_descriptor, *arguments_list)

        executor.shutdown(wait=True)

        if (len(
                self.pri_dut.droid.bluetoothGetConnectedLeDevices(
                    bt_profile_constants['gatt_server'])) !=
                self.max_connections):
            self.log.error("Failed to write concurrently.")
            return False

        return True
