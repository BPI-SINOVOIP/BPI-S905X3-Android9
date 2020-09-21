#/usr/bin/env python3.4
#
# Copyright (C) 2017 The Android Open Source Project
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
This test script exercises different scan filters with different screen states.
"""

import concurrent
import json
import pprint
import time

from queue import Empty
from acts import utils
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import adv_succ
from acts.test_utils.bt.bt_constants import ble_advertise_settings_modes
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_constants import bt_default_timeout
from acts.test_utils.bt.bt_constants import scan_result
from acts.test_utils.bt.bt_test_utils import batch_scan_result
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.test_utils.bt.bt_test_utils import reset_bluetooth


class BleScanScreenStateTest(BluetoothBaseTest):
    advertise_callback = -1
    max_concurrent_scans = 27
    scan_callback = -1
    shorter_scan_timeout = 2

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.scn_ad = self.android_devices[0]
        self.adv_ad = self.android_devices[1]

    def setup_class(self):
        super(BluetoothBaseTest, self).setup_class()
        utils.set_location_service(self.scn_ad, True)
        utils.set_location_service(self.adv_ad, True)
        return True

    def _setup_generic_advertisement(self):
        self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
            ble_advertise_settings_modes['low_latency'])
        self.advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            self.advertise_callback, advertise_data, advertise_settings)
        try:
            self.adv_ad.ed.pop_event(adv_succ.format(self.advertise_callback))
        except Empty:
            self.log.error("Failed to start advertisement.")
            return False
        return True

    def _setup_scan_with_no_filters(self):
        filter_list, scan_settings, self.scan_callback = \
            generate_ble_scan_objects(self.scn_ad.droid)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          self.scan_callback)

    def _scan_found_results(self):
        try:
            self.scn_ad.ed.pop_event(
                scan_result.format(self.scan_callback), bt_default_timeout)
            self.log.info("Found an advertisement.")
        except Empty:
            self.log.info("Did not find an advertisement.")
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9b695819-e5a8-48b3-87a0-f90422998bf9')
    def test_scan_no_filters_screen_on(self):
        """Test LE scanning is successful with no filters and screen on.

        Test LE scanning is successful with no filters and screen on. Scan
        results should be found.

        Steps:
        1. Setup advertisement
        2. Turn on screen
        3. Start scanner without filters
        4. Verify scan results are found
        5. Teardown advertisement and scanner

        Expected Result:
        Scan results should be found.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, Screen
        Priority: 2
        """
        # Step 1
        if not self._setup_generic_advertisement():
            return False

        # Step 2
        self.scn_ad.droid.wakeUpNow()

        # Step 3
        self._setup_scan_with_no_filters()

        # Step 4
        if not self._scan_found_results():
            return False

        # Step 5
        self.adv_ad.droid.bleStopBleAdvertising(self.advertise_callback)
        self.scn_ad.droid.bleStopBleScan(self.scan_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='38fb6959-f07b-4501-814b-81a498e3efc4')
    def test_scan_no_filters_screen_off(self):
        """Test LE scanning is successful with no filters and screen off.

        Test LE scanning is successful with no filters and screen off. No scan
        results should be found.

        Steps:
        1. Setup advertisement
        2. Turn off screen
        3. Start scanner without filters
        4. Verify no scan results are found
        5. Teardown advertisement and scanner

        Expected Result:
        No scan results should be found.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, Screen
        Priority: 1
        """
        # Step 1
        if not self._setup_generic_advertisement():
            return False

        # Step 2
        self.scn_ad.droid.goToSleepNow()
        # Give the device time to go to sleep
        time.sleep(2)

        # Step 3
        self._setup_scan_with_no_filters()

        # Step 4
        if self._scan_found_results():
            return False

        # Step 5
        self.adv_ad.droid.bleStopBleAdvertising(self.advertise_callback)
        self.scn_ad.droid.bleStopBleScan(self.scan_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7186ef2f-096a-462e-afde-b0e3d4ecdd83')
    def test_scan_filters_works_with_screen_off(self):
        """Test LE scanning is successful with filters and screen off.

        Test LE scanning is successful with no filters and screen off. No scan
        results should be found.

        Steps:
        1. Setup advertisement
        2. Turn off screen
        3. Start scanner with filters
        4. Verify scan results are found
        5. Teardown advertisement and scanner

        Expected Result:
        Scan results should be found.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, Screen
        Priority: 1
        """
        # Step 1
        adv_device_name = self.adv_ad.droid.bluetoothGetLocalName()
        print(adv_device_name)
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        if not self._setup_generic_advertisement():
            return False

        # Step 2
        self.scn_ad.droid.goToSleepNow()

        # Step 3
        self.scn_ad.droid.bleSetScanFilterDeviceName(adv_device_name)
        filter_list, scan_settings, self.scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          self.scan_callback)

        # Step 4
        if not self._scan_found_results():
            return False

        # Step 5
        self.adv_ad.droid.bleStopBleAdvertising(self.advertise_callback)
        self.scn_ad.droid.bleStopBleScan(self.scan_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='02cd6dca-149e-439b-8427-a2edc7864265')
    def test_scan_no_filters_screen_off_then_turn_on(self):
        """Test start LE scan with no filters while screen is off then turn on.

        Test that a scan without filters will not return results while the
        screen is off but will return results when the screen turns on.

        Steps:
        1. Setup advertisement
        2. Turn off screen
        3. Start scanner without filters
        4. Verify no scan results are found
        5. Turn screen on
        6. Verify scan results are found
        7. Teardown advertisement and scanner

        Expected Result:
        Scan results should only come in when the screen is on.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, Screen
        Priority: 2
        """
        # Step 1
        if not self._setup_generic_advertisement():
            return False

        # Step 2
        self.scn_ad.droid.goToSleepNow()
        # Give the device time to go to sleep
        time.sleep(2)

        # Step 3
        self._setup_scan_with_no_filters()

        # Step 4
        if self._scan_found_results():
            return False

        # Step 5
        self.scn_ad.droid.wakeUpNow()

        # Step 6
        if not self._scan_found_results():
            return False

        # Step 7
        self.adv_ad.droid.bleStopBleAdvertising(self.advertise_callback)
        self.scn_ad.droid.bleStopBleScan(self.scan_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='eb9fc373-f5e8-4a55-9750-02b7a11893d1')
    def test_scan_no_filters_screen_on_then_turn_off(self):
        """Test start LE scan with no filters while screen is on then turn off.

        Test that a scan without filters will not return results while the
        screen is off but will return results when the screen turns on.

        Steps:
        1. Setup advertisement
        2. Turn off screen
        3. Start scanner without filters
        4. Verify no scan results are found
        5. Turn screen on
        6. Verify scan results are found
        7. Teardown advertisement and scanner

        Expected Result:
        Scan results should only come in when the screen is on.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, Screen
        Priority: 2
        """
        # Step 1
        if not self._setup_generic_advertisement():
            return False

        # Step 2
        self.scn_ad.droid.wakeUpNow()

        # Step 3
        self._setup_scan_with_no_filters()

        # Step 4
        if not self._scan_found_results():
            return False

        # Step 5
        self.scn_ad.droid.goToSleepNow()
        # Give the device time to go to sleep
        time.sleep(2)
        self.scn_ad.ed.clear_all_events()

        # Step 6
        if self._scan_found_results():
            return False

        # Step 7
        self.adv_ad.droid.bleStopBleAdvertising(self.advertise_callback)
        self.scn_ad.droid.bleStopBleScan(self.scan_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='41d90e11-b0a8-4eed-bff1-c19678920762')
    def test_scan_no_filters_screen_toggling(self):
        """Test start LE scan with no filters and test screen toggling.

        Test that a scan without filters will not return results while the
        screen is off and return results while the screen is on.

        Steps:
        1. Setup advertisement
        2. Turn off screen
        3. Start scanner without filters
        4. Verify no scan results are found
        5. Turn screen on
        6. Verify scan results are found
        7. Repeat steps 1-6 10 times
        7. Teardown advertisement and scanner

        Expected Result:
        Scan results should only come in when the screen is on.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, Screen
        Priority: 3
        """
        iterations = 10
        # Step 1
        if not self._setup_generic_advertisement():
            return False

        for i in range(iterations):
            self.log.info("Starting iteration {}".format(i + 1))
            # Step 2
            self.scn_ad.droid.goToSleepNow()
            # Give the device time to go to sleep
            time.sleep(2)
            self.scn_ad.ed.clear_all_events()

            # Step 3
            self._setup_scan_with_no_filters()

            # Step 4
            if self._scan_found_results():
                return False

            # Step 5
            self.scn_ad.droid.wakeUpNow()

            # Step 6
            if not self._scan_found_results():
                return False

        # Step 7
        self.adv_ad.droid.bleStopBleAdvertising(self.advertise_callback)
        self.scn_ad.droid.bleStopBleScan(self.scan_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7a2fe7ef-b15f-4e93-a2f0-40e2f7d9cbcb')
    def test_opportunistic_scan_no_filters_screen_off_then_on(self):
        """Test opportunistic scanning does not find results with screen off.

        Test LE scanning is successful with no filters and screen off. No scan
        results should be found.

        Steps:
        1. Setup advertisement
        2. Turn off screen
        3. Start opportunistic scan without filters
        4. Start scan without filters
        5. Verify no scan results are found on either scan instance
        6. Wake up phone
        7. Verify scan results on each scan instance
        8. Teardown advertisement and scanner

        Expected Result:
        No scan results should be found.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, Screen
        Priority: 1
        """
        # Step 1
        if not self._setup_generic_advertisement():
            return False

        # Step 2
        self.scn_ad.droid.goToSleepNow()
        # Give the device time to go to sleep
        time.sleep(2)

        # Step 3
        self.scn_ad.droid.bleSetScanSettingsScanMode(ble_scan_settings_modes[
            'opportunistic'])
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)

        # Step 4
        filter_list2, scan_settings2, scan_callback2 = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleStartBleScan(filter_list2, scan_settings2,
                                          scan_callback2)

        # Step 5
        try:
            self.scn_ad.ed.pop_event(
                scan_result.format(scan_callback), self.shorter_scan_timeout)
            self.log.error("Found an advertisement on opportunistic scan.")
            return False
        except Empty:
            self.log.info("Did not find an advertisement.")
        try:
            self.scn_ad.ed.pop_event(
                scan_result.format(scan_callback2), self.shorter_scan_timeout)
            self.log.error("Found an advertisement on scan instance.")
            return False
        except Empty:
            self.log.info("Did not find an advertisement.")

        # Step 6
        self.scn_ad.droid.wakeUpNow()

        # Step 7
        try:
            self.scn_ad.ed.pop_event(
                scan_result.format(scan_callback), self.shorter_scan_timeout)
            self.log.info("Found an advertisement on opportunistic scan.")
        except Empty:
            self.log.error(
                "Did not find an advertisement on opportunistic scan.")
            return False
        try:
            self.scn_ad.ed.pop_event(
                scan_result.format(scan_callback2), self.shorter_scan_timeout)
            self.log.info("Found an advertisement on scan instance.")
        except Empty:
            self.log.info("Did not find an advertisement.")
            return False

        # Step 8
        self.adv_ad.droid.bleStopBleAdvertising(self.advertise_callback)
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        self.scn_ad.droid.bleStopBleScan(scan_callback2)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='406f1a2e-160f-4fb2-8a87-6403996df36e')
    def test_max_scan_no_filters_screen_off_then_turn_on(self):
        """Test start max scans with no filters while screen is off then turn on

        Test that max LE scan without filters will not return results while the
        screen is off but will return results when the screen turns on.

        Steps:
        1. Setup advertisement
        2. Turn off screen
        3. Start scanner without filters and verify no scan results
        4. Turn screen on
        5. Verify scan results are found on each scan callback
        6. Teardown advertisement and all scanner

        Expected Result:
        Scan results should only come in when the screen is on.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, Screen
        Priority: 2
        """
        # Step 1
        if not self._setup_generic_advertisement():
            return False

        # Step 2
        self.scn_ad.droid.goToSleepNow()
        # Give the device time to go to sleep
        time.sleep(2)

        # Step 3
        scan_callback_list = []
        for _ in range(self.max_concurrent_scans):
            filter_list, scan_settings, scan_callback = \
                generate_ble_scan_objects(self.scn_ad.droid)
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                              scan_callback)
            scan_callback_list.append(scan_callback)
            try:
                self.scn_ad.ed.pop_event(
                    scan_result.format(self.scan_callback),
                    self.shorter_scan_timeout)
                self.log.info("Found an advertisement.")
                return False
            except Empty:
                self.log.info("Did not find an advertisement.")

        # Step 4
        self.scn_ad.droid.wakeUpNow()

        # Step 5
        for callback in scan_callback_list:
            try:
                self.scn_ad.ed.pop_event(
                    scan_result.format(callback), self.shorter_scan_timeout)
                self.log.info("Found an advertisement.")
            except Empty:
                self.log.info("Did not find an advertisement.")
                return False

        # Step 7
        self.adv_ad.droid.bleStopBleAdvertising(self.advertise_callback)
        for callback in scan_callback_list:
            self.scn_ad.droid.bleStopBleScan(callback)
        return True
