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
Test script to test the integrity of LE scan results upon resetting the
Bluetooth stack.
"""

import concurrent
import os
import time

from queue import Empty
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import ble_advertise_settings_modes
from acts.test_utils.bt.bt_constants import ble_scan_settings_callback_types
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_constants import adv_succ
from acts.test_utils.bt.bt_constants import scan_result
from acts.test_utils.bt.bt_test_utils import BtTestUtilsError
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.test_utils.bt.bt_test_utils import get_advanced_droid_list
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import setup_n_advertisements
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs
from acts.test_utils.bt.bt_test_utils import teardown_n_advertisements
from acts.test_utils.bt.bt_test_utils import scan_and_verify_n_advertisements


class ConcurrentBleAdvertisementDiscoveryTest(BluetoothBaseTest):
    default_timeout = 10
    max_advertisements = -1
    advertise_callback_list = []

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.droid_list = get_advanced_droid_list(self.android_devices)
        self.scn_ad = self.android_devices[0]
        self.adv_ad = self.android_devices[1]
        self.max_advertisements = self.droid_list[1]['max_advertisements']

    def setup_test(self):
        return reset_bluetooth(self.android_devices)

    def setup_test(self):
        super(BluetoothBaseTest, self).setup_test()
        self.log.info("Setting up advertisements")
        try:
            self.advertise_callback_list = setup_n_advertisements(
                self.adv_ad, self.max_advertisements)
        except BtTestUtilsError:
            return False
        return True

    def teardown_test(self):
        super(BluetoothBaseTest, self).teardown_test()
        self.log.info("Tearing down advertisements")
        teardown_n_advertisements(self.adv_ad,
                                  len(self.advertise_callback_list),
                                  self.advertise_callback_list)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e02d6ca6-4db3-4a1d-adaf-98db7c7c2c7a')
    def test_max_advertisements_defaults(self):
        """Test scan integrity after BT state is reset

        This test is to verify that LE devices are found
        successfully after the Bluetooth stack is
        reset. This is repeated multiple times in order
        to verify that LE devices are not lost in scanning
        when the stack is brought back up.

        Steps:
        1. Pre-Condition: Max advertisements are active
        2. With the DUT android device, scan for all advertisements
        and verify that all expected advertisements are found.
        3. Reset Bluetooth on DUT.
        4. Repeat steps 2-3 for defined iterations

        Expected Result:
        All expected advertisements should be found on all iterations.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency, Scanning
        Priority: 2
        """
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleSetScanSettingsCallbackType(
            ble_scan_settings_callback_types['all_matches'])
        self.scn_ad.droid.bleSetScanSettingsScanMode(ble_scan_settings_modes[
            'low_latency'])
        iterations = 20
        for _ in range(iterations):
            self.log.info("Verify all advertisements found")
            try:
                if not scan_and_verify_n_advertisements(
                        self.scn_ad, self.max_advertisements):
                    self.log.error("Failed to find all advertisements")
                    return False
            except BtTestUtilsError:
                return False
            if not reset_bluetooth([self.scn_ad]):
                self.log.error("Failed to reset Bluetooth state")
                return False
        return True
