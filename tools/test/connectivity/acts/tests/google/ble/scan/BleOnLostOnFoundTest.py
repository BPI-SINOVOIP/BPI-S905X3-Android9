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
This test script exercises different onLost/onFound scenarios.
"""

from queue import Empty
from acts import utils
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import ble_advertise_settings_modes
from acts.test_utils.bt.bt_constants import ble_scan_settings_callback_types
from acts.test_utils.bt.BleEnum import ScanSettingsMatchMode
from acts.test_utils.bt.BleEnum import ScanSettingsMatchNum
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_constants import ble_scan_settings_match_modes
from acts.test_utils.bt.bt_constants import ble_scan_settings_match_nums
from acts.test_utils.bt.bt_constants import adv_succ
from acts.test_utils.bt.bt_test_utils import cleanup_scanners_and_advertisers
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_constants import scan_result


class BleOnLostOnFoundTest(BluetoothBaseTest):
    default_timeout = 10
    active_scan_callback_list = []
    active_adv_callback_list = []

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.scn_ad = self.android_devices[0]
        self.adv_ad = self.android_devices[1]

    def setup_class(self):
        super(BluetoothBaseTest, self).setup_class()
        utils.set_location_service(self.scn_ad, True)
        utils.set_location_service(self.adv_ad, True)
        return True

    def teardown_test(self):
        cleanup_scanners_and_advertisers(
            self.scn_ad, self.active_adv_callback_list, self.adv_ad,
            self.active_adv_callback_list)
        self.active_adv_callback_list = []
        self.active_scan_callback_list = []

    def on_exception(self, test_name, begin_time):
        reset_bluetooth(self.android_devices)

    def _start_generic_advertisement_include_device_name(self):
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
            ble_advertise_settings_modes['low_latency'])
        advertise_data = self.adv_ad.droid.bleBuildAdvertiseData()
        advertise_settings = self.adv_ad.droid.bleBuildAdvertiseSettings()
        advertise_callback = self.adv_ad.droid.bleGenBleAdvertiseCallback()
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        self.adv_ad.ed.pop_event(
            adv_succ.format(advertise_callback), self.default_timeout)
        self.active_adv_callback_list.append(advertise_callback)
        return advertise_callback

    def _verify_no_events_found(self, event_name):
        try:
            self.scn_ad.ed.pop_event(event_name, self.default_timeout)
            self.log.error("Found an event when none was expected.")
            return False
        except Empty:
            self.log.info("No scan result found as expected.")
            return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9bd7fd09-71c9-4623-90f0-9a895eb37409')
    def test_onlost_onfound_defaults(self):
        """Test generic onlost/onfound defaults.

        Tests basic onFound/onLost functionality.

        Steps:
        1. Setup dut0 scanner and start scan with this setup:
          Scan Mode: SCAN_MODE_LOW_LATENCY
          Callback Type: CALLBACK_TYPE_FOUND_AND_LOST
          Match Mode: AGGRESSIVE
          Num of Matches: MATCH_NUM_ONE_ADVERTISEMENT
          Filter: Device name of dut1
        2. Start an advertisement on dut1, include device name
        3. Find an onFound event
        4. Stop the advertisement on dut1
        5. Find an onLost event

        Expected Result:
        Find an onLost and an onFound event successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, onLost, onFound
        Priority: 0
        """
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleSetScanSettingsScanMode(ble_scan_settings_modes[
            'low_latency'])
        self.scn_ad.droid.bleSetScanSettingsCallbackType(
            ble_scan_settings_callback_types['found_and_lost'])
        self.scn_ad.droid.bleSetScanSettingsMatchMode(
            ble_scan_settings_match_modes['aggresive'])
        self.scn_ad.droid.bleSetScanSettingsNumOfMatches(
            ble_scan_settings_match_nums['one'])
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback = self.scn_ad.droid.bleGenScanCallback()
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        adv_callback = self._start_generic_advertisement_include_device_name()
        event = self.scn_ad.ed.pop_event(
            scan_result.format(scan_callback), self.default_timeout * 3)
        found_callback_type = event['data']['CallbackType']
        if event['data']['CallbackType'] != ble_scan_settings_callback_types[
                'first_match']:
            self.log.info(
                "Found Callbacreset_bluetoothkType:{}, Expected CallbackType:{}".
                format(found_callback_type, ble_scan_settings_callback_types[
                    'first_match']))
            return False
        self.adv_ad.droid.bleStopBleAdvertising(adv_callback)
        event = self.scn_ad.ed.pop_event(
            scan_result.format(scan_callback), self.default_timeout * 4)
        found_callback_type = event['data']['CallbackType']
        if found_callback_type != ble_scan_settings_callback_types[
                'match_lost']:
            self.log.info(
                "Found CallbackType:{}, Expected CallbackType:{}".format(
                    found_callback_type, ble_scan_settings_callback_types[
                        'match_lost']))
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='10b48dc9-0c2a-46a3-8890-5cde3004a996')
    def test_onlost_onfound_match_mode_sticky(self):
        """Test generic onlost/onfound in sticky mode.

        Tests basic onFound/onLost functionality.

        Steps:
        1. Setup dut0 scanner and start scan with this setup:
          Scan Mode: SCAN_MODE_LOW_LATENCY
          Callback Type: CALLBACK_TYPE_FOUND_AND_LOST
          Match Mode: STICKY
          Num of Matches: MATCH_NUM_ONE_ADVERTISEMENT
          Filter: Device name of dut1
        2. Start an advertisement on dut1, include device name
        3. Find an onFound event
        4. Stop the advertisement on dut1
        5. Find an onLost event

        Expected Result:
        Find an onLost and an onFound event successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, onLost, onFound
        Priority: 1
        """
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleSetScanSettingsScanMode(ble_scan_settings_modes[
            'low_latency'])
        self.scn_ad.droid.bleSetScanSettingsCallbackType(
            ble_scan_settings_callback_types['found_and_lost'])
        self.scn_ad.droid.bleSetScanSettingsMatchMode(
            ble_scan_settings_match_modes['sticky'])
        self.scn_ad.droid.bleSetScanSettingsNumOfMatches(
            ble_scan_settings_match_nums['one'])
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback = self.scn_ad.droid.bleGenScanCallback()
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        adv_callback = self._start_generic_advertisement_include_device_name()
        event = self.scn_ad.ed.pop_event(
            scan_result.format(scan_callback), self.default_timeout * 3)
        found_callback_type = event['data']['CallbackType']
        if event['data']['CallbackType'] != ble_scan_settings_callback_types[
                'first_match']:
            self.log.info(
                "Found CallbackType:{}, Expected CallbackType:{}".format(
                    found_callback_type, ble_scan_settings_callback_types[
                        'first_match']))
            return False
        self.adv_ad.droid.bleStopBleAdvertising(adv_callback)
        event = self.scn_ad.ed.pop_event(
            scan_result.format(scan_callback), self.default_timeout * 4)
        found_callback_type = event['data']['CallbackType']
        if found_callback_type != ble_scan_settings_callback_types[
                'match_lost']:
            self.log.info(
                "Found CallbackType:{}, Expected CallbackType:{}".format(
                    found_callback_type, ble_scan_settings_callback_types[
                        'match_lost']))
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4fefed82-7800-41be-8272-aac076640fed')
    def test_onlost_onfound_match_num_few(self):
        """Test generic onlost/onfound num few.

        Tests basic onFound/onLost functionality.

        Steps:
        1. Setup dut0 scanner and start scan with this setup:
          Scan Mode: SCAN_MODE_LOW_LATENCY
          Callback Type: CALLBACK_TYPE_FOUND_AND_LOST
          Match Mode: AGGRESSIVE
          Num of Matches: MATCH_NUM_FEW_ADVERTISEMENT
          Filter: Device name of dut1
        2. Start an advertisement on dut1, include device name
        3. Find an onFound event
        4. Stop the advertisement on dut1
        5. Find an onLost event

        Expected Result:
        Find an onLost and an onFound event successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, onLost, onFound
        Priority: 1
        """
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleSetScanSettingsScanMode(ble_scan_settings_modes[
            'low_latency'])
        self.scn_ad.droid.bleSetScanSettingsCallbackType(
            ble_scan_settings_callback_types['found_and_lost'])
        self.scn_ad.droid.bleSetScanSettingsMatchMode(
            ble_scan_settings_match_modes['aggresive'])
        self.scn_ad.droid.bleSetScanSettingsNumOfMatches(
            ble_scan_settings_match_nums['few'])
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback = self.scn_ad.droid.bleGenScanCallback()
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        adv_callback = self._start_generic_advertisement_include_device_name()
        event = self.scn_ad.ed.pop_event(
            scan_result.format(scan_callback), self.default_timeout * 3)
        found_callback_type = event['data']['CallbackType']
        if event['data']['CallbackType'] != ble_scan_settings_callback_types[
                'first_match']:
            self.log.info(
                "Found CallbackType:{}, Expected CallbackType:{}".format(
                    found_callback_type, ble_scan_settings_callback_types[
                        'first_match']))
            return False
        self.adv_ad.droid.bleStopBleAdvertising(adv_callback)
        event = self.scn_ad.ed.pop_event(
            scan_result.format(scan_callback), self.default_timeout * 4)
        found_callback_type = event['data']['CallbackType']
        if found_callback_type != ble_scan_settings_callback_types[
                'match_lost']:
            self.log.info(
                "Found CallbackType:{}, Expected CallbackType:{}".format(
                    found_callback_type, ble_scan_settings_callback_types[
                        'match_lost']))
            return False
        return True
