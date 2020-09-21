#!/usr/bin/env python3.4
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
Bluetooth HID Device Test.
"""

from acts.base_test import BaseTestClass
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test
from acts.test_utils.bt.bt_test_utils import clear_bonded_devices
from acts.test_utils.bt.bt_test_utils import pair_pri_to_sec
from acts.test_utils.bt.bt_test_utils import hid_keyboard_report
from acts.test_utils.bt.bt_test_utils import hid_device_send_key_data_report
from acts.test_utils.bt.bt_constants import hid_connection_timeout
from acts.test_utils.bt import bt_constants
import time


class HidDeviceTest(BluetoothBaseTest):
    tests = None
    default_timeout = 10

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        self.host_ad = self.android_devices[0]
        self.device_ad = self.android_devices[1]

    def setup_test(self):
        for a in self.android_devices:
            if not clear_bonded_devices(a):
                return False
        for a in self.android_devices:
            a.ed.clear_all_events()

        i = 0
        while not self.device_ad.droid.bluetoothHidDeviceIsReady():
            time.sleep(1)
            i += 1
            self.log.info("BluetoothHidDevice NOT Ready")
            if i == 10:
                return False

        if not self.device_ad.droid.bluetoothHidDeviceRegisterApp():
            self.log.error("Device: registration failed")
            return False

        self.log.info("Device: registration done")
        return True

    def teardown_test(self):
        self.log.info("Device: unregister")
        self.device_ad.droid.bluetoothHidDeviceUnregisterApp()
        time.sleep(2)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='047afb31-96c5-4a56-acb5-2b216037f35d')
    def test_hid(self):
        """Test HID Host and Device basic functionality

        Test the HID Device framework app registration; test HID Host sending
        report through HID control channel and interrupt channel.

        Steps:
        1. Bluetooth HID device registers the Bluetooth input device service.
        2. Get the MAC address of the HID host and HID device.
        3. Establish HID profile connection from the HID host to the HID device.
        4. HID host sends set_report, get_report, set_protocol, send_data to
        the HID device, and check if the HID device receives them.
        5. HID device sends data report, report_error, reply_report commands to
        the HID host, and check if the HID host receives them.

        Expected Result:
        HID profile connection is successfully established; all commands and
        data reports are correctly handled.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, HID
        Priority: 1
        """

        test_result = True

        pair_pri_to_sec(self.host_ad, self.device_ad, attempts=3)

        self.log.info("Device bonded: {}".format(
                self.device_ad.droid.bluetoothGetBondedDevices()))
        self.log.info("Host bonded: {}".format(
                self.host_ad.droid.bluetoothGetBondedDevices()))

        host_id = self.host_ad.droid.bluetoothGetLocalAddress()
        device_id = self.device_ad.droid.bluetoothGetLocalAddress()

        self.host_ad.droid.bluetoothConnectBonded(device_id)

        time.sleep(hid_connection_timeout)
        self.log.info("Device: connected: {}".format(
                self.device_ad.droid.bluetoothHidDeviceGetConnectedDevices()))

        self.log.info("Host: set report")
        self.host_ad.droid.bluetoothHidSetReport(
                device_id, 1, bt_constants.hid_default_set_report_payload)

        try:
            hid_device_callback = self.device_ad.ed.pop_event(
                    bt_constants.hid_on_set_report_event,
                    bt_constants.hid_default_event_timeout)
        except Empty as err:
            self.log.error("Callback not received: {}".format(err))
            test_result = False

        self.log.info("Host: get report")
        self.host_ad.droid.bluetoothHidGetReport(device_id, 1, 1, 1024)

        try:
            hid_device_callback = self.device_ad.ed.pop_event(
                    bt_constants.hid_on_get_report_event,
                    bt_constants.hid_default_event_timeout)
        except Empty as err:
            self.log.error("Callback not received: {}".format(err))
            test_result = False

        self.log.info("Host: set_protocol")
        self.host_ad.droid.bluetoothHidSetProtocolMode(device_id, 1)

        try:
            hid_device_callback = self.device_ad.ed.pop_event(
                    bt_constants.hid_on_set_protocol_event,
                    bt_constants.hid_default_event_timeout)
        except Empty as err:
            self.log.error("Callback not received: {}".format(err))
            test_result = False

        self.log.info("Host: send data")
        self.host_ad.droid.bluetoothHidSendData(device_id, "It's a report")

        try:
            hid_device_callback = self.device_ad.ed.pop_event(
                    bt_constants.hid_on_intr_data_event,
                    bt_constants.hid_default_event_timeout)
        except Empty as err:
            self.log.error("Callback not received: {}".format(err))
            test_result = False

        self.log.info("Device: send data report through interrupt channel")
        hid_device_send_key_data_report(host_id, self.device_ad, "04")
        hid_device_send_key_data_report(host_id, self.device_ad, "05")

        self.log.info("Device: report error")
        self.device_ad.droid.bluetoothHidDeviceReportError(host_id, 1)

        self.log.info("Device: reply report")
        self.device_ad.droid.bluetoothHidDeviceReplyReport(
                host_id, 1, 1, hid_keyboard_report("04"))

        self.log.info("Device bonded: {}".format(
                      self.device_ad.droid.bluetoothGetBondedDevices()))
        self.log.info("Host bonded: {}".format(
                      self.host_ad.droid.bluetoothGetBondedDevices()))

        return test_result

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5ddc3eb1-2b8d-43b5-bdc4-ba577d90481d')
    def test_hid_host_unplug(self):
        """Test HID Host Virtual_cable_unplug

        Test the HID host and HID device handle Virtual_cable_unplug correctly

        Steps:
        1. Bluetooth HID device registers the Bluetooth input device service.
        2. Get the MAC address of the HID host and HID device.
        3. Establish HID profile connection from the HID host to the HID device.
        4. HID host sends virtual_cable_unplug command to the HID device.

        Expected Result:
        HID profile connection is successfully established; After the HID host
        sends virtual_cable_unplug command to the HID device, both disconnect
        each other, but not unpair.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, HID
        Priority: 2
        """

        test_result = True
        pair_pri_to_sec(self.host_ad, self.device_ad, attempts=3)

        self.log.info("Device bonded: {}".format(
                      self.device_ad.droid.bluetoothGetBondedDevices()))
        self.log.info("Host bonded: {}".format(
                      self.host_ad.droid.bluetoothGetBondedDevices()))

        host_id = self.host_ad.droid.bluetoothGetLocalAddress()
        device_id = self.device_ad.droid.bluetoothGetLocalAddress()

        self.host_ad.droid.bluetoothConnectBonded(device_id)

        time.sleep(hid_connection_timeout)
        self.log.info("Device connected: {}".format(
                self.device_ad.droid.bluetoothHidDeviceGetConnectedDevices()))

        self.log.info("Device: send data report through interrupt channel")
        hid_device_send_key_data_report(host_id, self.device_ad, "04")
        hid_device_send_key_data_report(host_id, self.device_ad, "05")

        self.log.info("Host: virtual unplug")
        self.host_ad.droid.bluetoothHidVirtualUnplug(device_id)

        try:
            hid_device_callback = self.device_ad.ed.pop_event(
                    bt_constants.hid_on_virtual_cable_unplug_event,
                    bt_constants.hid_default_event_timeout)
        except Empty as err:
            self.log.error("Callback not received: {}".format(err))
            test_result = False

        self.log.info("Device bonded: {}".format(
                self.device_ad.droid.bluetoothGetBondedDevices()))
        self.log.info("Host bonded: {}".format(
                self.host_ad.droid.bluetoothGetBondedDevices()))

        if self.device_ad.droid.bluetoothGetBondedDevices():
            self.log.error("HID device didn't unbond on virtual_cable_unplug")
            test_result = False

        if self.host_ad.droid.bluetoothGetBondedDevices():
            self.log.error("HID host didn't unbond on virtual_cable_unplug")
            test_result = False

        return test_result

