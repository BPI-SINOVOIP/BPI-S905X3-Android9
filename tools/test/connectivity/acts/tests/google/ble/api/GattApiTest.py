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
Test script to exercise Gatt Apis.
"""

from acts.controllers.sl4a_lib import rpc_client
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test


class GattApiTest(BluetoothBaseTest):
    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.ad = self.android_devices[0]

    def setup_class(self):
        return setup_multiple_devices_for_bt_test(self.android_devices)

    def setup_test(self):
        for a in self.android_devices:
            a.ed.clear_all_events()
        return True

    def teardown_test(self):
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='fffe5c46-eb97-477b-ac3e-3f70700bb84e')
    def test_open_gatt_server(self):
        """Test a gatt server.

        Test opening a gatt server.

        Steps:
        1. Create a gatt server callback.
        2. Open the gatt server.

        Expected Result:
        Api to open gatt server should not fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, GATT
        Priority: 1
        """
        gatt_server_cb = self.ad.droid.gattServerCreateGattServerCallback()
        self.ad.droid.gattServerOpenGattServer(gatt_server_cb)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='12828c2c-b6ae-4670-a829-9867e75fb711')
    def test_open_gatt_server_on_same_callback(self):
        """Test repetitive opening of a gatt server.

        Test opening a gatt server on the same callback twice in a row.

        Steps:
        1. Create a gatt server callback.
        2. Open the gatt server.
        3. Open the gatt server on the same callback as step 2.

        Expected Result:
        Api to open gatt server should not fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, GATT
        Priority: 2
        """
        gatt_server_cb = self.ad.droid.gattServerCreateGattServerCallback()
        self.ad.droid.gattServerOpenGattServer(gatt_server_cb)
        self.ad.droid.gattServerOpenGattServer(gatt_server_cb)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='63fc684d-6c1d-455e-afdb-1887123b4d2f')
    def test_open_gatt_server_on_invalid_callback(self):
        """Test gatt server an an invalid callback.

        Test opening a gatt server with an invalid callback.

        Steps:
        1. Open a gatt server with the gall callback set to -1.

        Expected Result:
        Api should fail to open a gatt server.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, GATT
        Priority: 2
        """
        invalid_callback_index = -1
        try:
            self.ad.droid.gattServerOpenGattServer(invalid_callback_index)
        except rpc_client.Sl4aApiError as e:
            self.log.info("Failed successfully with exception: {}.".format(e))
            return True
        return False
