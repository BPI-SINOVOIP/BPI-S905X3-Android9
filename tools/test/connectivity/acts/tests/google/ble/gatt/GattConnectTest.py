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
This test script exercises different GATT connection tests.
"""

import pprint
from queue import Empty
import time

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import ble_advertise_settings_modes
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_constants import ble_scan_settings_match_nums
from acts.test_utils.bt.bt_constants import bt_profile_constants
from acts.test_utils.bt.bt_constants import gatt_characteristic
from acts.test_utils.bt.bt_constants import gatt_descriptor
from acts.test_utils.bt.bt_constants import gatt_service_types
from acts.test_utils.bt.bt_constants import gatt_cb_err
from acts.test_utils.bt.bt_constants import gatt_cb_strings
from acts.test_utils.bt.bt_constants import gatt_connection_state
from acts.test_utils.bt.bt_constants import gatt_mtu_size
from acts.test_utils.bt.bt_constants import gatt_phy_mask
from acts.test_utils.bt.bt_constants import gatt_transport
from acts.test_utils.bt.bt_constants import scan_result
from acts.test_utils.bt.bt_gatt_utils import GattTestUtilsError
from acts.test_utils.bt.bt_gatt_utils import disconnect_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import wait_for_gatt_disconnect_event
from acts.test_utils.bt.bt_gatt_utils import close_gatt_client
from acts.test_utils.bt.bt_gatt_utils import log_gatt_server_uuids
from acts.test_utils.bt.bt_gatt_utils import orchestrate_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import setup_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import setup_multiple_services
from acts.test_utils.bt.bt_test_utils import get_mac_address_of_generic_advertisement
from acts.test_utils.bt.bt_test_utils import clear_bonded_devices

PHYSICAL_DISCONNECT_TIMEOUT = 5


class GattConnectTest(BluetoothBaseTest):
    adv_instances = []
    bluetooth_gatt_list = []
    gatt_server_list = []
    default_timeout = 10
    default_discovery_timeout = 3

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.cen_ad = self.android_devices[0]
        self.per_ad = self.android_devices[1]

    def setup_test(self):
        super(BluetoothBaseTest, self).setup_test()
        bluetooth_gatt_list = []
        self.gatt_server_list = []
        self.adv_instances = []
        # Ensure there is ample time for a physical disconnect in between
        # testcases.
        self.log.info(
            "Waiting for {} seconds for physical GATT disconnections".format(
                PHYSICAL_DISCONNECT_TIMEOUT))
        time.sleep(PHYSICAL_DISCONNECT_TIMEOUT)

    def teardown_test(self):
        for bluetooth_gatt in self.bluetooth_gatt_list:
            self.cen_ad.droid.gattClientClose(bluetooth_gatt)
        for gatt_server in self.gatt_server_list:
            self.per_ad.droid.gattServerClose(gatt_server)
        for adv in self.adv_instances:
            self.per_ad.droid.bleStopBleAdvertising(adv)
        return True

    def _orchestrate_gatt_disconnection(self, bluetooth_gatt, gatt_callback):
        self.log.info("Disconnecting from peripheral device.")
        try:
            disconnect_gatt_connection(self.cen_ad, bluetooth_gatt,
                                       gatt_callback)
            close_gatt_client(self.cen_ad, bluetooth_gatt)
            if bluetooth_gatt in self.bluetooth_gatt_list:
                self.bluetooth_gatt_list.remove(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        return True

    def _find_service_added_event(self, gatt_server_cb, uuid):
        expected_event = gatt_cb_strings['serv_added'].format(gatt_server_cb)
        try:
            event = self.per_ad.ed.pop_event(expected_event,
                                             self.default_timeout)
        except Empty:
            self.log.error(
                gatt_cb_strings['serv_added_err'].format(expected_event))
            return False
        if event['data']['serviceUuid'].lower() != uuid.lower():
            self.log.error("Uuid mismatch. Found: {}, Expected {}.".format(
                event['data']['serviceUuid'], uuid))
            return False
        return True

    def _verify_mtu_changed_on_client_and_server(
            self, expected_mtu, gatt_callback, gatt_server_callback):
        expected_event = gatt_cb_strings['mtu_changed'].format(gatt_callback)
        try:
            mtu_event = self.cen_ad.ed.pop_event(expected_event,
                                                 self.default_timeout)
            mtu_size_found = mtu_event['data']['MTU']
            if mtu_size_found != expected_mtu:
                self.log.error("MTU size found: {}, expected: {}".format(
                    mtu_size_found, expected_mtu))
                return False
        except Empty:
            self.log.error(
                gatt_cb_err['mtu_changed_err'].format(expected_event))
            return False

        expected_event = gatt_cb_strings['mtu_serv_changed'].format(
            gatt_server_callback)
        try:
            mtu_event = self.per_ad.ed.pop_event(expected_event,
                                                 self.default_timeout)
            mtu_size_found = mtu_event['data']['MTU']
            if mtu_size_found != expected_mtu:
                self.log.error("MTU size found: {}, expected: {}".format(
                    mtu_size_found, expected_mtu))
                return False
        except Empty:
            self.log.error(
                gatt_cb_err['mtu_serv_changed_err'].format(expected_event))
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8a3530a3-c8bb-466b-9710-99e694c38618')
    def test_gatt_connect(self):
        """Test GATT connection over LE.

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

        TAGS: LE, Advertising, Filtering, Scanning, GATT
        Priority: 0
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        self.adv_instances.append(adv_callback)
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a839b505-03ac-4783-be7e-1d43129a1948')
    def test_gatt_connect_stop_advertising(self):
        """Test GATT connection over LE then stop advertising

        A test case that verifies the GATT connection doesn't
        disconnect when LE advertisement is stopped.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a GATT connection between the scanner and advertiser.
        6. Stop the advertiser.
        7. Verify no connection state changed happened.
        8. Disconnect the GATT connection.

        Expected Result:
        Verify that a connection was established and not disconnected
        when advertisement stops.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, GATT
        Priority: 0
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        self.per_ad.droid.bleStopBleAdvertising(adv_callback)
        try:
            event = self.cen_ad.ed.pop_event(
                gatt_cb_strings['gatt_conn_change'].format(
                    gatt_callback, self.default_timeout))
            self.log.error(
                "Connection event found when not expected: {}".format(event))
            return False
        except Empty:
            self.log.info("No connection state change as expected")
        try:
            self._orchestrate_gatt_disconnection(bluetooth_gatt, gatt_callback)
        except Exception as err:
            self.log.info("Failed to orchestrate disconnect: {}".format(err))
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b82f91a8-54bb-4779-a117-73dc7fdb28cc')
    def test_gatt_connect_autoconnect(self):
        """Test GATT connection over LE.

        Test re-establishing a gatt connection using autoconnect
        set to True in order to test connection whitelist.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a GATT connection between the scanner and advertiser.
        6. Disconnect the GATT connection.
        7. Create a GATT connection with autoconnect set to True
        8. Disconnect the GATT connection.

        Expected Result:
        Verify that a connection was re-established and then disconnected
        successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, GATT
        Priority: 0
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        autoconnect = False
        mac_address, adv_callback, scan_callback = (
            get_mac_address_of_generic_advertisement(self.cen_ad, self.per_ad))
        try:
            bluetooth_gatt, gatt_callback = setup_gatt_connection(
                self.cen_ad, mac_address, autoconnect)
            self.cen_ad.droid.bleStopBleScan(scan_callback)
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        try:
            disconnect_gatt_connection(self.cen_ad, bluetooth_gatt,
                                       gatt_callback)
            close_gatt_client(self.cen_ad, bluetooth_gatt)
            if bluetooth_gatt in self.bluetooth_gatt_list:
                self.bluetooth_gatt_list.remove(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        autoconnect = True
        bluetooth_gatt = self.cen_ad.droid.gattClientConnectGatt(
            gatt_callback, mac_address, autoconnect, gatt_transport['auto'],
            False, gatt_phy_mask['1m_mask'])
        self.bluetooth_gatt_list.append(bluetooth_gatt)
        expected_event = gatt_cb_strings['gatt_conn_change'].format(
            gatt_callback)
        try:
            event = self.cen_ad.ed.pop_event(expected_event,
                                             self.default_timeout)
        except Empty:
            self.log.error(
                gatt_cb_err['gatt_conn_change_err'].format(expected_event))
            test_result = False
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e506fa50-7cd9-4bd8-938a-6b85dcfe6bc6')
    def test_gatt_connect_opportunistic(self):
        """Test opportunistic GATT connection over LE.

        Test establishing a gatt connection between a GATT server and GATT
        client in opportunistic mode.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create GATT connection 1 between the scanner and advertiser normally
        6. Create GATT connection 2 between the scanner and advertiser using
           opportunistic mode
        7. Disconnect GATT connection 1

        Expected Result:
        Verify GATT connection 2 automatically disconnects when GATT connection
        1 disconnect

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, GATT
        Priority: 0
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        mac_address, adv_callback, scan_callback = (
            get_mac_address_of_generic_advertisement(self.cen_ad, self.per_ad))
        # Make GATT connection 1
        try:
            bluetooth_gatt_1, gatt_callback_1 = setup_gatt_connection(
                self.cen_ad,
                mac_address,
                False,
                transport=gatt_transport['auto'],
                opportunistic=False)
            self.cen_ad.droid.bleStopBleScan(scan_callback)
            self.bluetooth_gatt_list.append(bluetooth_gatt_1)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        # Make GATT connection 2
        try:
            bluetooth_gatt_2, gatt_callback_2 = setup_gatt_connection(
                self.cen_ad,
                mac_address,
                False,
                transport=gatt_transport['auto'],
                opportunistic=True)
            self.bluetooth_gatt_list.append(bluetooth_gatt_2)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        # Disconnect GATT connection 1
        try:
            disconnect_gatt_connection(self.cen_ad, bluetooth_gatt_1,
                                       gatt_callback_1)
            close_gatt_client(self.cen_ad, bluetooth_gatt_1)
            if bluetooth_gatt_1 in self.bluetooth_gatt_list:
                self.bluetooth_gatt_list.remove(bluetooth_gatt_1)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        # Confirm that GATT connection 2 also disconnects
        wait_for_gatt_disconnect_event(self.cen_ad, gatt_callback_2)
        close_gatt_client(self.cen_ad, bluetooth_gatt_2)
        if bluetooth_gatt_2 in self.bluetooth_gatt_list:
            self.bluetooth_gatt_list.remove(bluetooth_gatt_2)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1e01838e-c4de-4720-9adf-9e0419378226')
    def test_gatt_request_min_mtu(self):
        """Test GATT connection over LE and exercise MTU sizes.

        Test establishing a gatt connection between a GATT server and GATT
        client. Request an MTU size that matches the correct minimum size.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a GATT connection between the scanner and advertiser.
        6. From the scanner (client) request MTU size change to the
        minimum value.
        7. Find the MTU changed event on the client.
        8. Disconnect the GATT connection.

        Expected Result:
        Verify that a connection was established and the MTU value found
        matches the expected MTU value.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, GATT, MTU
        Priority: 0
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        self.adv_instances.append(adv_callback)
        expected_mtu = gatt_mtu_size['min']
        self.cen_ad.droid.gattClientRequestMtu(bluetooth_gatt, expected_mtu)
        if not self._verify_mtu_changed_on_client_and_server(
                expected_mtu, gatt_callback, gatt_server_cb):
            return False
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c1fa3a2d-fb47-47db-bdd1-458928cd6a5f')
    def test_gatt_request_max_mtu(self):
        """Test GATT connection over LE and exercise MTU sizes.

        Test establishing a gatt connection between a GATT server and GATT
        client. Request an MTU size that matches the correct maximum size.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a GATT connection between the scanner and advertiser.
        6. From the scanner (client) request MTU size change to the
        maximum value.
        7. Find the MTU changed event on the client.
        8. Disconnect the GATT connection.

        Expected Result:
        Verify that a connection was established and the MTU value found
        matches the expected MTU value.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, GATT, MTU
        Priority: 0
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        self.adv_instances.append(adv_callback)
        expected_mtu = gatt_mtu_size['max']
        self.cen_ad.droid.gattClientRequestMtu(bluetooth_gatt, expected_mtu)
        if not self._verify_mtu_changed_on_client_and_server(
                expected_mtu, gatt_callback, gatt_server_cb):
            return False
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4416d483-dec3-46cb-8038-4d82620f873a')
    def test_gatt_request_out_of_bounds_mtu(self):
        """Test GATT connection over LE and exercise an out of bound MTU size.

        Test establishing a gatt connection between a GATT server and GATT
        client. Request an MTU size that is the MIN value minus 1.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a GATT connection between the scanner and advertiser.
        6. From the scanner (client) request MTU size change to the
        minimum value minus one.
        7. Find the MTU changed event on the client.
        8. Disconnect the GATT connection.

        Expected Result:
        Verify that an MTU changed event was not discovered and that
        it didn't cause an exception when requesting an out of bounds
        MTU.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, GATT, MTU
        Priority: 0
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        self.adv_instances.append(adv_callback)
        unexpected_mtu = gatt_mtu_size['min'] - 1
        self.cen_ad.droid.gattClientRequestMtu(bluetooth_gatt, unexpected_mtu)
        if self._verify_mtu_changed_on_client_and_server(
                unexpected_mtu, gatt_callback, gatt_server_cb):
            return False
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='31ffb9ca-cc75-43fb-9802-c19f1c5856b6')
    def test_gatt_connect_trigger_on_read_rssi(self):
        """Test GATT connection over LE read RSSI.

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

        TAGS: LE, Advertising, Filtering, Scanning, GATT, RSSI
        Priority: 1
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        self.adv_instances.append(adv_callback)
        expected_event = gatt_cb_strings['rd_remote_rssi'].format(
            gatt_callback)
        if self.cen_ad.droid.gattClientReadRSSI(bluetooth_gatt):
            try:
                self.cen_ad.ed.pop_event(expected_event, self.default_timeout)
            except Empty:
                self.log.error(
                    gatt_cb_err['rd_remote_rssi_err'].format(expected_event))
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='dee9ef28-b872-428a-821b-cc62f27ba936')
    def test_gatt_connect_trigger_on_services_discovered(self):
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

        TAGS: LE, Advertising, Filtering, Scanning, GATT, Services
        Priority: 1
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        self.adv_instances.append(adv_callback)
        if self.cen_ad.droid.gattClientDiscoverServices(bluetooth_gatt):
            expected_event = gatt_cb_strings['gatt_serv_disc'].format(
                gatt_callback)
            try:
                event = self.cen_ad.ed.pop_event(expected_event,
                                                 self.default_timeout)
            except Empty:
                self.log.error(
                    gatt_cb_err['gatt_serv_disc'].format(expected_event))
                return False
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='01883bdd-0cf8-48fb-bf15-467bbd4f065b')
    def test_gatt_connect_trigger_on_services_discovered_iterate_attributes(
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

        TAGS: LE, Advertising, Filtering, Scanning, GATT, Services
        Characteristics, Descriptors
        Priority: 1
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        self.adv_instances.append(adv_callback)
        if self.cen_ad.droid.gattClientDiscoverServices(bluetooth_gatt):
            expected_event = gatt_cb_strings['gatt_serv_disc'].format(
                gatt_callback)
            try:
                event = self.cen_ad.ed.pop_event(expected_event,
                                                 self.default_timeout)
                discovered_services_index = event['data']['ServicesIndex']
            except Empty:
                self.log.error(
                    gatt_cb_err['gatt_serv_disc'].format(expected_event))
                return False
            log_gatt_server_uuids(self.cen_ad, discovered_services_index)
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d4277bee-da99-4f48-8a4d-f81b5389da18')
    def test_gatt_connect_with_service_uuid_variations(self):
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

        TAGS: LE, Advertising, Filtering, Scanning, GATT, Services
        Priority: 2
        """
        try:
            gatt_server_cb, gatt_server = setup_multiple_services(self.per_ad)
            self.gatt_server_list.append(gatt_server)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        self.adv_instances.append(adv_callback)
        if self.cen_ad.droid.gattClientDiscoverServices(bluetooth_gatt):
            expected_event = gatt_cb_strings['gatt_serv_disc'].format(
                gatt_callback)
            try:
                event = self.cen_ad.ed.pop_event(expected_event,
                                                 self.default_timeout)
            except Empty:
                self.log.error(
                    gatt_cb_err['gatt_serv_disc'].format(expected_event))
                return False
            discovered_services_index = event['data']['ServicesIndex']
            log_gatt_server_uuids(self.cen_ad, discovered_services_index)

        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7d3442c5-f71f-44ae-bd35-f2569f01b3b8')
    def test_gatt_connect_in_quick_succession(self):
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

        TAGS: LE, Advertising, Filtering, Scanning, GATT, Stress
        Priority: 1
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        mac_address, adv_callback, scan_callback = get_mac_address_of_generic_advertisement(
            self.cen_ad, self.per_ad)
        autoconnect = False
        for i in range(1000):
            self.log.info("Starting connection iteration {}".format(i + 1))
            try:
                bluetooth_gatt, gatt_callback = setup_gatt_connection(
                    self.cen_ad, mac_address, autoconnect)
                self.cen_ad.droid.bleStopBleScan(scan_callback)
            except GattTestUtilsError as err:
                self.log.error(err)
                return False
            test_result = self._orchestrate_gatt_disconnection(
                bluetooth_gatt, gatt_callback)
            if not test_result:
                self.log.info("Failed to disconnect from peripheral device.")
                return False
        self.adv_instances.append(adv_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='148469d9-7ab0-4c08-b2e9-7e49e88da1fc')
    def test_gatt_connect_mitm_attack(self):
        """Test GATT connection with permission write encrypted mitm.

        Test establishing a gatt connection between a GATT server and GATT
        client while the GATT server's characteristic includes the property
        write value and the permission write encrypted mitm value. This will
        prompt LE pairing and then the devices will create a bond.

        Steps:
        1. Create a GATT server and server callback on the peripheral device.
        2. Create a unique service and characteristic uuid on the peripheral.
        3. Create a characteristic on the peripheral with these properties:
            gatt_characteristic['property_write'],
            gatt_characteristic['permission_write_encrypted_mitm']
        4. Create a GATT service on the peripheral.
        5. Add the characteristic to the GATT service.
        6. Create a GATT connection between your central and peripheral device.
        7. From the central device, discover the peripheral's services.
        8. Iterate the services found until you find the unique characteristic
            created in step 3.
        9. Once found, write a random but valid value to the characteristic.
        10. Start pairing helpers on both devices immediately after attempting
            to write to the characteristic.
        11. Within 10 seconds of writing the characteristic, there should be
            a prompt to bond the device from the peripheral. The helpers will
            handle the UI interaction automatically. (see
            BluetoothConnectionFacade.java bluetoothStartPairingHelper).
        12. Verify that the two devices are bonded.

        Expected Result:
        Verify that a connection was established and the devices are bonded.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, GATT, Characteristic, MITM
        Priority: 1
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        service_uuid = "3846D7A0-69C8-11E4-BA00-0002A5D5C51B"
        test_uuid = "aa7edd5a-4d1d-4f0e-883a-d145616a1630"
        bonded = False
        characteristic = self.per_ad.droid.gattServerCreateBluetoothGattCharacteristic(
            test_uuid, gatt_characteristic['property_write'],
            gatt_characteristic['permission_write_encrypted_mitm'])
        gatt_service = self.per_ad.droid.gattServerCreateService(
            service_uuid, gatt_service_types['primary'])
        self.per_ad.droid.gattServerAddCharacteristicToService(
            gatt_service, characteristic)
        self.per_ad.droid.gattServerAddService(gatt_server, gatt_service)
        result = self._find_service_added_event(gatt_server_cb, service_uuid)
        if not result:
            return False
        bluetooth_gatt, gatt_callback, adv_callback = (
            orchestrate_gatt_connection(self.cen_ad, self.per_ad))
        self.bluetooth_gatt_list.append(bluetooth_gatt)
        self.adv_instances.append(adv_callback)
        if self.cen_ad.droid.gattClientDiscoverServices(bluetooth_gatt):
            expected_event = gatt_cb_strings['gatt_serv_disc'].format(
                gatt_callback)
            try:
                event = self.cen_ad.ed.pop_event(expected_event,
                                                 self.default_timeout)
            except Empty:
                self.log.error(
                    gatt_cb_err['gatt_serv_disc'].format(expected_event))
                return False
            discovered_services_index = event['data']['ServicesIndex']
        else:
            self.log.info("Failed to discover services.")
            return False
        test_value = [1, 2, 3, 4, 5, 6, 7]
        services_count = self.cen_ad.droid.gattClientGetDiscoveredServicesCount(
            discovered_services_index)
        for i in range(services_count):
            characteristic_uuids = (
                self.cen_ad.droid.gattClientGetDiscoveredCharacteristicUuids(
                    discovered_services_index, i))
            for characteristic_uuid in characteristic_uuids:
                if characteristic_uuid == test_uuid:
                    self.cen_ad.droid.bluetoothStartPairingHelper()
                    self.per_ad.droid.bluetoothStartPairingHelper()
                    self.cen_ad.droid.gattClientCharacteristicSetValue(
                        bluetooth_gatt, discovered_services_index, i,
                        characteristic_uuid, test_value)
                    self.cen_ad.droid.gattClientWriteCharacteristic(
                        bluetooth_gatt, discovered_services_index, i,
                        characteristic_uuid)
                    start_time = time.time() + self.default_timeout
                    target_name = self.per_ad.droid.bluetoothGetLocalName()
                    while time.time() < start_time and bonded == False:
                        bonded_devices = \
                            self.cen_ad.droid.bluetoothGetBondedDevices()
                        for device in bonded_devices:
                            if ('name' in device.keys()
                                    and device['name'] == target_name):
                                bonded = True
                                break
                    bonded = False
                    target_name = self.cen_ad.droid.bluetoothGetLocalName()
                    while time.time() < start_time and bonded == False:
                        bonded_devices = \
                            self.per_ad.droid.bluetoothGetBondedDevices()
                        for device in bonded_devices:
                            if ('name' in device.keys()
                                    and device['name'] == target_name):
                                bonded = True
                                break
        for ad in [self.cen_ad, self.per_ad]:
            if not clear_bonded_devices(ad):
                return False
            # Necessary sleep time for entries to update unbonded state
            time.sleep(2)
            bonded_devices = ad.droid.bluetoothGetBondedDevices()
            if len(bonded_devices) > 0:
                self.log.error(
                    "Failed to unbond devices: {}".format(bonded_devices))
                return False
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cc3fc361-7bf1-4ee2-9e46-4a27c88ce6a8')
    def test_gatt_connect_get_connected_devices(self):
        """Test GATT connections show up in getConnectedDevices

        Test establishing a gatt connection between a GATT server and GATT
        client. Verify that active connections show up using
        BluetoothManager.getConnectedDevices API.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a GATT connection between the scanner and advertiser.
        7. Verify the GATT Client has an open connection to the GATT Server.
        8. Verify the GATT Server has an open connection to the GATT Client.
        9. Disconnect the GATT connection.

        Expected Result:
        Verify that a connection was established, connected devices are found
        on both the central and peripheral devices, and then disconnected
        successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, GATT
        Priority: 2
        """
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.cen_ad, self.per_ad))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        conn_cen_devices = self.cen_ad.droid.bluetoothGetConnectedLeDevices(
            bt_profile_constants['gatt'])
        conn_per_devices = self.per_ad.droid.bluetoothGetConnectedLeDevices(
            bt_profile_constants['gatt_server'])
        target_name = self.per_ad.droid.bluetoothGetLocalName()
        error_message = ("Connected device {} not found in list of connected "
                         "devices {}")
        if not any(d['name'] == target_name for d in conn_cen_devices):
            self.log.error(error_message.format(target_name, conn_cen_devices))
            return False
        # For the GATT server only check the size of the list since
        # it may or may not include the device name.
        target_name = self.cen_ad.droid.bluetoothGetLocalName()
        if not conn_per_devices:
            self.log.error(error_message.format(target_name, conn_per_devices))
            return False
        self.adv_instances.append(adv_callback)
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a0a37ca6-9fa8-4d35-9fdb-0e25b4b8a363')
    def test_gatt_connect_second_adv_after_canceling_first_adv(self):
        """Test GATT connection to peripherals second advertising address.

        The the ability of cancelling GATT connections and trying to reconnect
        to the same device via a different address.

        Steps:
        1. A starts advertising
        2. B starts scanning and finds A's mac address
        3. Stop advertisement from step 1. Start a new advertisement on A and
            find the new new mac address, B knows of both old and new address.
        4. B1 sends connect request to old address of A
        5. B1 cancel connect attempt after 10 seconds
        6. B1 sends connect request to new address of A
        7. Verify B1 establish connection to A in less than 10 seconds

        Expected Result:
        Verify that a connection was established only on the second
            advertisement's mac address.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, GATT
        Priority: 3
        """
        autoconnect = False
        transport = gatt_transport['auto']
        opportunistic = False
        # Setup a basic Gatt server on the peripheral
        gatt_server_cb = self.per_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.per_ad.droid.gattServerOpenGattServer(
            gatt_server_cb)

        # Set advertisement settings to include local name in advertisement
        # and set the advertising mode to low_latency.
        self.per_ad.droid.bleSetAdvertiseSettingsIsConnectable(True)
        self.per_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.per_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
            ble_advertise_settings_modes['low_latency'])

        # Setup necessary advertisement objects.
        advertise_data = self.per_ad.droid.bleBuildAdvertiseData()
        advertise_settings = self.per_ad.droid.bleBuildAdvertiseSettings()
        advertise_callback = self.per_ad.droid.bleGenBleAdvertiseCallback()

        # Step 1: Start advertisement
        self.per_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)

        # Setup scan settings for low_latency scanning and to include the local name
        # of the advertisement started in step 1.
        filter_list = self.cen_ad.droid.bleGenFilterList()
        self.cen_ad.droid.bleSetScanSettingsNumOfMatches(
            ble_scan_settings_match_nums['one'])
        self.cen_ad.droid.bleSetScanFilterDeviceName(
            self.per_ad.droid.bluetoothGetLocalName())
        self.cen_ad.droid.bleBuildScanFilter(filter_list)
        self.cen_ad.droid.bleSetScanSettingsScanMode(
            ble_scan_settings_modes['low_latency'])

        # Setup necessary scan objects.
        scan_settings = self.cen_ad.droid.bleBuildScanSetting()
        scan_callback = self.cen_ad.droid.bleGenScanCallback()

        # Step 2: Start scanning on central Android device and find peripheral
        # address.
        self.cen_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        expected_event_name = scan_result.format(scan_callback)
        try:
            mac_address_pre_restart = self.cen_ad.ed.pop_event(
                expected_event_name, self.default_timeout)['data']['Result'][
                    'deviceInfo']['address']
            self.log.info(
                "Peripheral advertisement found with mac address: {}".format(
                    mac_address_pre_restart))
        except Empty:
            self.log.info("Peripheral advertisement not found")
            test_result = False

        # Step 3: Restart peripheral advertising such that a new mac address is
        # created.
        self.per_ad.droid.bleStopBleAdvertising(advertise_callback)
        self.per_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)

        try:
            mac_address_post_restart = self.cen_ad.ed.pop_event(
                expected_event_name, self.default_timeout)['data']['Result'][
                    'deviceInfo']['address']
            self.log.info(
                "Peripheral advertisement found with mac address: {}".format(
                    mac_address_post_restart))
        except Empty:
            self.log.info("Peripheral advertisement not found")
            test_result = False

        # Steps 4: Try to connect to the first mac address
        gatt_callback = self.cen_ad.droid.gattCreateGattCallback()
        self.log.info(
            "Gatt Connect to mac address {}.".format(mac_address_pre_restart))
        bluetooth_gatt = self.cen_ad.droid.gattClientConnectGatt(
            gatt_callback, mac_address_pre_restart, autoconnect, transport,
            opportunistic, gatt_phy_mask['1m_mask'])
        self.bluetooth_gatt_list.append(bluetooth_gatt)
        expected_event = gatt_cb_strings['gatt_conn_change'].format(
            gatt_callback)
        try:
            event = self.cen_ad.ed.pop_event(expected_event,
                                             self.default_timeout)
            self.log.error(
                "Connection callback updated unexpectedly: {}".format(event))
            return False
        except Empty:
            self.log.info("No connection update as expected.")

        # Step 5: Cancel connection request.
        self.cen_ad.droid.gattClientDisconnect(bluetooth_gatt)

        # Step 6: Connect to second mac address.
        self.log.info(
            "Gatt Connect to mac address {}.".format(mac_address_post_restart))
        bluetooth_gatt = self.cen_ad.droid.gattClientConnectGatt(
            gatt_callback, mac_address_post_restart, autoconnect, transport,
            opportunistic, gatt_phy_mask['1m_mask'])
        self.bluetooth_gatt_list.append(bluetooth_gatt)
        expected_event = gatt_cb_strings['gatt_conn_change'].format(
            gatt_callback)

        # Step 7: Verify connection was setup successfully.
        try:
            event = self.cen_ad.ed.pop_event(expected_event,
                                             self.default_timeout)
            self.log.info(
                "Connection callback updated successfully: {}".format(event))
            if event['data']['State'] != gatt_connection_state['connected']:
                self.log.error(
                    "Could not establish a connection to the peripheral.")
                return False
        except Empty:
            self.log.error("No connection update was found.")
            return False
        return self.cen_ad.droid.gattClientDisconnect(bluetooth_gatt)
