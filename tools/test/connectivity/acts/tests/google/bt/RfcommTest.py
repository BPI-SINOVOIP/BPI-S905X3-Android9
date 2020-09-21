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
Test script to execute Bluetooth basic functionality test cases.
This test was designed to be run in a shield box.
"""

import threading
import time

from queue import Empty
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import bt_rfcomm_uuids
from acts.test_utils.bt.bt_test_utils import clear_bonded_devices
from acts.test_utils.bt.bt_test_utils import kill_bluetooth_process
from acts.test_utils.bt.bt_test_utils import orchestrate_rfcomm_connection
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs
from acts.test_utils.bt.bt_test_utils import write_read_verify_data
from acts.test_utils.bt.bt_test_utils import verify_server_and_client_connected

from acts.test_utils.bt.BtEnum import RfcommUuid


class RfcommTest(BluetoothBaseTest):
    default_timeout = 10
    rf_client_th = 0
    scan_discovery_time = 5
    message = (
        "Space: the final frontier. These are the voyages of "
        "the starship Enterprise. Its continuing mission: to explore "
        "strange new worlds, to seek out new life and new civilizations,"
        " to boldly go where no man has gone before.")

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.client_ad = self.android_devices[0]
        self.server_ad = self.android_devices[1]

    def setup_class(self):
        return setup_multiple_devices_for_bt_test(self.android_devices)

    def teardown_test(self):
        self.client_ad.droid.bluetoothRfcommCloseClientSocket()
        self.server_ad.droid.bluetoothRfcommCloseServerSocket()
        return True

    def teardown_test(self):
        if verify_server_and_client_connected(
                self.client_ad, self.server_ad, log=False):
            self.client_ad.droid.bluetoothRfcommStop()
            self.server_ad.droid.bluetoothRfcommStop()

    def _test_rfcomm_connection_with_uuid(self, uuid):
        if not orchestrate_rfcomm_connection(
                self.client_ad, self.server_ad, uuid=uuid):
            return False

        self.client_ad.droid.bluetoothRfcommStop()
        self.server_ad.droid.bluetoothRfcommStop()
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f0bd466f-9a59-4612-8b75-ae4f691eef77')
    def test_rfcomm_connection(self):
        """Test Bluetooth RFCOMM connection

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 1
        """
        return self._test_rfcomm_connection_with_uuid(None)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='240e106a-efd0-4795-8baa-9c0ea88b8b25')
    def test_rfcomm_connection_write_ascii(self):
        """Test Bluetooth RFCOMM writing and reading ascii data

        Test RFCOMM though establishing a connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.
        4. Write data from the client and read received data from the server.
        5. Verify data matches from client and server
        6. Disconnect the RFCOMM connection.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 1
        """
        if not orchestrate_rfcomm_connection(self.client_ad, self.server_ad):
            return False
        if not write_read_verify_data(self.client_ad, self.server_ad,
                                      self.message, False):
            return False
        if not verify_server_and_client_connected(self.client_ad,
                                                  self.server_ad):
            return False

        self.client_ad.droid.bluetoothRfcommStop()
        self.server_ad.droid.bluetoothRfcommStop()
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c6ebf4aa-1ccb-415f-98c2-cbffb067d1ea')
    def test_rfcomm_write_binary(self):
        """Test Bluetooth RFCOMM writing and reading binary data

        Test profile though establishing an RFCOMM connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.
        4. Write data from the client and read received data from the server.
        5. Verify data matches from client and server
        6. Disconnect the RFCOMM connection.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 1
        """
        if not orchestrate_rfcomm_connection(self.client_ad, self.server_ad):
            return False
        binary_message = "11010101"
        if not write_read_verify_data(self.client_ad, self.server_ad,
                                      binary_message, True):
            return False

        if not verify_server_and_client_connected(self.client_ad,
                                                  self.server_ad):
            return False

        self.client_ad.droid.bluetoothRfcommStop()
        self.server_ad.droid.bluetoothRfcommStop()
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2b36d71e-102b-469e-b064-e0da8cefdbfe')
    def test_rfcomm_accept_timeout(self):
        """Test Bluetooth RFCOMM accept socket timeout

        Verify that RFCOMM connections are unsuccessful if
        the socket timeout is exceeded.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 1
        """
        # Socket timeout set to 999ms
        short_socket_timeout = 999
        # Wait time in seconds before attempting a connection
        wait_time_before_connect_attempt = 1
        self.server_ad.droid.bluetoothStartPairingHelper()
        self.client_ad.droid.bluetoothStartPairingHelper()
        self.server_ad.droid.bluetoothRfcommBeginAcceptThread(
            bt_rfcomm_uuids['default_uuid'], short_socket_timeout)
        time.sleep(wait_time_before_connect_attempt)

        # Try to connect
        self.client_ad.droid.bluetoothRfcommBeginConnectThread(
            self.server_ad.droid.bluetoothGetLocalAddress())
        # Give the connection time to fail
        #time.sleep(self.default_timeout)
        time.sleep(2)
        if verify_server_and_client_connected(self.client_ad, self.server_ad):
            return False
        self.log.info("No active connections found as expected")
        # AcceptThread has finished, kill hanging ConnectThread
        self.client_ad.droid.bluetoothRfcommKillConnThread()
        reset_bluetooth(self.android_devices)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='88c70db6-651e-4d43-ab0c-c9f584094fb2')
    def test_rfcomm_connection_base_uuid(self):
        """Test Bluetooth RFCOMM connection using BASE uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(
            bt_rfcomm_uuids['base_uuid'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='42c8d861-48b3-423b-ae8c-df140ebaad9d')
    def test_rfcomm_connection_sdp_uuid(self):
        """Test Bluetooth RFCOMM connection using SDP uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['sdp'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='97cc310d-4096-481e-940f-abe6811784f3')
    def test_rfcomm_connection_udp_uuid(self):
        """Test Bluetooth RFCOMM connection using UDP uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['udp'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5998a0cf-fc05-433a-abd8-c52717ea755c')
    def test_rfcomm_connection_rfcomm_uuid(self):
        """Test Bluetooth RFCOMM connection using RFCOMM uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(
            bt_rfcomm_uuids['rfcomm'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e3c05357-99ec-4819-86e4-1363e3359317')
    def test_rfcomm_connection_tcp_uuid(self):
        """Test Bluetooth RFCOMM connection using TCP uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['tcp'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7304f8dc-f568-4489-9926-0b940ba7a45b')
    def test_rfcomm_connection_tcs_bin_uuid(self):
        """Test Bluetooth RFCOMM connection using TCS_BIN uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(
            bt_rfcomm_uuids['tcs_bin'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ea1cfc32-d3f0-4420-a8e5-793c6ddf5820')
    def test_rfcomm_connection_tcs_at_uuid(self):
        """Test Bluetooth RFCOMM connection using TCS_AT uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(
            bt_rfcomm_uuids['tcs_at'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5b0d5608-38a5-48f7-b3e5-dc52a4a681dd')
    def test_rfcomm_connection_att_uuid(self):
        """Test Bluetooth RFCOMM connection using ATT uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['att'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e81f37ba-e914-4eb1-b144-b079f91c6734')
    def test_rfcomm_connection_obex_uuid(self):
        """Test Bluetooth RFCOMM connection using OBEX uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['obex'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5edd766f-17fb-459c-985e-9c21afe1b104')
    def test_rfcomm_connection_ip_uuid(self):
        """Test Bluetooth RFCOMM connection using IP uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['ip'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7a429cca-bc65-4344-8fa5-13ca0d49a351')
    def test_rfcomm_connection_ftp_uuid(self):
        """Test Bluetooth RFCOMM connection using FTP uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['ftp'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a8ecdd7b-8529-4e0b-ad18-0d0cf61f4b02')
    def test_rfcomm_connection_http_uuid(self):
        """Test Bluetooth RFCOMM connection using HTTP uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['http'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='816569e6-6189-45b5-95c3-ea27b69698ff')
    def test_rfcomm_connection_wsp_uuid(self):
        """Test Bluetooth RFCOMM connection using WSP uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['wsp'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cd5e8c87-4df9-4f1d-ae0b-b47f84c75e44')
    def test_rfcomm_connection_bnep_uuid(self):
        """Test Bluetooth RFCOMM connection using BNEP uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['bnep'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='fda073d3-d856-438b-b208-61cce67689dd')
    def test_rfcomm_connection_upnp_uuid(self):
        """Test Bluetooth RFCOMM connection using UPNP uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['upnp'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0ab329bb-ef61-4574-a5c1-440fb45938ff')
    def test_rfcomm_connection_hidp_uuid(self):
        """Test Bluetooth RFCOMM connection using HIDP uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['hidp'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5b1d8c64-4f92-4a22-b61b-28b1a1086b39')
    def test_rfcomm_connection_hardcopy_control_channel_uuid(self):
        """Test Bluetooth RFCOMM connection using HARDCOPY_CONTROL_CHANNEL uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(
            bt_rfcomm_uuids['hardcopy_control_channel'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1ae6ca34-87ab-48ad-8da8-98c997538af4')
    def test_rfcomm_connection_hardcopy_data_channel_uuid(self):
        """Test Bluetooth RFCOMM connection using HARDCOPY_DATA_CHANNEL uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(
            bt_rfcomm_uuids['hardcopy_data_channel'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d18ed311-a533-4306-944a-6f0f95eac141')
    def test_rfcomm_connection_hardcopy_notification_uuid(self):
        """Test Bluetooth RFCOMM connection using HARDCOPY_NOTIFICATION uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(
            bt_rfcomm_uuids['hardcopy_notification'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ab0af819-7d26-451d-8275-1119ee3c8df8')
    def test_rfcomm_connection_avctp_uuid(self):
        """Test Bluetooth RFCOMM connection using AVCTP uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['avctp'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='124b545e-e842-433d-b541-9710a139c8fb')
    def test_rfcomm_connection_avdtp_uuid(self):
        """Test Bluetooth RFCOMM connection using AVDTP uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['avdtp'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='aea354b9-2ba5-4d7e-90a9-b637cb2fd48c')
    def test_rfcomm_connection_cmtp_uuid(self):
        """Test Bluetooth RFCOMM connection using CMTP uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(bt_rfcomm_uuids['cmtp'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b547b8d9-6453-41af-959f-8bc0d9a6c89a')
    def test_rfcomm_connection_mcap_control_channel_uuid(self):
        """Test Bluetooth RFCOMM connection using MCAP_CONTROL_CHANNEL uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(
            bt_rfcomm_uuids['mcap_control_channel'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ba3ab84c-bc61-442c-944c-af4fbca157f1')
    def test_rfcomm_connection_mcap_data_channel_uuid(self):
        """Test Bluetooth RFCOMM connection using MCAP_DATA_CHANNEL uuid

        Test RFCOMM though establishing a basic connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 3
        """
        return self._test_rfcomm_connection_with_uuid(
            bt_rfcomm_uuids['mcap_data_channel'])
