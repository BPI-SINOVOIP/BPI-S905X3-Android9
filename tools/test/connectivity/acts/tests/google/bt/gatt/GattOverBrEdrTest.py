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
Test suite for GATT over BR/EDR.
"""

import time
from queue import Empty

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_constants import gatt_characteristic
from acts.test_utils.bt.bt_constants import gatt_service_types
from acts.test_utils.bt.bt_constants import gatt_transport
from acts.test_utils.bt.bt_constants import gatt_cb_strings
from acts.test_utils.bt.bt_gatt_utils import GattTestUtilsError
from acts.test_utils.bt.bt_gatt_utils import disconnect_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import log_gatt_server_uuids
from acts.test_utils.bt.bt_gatt_utils import orchestrate_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import setup_gatt_characteristics
from acts.test_utils.bt.bt_gatt_utils import setup_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import setup_gatt_descriptors
from acts.test_utils.bt.bt_gatt_utils import setup_multiple_services
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs


class GattOverBrEdrTest(BluetoothBaseTest):
    adv_instances = []
    bluetooth_gatt_list = []
    gatt_server_list = []
    default_timeout = 10
    default_discovery_timeout = 3
    per_droid_mac_address = None

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.cen_ad = self.android_devices[0]
        self.per_ad = self.android_devices[1]

    def setup_class(self):
        super(BluetoothBaseTest, self).setup_class()
        self.per_droid_mac_address = self.per_ad.droid.bluetoothGetLocalAddress(
        )
        if not self.per_droid_mac_address:
            return False
        return True

    def setup_test(self):
        super(BluetoothBaseTest, self).setup_test()
        bluetooth_gatt_list = []
        self.gatt_server_list = []
        self.adv_instances = []

    def teardown_test(self):
        for bluetooth_gatt in self.bluetooth_gatt_list:
            self.cen_ad.droid.gattClientClose(bluetooth_gatt)
        for gatt_server in self.gatt_server_list:
            self.per_ad.droid.gattServerClose(gatt_server)
        return True

    def on_fail(self, test_name, begin_time):
        take_btsnoop_logs(self.android_devices, self, test_name)
        reset_bluetooth(self.android_devices)

    def _orchestrate_gatt_disconnection(self, bluetooth_gatt, gatt_callback):
        self.log.info("Disconnecting from peripheral device.")
        try:
            disconnect_gatt_connection(self.cen_ad, bluetooth_gatt,
                                       gatt_callback)
            if bluetooth_gatt in self.bluetooth_gatt_list:
                self.bluetooth_gatt_list.remove(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        return True

    def _find_service_added_event(self, gatt_server_callback, uuid):
        event = self.per_ad.ed.pop_event(
            gatt_cb_strings['serv_added'].format(gatt_server_callback),
            self.default_timeout)
        if event['data']['serviceUuid'].lower() != uuid.lower():
            self.log.info("Uuid mismatch. Found: {}, Expected {}.".format(
                event['data']['serviceUuid'], uuid))
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='32d32c87-911e-4f14-9654-29fe1431e995')
    def test_gatt_bredr_connect(self):
        """Test GATT connection over BR/EDR.

        Test establishing a gatt connection between a GATT server and GATT
        client.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a GATT connection between the scanner and advertiser.
        6. Disconnect the GATT connection.

        Expected Result:
        Verify that a connection was established and then disconnected
        successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: BR/EDR, Filtering, GATT, Scanning
        Priority: 0
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad,
                                            gatt_transport['bredr'],
                                            self.per_droid_mac_address))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='357b697b-a52c-4c2a-997c-00876a018f37')
    def test_gatt_bredr_connect_trigger_on_read_rssi(self):
        """Test GATT connection over BR/EDR read RSSI.

        Test establishing a gatt connection between a GATT server and GATT
        client then read the RSSI.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a GATT connection between the scanner and advertiser.
        6. From the scanner, request to read the RSSI of the advertiser.
        7. Disconnect the GATT connection.

        Expected Result:
        Verify that a connection was established and then disconnected
        successfully. Verify that the RSSI was ready correctly.

        Returns:
          Pass if True
          Fail if False

        TAGS: BR/EDR, Scanning, GATT, RSSI
        Priority: 1
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad,
                                            gatt_transport['bredr'],
                                            self.per_droid_mac_address))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        if self.cen_ad.droid.gattClientReadRSSI(bluetooth_gatt):
            self.cen_ad.ed.pop_event(
                gatt_cb_strings['rd_remote_rssi'].format(gatt_callback),
                self.default_timeout)
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='dee9ef28-b872-428a-821b-cc62f27ba936')
    def test_gatt_bredr_connect_trigger_on_services_discovered(self):
        """Test GATT connection and discover services of peripheral.

        Test establishing a gatt connection between a GATT server and GATT
        client the discover all services from the connected device.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a GATT connection between the scanner and advertiser.
        6. From the scanner (central device), discover services.
        7. Disconnect the GATT connection.

        Expected Result:
        Verify that a connection was established and then disconnected
        successfully. Verify that the service were discovered.

        Returns:
          Pass if True
          Fail if False

        TAGS: BR/EDR, Scanning, GATT, Services
        Priority: 1
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad,
                                            gatt_transport['bredr'],
                                            self.per_droid_mac_address))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        discovered_services_index = -1
        if self.cen_ad.droid.gattClientDiscoverServices(bluetooth_gatt):
            event = self.cen_ad.ed.pop_event(
                gatt_cb_strings['gatt_serv_disc'].format(gatt_callback),
                self.default_timeout)
            discovered_services_index = event['data']['ServicesIndex']
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='01883bdd-0cf8-48fb-bf15-467bbd4f065b')
    def test_gatt_bredr_connect_trigger_on_services_discovered_iterate_attributes(
            self):
        """Test GATT connection and iterate peripherals attributes.

        Test establishing a gatt connection between a GATT server and GATT
        client and iterate over all the characteristics and descriptors of the
        discovered services.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a GATT connection between the scanner and advertiser.
        6. From the scanner (central device), discover services.
        7. Iterate over all the characteristics and descriptors of the
        discovered features.
        8. Disconnect the GATT connection.

        Expected Result:
        Verify that a connection was established and then disconnected
        successfully. Verify that the services, characteristics, and descriptors
        were discovered.

        Returns:
          Pass if True
          Fail if False

        TAGS: BR/EDR, Scanning, GATT, Services
        Characteristics, Descriptors
        Priority: 1
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad,
                                            gatt_transport['bredr'],
                                            self.per_droid_mac_address))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        discovered_services_index = -1
        if self.cen_ad.droid.gattClientDiscoverServices(bluetooth_gatt):
            event = self.cen_ad.ed.pop_event(
                gatt_cb_strings['gatt_serv_disc'].format(gatt_callback),
                self.default_timeout)
            discovered_services_index = event['data']['ServicesIndex']
            log_gatt_server_uuids(self.cen_ad, discovered_services_index)
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d4277bee-da99-4f48-8a4d-f81b5389da18')
    def test_gatt_bredr_connect_with_service_uuid_variations(self):
        """Test GATT connection with multiple service uuids.

        Test establishing a gatt connection between a GATT server and GATT
        client with multiple service uuid variations.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a GATT connection between the scanner and advertiser.
        6. From the scanner (central device), discover services.
        7. Verify that all the service uuid variations are found.
        8. Disconnect the GATT connection.

        Expected Result:
        Verify that a connection was established and then disconnected
        successfully. Verify that the service uuid variations are found.

        Returns:
          Pass if True
          Fail if False

        TAGS: BR/EDR, Scanning, GATT, Services
        Priority: 2
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            gatt_server_callback, gatt_server = setup_multiple_services(
                self.per_ad)
            self.gatt_server_list.append(gatt_server)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad,
                                            gatt_transport['bredr'],
                                            self.per_droid_mac_address))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        discovered_services_index = -1
        if self.cen_ad.droid.gattClientDiscoverServices(bluetooth_gatt):
            event = self.cen_ad.ed.pop_event(
                gatt_cb_strings['gatt_serv_disc'].format(gatt_callback),
                self.default_timeout)
            discovered_services_index = event['data']['ServicesIndex']
            log_gatt_server_uuids(self.cen_ad, discovered_services_index)
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='15c726dc-788a-4400-9a90-8c6866b24a3a')
    def test_gatt_bredr_connect_multiple_iterations(self):
        """Test GATT connections multiple times.

        Test establishing a gatt connection between a GATT server and GATT
        client with multiple iterations.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a GATT connection between the scanner and advertiser.
        6. Disconnect the GATT connection.
        7. Repeat steps 5 and 6 twenty times.

        Expected Result:
        Verify that a connection was established and then disconnected
        successfully twenty times.

        Returns:
          Pass if True
          Fail if False

        TAGS: BR/EDR, Scanning, GATT, Stress
        Priority: 1
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        autoconnect = False
        mac_address = self.per_ad.droid.bluetoothGetLocalAddress()
        for i in range(20):
            try:
                bluetooth_gatt, gatt_callback, adv_callback = (
                    orchestrate_gatt_connection(self.cen_ad, self.per_ad,
                                                gatt_transport['bredr'],
                                                self.per_droid_mac_address))
                self.bluetooth_gatt_list.append(bluetooth_gatt)
            except GattTestUtilsError as err:
                self.log.error(err)
                return False
            self.log.info("Disconnecting from peripheral device.")
            test_result = self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                               gatt_callback)
            if not test_result:
                self.log.info("Failed to disconnect from peripheral device.")
                return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6ec766ca-6358-48ff-9d85-ede4d2756546')
    def test_bredr_write_descriptor_stress(self):
        """Test GATT connection writing and reading descriptors.

        Test establishing a gatt connection between a GATT server and GATT
        client with multiple service uuid variations.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a GATT connection between the scanner and advertiser.
        6. Discover services.
        7. Write data to the descriptors of each characteristic 100 times.
        8. Read the data sent to the descriptors.
        9. Disconnect the GATT connection.

        Expected Result:
        Each descriptor in each characteristic is written and read 100 times.

        Returns:
          Pass if True
          Fail if False

        TAGS: BR/EDR, Scanning, GATT, Stress, Characteristics, Descriptors
        Priority: 1
        """
        try:
            gatt_server_callback, gatt_server = setup_multiple_services(
                self.per_ad)
            self.gatt_server_list.append(gatt_server)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad,
                                            gatt_transport['bredr'],
                                            self.per_droid_mac_address))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        if self.cen_ad.droid.gattClientDiscoverServices(bluetooth_gatt):
            try:
                event = self.cen_ad.ed.pop_event(
                    gatt_cb_strings['gatt_serv_disc'].format(gatt_callback),
                    self.default_timeout)
            except Empty as err:
                self.log.error("Event not found: {}".format(err))
                return False
            discovered_services_index = event['data']['ServicesIndex']
        else:
            self.log.info("Failed to discover services.")
            return False
        services_count = self.cen_ad.droid.gattClientGetDiscoveredServicesCount(
            discovered_services_index)

        connected_device_list = self.per_ad.droid.gattServerGetConnectedDevices(
            gatt_server)
        if len(connected_device_list) == 0:
            self.log.info("No devices connected from peripheral.")
            return False
        bt_device_id = 0
        status = 1
        offset = 1
        test_value = [1, 2, 3, 4, 5, 6, 7]
        test_value_return = [1, 2, 3]
        for i in range(services_count):
            characteristic_uuids = (
                self.cen_ad.droid.gattClientGetDiscoveredCharacteristicUuids(
                    discovered_services_index, i))
            for characteristic in characteristic_uuids:
                descriptor_uuids = (
                    self.cen_ad.droid.gattClientGetDiscoveredDescriptorUuids(
                        discovered_services_index, i, characteristic))
                for _ in range(100):
                    for descriptor in descriptor_uuids:
                        self.cen_ad.droid.gattClientDescriptorSetValue(
                            bluetooth_gatt, discovered_services_index, i,
                            characteristic, descriptor, test_value)
                        self.cen_ad.droid.gattClientWriteDescriptor(
                            bluetooth_gatt, discovered_services_index, i,
                            characteristic, descriptor)
                        event = self.per_ad.ed.pop_event(gatt_cb_strings[
                            'desc_write_req'].format(gatt_server_callback),
                                                         self.default_timeout)
                        self.log.info(
                            "onDescriptorWriteRequest event found: {}".format(
                                event))
                        request_id = event['data']['requestId']
                        found_value = event['data']['value']
                        if found_value != test_value:
                            self.log.info("Values didn't match. Found: {}, "
                                          "Expected: {}".format(found_value,
                                                                test_value))
                            return False
                        self.per_ad.droid.gattServerSendResponse(
                            gatt_server, bt_device_id, request_id, status,
                            offset, test_value_return)
                        self.log.info(
                            "onDescriptorWrite event found: {}".format(
                                self.cen_ad.ed.pop_event(gatt_cb_strings[
                                    'desc_write'].format(
                                        gatt_callback), self.default_timeout)))
        return True
