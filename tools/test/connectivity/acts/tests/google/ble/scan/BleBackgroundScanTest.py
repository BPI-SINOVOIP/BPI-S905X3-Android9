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
This test script exercises background scan test scenarios.
"""

from queue import Empty

from acts import utils
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import bluetooth_off
from acts.test_utils.bt.bt_test_utils import bluetooth_on
from acts.test_utils.bt.bt_test_utils import cleanup_scanners_and_advertisers
from acts.test_utils.bt.bt_test_utils import enable_bluetooth
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.test_utils.bt.bt_constants import bluetooth_le_off
from acts.test_utils.bt.bt_constants import bluetooth_le_on
from acts.test_utils.bt.bt_constants import bt_adapter_states
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_constants import scan_result

import time


class BleBackgroundScanTest(BluetoothBaseTest):
    default_timeout = 10
    report_delay = 2000
    scan_callbacks = []
    adv_callbacks = []
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

    def setup_test(self):
        # Always start tests with Bluetooth enabled and BLE disabled.
        enable_bluetooth(self.scn_ad.droid, self.scn_ad.ed)
        self.scn_ad.droid.bluetoothDisableBLE()
        for a in self.android_devices:
            a.ed.clear_all_events()
        return True

    def teardown_test(self):
        cleanup_scanners_and_advertisers(
            self.scn_ad, self.active_adv_callback_list, self.adv_ad,
            self.active_adv_callback_list)
        self.active_adv_callback_list = []
        self.active_scan_callback_list = []

    def _setup_generic_advertisement(self):
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        adv_callback, adv_data, adv_settings = generate_ble_advertise_objects(
            self.adv_ad.droid)
        self.adv_ad.droid.bleStartBleAdvertising(adv_callback, adv_data,
                                                 adv_settings)
        self.active_adv_callback_list.append(adv_callback)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4d13c3a8-1805-44ef-a92a-e385540767f1')
    def test_background_scan(self):
        """Test generic background scan.

        Tests LE background scan. The goal is to find scan results even though
        Bluetooth is turned off.

        Steps:
        1. Setup an advertisement on dut1
        2. Enable LE on the Bluetooth Adapter on dut0
        3. Toggle BT off on dut1
        4. Start a LE scan on dut0
        5. Find the advertisement from dut1

        Expected Result:
        Find a advertisement from the scan instance.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Background Scanning
        Priority: 0
        """
        self.scn_ad.droid.bluetoothEnableBLE()
        self._setup_generic_advertisement()
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ble_scan_settings_modes['low_latency'])
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bluetoothToggleState(False)
        try:
            self.scn_ad.ed.pop_event(bluetooth_off, self.default_timeout)
        except Empty:
            self.log.error("Bluetooth Off event not found. Expected {}".format(
                bluetooth_off))
            return False
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        expected_event = scan_result.format(scan_callback)
        try:
            self.scn_ad.ed.pop_event(expected_event, self.default_timeout)
        except Empty:
            self.log.error("Scan Result event not found. Expected {}".format(
                expected_event))
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9c4577f8-5e06-4034-b977-285956734974')
    def test_background_scan_ble_disabled(self):
        """Test background LE scanning with LE disabled.

        Tests LE background scan. The goal is to find scan results even though
        Bluetooth is turned off.

        Steps:
        1. Setup an advertisement on dut1
        2. Enable LE on the Bluetooth Adapter on dut0
        3. Toggle BT off on dut1
        4. Start a LE scan on dut0
        5. Find the advertisement from dut1

        Expected Result:
        Find a advertisement from the scan instance.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Background Scanning
        Priority: 0
        """
        self._setup_generic_advertisement()
        self.scn_ad.droid.bluetoothEnableBLE()
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ble_scan_settings_modes['low_latency'])
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bluetoothToggleState(False)
        try:
            self.scn_ad.ed.pop_event(bluetooth_off, self.default_timeout)
        except Empty:
            self.log.info(self.scn_ad.droid.bluetoothCheckState())
            self.log.error("Bluetooth Off event not found. Expected {}".format(
                bluetooth_off))
            return False
        try:
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                              scan_callback)
            expected_event = scan_result.format(scan_callback)
            try:
                self.scn_ad.ed.pop_event(expected_event, self.default_timeout)
            except Empty:
                self.log.error(
                    "Scan Result event not found. Expected {}".format(
                        expected_event))
                return False
        except Exception:
            self.log.info(
                "Was not able to start a background scan as expected.")
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0bdd1764-3dc6-4a82-b041-76e48ed0f424')
    def test_airplane_mode_disables_ble(self):
        """Try to start LE mode in Airplane Mode.

        This test will enable airplane mode, then attempt to start LE scanning
        mode.  This should result in bluetooth still being turned off, LE
        not enabled.

        Steps:
        1. Start LE only mode.
        2. Bluetooth should be in LE ONLY mode
        2. Turn on airplane mode.
        3. Bluetooth should be OFF
        4. Try to start LE only mode.
        5. Bluetooth should stay in OFF mode (LE only start should fail)
        6. Turn off airplane mode.
        7. Bluetooth should be OFF.

        Expected Result:
        No unexpected bluetooth state changes.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Airplane
        Priority: 1
        """
        ble_state_error_msg = "Bluetooth LE State not OK {}. Expected {} got {}"
        # Enable BLE always available (effectively enabling BT in location)
        self.scn_ad.shell.enable_ble_scanning()
        self.scn_ad.droid.bluetoothEnableBLE()
        self.scn_ad.droid.bluetoothToggleState(False)
        try:
            self.scn_ad.ed.pop_event(bluetooth_off, self.default_timeout)
        except Empty:
            self.log.error("Bluetooth Off event not found. Expected {}".format(
                bluetooth_off))
            self.log.info(self.scn_ad.droid.bluetoothCheckState())
            return False

        # Sleep because LE turns off after the bluetooth off event fires
        time.sleep(self.default_timeout)
        state = self.scn_ad.droid.bluetoothGetLeState()
        if state != bt_adapter_states['ble_on']:
            self.log.error(
                ble_state_error_msg.format("after BT Disable",
                                           bt_adapter_states['ble_on'], state))
            return False

        self.scn_ad.droid.bluetoothListenForBleStateChange()
        self.scn_ad.droid.connectivityToggleAirplaneMode(True)
        try:
            self.scn_ad.ed.pop_event(bluetooth_le_off, self.default_timeout)
        except Empty:
            self.log.error(
                "Bluetooth LE Off event not found. Expected {}".format(
                    bluetooth_le_off))
            return False
        state = self.scn_ad.droid.bluetoothGetLeState()
        if state != bt_adapter_states['off']:
            self.log.error(
                ble_state_error_msg.format("after Airplane Mode ON",
                                           bt_adapter_states['off'], state))
            return False
        result = self.scn_ad.droid.bluetoothEnableBLE()
        if result:
            self.log.error(
                "Bluetooth Enable command succeded when it should have failed (in airplane mode)"
            )
            return False
        state = self.scn_ad.droid.bluetoothGetLeState()
        if state != bt_adapter_states['off']:
            self.log.error(
                "Bluetooth LE State not OK after attempted enable. Expected {} got {}".
                format(bt_adapter_states['off'], state))
            return False
        self.scn_ad.droid.connectivityToggleAirplaneMode(False)
        # Sleep to let Airplane Mode disable propogate through the system
        time.sleep(self.default_timeout)
        state = self.scn_ad.droid.bluetoothGetLeState()
        if state != bt_adapter_states['off']:
            self.log.error(
                ble_state_error_msg.format("after Airplane Mode OFF",
                                           bt_adapter_states['off'], state))
            return False
        return True
