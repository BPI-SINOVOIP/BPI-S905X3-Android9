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
Test script to exercises Ble Scans can run in concurrency.
This test was designed to be run in a shield box.
"""

import concurrent
import time

from queue import Empty
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import ble_advertise_settings_modes
from acts.test_utils.bt.bt_constants import ble_scan_settings_callback_types
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_constants import adv_succ
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import scan_failed
from acts.test_utils.bt.bt_constants import scan_result
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs


class ConcurrentBleScanningTest(BluetoothBaseTest):
    default_timeout = 20
    max_concurrent_scans = 27

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.scn_ad = self.android_devices[0]
        self.adv_ad = self.android_devices[1]

    def on_fail(self, test_name, begin_time):
        self.log.debug("Test {} failed. Gathering bugreport and btsnoop logs."
                       .format(test_name))
        take_btsnoop_logs(self.android_devices, self, test_name)
        reset_bluetooth(self.android_devices)

    def setup_test(self):
        return reset_bluetooth(self.android_devices)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e7f68b9b-fb3f-48e9-a272-e41c2a32b4bd')
    def test_max_concurrent_ble_scans(self):
        """Test max LE scans.

        Test that a single device can have max scans concurrently scanning.

        Steps:
        1. Initialize scanner
        2. Initialize advertiser
        3. Start advertising on the device from step 2
        4. Create max ble scan callbacks
        5. Start ble scan on each callback
        6. Verify that each callback triggers
        7. Stop all scans and advertisements

        Expected Result:
        All scanning instances should start without errors and the advertisement
        should be found on each scan instance.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning, Concurrency
        Priority: 0
        """
        test_result = True
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.scn_ad.droid.bleSetScanSettingsCallbackType(
            ble_scan_settings_callback_types['all_matches'])
        self.scn_ad.droid.bleSetScanSettingsScanMode(ble_scan_settings_modes[
            'low_latency'])
        self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
            ble_advertise_settings_modes['low_latency'])
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleSetAdvertiseSettingsIsConnectable(False)
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        try:
            self.adv_ad.ed.pop_event(
                adv_succ.format(advertise_callback), self.default_timeout)
        except Empty as error:
            self.log.exception("Test failed with Empty error: {}".format(
                error))
            test_result = False
        except concurrent.futures._base.TimeoutError as error:
            self.log.exception(
                "Test failed callback onSuccess never occurred: "
                "{}".format(error))
            test_result = False
        if not test_result:
            return test_result
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback_list = []
        for i in range(self.max_concurrent_scans):
            self.log.debug("Concurrent Ble Scan iteration {}".format(i + 1))
            scan_callback = self.scn_ad.droid.bleGenScanCallback()
            scan_callback_list.append(scan_callback)
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                              scan_callback)
            try:
                self.scn_ad.ed.pop_event(
                    scan_result.format(scan_callback), self.default_timeout)
                self.log.info("Found scan event successfully. Iteration {} "
                              "successful.".format(i))
            except Exception:
                self.log.info("Failed to find a scan result for callback {}"
                              .format(scan_callback))
                test_result = False
                break
        for callback in scan_callback_list:
            self.scn_ad.droid.bleStopBleScan(callback)
        self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        if not test_result:
            return test_result
        self.log.info("Waiting for scan callbacks to stop completely.")
        # Wait for all scan callbacks to stop. There is no confirmation
        # otherwise.
        time.sleep(10)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='58b0c45e-1cbc-420a-9e89-901518ffe3d1')
    def test_max_concurrent_ble_scans_then_discover_advertisement(self):
        """Test max LE scans variant.

        Test that a single device can have max scans concurrently scanning.

        Steps:
        1. Initialize scanner
        2. Initialize advertiser
        3. Create max ble scan callbacks
        4. Start ble scan on each callback
        5. Start advertising on the device from step 2
        6. Verify that each callback triggers
        7. Stop all scans and advertisements

        Expected Result:
        All scanning instances should start without errors and the advertisement
        should be found on each scan instance.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning, Concurrency
        Priority: 1
        """
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.scn_ad.droid.bleSetScanSettingsCallbackType(
            ble_scan_settings_callback_types['all_matches'])
        self.scn_ad.droid.bleSetScanSettingsScanMode(ble_scan_settings_modes[
            'low_latency'])
        self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
            ble_advertise_settings_modes['low_latency'])
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback_list = []
        for i in range(self.max_concurrent_scans):
            self.log.debug("Concurrent Ble Scan iteration {}".format(i + 1))
            scan_callback = self.scn_ad.droid.bleGenScanCallback()
            scan_callback_list.append(scan_callback)
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                              scan_callback)
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        try:
            self.adv_ad.ed.pop_event(
                adv_succ.format(advertise_callback), self.default_timeout)
        except Empty as error:
            self.log.exception("Test failed with Empty error: {}".format(
                error))
            return False
        except concurrent.futures._base.TimeoutError as error:
            self.log.exception("Test failed, filtering callback onSuccess "
                               "never occurred: {}".format(error))
            return False
        i = 0
        for callback in scan_callback_list:
            try:
                self.scn_ad.ed.pop_event(
                    scan_result.format(scan_callback), self.default_timeout)
                self.log.info(
                    "Found scan event successfully. Iteration {} successful."
                    .format(i))
            except Exception:
                self.log.info("Failed to find a scan result for callback {}"
                              .format(scan_callback))
                return False
            i += 1
        for callback in scan_callback_list:
            self.scn_ad.droid.bleStopBleScan(callback)
        self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7a45e45c-faf3-4e89-abb7-a52f63e53208')
    def test_max_concurrent_ble_scans_plus_one(self):
        """Test mac LE scans variant.

        Test that a single device can have max scans concurrently scanning.

        Steps:
        1. Initialize scanner
        3. Create max ble scan callbacks plus one
        5. Start ble scan on each callback
        6. Verify that the n+1th scan fails.
        7. Stop all scans

        Expected Result:
        The n+1th scan should fail to start.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning, Concurrency
        Priority: 1
        """
        test_result = True
        self.scn_ad.droid.bleSetScanSettingsCallbackType(
            ble_scan_settings_callback_types['all_matches'])
        self.scn_ad.droid.bleSetScanSettingsScanMode(ble_scan_settings_modes[
            'low_latency'])
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback_list = []
        for i in range(self.max_concurrent_scans):
            self.log.debug("Concurrent Ble Scan iteration {}".format(i + 1))
            scan_callback = self.scn_ad.droid.bleGenScanCallback()
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                              scan_callback)
            scan_callback_list.append(scan_callback)
        scan_callback = self.scn_ad.droid.bleGenScanCallback()
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        try:
            self.scn_ad.ed.pop_event(
                scan_failed.format(scan_callback), self.default_timeout)
            self.log.info(
                "Found scan event successfully. Iteration {} successful."
                .format(i))
        except Exception:
            self.log.info("Failed to find a onScanFailed event for callback {}"
                          .format(scan_callback))
            test_result = False
        for callback in scan_callback_list:
            self.scn_ad.droid.bleStopBleScan(callback)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5a91f612-69e5-490f-b9d0-50d58a3db736')
    def test_max_concurrent_ble_scans_verify_scans_stop_independently(self):
        """Test max LE scans variant.

        Test that a single device can have max scans concurrently scanning.

        Steps:
        1. Initialize scanner
        2. Initialize advertiser
        3. Create max ble scan callbacks
        4. Start ble scan on each callback
        5. Start advertising on the device from step 2
        6. Verify that the first callback triggers
        7. Stop the scan and repeat steps 6 and 7 until all scans stopped

        Expected Result:
        All scanning instances should start without errors and the advertisement
        should be found on each scan instance. All scanning instances should
        stop successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning, Concurrency
        Priority: 1
        """
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.scn_ad.droid.bleSetScanSettingsCallbackType(
            ble_scan_settings_callback_types['all_matches'])
        self.scn_ad.droid.bleSetScanSettingsScanMode(ble_scan_settings_modes[
            'low_latency'])
        self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
            ble_advertise_settings_modes['low_latency'])
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback_list = []
        for i in range(self.max_concurrent_scans):
            self.log.debug("Concurrent Ble Scan iteration {}".format(i + 1))
            scan_callback = self.scn_ad.droid.bleGenScanCallback()
            scan_callback_list.append(scan_callback)
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                              scan_callback)
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        try:
            self.adv_ad.ed.pop_event(
                adv_succ.format(advertise_callback), self.default_timeout)
        except Empty as error:
            self.log.exception("Test failed with Empty error: {}".format(
                error))
            return False
        except concurrent.futures._base.TimeoutError as error:
            self.log.exception(
                "Test failed, filtering callback onSuccess never"
                " occurred: {}".format(error))
            return False
        i = 0
        for callback in scan_callback_list:
            expected_scan_event_name = scan_result.format(scan_callback)
            try:
                self.scn_ad.ed.pop_event(expected_scan_event_name,
                                         self.default_timeout)
                self.log.info(
                    "Found scan event successfully. Iteration {} successful.".
                    format(i))
                i += 1
            except Exception:
                self.log.info("Failed to find a scan result for callback {}".
                              format(scan_callback))
                return False
            self.scn_ad.droid.bleStopBleScan(callback)
        self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        return True
