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

import time

from queue import Empty
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import bt_scan_mode_types
from acts.test_utils.bt.bt_test_utils import check_device_supported_profiles
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import set_device_name
from acts.test_utils.bt.bt_test_utils import set_bt_scan_mode
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs


class BtBasicFunctionalityTest(BluetoothBaseTest):
    default_timeout = 10
    scan_discovery_time = 5

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.droid_ad = self.android_devices[0]
        self.droid1_ad = self.android_devices[1]

    def setup_class(self):
        return setup_multiple_devices_for_bt_test(self.android_devices)

    def setup_test(self):
        for a in self.android_devices:
            a.ed.clear_all_events()
        return True

    def teardown_test(self):
        return True

    def on_fail(self, test_name, begin_time):
        take_btsnoop_logs(self.android_devices, self, test_name)
        reset_bluetooth(self.android_devices)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5a5dcf94-8114-405c-a048-b80d73e80ecc')
    def test_bluetooth_reset(self):
        """Test resetting bluetooth.

        Test the integrity of resetting bluetooth on Android.

        Steps:
        1. Toggle bluetooth off.
        2. Toggle bluetooth on.

        Expected Result:
        Bluetooth should toggle on and off without failing.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        return reset_bluetooth([self.droid_ad])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='fc205cb8-6878-4f97-b9c8-7ed532742a1b')
    def test_make_device_discoverable(self):
        """Test device discoverablity.

        Test that verifies devices is discoverable.

        Steps:
        1. Initialize two android devices
        2. Make device1 discoverable
        3. Check discoverable device1 scan mode
        4. Make device2 start discovery
        5. Use device2 get all discovered bluetooth devices list
        6. Verify device1 is in the list

        Expected Result:
        Device1 is in the discovered devices list.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        self.droid_ad.droid.bluetoothMakeDiscoverable()
        scan_mode = self.droid_ad.droid.bluetoothGetScanMode()
        if (scan_mode == bt_scan_mode_types['connectable_discoverable']):
            self.log.debug("Android device1 scan mode is "
                           "SCAN_MODE_CONNECTABLE_DISCOVERABLE")
        else:
            self.log.debug("Android device1 scan mode is not "
                           "SCAN_MODE_CONNECTABLE_DISCOVERABLE")
            return False
        if self.droid1_ad.droid.bluetoothStartDiscovery():
            self.log.debug("Android device2 start discovery process success")
            # Give Bluetooth time to discover advertising devices
            time.sleep(self.scan_discovery_time)
            droid_name = self.droid_ad.droid.bluetoothGetLocalName()
            get_all_discovered_devices = self.droid1_ad.droid.bluetoothGetDiscoveredDevices(
            )
            find_flag = False
            if get_all_discovered_devices:
                self.log.debug(
                    "Android device2 get all the discovered devices "
                    "list {}".format(get_all_discovered_devices))
                for i in get_all_discovered_devices:
                    if 'name' in i and i['name'] == droid_name:
                        self.log.debug("Android device1 is in the discovery "
                                       "list of device2")
                        find_flag = True
                        break
            else:
                self.log.debug(
                    "Android device2 get all the discovered devices "
                    "list is empty")
                return False
        else:
            self.log.debug("Android device2 start discovery process error")
            return False
        if not find_flag:
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c4d77bde-04ed-4805-9185-9bc46dc8af4b')
    def test_make_device_undiscoverable(self):
        """Test device un-discoverability.

        Test that verifies device is un-discoverable.

        Steps:
        1. Initialize two android devices
        2. Make device1 un-discoverable
        3. Check un-discoverable device1 scan mode
        4. Make device2 start discovery
        5. Use device2 get all discovered bluetooth devices list
        6. Verify device1 is not in the list

        Expected Result:
        Device1 should not be in the discovered devices list.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        self.droid_ad.droid.bluetoothMakeUndiscoverable()
        set_bt_scan_mode(self.droid1_ad, bt_scan_mode_types['none'])
        scan_mode = self.droid1_ad.droid.bluetoothGetScanMode()
        if scan_mode == bt_scan_mode_types['none']:
            self.log.debug("Android device1 scan mode is SCAN_MODE_NONE")
        else:
            self.log.debug("Android device1 scan mode is not SCAN_MODE_NONE")
            return False
        if self.droid1_ad.droid.bluetoothStartDiscovery():
            self.log.debug("Android device2 start discovery process success")
            # Give Bluetooth time to discover advertising devices
            time.sleep(self.scan_discovery_time)
            droid_name = self.droid_ad.droid.bluetoothGetLocalName()
            get_all_discovered_devices = self.droid1_ad.droid.bluetoothGetDiscoveredDevices(
            )
            find_flag = False
            if get_all_discovered_devices:
                self.log.debug(
                    "Android device2 get all the discovered devices "
                    "list {}".format(get_all_discovered_devices))
                for i in get_all_discovered_devices:
                    if 'name' in i and i['name'] == droid_name:
                        self.log.debug(
                            "Android device1 is in the discovery list of "
                            "device2")
                        find_flag = True
                        break
            else:
                self.log.debug("Android device2 found no devices.")
                return True
        else:
            self.log.debug("Android device2 start discovery process error")
            return False
        if find_flag:
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2bcb6288-64c3-437e-bc89-bcd416310135')
    def test_set_device_name(self):
        """Test bluetooth device name.

        Test that a single device can be set device name.

        Steps:
        1. Initialize one android devices
        2. Set device name
        3. Return true is set device name success

        Expected Result:
        Bluetooth device name is set correctly.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        name = "SetDeviceName"
        return set_device_name(self.droid_ad.droid, name)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b38fb110-a707-47cf-b1c3-981266373786')
    def test_scan_mode_off(self):
        """Test disabling bluetooth scanning.

        Test that changes scan mode to off.

        Steps:
        1. Initialize android device.
        2. Set scan mode STATE_OFF by disabling bluetooth.
        3. Verify scan state.

        Expected Result:
        Verify scan state is off.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        self.log.debug("Test scan mode STATE_OFF.")
        return set_bt_scan_mode(self.droid_ad, bt_scan_mode_types['state_off'])

    #@BluetoothTest(UUID=27576aa8-d52f-45ad-986a-f44fb565167d)
    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='702c3d58-94fd-47ee-9323-2421ce182ddb')
    def test_scan_mode_none(self):
        """Test bluetooth scan mode none.

        Test that changes scan mode to none.

        Steps:
        1. Initialize android device.
        2. Set scan mode SCAN_MODE_NONE by disabling bluetooth.
        3. Verify scan state.

        Expected Result:
        Verify that scan mode is set to none.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        self.log.debug("Test scan mode SCAN_MODE_NONE.")
        return set_bt_scan_mode(self.droid_ad, bt_scan_mode_types['none'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cb998a99-31a6-46b6-9de6-a9a17081a604')
    def test_scan_mode_connectable(self):
        """Test bluetooth scan mode connectable.

        Test that changes scan mode to connectable.

        Steps:
        1. Initialize android device.
        2. Set scan mode SCAN_MODE_CONNECTABLE.
        3. Verify scan state.

        Expected Result:
        Verify that scan mode is set to connectable.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 2
        """
        self.log.debug("Test scan mode SCAN_MODE_CONNECTABLE.")
        return set_bt_scan_mode(self.droid_ad,
                                bt_scan_mode_types['connectable'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='59bec55c-c64e-43e4-9a9a-e44408a801d7')
    def test_scan_mode_connectable_discoverable(self):
        """Test bluetooth scan mode connectable.

        Test that changes scan mode to connectable.

        Steps:
        1. Initialize android device.
        2. Set scan mode SCAN_MODE_DISCOVERABLE.
        3. Verify scan state.

        Expected Result:
        Verify that scan mode is set to discoverable.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 2
        """
        self.log.debug("Test scan mode SCAN_MODE_CONNECTABLE_DISCOVERABLE.")
        return set_bt_scan_mode(self.droid_ad,
                                bt_scan_mode_types['connectable_discoverable'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cd20a09d-a68d-4f55-b016-ba283b0460df')
    def test_if_support_hid_profile(self):
        """ Test that a single device can support HID profile.
        Steps
        1. Initialize one android devices
        2. Check devices support profiles and return a dictionary
        3. Check the value of key 'hid'

        Expected Result:
        Device1 is in the discovered devices list.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        profiles = check_device_supported_profiles(self.droid_ad.droid)
        if not profiles['hid']:
            self.log.debug("Android device do not support HID profile.")
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a110d330-7090-4784-a33b-33089dc5f67f')
    def test_if_support_hsp_profile(self):
        """ Test that a single device can support HSP profile.
        Steps
        1. Initialize one android devices
        2. Check devices support profiles and return a dictionary
        3. Check the value of key 'hsp'
        :return: test_result: bool
        """
        profiles = check_device_supported_profiles(self.droid_ad.droid)
        if not profiles['hsp']:
            self.log.debug("Android device do not support HSP profile.")
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9ccefdd9-62a9-4aed-b4d9-7b0a55c338b2')
    def test_if_support_a2dp_profile(self):
        """ Test that a single device can support A2DP profile.
        Steps
        1. Initialize one android devices
        2. Check devices support profiles and return a dictionary
        3. Check the value of key 'a2dp'
        :return: test_result: bool
        """
        profiles = check_device_supported_profiles(self.droid_ad.droid)
        if not profiles['a2dp']:
            self.log.debug("Android device do not support A2DP profile.")
            return False
        return True
