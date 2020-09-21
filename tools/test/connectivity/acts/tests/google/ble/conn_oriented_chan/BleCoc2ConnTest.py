#/usr/bin/env python3.4
#
# Copyright 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""
Test script to execute Bluetooth Connection-orient Channel (CoC) functionality for
2 connections test cases. This test was designed to be run in a shield box.
"""

import threading
import time

from queue import Empty
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_coc_test_utils import orchestrate_coc_connection
from acts.test_utils.bt.bt_coc_test_utils import do_multi_connection_throughput
from acts.test_utils.bt.bt_constants import default_le_data_length
from acts.test_utils.bt.bt_constants import default_bluetooth_socket_timeout_ms
from acts.test_utils.bt.bt_constants import l2cap_coc_header_size
from acts.test_utils.bt.bt_constants import l2cap_max_inactivity_delay_after_disconnect
from acts.test_utils.bt.bt_constants import le_connection_event_time_step_ms
from acts.test_utils.bt.bt_test_utils import clear_bonded_devices
from acts.test_utils.bt.bt_test_utils import kill_bluetooth_process
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs
from acts.test_utils.bt.bt_test_utils import write_read_verify_data
from acts.test_utils.bt.bt_test_utils import verify_server_and_client_connected


class BleCoc2ConnTest(BluetoothBaseTest):
    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.client_ad = self.android_devices[0]
        self.server_ad = self.android_devices[1]
        # Note that some tests required a third device.
        if len(self.android_devices) > 2:
            self.server2_ad = self.android_devices[2]

    def setup_class(self):
        return setup_multiple_devices_for_bt_test(self.android_devices)

    def teardown_test(self):
        self.client_ad.droid.bluetoothSocketConnStop()
        self.server_ad.droid.bluetoothSocketConnStop()
        # Give sufficient time for the physical LE link to be disconnected.
        time.sleep(l2cap_max_inactivity_delay_after_disconnect)

    # This utility function calculates the max and min connection event (ce) time.
    # The formula is that the min/max ce time should be less than half the connection
    # interval and must be multiples of the le_connection_event_time_step.
    def _calc_min_max_ce_time(self, le_connection_interval):
        conn_event_time_steps = int((le_connection_interval/2)/le_connection_event_time_step_ms)
        conn_event_time_steps -= 1
        return (le_connection_event_time_step_ms * conn_event_time_steps)

    def _run_coc_connection_throughput_2_conn(
            self,
            is_secured,
            buffer_size,
            le_connection_interval=0,
            le_tx_data_length=default_le_data_length,
            min_ce_len=0,
            max_ce_len=0):

        # The num_iterations is that number of repetitions of each
        # set of buffers r/w.
        # number_buffers is the total number of data buffers to transmit per
        # set of buffers r/w.
        # buffer_size is the number of bytes per L2CAP data buffer.
        num_iterations = 10
        number_buffers = 100

        # Make sure at least 3 phones are setup
        if len(self.android_devices) <= 2:
            self.log.info("test_coc_connection_throughput_2_conn: "
                          "Error: 3rd phone not configured in file")
            return False

        self.log.info(
            "_run_coc_connection_throughput_2_conn: is_secured={}, Interval={}, buffer_size={}, "
            "le_tx_data_length={}, min_ce_len={}".format(is_secured, le_connection_interval,
                                          buffer_size, le_tx_data_length, min_ce_len))
        status, client_conn_id1, server_conn_id1 = orchestrate_coc_connection(
            self.client_ad, self.server_ad, True, is_secured,
            le_connection_interval, le_tx_data_length, default_bluetooth_socket_timeout_ms,
            min_ce_len, max_ce_len)
        if not status:
            return False

        status, client_conn_id2, server_conn_id2 = orchestrate_coc_connection(
            self.client_ad, self.server2_ad, True, is_secured,
            le_connection_interval, le_tx_data_length, default_bluetooth_socket_timeout_ms,
            min_ce_len, max_ce_len)
        if not status:
            return False

        list_server_ad = [self.server_ad, self.server2_ad]
        list_client_conn_id = [client_conn_id1, client_conn_id2]
        data_rate = do_multi_connection_throughput(
            self.client_ad, list_server_ad, list_client_conn_id,
            num_iterations, number_buffers, buffer_size)
        if data_rate <= 0:
            return False

        self.log.info(
            "test_coc_connection_throughput_2_conn: throughput=%d bytes per "
            "sec", data_rate)

        self.client_ad.droid.bluetoothSocketConnStop(client_conn_id1)
        self.client_ad.droid.bluetoothSocketConnStop(client_conn_id2)
        self.server_ad.droid.bluetoothSocketConnStop(server_conn_id1)
        self.server2_ad.droid.bluetoothSocketConnStop(server_conn_id2)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='27226006-b725-4312-920e-6193cf0539d4')
    def test_coc_insecured_connection_throughput_2_conn(self):
        """Test LE CoC data throughput on two insecured connections

        Test Data Throughput of 2 L2CAP CoC insecured connections.
        3 phones are required.

        Steps:
        1. Get the mac address of the server device.
        2. Establish a L2CAP CoC connection from the client to the server#1 AD.
        The connection is insecured.
        3. Verify that the L2CAP CoC connection is active from both the client
        and server.
        4. Establish a L2CAP CoC connection from the client to the server#2 AD.
        The connection is insecured.
        5. Verify that the L2CAP CoC connection is active from both the client
        and server.
        6. Write data from the client to both server#1 and server#2.
        7. Verify data matches from client and server

        Expected Result:
        L2CAP CoC connections are established and data written correctly to both servers.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 2
        """

        # Note: A 117 octets buffer size would fix nicely to a 123 bytes Data Length
        status = self._run_coc_connection_throughput_2_conn(False, 117)
        return status

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1a5fb032-8a27-42f1-933f-3e39311c09a6')
    def test_coc_secured_connection_throughput_2_conn(self):
        """Test LE CoC data throughput on two secured connections

        Test Data Throughput of 2 L2CAP CoC secured connections.
        3 phones are required.

        Steps:
        1. Get the mac address of the server device.
        2. Establish a L2CAP CoC connection from the client to the server#1 AD.
        The connection is secured.
        3. Verify that the L2CAP CoC connection is active from both the client
        and server.
        4. Establish a L2CAP CoC connection from the client to the server#2 AD.
        The connection is secured.
        5. Verify that the L2CAP CoC connection is active from both the client
        and server.
        6. Write data from the client to both server#1 and server#2.
        7. Verify data matches from client and server

        Expected Result:
        L2CAP CoC connections are established and data written correctly to both servers.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 2
        """

        # Note: A 117 octets buffer size would fix nicely to a 123 bytes Data Length
        status = self._run_coc_connection_throughput_2_conn(True, 117)
        return status

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b198f8cc-26af-44bd-bb4d-7dc8f8645617')
    def test_coc_connection_throughput_2_conn_NOSEC_10CI_60SIZE(self):
        """Test LE CoC data throughput with 10msec CI and 60bytes buffer size.

        Test data throughput of 2 L2CAP CoC insecured connections with 20msec connection interval
        and 60 bytes buffer size. 3 phones are required.

        Steps:
        1. Get the mac address of the server device.
        2. Establish a L2CAP CoC connection from the client to the server#1 AD.
        The connection is insecured.
        3. Set the connection interval to 20 msec and buffer size to 60 bytes.
        4. Verify that the L2CAP CoC connection is active from both the client
        and server.
        5. Establish a L2CAP CoC connection from the client to the server#2 AD.
        The connection is insecured.
        6. Set the connection interval to 20 msec and buffer size to 60 bytes.
        7. Verify that the L2CAP CoC connection is active from both the client
        and server.
        8. Write data from the client to both server#1 and server#2.
        9. Verify data matches from client and server

        Expected Result:
        L2CAP CoC connections are established and data written correctly to both servers.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 1
        """

        is_secured = False
        le_connection_interval = 10
        buffer_size = 60
        le_tx_data_length = buffer_size + l2cap_coc_header_size

        status = self._run_coc_connection_throughput_2_conn(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length,
            self._calc_min_max_ce_time(le_connection_interval),
            self._calc_min_max_ce_time(le_connection_interval))
        return status

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='12dc2a6c-8283-4617-a911-42335dd693a8')
    def test_coc_connection_throughput_2_conn_NOSEC_10CI_80SIZE(self):
        """Test LE CoC data throughput with 10msec CI and 80bytes buffer size.

        Test data throughput of 2 L2CAP CoC insecured connections with 20msec connection interval
        and 80 bytes buffer size. 3 phones are required.

        Steps:
        1. Get the mac address of the server device.
        2. Establish a L2CAP CoC connection from the client to the server#1 AD.
        The connection is insecured.
        3. Set the connection interval to 20 msec and buffer size to 80 bytes.
        4. Verify that the L2CAP CoC connection is active from both the client
        and server.
        5. Establish a L2CAP CoC connection from the client to the server#2 AD.
        The connection is insecured.
        6. Set the connection interval to 20 msec and buffer size to 80 bytes.
        7. Verify that the L2CAP CoC connection is active from both the client
        and server.
        8. Write data from the client to both server#1 and server#2.
        9. Verify data matches from client and server

        Expected Result:
        L2CAP CoC connections are established and data written correctly to both servers.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 1
        """

        is_secured = False
        le_connection_interval = 10
        buffer_size = 80
        le_tx_data_length = buffer_size + l2cap_coc_header_size
        status = self._run_coc_connection_throughput_2_conn(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length,
            self._calc_min_max_ce_time(le_connection_interval),
            self._calc_min_max_ce_time(le_connection_interval))
        return status

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4730df05-3909-4adf-a365-7f0c3258c402')
    def test_coc_connection_throughput_2_conn_NOSEC_10CI_120SIZE(self):
        """Test LE CoC data throughput with 10msec CI and 120bytes buffer size.

        Test data throughput of 2 L2CAP CoC insecured connections with 20msec connection interval
        and 120 bytes buffer size. 3 phones are required.

        Steps:
        1. Get the mac address of the server device.
        2. Establish a L2CAP CoC connection from the client to the server#1 AD.
        The connection is insecured.
        3. Set the connection interval to 20 msec and buffer size to 120 bytes.
        4. Verify that the L2CAP CoC connection is active from both the client
        and server.
        5. Establish a L2CAP CoC connection from the client to the server#2 AD.
        The connection is insecured.
        6. Set the connection interval to 20 msec and buffer size to 120 bytes.
        7. Verify that the L2CAP CoC connection is active from both the client
        and server.
        8. Write data from the client to both server#1 and server#2.
        9. Verify data matches from client and server

        Expected Result:
        L2CAP CoC connections are established and data written correctly to both servers.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 1
        """

        is_secured = False
        le_connection_interval = 10
        buffer_size = 120
        le_tx_data_length = buffer_size + l2cap_coc_header_size
        status = self._run_coc_connection_throughput_2_conn(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length,
            self._calc_min_max_ce_time(le_connection_interval),
            self._calc_min_max_ce_time(le_connection_interval))
        return status

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='471a8748-b0a5-4be5-9322-7c75e2b5d048')
    def test_coc_connection_throughput_2_conn_NOSEC_15CI_120SIZE(self):
        """Test LE CoC data throughput with 15msec CI and 120bytes buffer size.

        Test data throughput of 2 L2CAP CoC insecured connections with 15msec connection interval
        and 120 bytes buffer size. 3 phones are required.

        Steps:
        1. Get the mac address of the server device.
        2. Establish a L2CAP CoC connection from the client to the server#1 AD.
        The connection is insecured.
        3. Set the connection interval to 15 msec and buffer size to 120 bytes.
        4. Verify that the L2CAP CoC connection is active from both the client
        and server.
        5. Establish a L2CAP CoC connection from the client to the server#2 AD.
        The connection is insecured.
        6. Set the connection interval to 15 msec and buffer size to 120 bytes.
        7. Verify that the L2CAP CoC connection is active from both the client
        and server.
        8. Write data from the client to both server#1 and server#2.
        9. Verify data matches from client and server

        Expected Result:
        L2CAP CoC connections are established and data written correctly to both servers.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 1
        """

        is_secured = False
        le_connection_interval = 15
        buffer_size = 120
        le_tx_data_length = buffer_size + l2cap_coc_header_size
        status = self._run_coc_connection_throughput_2_conn(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length,
            self._calc_min_max_ce_time(le_connection_interval),
            self._calc_min_max_ce_time(le_connection_interval))
        return status

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='053e59c2-f312-4bec-beaf-9e4efdce063a')
    def test_coc_connection_throughput_2_conn_NOSEC_15CI_180SIZE(self):
        """Test LE CoC data throughput with 15msec CI and 180bytes buffer size.

        Test data throughput of 2 L2CAP CoC insecured connections with 15msec connection interval
        and 120 bytes buffer size. 3 phones are required.

        Steps:
        1. Get the mac address of the server device.
        2. Establish a L2CAP CoC connection from the client to the server#1 AD.
        The connection is insecured.
        3. Set the connection interval to 15 msec and buffer size to 180 bytes.
        4. Verify that the L2CAP CoC connection is active from both the client
        and server.
        5. Establish a L2CAP CoC connection from the client to the server#2 AD.
        The connection is insecured.
        6. Set the connection interval to 15 msec and buffer size to 180 bytes.
        7. Verify that the L2CAP CoC connection is active from both the client
        and server.
        8. Write data from the client to both server#1 and server#2.
        9. Verify data matches from client and server

        Expected Result:
        L2CAP CoC connections are established and data written correctly to both servers.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 1
        """

        is_secured = False
        le_connection_interval = 15
        buffer_size = 180
        le_tx_data_length = buffer_size + l2cap_coc_header_size
        status = self._run_coc_connection_throughput_2_conn(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length,
            self._calc_min_max_ce_time(le_connection_interval),
            self._calc_min_max_ce_time(le_connection_interval))
        return status

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2b43caa6-76b3-48c5-b342-32ebb31ac52c')
    def test_coc_connection_throughput_2_conn_NOSEC_20CI_240SIZE(self):
        """Test LE CoC data throughput with 20msec CI and 240bytes buffer size.

        Test data throughput of 2 L2CAP CoC insecured connections with 20msec connection interval
        and 240 bytes buffer size. 3 phones are required.

        Steps:
        1. Get the mac address of the server device.
        2. Establish a L2CAP CoC connection from the client to the server#1 AD.
        The connection is insecured.
        3. Set the connection interval to 20 msec and buffer size to 240 bytes.
        4. Verify that the L2CAP CoC connection is active from both the client
        and server.
        5. Establish a L2CAP CoC connection from the client to the server#2 AD.
        The connection is insecured.
        6. Set the connection interval to 20 msec and buffer size to 240 bytes.
        7. Verify that the L2CAP CoC connection is active from both the client
        and server.
        8. Write data from the client to both server#1 and server#2.
        9. Verify data matches from client and server

        Expected Result:
        L2CAP CoC connections are established and data written correctly to both servers.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 1
        """

        is_secured = False
        le_connection_interval = 20
        buffer_size = 240
        le_tx_data_length = buffer_size + l2cap_coc_header_size
        status = self._run_coc_connection_throughput_2_conn(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length,
            self._calc_min_max_ce_time(le_connection_interval),
            self._calc_min_max_ce_time(le_connection_interval))
        return status

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f630df02-3fd6-4aa0-bc15-06837b705e97')
    def test_coc_connection_throughput_2_conn_NOSEC_30CI_240SIZE(self):
        """Test LE CoC data throughput with 30msec CI and 240bytes buffer size.

        Test data throughput of 2 L2CAP CoC insecured connections with 20msec connection interval
        and 240 bytes buffer size. 3 phones are required.

        Steps:
        1. Get the mac address of the server device.
        2. Establish a L2CAP CoC connection from the client to the server#1 AD.
        The connection is insecured.
        3. Set the connection interval to 30 msec and buffer size to 240 bytes.
        4. Verify that the L2CAP CoC connection is active from both the client
        and server.
        5. Establish a L2CAP CoC connection from the client to the server#2 AD.
        The connection is insecured.
        6. Set the connection interval to 30 msec and buffer size to 240 bytes.
        7. Verify that the L2CAP CoC connection is active from both the client
        and server.
        8. Write data from the client to both server#1 and server#2.
        9. Verify data matches from client and server

        Expected Result:
        L2CAP CoC connections are established and data written correctly to both servers.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 2
        """

        is_secured = False
        le_connection_interval = 30
        buffer_size = 240
        le_tx_data_length = buffer_size + l2cap_coc_header_size
        status = self._run_coc_connection_throughput_2_conn(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length,
            self._calc_min_max_ce_time(le_connection_interval),
            self._calc_min_max_ce_time(le_connection_interval))
        return status
