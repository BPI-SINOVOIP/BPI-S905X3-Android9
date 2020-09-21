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
Ble libraries
"""

from acts.test_utils.bt.bt_constants import ble_advertise_settings_modes
from acts.test_utils.bt.bt_constants import ble_advertise_settings_tx_powers
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_constants import small_timeout
from acts.test_utils.bt.bt_constants import adv_fail
from acts.test_utils.bt.bt_constants import adv_succ
from acts.test_utils.bt.bt_constants import advertising_set_on_own_address_read
from acts.test_utils.bt.bt_constants import advertising_set_started
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects

import time
import os


class BleLib():
    def __init__(self, log, dut):
        self.advertisement_list = []
        self.dut = dut
        self.log = log
        self.default_timeout = 5
        self.set_advertisement_list = []
        self.generic_uuid = "0000{}-0000-1000-8000-00805f9b34fb"

    def _verify_ble_adv_started(self, advertise_callback):
        """Helper for verifying if an advertisment started or not"""
        regex = "({}|{})".format(
            adv_succ.format(advertise_callback),
            adv_fail.format(advertise_callback))
        try:
            event = self.dut.ed.pop_events(regex, 5, small_timeout)
        except Empty:
            self.dut.log.error("Failed to get success or failed event.")
            return
        if event[0]["name"] == adv_succ.format(advertise_callback):
            self.dut.log.info("Advertisement started successfully.")
            return True
        else:
            self.dut.log.info("Advertisement failed to start.")
            return False

    def start_generic_connectable_advertisement(self, line):
        """Start a connectable LE advertisement"""
        scan_response = None
        if line:
            scan_response = bool(line)
        self.dut.droid.bleSetAdvertiseSettingsAdvertiseMode(
            ble_advertise_settings_modes['low_latency'])
        self.dut.droid.bleSetAdvertiseSettingsIsConnectable(True)
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.dut.droid))
        if scan_response:
            self.dut.droid.bleStartBleAdvertisingWithScanResponse(
                advertise_callback, advertise_data, advertise_settings,
                advertise_data)
        else:
            self.dut.droid.bleStartBleAdvertising(
                advertise_callback, advertise_data, advertise_settings)
        if self._verify_ble_adv_started(advertise_callback):
            self.log.info(
                "Tracking Callback ID: {}".format(advertise_callback))
            self.advertisement_list.append(advertise_callback)
            self.log.info(self.advertisement_list)

    def start_connectable_advertisement_set(self, line):
        """Start Connectable Advertisement Set"""
        adv_callback = self.dut.droid.bleAdvSetGenCallback()
        adv_data = {
            "includeDeviceName": True,
        }
        self.dut.droid.bleAdvSetStartAdvertisingSet(
            {
                "connectable": True,
                "legacyMode": False,
                "primaryPhy": "PHY_LE_1M",
                "secondaryPhy": "PHY_LE_1M",
                "interval": 320
            }, adv_data, None, None, None, 0, 0, adv_callback)
        evt = self.dut.ed.pop_event(
            advertising_set_started.format(adv_callback), self.default_timeout)
        set_id = evt['data']['setId']
        self.log.error("did not receive the set started event!")
        evt = self.dut.ed.pop_event(
            advertising_set_on_own_address_read.format(set_id),
            self.default_timeout)
        address = evt['data']['address']
        self.log.info("Advertiser address is: {}".format(str(address)))
        self.set_advertisement_list.append(adv_callback)

    def stop_all_advertisement_set(self, line):
        """Stop all Advertisement Sets"""
        for adv in self.set_advertisement_list:
            try:
                self.dut.droid.bleAdvSetStopAdvertisingSet(adv)
            except Exception as err:
                self.log.error("Failed to stop advertisement: {}".format(err))

    def adv_add_service_uuid_list(self, line):
        """Add service UUID to the LE advertisement inputs:
         [uuid1 uuid2 ... uuidN]"""
        uuids = line.split()
        uuid_list = []
        for uuid in uuids:
            if len(uuid) == 4:
                uuid = self.generic_uuid.format(line)
            uuid_list.append(uuid)
        self.dut.droid.bleSetAdvertiseDataSetServiceUuids(uuid_list)

    def adv_data_include_local_name(self, is_included):
        """Include local name in the advertisement. inputs: [true|false]"""
        self.dut.droid.bleSetAdvertiseDataIncludeDeviceName(bool(is_included))

    def adv_data_include_tx_power_level(self, is_included):
        """Include tx power level in the advertisement. inputs: [true|false]"""
        self.dut.droid.bleSetAdvertiseDataIncludeTxPowerLevel(
            bool(is_included))

    def adv_data_add_manufacturer_data(self, line):
        """Include manufacturer id and data to the advertisment:
        [id data1 data2 ... dataN]"""
        info = line.split()
        manu_id = int(info[0])
        manu_data = []
        for data in info[1:]:
            manu_data.append(int(data))
        self.dut.droid.bleAddAdvertiseDataManufacturerId(manu_id, manu_data)

    def start_generic_nonconnectable_advertisement(self, line):
        """Start a nonconnectable LE advertisement"""
        self.dut.droid.bleSetAdvertiseSettingsAdvertiseMode(
            ble_advertise_settings_modes['low_latency'])
        self.dut.droid.bleSetAdvertiseSettingsIsConnectable(False)
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.dut.droid))
        self.dut.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        if self._verify_ble_adv_started(advertise_callback):
            self.log.info(
                "Tracking Callback ID: {}".format(advertise_callback))
            self.advertisement_list.append(advertise_callback)
            self.log.info(self.advertisement_list)

    def stop_all_advertisements(self, line):
        """Stop all LE advertisements"""
        for callback_id in self.advertisement_list:
            self.log.info("Stopping Advertisement {}".format(callback_id))
            self.dut.droid.bleStopBleAdvertising(callback_id)
            time.sleep(1)
        self.advertisement_list = []

    def ble_stop_advertisement(self, callback_id):
        """Stop an LE advertisement"""
        if not callback_id:
            self.log.info("Need a callback ID")
            return
        callback_id = int(callback_id)
        if callback_id not in self.advertisement_list:
            self.log.info("Callback not in list of advertisements.")
            return
        self.dut.droid.bleStopBleAdvertising(callback_id)
        self.advertisement_list.remove(callback_id)

    def start_max_advertisements(self, line):
        scan_response = None
        if line:
            scan_response = bool(line)
        while (True):
            try:
                self.dut.droid.bleSetAdvertiseSettingsAdvertiseMode(
                    ble_advertise_settings_modes['low_latency'])
                self.dut.droid.bleSetAdvertiseSettingsIsConnectable(True)
                advertise_callback, advertise_data, advertise_settings = (
                    generate_ble_advertise_objects(self.dut.droid))
                if scan_response:
                    self.dut.droid.bleStartBleAdvertisingWithScanResponse(
                        advertise_callback, advertise_data, advertise_settings,
                        advertise_data)
                else:
                    self.dut.droid.bleStartBleAdvertising(
                        advertise_callback, advertise_data, advertise_settings)
                if self._verify_ble_adv_started(advertise_callback):
                    self.log.info(
                        "Tracking Callback ID: {}".format(advertise_callback))
                    self.advertisement_list.append(advertise_callback)
                    self.log.info(self.advertisement_list)
                else:
                    self.log.info("Advertisements active: {}".format(
                        len(self.advertisement_list)))
                    return False
            except Exception as err:
                self.log.info("Advertisements active: {}".format(
                    len(self.advertisement_list)))
                return True
