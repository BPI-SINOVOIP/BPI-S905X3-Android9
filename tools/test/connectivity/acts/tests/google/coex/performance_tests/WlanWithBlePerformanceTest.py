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

import time

from acts.test_utils.bt.bt_gatt_utils import close_gatt_client
from acts.test_utils.bt.bt_gatt_utils import disconnect_gatt_connection
from acts.test_utils.bt.bt_gatt_utils import GattTestUtilsError
from acts.test_utils.bt.bt_gatt_utils import orchestrate_gatt_connection
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.test_utils.coex.CoexBaseTest import CoexBaseTest


class WlanWithBlePerformanceTest(CoexBaseTest):
    default_timeout = 10
    adv_instances = []
    bluetooth_gatt_list = []
    gatt_server_list = []

    def __init__(self, controllers):
        CoexBaseTest.__init__(self, controllers)

    def setup_test(self):
        CoexBaseTest.setup_test(self)
        self.pri_ad.droid.bluetoothDisableBLE()
        self.gatt_server_list = []
        self.adv_instances = []

    def teardown_test(self):
        CoexBaseTest.teardown_test(self)
        for bluetooth_gatt in self.bluetooth_gatt_list:
            self.pri_ad.droid.gattClientClose(bluetooth_gatt)
        for gatt_server in self.gatt_server_list:
            self.sec_ad.droid.gattServerClose(gatt_server)
        for adv in self.adv_instances:
            self.sec_ad.droid.bleStopBleAdvertising(adv)
        return True

    def _orchestrate_gatt_disconnection(self, bluetooth_gatt, gatt_callback):
        """Disconnect gatt connection between two devices.

        Args:
            bluetooth_gatt: Index of the BluetoothGatt object
            gatt_callback: Index of gatt callback object.

        Steps:
        1. Disconnect gatt connection.
        2. Close bluetooth gatt object.

        Returns:
            True if successful, False otherwise.
        """
        self.log.info("Disconnecting from peripheral device.")
        try:
            disconnect_gatt_connection(self.pri_ad, bluetooth_gatt,
                                       gatt_callback)
            close_gatt_client(self.pri_ad, bluetooth_gatt)
            if bluetooth_gatt in self.bluetooth_gatt_list:
                self.bluetooth_gatt_list.remove(bluetooth_gatt)
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        return True

    def ble_start_stop_scan(self):
        """Convenience method to start BLE scan and stop BLE scan.

        Steps:
        1. Enable ble.
        2. Create LE scan objects.
        3. Start scan.
        4. Stop scan.

        Returns:
            True if successful, False otherwise.
        """
        self.pri_ad.droid.bluetoothEnableBLE()
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.pri_ad.droid)
        self.pri_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        time.sleep(self.iperf["duration"])
        try:
            self.pri_ad.droid.bleStopBleScan(scan_callback)
        except Exception as err:
            self.log.error(str(err))
            return False
        return True

    def initiate_ble_gatt_connection(self):
        """Creates gatt connection and disconnect gatt connection.

        Steps:
        1. Initializes gatt objects.
        2. Start a generic advertisement.
        3. Start a generic scanner.
        4. Find the advertisement and extract the mac address.
        5. Stop the first scanner.
        6. Create a GATT connection between the scanner and advertiser.
        7. Disconnect the GATT connection.

        Returns:
            True if successful, False otherwise.
        """
        self.pri_ad.droid.bluetoothEnableBLE()
        gatt_server_cb = self.sec_ad.droid.gattServerCreateGattServerCallback()
        gatt_server = self.sec_ad.droid.gattServerOpenGattServer(gatt_server_cb)
        self.gatt_server_list.append(gatt_server)
        try:
            bluetooth_gatt, gatt_callback, adv_callback = (
                orchestrate_gatt_connection(self.pri_ad, self.sec_ad))
            self.bluetooth_gatt_list.append(bluetooth_gatt)
            time.sleep(self.iperf["duration"])
        except GattTestUtilsError as err:
            self.log.error(err)
            return False
        self.adv_instances.append(adv_callback)
        return self._orchestrate_gatt_disconnection(bluetooth_gatt,
                                                    gatt_callback)

    def ble_start_stop_scan_with_iperf(self):
        self.run_iperf_and_get_result()
        if not self.ble_start_stop_scan():
            return False
        return self.teardown_result()

    def ble_gatt_connection_with_iperf(self):
        self.run_iperf_and_get_result()
        if not self.initiate_ble_gatt_connection():
            return False
        return self.teardown_result()

    def test_performance_ble_scan_tcp_ul(self):
        """Test performance with ble scan.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the wlan throughput when performing ble scan.

        Steps:
        1. Start TCP-uplink traffic.
        2. Start and stop BLE scan.

        Returns:
            True if pass, False otherwise.

        Test Id: Bt_CoEx_Kpi_021
        """
        if not self.ble_start_stop_scan_with_iperf():
            return False
        return True

    def test_performance_ble_scan_tcp_dl(self):
        """Test performance with ble scan.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the wlan throughput when performing ble scan.

        Steps:
        1. Start TCP-downlink traffic.
        2. Start and stop BLE scan.

        Returns:
            True if pass, False otherwise.

        Test Id: Bt_CoEx_Kpi_022
        """
        if not self.ble_start_stop_scan_with_iperf():
            return False
        return True

    def test_performance_ble_scan_udp_ul(self):
        """Test performance with ble scan.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the wlan throughput when performing ble scan.

        Steps:
        1. Start UDP-uplink traffic.
        2. Start and stop BLE scan.

        Returns:
            True if pass, False otherwise.

        Test Id: Bt_CoEx_Kpi_023
        """
        if not self.ble_start_stop_scan_with_iperf():
            return False
        return True

    def test_performance_ble_scan_udp_dl(self):
        """Test performance with ble scan.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the wlan throughput when performing ble scan.

        Steps:
        1. Start UDP-uplink traffic.
        2. Start and stop BLE scan.

        Returns:
            True if pass, False otherwise.

        Test Id: Bt_CoEx_Kpi_024
        """
        if not self.ble_start_stop_scan_with_iperf():
            return False
        return True

    def test_performance_ble_connect_tcp_ul(self):
        """Test performance with ble gatt connection.

        This test is to start TCP-uplink traffic between host machine and
        android device and test the wlan throughput when ble gatt connection
        is established.

        Steps:
        1. Start TCP-uplink traffic.
        2. Initiate gatt connection.

        Returns:
            True if pass, False otherwise.

        Test Id: Bt_CoEx_Kpi_025
        """
        if not self.ble_gatt_connection_with_iperf():
            return False
        return True

    def test_performance_ble_connect_tcp_dl(self):
        """Test performance with ble gatt connection.

        This test is to start TCP-downlink traffic between host machine and
        android device and test the wlan throughput when ble gatt connection
        is established.

        Steps:
        1. Start TCP-downlink traffic.
        2. Initiate gatt connection.

        Returns:
            True if pass, False otherwise.

        Test Id: Bt_CoEx_Kpi_026
        """
        if not self.ble_gatt_connection_with_iperf():
            return False
        return True

    def test_performance_ble_connect_udp_ul(self):
        """Test performance with ble gatt connection.

        This test is to start UDP-uplink traffic between host machine and
        android device and test the wlan throughput when ble gatt connection
        is established.

        Steps:
        1. Start UDP-uplink traffic.
        2. Initiate gatt connection.

        Returns:
            True if pass, False otherwise.

        Test Id: Bt_CoEx_Kpi_027
        """
        if not self.ble_gatt_connection_with_iperf():
            return False
        return True

    def test_performance_ble_connect_udp_dl(self):
        """Test performance with ble gatt connection.

        This test is to start UDP-downlink traffic between host machine and
        android device and test the wlan throughput when ble gatt connection
        is established.

        Steps:
        1. Start UDP-downlink traffic.
        2. Initiate gatt connection.

        Returns:
            True if pass, False otherwise.

        Test Id: Bt_CoEx_Kpi_028
        """
        if not self.ble_gatt_connection_with_iperf():
            return False
        return True
