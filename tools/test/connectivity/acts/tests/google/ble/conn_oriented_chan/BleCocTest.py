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
Test script to execute Bluetooth Connection-orient Channel (CoC) functionality
test cases. This test was designed to be run in a shield box.
"""

import threading
import time

from queue import Empty
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_coc_test_utils import orchestrate_coc_connection
from acts.test_utils.bt.bt_coc_test_utils import do_multi_connection_throughput
from acts.test_utils.bt.bt_constants import default_le_data_length
from acts.test_utils.bt.bt_constants import l2cap_coc_header_size
from acts.test_utils.bt.bt_constants import l2cap_max_inactivity_delay_after_disconnect
from acts.test_utils.bt.bt_test_utils import clear_bonded_devices
from acts.test_utils.bt.bt_test_utils import kill_bluetooth_process
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs
from acts.test_utils.bt.bt_test_utils import write_read_verify_data
from acts.test_utils.bt.bt_test_utils import verify_server_and_client_connected


class BleCocTest(BluetoothBaseTest):
    message = (
        "Space: the final frontier. These are the voyages of "
        "the starship Enterprise. Its continuing mission: to explore "
        "strange new worlds, to seek out new life and new civilizations,"
        " to boldly go where no man has gone before.")

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

    def _run_coc_connection_throughput(
            self,
            is_secured,
            buffer_size,
            le_connection_interval=0,
            le_tx_data_length=default_le_data_length):

        # The num_iterations is that number of repetitions of each
        # set of buffers r/w.
        # number_buffers is the total number of data buffers to transmit per
        # set of buffers r/w.
        # buffer_size is the number of bytes per L2CAP data buffer.
        number_buffers = 100
        num_iterations = 10

        self.log.info(
            "_run_coc_connection_throughput: calling "
            "orchestrate_coc_connection. is_secured={}, Connection Interval={}msec, "
            "buffer_size={}bytes".format(is_secured, le_connection_interval,
                                         buffer_size))
        status, client_conn_id, server_conn_id = orchestrate_coc_connection(
            self.client_ad, self.server_ad, True, is_secured,
            le_connection_interval, le_tx_data_length)
        if not status:
            return False

        list_server_ad = [self.server_ad]
        list_client_conn_id = [client_conn_id]
        data_rate = do_multi_connection_throughput(
            self.client_ad, list_server_ad, list_client_conn_id,
            num_iterations, number_buffers, buffer_size)
        if data_rate <= 0:
            return False
        self.log.info(
            "_run_coc_connection_throughput: throughput=%d bytes per sec",
            data_rate)

        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b6989966-c504-4934-bcd7-57fb4f7fde9c')
    def test_coc_secured_connection(self):
        """Test Bluetooth LE CoC secured connection

        Test LE CoC though establishing a basic connection with security.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an LE CoC Secured connection from the client to the server AD.
        3. Verify that the LE CoC connection is active from both the client and
        server.
        Expected Result:
        LE CoC connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 1
        """
        is_secured = True
        self.log.info(
            "_test_coc_secured_connection: calling orchestrate_coc_connection but "
            "isBle=1 and securedConn={}".format(is_secured))
        status, client_conn_id, server_conn_id = orchestrate_coc_connection(
            self.client_ad, self.server_ad, True, is_secured)
        if not status:
            return False

        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6587792c-78fb-469f-9084-772c249f97de')
    def test_coc_insecured_connection(self):
        """Test Bluetooth LE CoC insecured connection

        Test LE CoC though establishing a basic connection with no security.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an LE CoC Secured connection from the client to the server AD.
        3. Verify that the LE CoC connection is active from both the client and
        server.
        Expected Result:
        LE CoC connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 1
        """
        is_secured = False
        self.log.info(
            "test_coc_insecured_connection: calling orchestrate_coc_connection but "
            "isBle=1 and securedConn={}".format(is_secured))
        status, client_conn_id, server_conn_id = orchestrate_coc_connection(
            self.client_ad, self.server_ad, True, is_secured)
        if not status:
            return False

        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='32a7b02e-f2b5-4193-b414-36c8815ac407')
    def test_coc_secured_connection_write_ascii(self):
        """Test LE CoC secured connection writing and reading ascii data

        Test LE CoC though establishing a secured connection and reading and writing ascii data.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an LE CoC connection from the client to the server AD. The security of
        connection is TRUE.
        3. Verify that the LE CoC connection is active from both the client and
        server.
        4. Write data from the client and read received data from the server.
        5. Verify data matches from client and server
        6. Disconnect the LE CoC connection.

        Expected Result:
        LE CoC connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 1
        """
        is_secured = True
        self.log.info(
            "test_coc_secured_connection_write_ascii: calling "
            "orchestrate_coc_connection. is_secured={}".format(is_secured))
        status, client_conn_id, server_conn_id = orchestrate_coc_connection(
            self.client_ad, self.server_ad, True, is_secured)
        if not status:
            return False
        if not write_read_verify_data(self.client_ad, self.server_ad,
                                      self.message, False):
            return False
        if not verify_server_and_client_connected(self.client_ad,
                                                  self.server_ad):
            return False

        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='12537d27-79c9-40a0-8bdb-d023b0e36b58')
    def test_coc_insecured_connection_write_ascii(self):
        """Test LE CoC insecured connection writing and reading ascii data

        Test LE CoC though establishing a connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an LE CoC connection from the client to the server AD. The security of
        connection is FALSE.
        3. Verify that the LE CoC connection is active from both the client and
        server.
        4. Write data from the client and read received data from the server.
        5. Verify data matches from client and server
        6. Disconnect the LE CoC connection.

        Expected Result:
        LE CoC connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 1
        """
        is_secured = False
        self.log.info(
            "test_coc_secured_connection_write_ascii: calling "
            "orchestrate_coc_connection. is_secured={}".format(is_secured))
        status, client_conn_id, server_conn_id = orchestrate_coc_connection(
            self.client_ad, self.server_ad, True, is_secured)
        if not status:
            return False
        if not write_read_verify_data(self.client_ad, self.server_ad,
                                      self.message, False):
            return False
        if not verify_server_and_client_connected(self.client_ad,
                                                  self.server_ad):
            return False

        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='214037f4-f0d1-47db-86a7-5230c71bdcac')
    def test_coc_secured_connection_throughput(self):
        """Test LE CoC writing and measured data throughput with security

        Test CoC thoughput by establishing a secured connection and sending data.

        Steps:
        1. Get the mac address of the server device.
        2. Establish a L2CAP CoC connection from the client to the server AD.
        3. Verify that the L2CAP CoC connection is active from both the client
        and server.
        4. Write data from the client to server.
        5. Verify data matches from client and server
        6. Disconnect the L2CAP CoC connections.

        Expected Result:
        CoC connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 1
        """

        is_secured = True
        # Note: A 117 octets buffer size would fix nicely to a 123 bytes Data Length
        return self._run_coc_connection_throughput(is_secured, 117)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6dc019bb-c3bf-4c98-978e-e2c5755058d7')
    def test_coc_insecured_connection_throughput(self):
        """Test LE CoC writing and measured data throughput without security.

        Test CoC thoughput by establishing an insecured connection and sending data.

        Steps:
        1. Get the mac address of the server device.
        2. Establish a L2CAP CoC connection from the client to the server AD.
        3. Verify that the L2CAP CoC connection is active from both the client
        and server.
        4. Write data from the client to server.
        5. Verify data matches from client and server
        6. Disconnect the L2CAP CoC connections.

        Expected Result:
        CoC connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 1
        """

        is_secured = False
        # Note: A 117 octets buffer size would fix nicely to a 123 bytes Data Length
        return self._run_coc_connection_throughput(is_secured, 117)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0af94805-1550-426c-bfdd-191b8b3a4c12')
    def test_coc_connection_throughput_NOSEC_10CI_60SIZE(self):
        """Test LE CoC data throughput with 10msec CI and 60bytes buffer size.

        Test CoC thoughput by establishing a connection and sending data with 10msec
        Connection Interval and 60 bytes data buffer size.

        Steps:
        1. Get the mac address of the server device.
        2. Change the Connection Interval to 10msec.
        3. Change Payload Buffer Size to 60 bytes.
        4. Establish a L2CAP CoC connection from the client to the server AD.
        5. Verify that the L2CAP CoC connection is active from both the client
        and server.
        6. Write data from the client to server.
        7. Verify data matches from client and server
        8. Disconnect the L2CAP CoC connections.

        Expected Result:
        CoC connection is established, check transmitted data contents, then disconnected
        succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 2
        """

        is_secured = False
        le_connection_interval = 10
        buffer_size = 60
        le_tx_data_length = buffer_size + l2cap_coc_header_size
        return self._run_coc_connection_throughput(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c32dac07-623a-4fdd-96c6-387a76afb2af')
    def test_coc_connection_throughput_NOSEC_10CI_80SIZE(self):
        """Test LE CoC data throughput with 10msec CI and 80bytes buffer size.

        Test CoC thoughput by establishing a connection and sending data with 10msec
        Connection Interval and 80 bytes data buffer size.

        Steps:
        1. Get the mac address of the server device.
        2. Change the Connection Interval to 10msec.
        3. Change Payload Buffer Size to 80 bytes.
        4. Establish a L2CAP CoC connection from the client to the server AD.
        5. Verify that the L2CAP CoC connection is active from both the client
        and server.
        6. Write data from the client to server.
        7. Verify data matches from client and server
        8. Disconnect the L2CAP CoC connections.

        Expected Result:
        CoC connection is established, check transmitted data contents, then disconnected
        succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 2
        """

        is_secured = False
        le_connection_interval = 10
        buffer_size = 80
        le_tx_data_length = buffer_size + l2cap_coc_header_size
        return self._run_coc_connection_throughput(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='45d1b0c1-73b6-483f-ac6b-c3cec805da32')
    def test_coc_connection_throughput_NOSEC_10CI_120SIZE(self):
        """Test LE CoC data throughput with 10msec CI and 120bytes buffer size.

        Test CoC thoughput by establishing a connection and sending data with 10msec
        Connection Interval and 120 bytes data buffer size.

        Steps:
        1. Get the mac address of the server device.
        2. Change the Connection Interval to 10msec.
        3. Change Payload Buffer Size to 120 bytes.
        4. Establish a L2CAP CoC connection from the client to the server AD.
        5. Verify that the L2CAP CoC connection is active from both the client
        and server.
        6. Write data from the client to server.
        7. Verify data matches from client and server
        8. Disconnect the L2CAP CoC connections.

        Expected Result:
        CoC connection is established, check transmitted data contents, then disconnected
        succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 2
        """

        is_secured = False
        le_connection_interval = 10
        buffer_size = 120
        le_tx_data_length = buffer_size + l2cap_coc_header_size
        return self._run_coc_connection_throughput(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='85f07f07-1017-42db-b38d-df0bf2fce804')
    def test_coc_connection_throughput_NOSEC_15CI_120SIZE(self):
        """Test LE CoC data throughput with 15msec CI and 120bytes buffer size.

        Test CoC thoughput by establishing a connection and sending data with 15msec
        Connection Interval and 120 bytes data buffer size.

        Steps:
        1. Get the mac address of the server device.
        2. Change the Connection Interval to 15msec.
        3. Change Payload Buffer Size to 120 bytes.
        4. Establish a L2CAP CoC connection from the client to the server AD.
        5. Verify that the L2CAP CoC connection is active from both the client
        and server.
        6. Write data from the client to server.
        7. Verify data matches from client and server
        8. Disconnect the L2CAP CoC connections.

        Expected Result:
        CoC connection is established, check transmitted data contents, then disconnected
        succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 2
        """

        is_secured = False
        le_connection_interval = 15
        buffer_size = 120
        le_tx_data_length = buffer_size + l2cap_coc_header_size
        return self._run_coc_connection_throughput(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4d3d4a06-7bbb-4a8c-9016-f326560cebb0')
    def test_coc_connection_throughput_NOSEC_15CI_180SIZE(self):
        """Test LE CoC data throughput with 15msec CI and 180bytes buffer size.

        Test CoC thoughput by establishing a connection and sending data with 15msec
        Connection Interval and 180 bytes data buffer size.

        Steps:
        1. Get the mac address of the server device.
        2. Change the Connection Interval to 15msec.
        3. Change Payload Buffer Size to 180 bytes.
        4. Establish a L2CAP CoC connection from the client to the server AD.
        5. Verify that the L2CAP CoC connection is active from both the client
        and server.
        6. Write data from the client to server.
        7. Verify data matches from client and server
        8. Disconnect the L2CAP CoC connections.

        Expected Result:
        CoC connection is established, check transmitted data contents, then disconnected
        succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 2
        """

        is_secured = False
        le_connection_interval = 15
        buffer_size = 180
        le_tx_data_length = buffer_size + l2cap_coc_header_size
        return self._run_coc_connection_throughput(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='124d85ba-41e6-4ab7-a017-99a88db7524a')
    def test_coc_connection_throughput_NOSEC_20CI_240SIZE(self):
        """Test LE CoC data throughput with 20msec CI and 240bytes buffer size.

        Test CoC thoughput by establishing a connection and sending data with 20msec
        Connection Interval and 240 bytes data buffer size.

        Steps:
        1. Get the mac address of the server device.
        2. Change the Connection Interval to 20msec.
        3. Change Payload Buffer Size to 240 bytes.
        4. Establish a L2CAP CoC connection from the client to the server AD.
        5. Verify that the L2CAP CoC connection is active from both the client
        and server.
        6. Write data from the client to server.
        7. Verify data matches from client and server
        8. Disconnect the L2CAP CoC connections.

        Expected Result:
        CoC connection is established, check transmitted data contents, then disconnected
        succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: BLE, CoC
        Priority: 2
        """

        is_secured = False
        le_connection_interval = 20
        buffer_size = 240
        le_tx_data_length = buffer_size + l2cap_coc_header_size
        return self._run_coc_connection_throughput(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='218932bc-ebb0-4c2b-96ad-220c600b50b1')
    def test_coc_connection_throughput_NOSEC_30CI_240SIZE(self):
        """Test LE CoC data throughput with 30msec CI and 240bytes buffer size.

        Test CoC thoughput by establishing a connection and sending data with 30msec
        Connection Interval and 240 bytes data buffer size.

        Steps:
        1. Get the mac address of the server device.
        2. Change the Connection Interval to 30msec.
        3. Change Payload Buffer Size to 240 bytes.
        4. Establish a L2CAP CoC connection from the client to the server AD.
        5. Verify that the L2CAP CoC connection is active from both the client
        and server.
        6. Write data from the client to server.
        7. Verify data matches from client and server
        8. Disconnect the L2CAP CoC connections.

        Expected Result:
        CoC connection is established, check transmitted data contents, then disconnected
        succcessfully.

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
        return self._run_coc_connection_throughput(
            is_secured, buffer_size, le_connection_interval, le_tx_data_length)
