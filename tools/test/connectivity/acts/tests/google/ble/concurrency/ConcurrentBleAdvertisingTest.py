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
Test script to exercises different ways Ble Advertisements can run in
concurrency. This test was designed to be run in a shield box.
"""

import concurrent
import os
import time

from queue import Empty
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import BtTestUtilsError
from acts.test_utils.bt.bt_constants import ble_advertise_settings_modes
from acts.test_utils.bt.bt_constants import ble_scan_settings_callback_types
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_constants import adv_succ
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.test_utils.bt.bt_test_utils import get_advanced_droid_list
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import scan_and_verify_n_advertisements
from acts.test_utils.bt.bt_constants import scan_result
from acts.test_utils.bt.bt_test_utils import setup_n_advertisements
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs
from acts.test_utils.bt.bt_test_utils import teardown_n_advertisements


class ConcurrentBleAdvertisingTest(BluetoothBaseTest):
    default_timeout = 10
    max_advertisements = -1

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.droid_list = get_advanced_droid_list(self.android_devices)
        self.scn_ad = self.android_devices[0]
        self.adv_ad = self.android_devices[1]
        self.max_advertisements = self.droid_list[1]['max_advertisements']

    def setup_test(self):
        super(BluetoothBaseTest, self).setup_test()
        return reset_bluetooth(self.android_devices)

    def _verify_n_advertisements(self, num_advertisements):
        try:
            advertise_callback_list = setup_n_advertisements(
                self.adv_ad, num_advertisements)
        except BtTestUtilsError:
            return False
        try:
            scan_and_verify_n_advertisements(self.scn_ad, num_advertisements)
        except BtTestUtilsError:
            return False
        teardown_n_advertisements(self.adv_ad,
                                  len(advertise_callback_list),
                                  advertise_callback_list)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='abc03874-6d7a-4b5d-9f29-18731a102793')
    def test_max_advertisements_defaults(self):
        """Testing max advertisements.

        Test that a single device can have the max advertisements
        concurrently advertising.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Start scanning on the max_advertisements as defined in the script.
        4. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 0
        """
        return self._verify_n_advertisements(self.max_advertisements)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='50ee137e-eb71-40ef-b72f-a5fd646190d2')
    def test_max_advertisements_include_device_name_and_filter_device_name(
            self):
        """Testing max advertisement variant.

        Test that a single device can have the max advertisements
        concurrently advertising. Include the device name as a part of the filter
        and advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include device name in each advertisement.
        4. Include device name filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        return self._verify_n_advertisements(self.max_advertisements)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f7e9ba2b-6286-4510-a8a0-f1df831056c0')
    def test_max_advertisements_exclude_device_name_and_filter_device_name(
            self):
        """Test max advertisement variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the device name as a part of the filter but not the
        advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include device name filter in the scanner.
        4. Start scanning on the max_advertisements as defined in the script.
        5. Verify that no advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(False)
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        return not self._verify_n_advertisements(self.max_advertisements)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6ce102d7-61e1-4ca0-bcfb-767437b86c2b')
    def test_max_advertisements_with_manufacturer_data(self):
        """Test max advertisement variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the manufacturer data as a part of the filter and
        advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include manufacturer data in each advertisement.
        4. Include manufacturer data filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        self.scn_ad.droid.bleSetScanFilterManufacturerData(1, [1])
        self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(1, [1])
        return self._verify_n_advertisements(self.max_advertisements)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2fc7d5e8-1539-42a8-8681-ce0b8bfc0924')
    def test_max_advertisements_with_manufacturer_data_mask(self):
        """Test max advertisements variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the manufacturer data mask as a part of the filter
        and advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include manufacturer data in each advertisement.
        4. Include manufacturer data mask filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        self.scn_ad.droid.bleSetScanFilterManufacturerData(1, [1], [1])
        self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(1, [1])
        return self._verify_n_advertisements(self.max_advertisements)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9ef615ed-1705-44ae-ab5b-f7e8fb4bb770')
    def test_max_advertisements_with_service_data(self):
        """Test max advertisement variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the service data as a part of the filter and
        advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include service data in each advertisement.
        4. Include service data filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        test_result = True
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterServiceData(
            "0000110A-0000-1000-8000-00805F9B34FB", [11, 17, 80])
        self.adv_ad.droid.bleAddAdvertiseDataServiceData(
            "0000110A-0000-1000-8000-00805F9B34FB", [11, 17, 80])
        return self._verify_n_advertisements(self.max_advertisements)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9ef615ed-1705-44ae-ab5b-f7e8fb4bb770')
    def test_max_advertisements_with_manufacturer_data_mask_and_include_device_name(
            self):
        """Test max advertisement variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the device name and manufacturer data as a part of
        the filter and advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include device name and manufacturer data in each advertisement.
        4. Include device name and manufacturer data filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleSetScanFilterManufacturerData(1, [1], [1])
        self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(1, [1])
        return self._verify_n_advertisements(self.max_advertisements)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c2ca85fb-6663-431d-aa30-5286a85dbbe0')
    def test_max_advertisements_with_service_uuids(self):
        """Test max advertisement variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the service uuid as a part of the filter and
        advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include service uuid in each advertisement.
        4. Include service uuid filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 1
        """
        self.scn_ad.droid.bleSetScanFilterServiceUuid(
            "00000000-0000-1000-8000-00805f9b34fb")
        self.adv_ad.droid.bleSetAdvertiseDataSetServiceUuids(
            ["00000000-0000-1000-8000-00805f9b34fb"])
        return self._verify_n_advertisements(self.max_advertisements)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='756e026f-64d7-4a2f-935a-3790c0ac4503')
    def test_max_advertisements_with_service_uuid_and_service_mask(self):
        """Test max advertisements variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the service mask as a part of the filter and
        advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include service uuid in each advertisement.
        4. Include service mask filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        self.scn_ad.droid.bleSetScanFilterServiceUuid(
            "00000000-0000-1000-8000-00805f9b34fb",
            "00000000-0000-1000-8000-00805f9b34fb")
        self.adv_ad.droid.bleSetAdvertiseDataSetServiceUuids(
            ["00000000-0000-1000-8000-00805f9b34fb"])
        return self._verify_n_advertisements(self.max_advertisements)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='79c4b6cd-9f07-49a9-829f-69b29ea8d322')
    def test_max_advertisements_plus_one(self):
        """Test max advertisements plus one.

        Test that a single device can have the max advertisements concurrently
        advertising but fail on starting the max advertisements plus one.
        filter and advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Start max_advertisements + 1.

        Expected Result:
        The last advertisement should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 0
        """
        return not self._verify_n_advertisements(self.max_advertisements + 1)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0bd6e490-a501-4fe1-88e5-9b77970c0b95')
    def test_start_two_advertisements_on_same_callback(self):
        """Test invalid advertisement scenario.

        Test that a single device cannot have two advertisements start on the
        same callback.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Call start ble advertising on the same callback.

        Expected Result:
        The second call of start advertising on the same callback should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 1
        """
        test_result = True
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        try:
            self.adv_ad.ed.pop_event(
                adv_succ.format(advertise_callback), self.default_timeout)
        except Empty as error:
            self.log.error("Test failed with Empty error: {}".format(error))
            return False
        except concurrent.futures._base.TimeoutError as error:
            self.log.debug(
                "Test failed, filtering callback onSuccess never occurred: {}"
                .format(error))
        try:
            self.adv_ad.droid.bleStartBleAdvertising(
                advertise_callback, advertise_data, advertise_settings)
            self.adv_ad.ed.pop_event(
                adv_succ.format(advertise_callback), self.default_timeout)
            test_result = False
        except Empty as error:
            self.log.debug("Test passed with Empty error: {}".format(error))
        except concurrent.futures._base.TimeoutError as error:
            self.log.debug(
                "Test passed, filtering callback onSuccess never occurred: {}"
                .format(error))

        return test_result

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='12632b31-22b9-4121-80b6-1263b9d90909')
    def test_toggle_advertiser_bt_state(self):
        """Test forcing stopping advertisements.

        Test that a single device resets its callbacks when the bluetooth state is
        reset. There should be no advertisements.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Call start ble advertising.
        4. Toggle bluetooth on and off.
        5. Scan for any advertisements.

        Expected Result:
        No advertisements should be found after toggling Bluetooth on the
        advertising device.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        test_result = True
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        try:
            self.adv_ad.ed.pop_event(
                adv_succ.format(advertise_callback), self.default_timeout)
        except Empty as error:
            self.log.error("Test failed with Empty error: {}".format(error))
            return False
        except concurrent.futures._base.TimeoutError as error:
            self.log.error(
                "Test failed, filtering callback onSuccess never occurred: {}".
                format(error))
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        try:
            self.scn_ad.ed.pop_event(
                scan_result.format(scan_callback), self.default_timeout)
        except Empty as error:
            self.log.error("Test failed with: {}".format(error))
            return False
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        test_result = reset_bluetooth([self.android_devices[1]])
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        if not test_result:
            return False
        try:
            expected_event = scan_result.format(scan_callback)
            event = self.scn_ad.ed.pop_event(expected_event,
                                             self.default_timeout)
            self.log.error("Event {} not expected. Found: {}".format(
                expected_event, event))
            return False
        except Empty as error:
            self.log.debug("Test passed with: {}".format(error))
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='785c5c77-d5d4-4d0f-8b7b-3eb1f1646d2c')
    def test_restart_advertise_callback_after_bt_toggle(self):
        """Test starting an advertisement on a cleared out callback.

        Test that a single device resets its callbacks when the bluetooth state
        is reset.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Call start ble advertising.
        4. Toggle bluetooth on and off.
        5. Call start ble advertising on the same callback.

        Expected Result:
        Starting an advertisement on a callback id after toggling bluetooth
        should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 1
        """
        test_result = True
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        try:
            self.adv_ad.ed.pop_event(
                adv_succ.format(advertise_callback), self.default_timeout)
        except Empty as error:
            self.log.error("Test failed with Empty error: {}".format(error))
            test_result = False
        except concurrent.futures._base.TimeoutError as error:
            self.log.debug(
                "Test failed, filtering callback onSuccess never occurred: {}".
                format(error))
        test_result = reset_bluetooth([self.android_devices[1]])
        if not test_result:
            return test_result
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        try:
            self.adv_ad.ed.pop_event(
                adv_succ.format(advertise_callback), self.default_timeout)
        except Empty as error:
            self.log.error("Test failed with Empty error: {}".format(error))
            test_result = False
        except concurrent.futures._base.TimeoutError as error:
            self.log.debug(
                "Test failed, filtering callback onSuccess never occurred: {}".
                format(error))
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='dd5529b7-6774-4580-8b29-d84568c15442')
    def test_timeout(self):
        """Test starting advertiser with timeout.

        Test that when a timeout is used, the advertiser is cleaned properly,
        and next one can be started.

        Steps:
        1. Setup the advertiser android device with 4 second timeout.
        2. Call start ble advertising.
        3. Wait 5 seconds, to make sure advertiser times out.
        4. Repeat steps 1-4 four times.

        Expected Result:
        Starting the advertising should succeed each time.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 1
        """
        advertise_timeout_s = 4
        num_iterations = 4
        test_result = True

        for i in range(0, num_iterations):
            advertise_callback, advertise_data, advertise_settings = (
                generate_ble_advertise_objects(self.adv_ad.droid))

            self.adv_ad.droid.bleSetAdvertiseSettingsTimeout(
                advertise_timeout_s * 1000)

            self.adv_ad.droid.bleStartBleAdvertising(
                advertise_callback, advertise_data, advertise_settings)
            try:
                self.adv_ad.ed.pop_event(
                    adv_succ.format(advertise_callback), self.default_timeout)
            except Empty as error:
                self.log.error("Test failed with Empty error: {}".format(
                    error))
                test_result = False
            except concurrent.futures._base.TimeoutError as error:
                self.log.debug(
                    "Test failed, filtering callback onSuccess never occurred: {}".
                    format(error))

            if not test_result:
                return test_result

            time.sleep(advertise_timeout_s + 1)

        return test_result
