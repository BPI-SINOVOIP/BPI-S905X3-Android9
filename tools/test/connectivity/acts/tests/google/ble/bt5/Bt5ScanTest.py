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
This test script exercises different Bluetooth 5 specific scan scenarios.
It is expected that the second AndroidDevice is able to advertise.

This test script was designed with this setup in mind:
Shield box one: Android Device, Android Device
"""

from queue import Empty

from acts import asserts
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import ble_scan_settings_phys
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_test_utils import batch_scan_result
from acts.test_utils.bt.bt_test_utils import cleanup_scanners_and_advertisers
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_constants import scan_result
from acts.test_utils.bt.bt_test_utils import advertising_set_on_own_address_read
from acts.test_utils.bt.bt_test_utils import advertising_set_started
from acts import signals


class Bt5ScanTest(BluetoothBaseTest):
    default_timeout = 10
    report_delay = 2000
    scan_callbacks = []
    adv_callbacks = []
    active_scan_callback_list = []
    big_adv_data = {
        "includeDeviceName": True,
        "manufacturerData": [0x0123, "00112233445566778899AABBCCDDEE"],
        "manufacturerData2":
        [0x2540, [0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0xFF]],
        "serviceData": [
            "b19d42dc-58ba-4b20-b6c1-6628e7d21de4",
            "00112233445566778899AABBCCDDEE"
        ],
        "serviceData2": [
            "000042dc-58ba-4b20-b6c1-6628e7d21de4",
            [0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0xFF]
        ]
    }

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.scn_ad = self.android_devices[0]
        self.adv_ad = self.android_devices[1]

    def setup_class(self):
        if not self.scn_ad.droid.bluetoothIsLeExtendedAdvertisingSupported():
            raise signals.TestSkipClass(
                "Scanner does not support LE Extended Advertising")

        if not self.adv_ad.droid.bluetoothIsLeExtendedAdvertisingSupported():
            raise signals.TestSkipClass(
                "Advertiser does not support LE Extended Advertising")

    def teardown_test(self):
        cleanup_scanners_and_advertisers(
            self.scn_ad, self.active_scan_callback_list, self.adv_ad, [])
        self.active_scan_callback_list = []

    def on_exception(self, test_name, begin_time):
        reset_bluetooth(self.android_devices)

    # This one does not relly test anything, but display very helpful
    # information that might help with debugging.
    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='787e0877-269f-4b9b-acb0-b98a8bb3770a')
    def test_capabilities(self):
        """Test capabilities

        Test BT 5.0 scan scapabilities

        Steps:
        1. Test various vapabilities.

        Expected Result:
        Pass

        Returns:
          Pass if True
          Fail if False

        TAGS: BT5.0, Scanning
        Priority: 2
        """
        d = self.scn_ad.droid
        sup2M = d.bluetoothIsLe2MPhySupported()
        supCoded = d.bluetoothIsLeCodedPhySupported()
        supExt = d.bluetoothIsLeExtendedAdvertisingSupported()
        supPeriodic = d.bluetoothIsLePeriodicAdvertisingSupported()
        maxDataLen = d.bluetoothGetLeMaximumAdvertisingDataLength()
        self.log.info("Scanner capabilities:")
        self.log.info("LE 2M: " + str(sup2M) + ", LE Coded: " + str(
            supCoded) + ", LE Extended Advertising: " + str(
                supExt) + ", LE Periodic Advertising: " + str(supPeriodic) +
                      ", maximum advertising data length: " + str(maxDataLen))
        d = self.adv_ad.droid
        sup2M = d.bluetoothIsLe2MPhySupported()
        supCoded = d.bluetoothIsLeCodedPhySupported()
        supExt = d.bluetoothIsLeExtendedAdvertisingSupported()
        supPeriodic = d.bluetoothIsLePeriodicAdvertisingSupported()
        maxDataLen = d.bluetoothGetLeMaximumAdvertisingDataLength()
        self.log.info("Advertiser capabilities:")
        self.log.info("LE 2M: " + str(sup2M) + ", LE Coded: " + str(
            supCoded) + ", LE Extended Advertising: " + str(
                supExt) + ", LE Periodic Advertising: " + str(supPeriodic) +
                      ", maximum advertising data length: " + str(maxDataLen))
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='62d36679-bb91-465e-897f-2635433aac2f')
    def test_1m_1m_extended_scan(self):
        """Test scan on LE 1M PHY using LE 1M PHY as secondary.

        Tests test verify that device is able to receive extended advertising
        on 1M PHY when secondary is 1M PHY.

        Steps:
        1. Start advertising set on dut1
        2. Start scanning on dut0, scan filter set to advertiser's device name
        3. Try to find an event, expect found
        4. Stop advertising

        Expected Result:
        Scan finds a advertisement.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE Advertising Extension, BT5, LE, Advertising, Scanning
        Priority: 1
        """
        adv_callback = self.adv_ad.droid.bleAdvSetGenCallback()
        self.adv_ad.droid.bleAdvSetStartAdvertisingSet({
            "connectable": True,
            "legacyMode": False,
            "primaryPhy": "PHY_LE_1M",
            "secondaryPhy": "PHY_LE_1M",
            "interval": 320
        }, self.big_adv_data, None, None, None, 0, 0, adv_callback)

        self.scn_ad.droid.bleSetScanSettingsLegacy(False)
        self.scn_ad.droid.bleSetScanSettingsPhy(ble_scan_settings_phys['1m'])

        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)

        adv_device_name = self.adv_ad.droid.bluetoothGetLocalName()
        self.scn_ad.droid.bleSetScanFilterDeviceName(adv_device_name)
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)

        try:
            self.scn_ad.ed.pop_event(
                scan_result.format(scan_callback), self.default_timeout)
        except Empty:
            self.log.error("Scan result not found")
            self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
            return False

        self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='3e3c9757-f7b6-4d1d-a2d6-8e2330d1a18e')
    def test_1m_2m_extended_scan(self):
        """Test scan on LE 1M PHY using LE 2M PHY as secondary.

        Tests test verify that device is able to receive extended advertising
        on 1M PHY when secondary is 2M PHY.

        Steps:
        1. Start advertising set on dut1
        2. Start scanning on dut0, scan filter set to advertiser's device name
        3. Try to find an event, expect found
        4. Stop advertising

        Expected Result:
        Scan finds a advertisement.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE Advertising Extension, BT5, LE, Advertising, Scanning
        Priority: 1
        """
        adv_callback = self.adv_ad.droid.bleAdvSetGenCallback()
        self.adv_ad.droid.bleAdvSetStartAdvertisingSet({
            "connectable": True,
            "legacyMode": False,
            "primaryPhy": "PHY_LE_1M",
            "secondaryPhy": "PHY_LE_2M",
            "interval": 320
        }, self.big_adv_data, None, None, None, 0, 0, adv_callback)

        self.scn_ad.droid.bleSetScanSettingsLegacy(False)
        self.scn_ad.droid.bleSetScanSettingsPhy(ble_scan_settings_phys['1m'])

        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)

        adv_device_name = self.adv_ad.droid.bluetoothGetLocalName()
        self.scn_ad.droid.bleSetScanFilterDeviceName(adv_device_name)
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)

        try:
            self.scn_ad.ed.pop_event(
                scan_result.format(scan_callback), self.default_timeout)
        except Empty:
            self.log.error("Scan result not found")
            self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
            return False

        self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='236e9e5b-3853-4762-81ae-e88db03d74f3')
    def test_legacy_scan_result_raw_length(self):
        """Test that raw scan record data in legacy scan is 62 bytes long.

        This is required for compability with older apps that make this
        assumption.

        Steps:
        1. Start legacy advertising set on dut1
        2. Start scanning on dut0, scan filter set to advertiser's device name
        3. Try to find an event, expect found, verify scan recurd data length
        4. Stop advertising

        Expected Result:
        Scan finds a legacy advertisement of proper size

        Returns:
          Pass if True
          Fail if False

        TAGS: LE Advertising Extension, BT5, LE, Advertising, Scanning
        Priority: 1
        """
        adv_callback = self.adv_ad.droid.bleAdvSetGenCallback()
        self.adv_ad.droid.bleAdvSetStartAdvertisingSet({
            "connectable": True,
            "scannable": True,
            "legacyMode": True,
            "interval": 320
        }, {"includeDeviceName": True}, None, None, None, 0, 0, adv_callback)

        self.scn_ad.droid.bleSetScanSettingsLegacy(True)
        self.scn_ad.droid.bleSetScanSettingsPhy(ble_scan_settings_phys['1m'])

        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)

        adv_device_name = self.adv_ad.droid.bluetoothGetLocalName()
        self.scn_ad.droid.bleSetScanFilterDeviceName(adv_device_name)
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)

        try:
            evt = self.scn_ad.ed.pop_event(
                scan_result.format(scan_callback), self.default_timeout)
            rawData = evt['data']['Result']['scanRecord']
            asserts.assert_true(62 == len(rawData.split(",")),
                                "Raw data should be 62 bytes long.")
        except Empty:
            self.log.error("Scan result not found")
            self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
            return False

        self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='04632d8d-4303-476f-8f83-52c16be3713a')
    def test_duration(self):
        """Test scanning duration

        Tests BT5.0 scanning duration

        Steps:
        1. Start advertising set
        2. Start 5.0 scan
        3. Scan for advertisement event

        Expected Result:
        Scan finds a legacy advertisement of proper size

        Returns:
          Pass if True
          Fail if False

        TAGS: BT5.0, LE, Advertising, Scanning
        Priority: 1
        """
        adv_callback = self.adv_ad.droid.bleAdvSetGenCallback()
        self.adv_ad.droid.bleAdvSetStartAdvertisingSet({
            "connectable": True,
            "legacyMode": False,
            "primaryPhy": "PHY_LE_1M",
            "secondaryPhy": "PHY_LE_2M",
            "interval": 320
        }, self.big_adv_data, None, None, None, 0, 0, adv_callback)

        self.scn_ad.droid.bleSetScanSettingsLegacy(False)
        self.scn_ad.droid.bleSetScanSettingsPhy(ble_scan_settings_phys['1m'])

        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)

        adv_device_name = self.adv_ad.droid.bluetoothGetLocalName()
        self.scn_ad.droid.bleSetScanFilterDeviceName(adv_device_name)
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)

        try:
            self.scn_ad.ed.pop_event(
                scan_result.format(scan_callback), self.default_timeout)
        except Empty:
            self.log.error("Scan result not found")
            self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
            return False

        self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a3704083-0f5c-4a46-b979-32ebc594d6ee')
    def test_anonymous_advertising(self):
        """Test anonymous advertising.

        Tests test verify that device is able to receive anonymous advertising
        on 1M PHY when secondary is 2M PHY.

        Steps:
        1. Start anonymous advertising set on dut1
        2. Start scanning on dut0, scan filter set to advertiser's device name
        3. Try to find an event, expect found
        4. Stop advertising

        Expected Result:
        Scan finds a advertisement.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE Advertising Extension, BT5, LE, Advertising, Scanning
        Priority: 1
        """
        adv_callback = self.adv_ad.droid.bleAdvSetGenCallback()
        self.adv_ad.droid.bleAdvSetStartAdvertisingSet({
            "connectable": False,
            "anonymous": True,
            "legacyMode": False,
            "primaryPhy": "PHY_LE_1M",
            "secondaryPhy": "PHY_LE_2M",
            "interval": 320
        }, self.big_adv_data, None, None, None, 0, 0, adv_callback)

        self.scn_ad.droid.bleSetScanSettingsLegacy(False)
        self.scn_ad.droid.bleSetScanSettingsPhy(ble_scan_settings_phys['1m'])

        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)

        adv_device_name = self.adv_ad.droid.bluetoothGetLocalName()
        self.scn_ad.droid.bleSetScanFilterDeviceName(adv_device_name)
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)

        try:
            evt = self.scn_ad.ed.pop_event(
                scan_result.format(scan_callback), self.default_timeout)
            address = evt['data']['Result']['deviceInfo']['address']
            asserts.assert_true(
                '00:00:00:00:00:00' == address,
                "Anonymous address should be 00:00:00:00:00:00, but was " +
                str(address))
        except Empty:
            self.log.error("Scan result not found")
            self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
            return False

        self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e3277355-eebf-4760-9502-e49a9289f6ab')
    def test_get_own_address(self):
        """Test obtaining own address for PTS.

        Test obtaining own address.

        Steps:
        1. Start advertising set dut1
        2. Grab address
        3. Stop advertising

        Expected Result:
        Callback with address is received.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE Advertising Extension, BT5, LE, Advertising
        Priority: 1
        """
        adv_callback = self.adv_ad.droid.bleAdvSetGenCallback()
        self.adv_ad.droid.bleAdvSetStartAdvertisingSet({
            "connectable": False,
            "anonymous": True,
            "legacyMode": False,
            "primaryPhy": "PHY_LE_1M",
            "secondaryPhy": "PHY_LE_2M",
            "interval": 320
        }, self.big_adv_data, None, None, None, 0, 0, adv_callback)

        set_id = -1

        try:
            evt = self.adv_ad.ed.pop_event(
                advertising_set_started.format(adv_callback),
                self.default_timeout)
            self.log.info("data: " + str(evt['data']))
            set_id = evt['data']['setId']
        except Empty:
            self.log.error("did not receive the set started event!")
            self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
            return False

        self.adv_ad.droid.bleAdvSetGetOwnAddress(set_id)

        try:
            evt = self.adv_ad.ed.pop_event(
                advertising_set_on_own_address_read.format(set_id),
                self.default_timeout)
            address = evt['data']['address']
            self.log.info("Advertiser address is: " + str(address))
        except Empty:
            self.log.error("onOwnAddressRead not received.")
            self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
            return False

        self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
        return True
