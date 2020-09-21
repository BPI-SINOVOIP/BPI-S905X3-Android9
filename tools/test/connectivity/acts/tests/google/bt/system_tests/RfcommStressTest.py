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
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import orchestrate_rfcomm_connection
from acts.test_utils.bt.bt_test_utils import write_read_verify_data


class RfcommStressTest(BluetoothBaseTest):
    default_timeout = 10
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

    @test_tracker_info(uuid='10452f63-e1fd-4345-a4da-e00fc4609a69')
    def test_rfcomm_connection_stress(self):
        """Stress test an RFCOMM connection

        Test the integrity of RFCOMM. Verify that file descriptors are cleared
        out properly.

        Steps:
        1. Establish a bonding between two Android devices.
        2. Write data to RFCOMM from the client droid.
        3. Read data from RFCOMM from the server droid.
        4. Stop the RFCOMM connection.
        5. Repeat steps 2-4 1000 times.

        Expected Result:
        Each iteration should read and write to the RFCOMM connection
        successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, Stress, RFCOMM
        Priority: 1
        """
        iterations = 1000
        for n in range(iterations):
            if not orchestrate_rfcomm_connection(self.client_ad,
                                                 self.server_ad):
                return False
            self.client_ad.droid.bluetoothRfcommStop()
            self.server_ad.droid.bluetoothRfcommStop()
            self.log.info("Iteration {} completed".format(n))
        return True

    @test_tracker_info(uuid='2dba96ec-4e5d-4394-85ef-b57b66399fbc')
    def test_rfcomm_connection_write_msg_stress(self):
        """Stress test an RFCOMM connection

        Test the integrity of RFCOMM. Verify that file descriptors are cleared
        out properly.

        Steps:
        1. Establish a bonding between two Android devices.
        2. Write data to RFCOMM from the client droid.
        3. Read data from RFCOMM from the server droid.
        4. Stop the RFCOMM connection.
        5. Repeat steps 2-4 1000 times.

        Expected Result:
        Each iteration should read and write to the RFCOMM connection
        successfully.

        Returns:
          Pass if True
          Fail if False0

        TAGS: Classic, Stress, RFCOMM
        Priority: 1
        """
        iterations = 1000
        for n in range(iterations):
            if not orchestrate_rfcomm_connection(self.client_ad,
                                                 self.server_ad):
                return False
            if not write_read_verify_data(self.client_ad, self.server_ad,
                                          self.message, False):
                return False
            self.client_ad.droid.bluetoothRfcommStop()
            self.server_ad.droid.bluetoothRfcommStop()
            self.log.info("Iteration {} completed".format(n))
        return True

    @test_tracker_info(uuid='78dca597-89c0-431c-a553-531f230fc3c0')
    def test_rfcomm_read_write_stress(self):
        """Stress test an RFCOMM connection's read and write capabilities

        Test the integrity of RFCOMM. Verify that file descriptors are cleared
        out properly.

        Steps:
        1. Establish a bonding between two Android devices.
        2. Write data to RFCOMM from the client droid.
        3. Read data from RFCOMM from the server droid.
        4. Repeat steps 2-3 10000 times.
        5. Stop the RFCOMM connection.

        Expected Result:
        Each iteration should read and write to the RFCOMM connection
        successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, Stress, RFCOMM
        Priority: 1
        """
        iterations = 1000
        if not orchestrate_rfcomm_connection(self.client_ad, self.server_ad):
            return False
        for n in range(iterations):
            self.log.info("Write message.")
            if not write_read_verify_data(self.client_ad, self.server_ad,
                                          self.message, False):
                return False
            self.log.info("Iteration {} completed".format(n))
        self.client_ad.droid.bluetoothRfcommStop()
        self.server_ad.droid.bluetoothRfcommStop()
        return True
