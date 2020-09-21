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
This test script exercises different Bluetooth 5 specific scan scenarios.
It is expected that the second AndroidDevice is able to advertise.

This test script was designed with this setup in mind:
Shield box one: Android Device, Android Device
"""

from queue import Empty

from acts.asserts import assert_equal
from acts.asserts import assert_true
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_test_utils import advertising_set_started
from acts.test_utils.bt.bt_test_utils import advertising_set_stopped
from acts.test_utils.bt.bt_test_utils import advertising_set_enabled
from acts.test_utils.bt.bt_test_utils import advertising_set_data_set
from acts.test_utils.bt.bt_test_utils import advertising_set_scan_response_set
from acts.test_utils.bt.bt_test_utils import advertising_set_parameters_update
from acts.test_utils.bt.bt_test_utils import advertising_set_periodic_parameters_updated
from acts.test_utils.bt.bt_test_utils import advertising_set_periodic_data_set
from acts.test_utils.bt.bt_test_utils import advertising_set_periodic_enable
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts import signals


class AdvertisingSetTest(BluetoothBaseTest):
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
        self.adv_ad = self.android_devices[0]

    def setup_class(self):
        if not self.adv_ad.droid.bluetoothIsLeExtendedAdvertisingSupported():
            raise signals.TestSkipClass(
                "Advertiser does not support LE Extended Advertising")

    def teardown_test(self):
        self.active_scan_callback_list = []

    def on_exception(self, test_name, begin_time):
        reset_bluetooth(self.android_devices)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='58182f7e-443f-47fb-b718-1be3ac850287')
    def test_reenabling(self):
        """Test re-enabling an Advertising Set

        Test GATT notify characteristic value.

        Steps:
        1. Start an advertising set that only lasts for a few seconds
        2. Restart advertising set
        3. Repeat steps 1-2

        Expected Result:
        Verify that advertising set re-enables

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, GATT, Characteristic
        Priority: 1
        """
        adv_callback = self.adv_ad.droid.bleAdvSetGenCallback()
        duration = 0
        max_ext_adv_events = 0
        self.adv_ad.droid.bleAdvSetStartAdvertisingSet({
            "connectable": False,
            "legacyMode": False,
            "primaryPhy": "PHY_LE_1M",
            "secondaryPhy": "PHY_LE_1M",
            "interval": 320
        }, self.big_adv_data, None, None, None, duration, max_ext_adv_events,
                                                       adv_callback)

        set_started_evt = self.adv_ad.ed.pop_event(
            advertising_set_started.format(adv_callback), self.default_timeout)
        set_id = set_started_evt['data']['setId']
        assert_equal(0, set_started_evt['data']['status'])
        assert_equal(0, set_started_evt['data']['status'])

        self.log.info("Successfully started set " + str(set_id))

        self.adv_ad.droid.bleAdvSetEnableAdvertising(set_id, False, duration,
                                                     max_ext_adv_events)
        enable_evt = self.adv_ad.ed.pop_event(
            advertising_set_enabled.format(adv_callback), self.default_timeout)
        assert_equal(set_id, enable_evt['data']['setId'])
        assert_equal(False, enable_evt['data']['enable'])
        assert_equal(0, enable_evt['data']['status'])
        self.log.info("Successfully disabled advertising set " + str(set_id))

        self.log.info("Enabling advertising for 2 seconds... ")
        duration_200 = 200
        self.adv_ad.droid.bleAdvSetEnableAdvertising(
            set_id, True, duration_200, max_ext_adv_events)
        enable_evt = self.adv_ad.ed.pop_event(
            advertising_set_enabled.format(adv_callback), self.default_timeout)
        assert_equal(set_id, enable_evt['data']['setId'])
        assert_equal(True, enable_evt['data']['enable'])
        assert_equal(0, enable_evt['data']['status'])
        self.log.info("Enabled. Waiting for disable event ~2s ...")

        enable_evt = self.adv_ad.ed.pop_event(
            advertising_set_enabled.format(adv_callback), self.default_timeout)
        assert_equal(set_id, enable_evt['data']['setId'])
        assert_equal(False, enable_evt['data']['enable'])
        assert_equal(0, enable_evt['data']['status'])
        self.log.info("Disable event received. Now trying to set data...")

        self.adv_ad.droid.bleAdvSetSetAdvertisingData(
            set_id,
            {"manufacturerData": [0x0123, "00112233445566778899AABBCCDDEE"]})
        data_set_evt = self.adv_ad.ed.pop_event(
            advertising_set_data_set.format(adv_callback),
            self.default_timeout)
        assert_equal(set_id, data_set_evt['data']['setId'])
        assert_equal(0, data_set_evt['data']['status'])
        self.log.info("Data changed successfully.")

        max_len = self.adv_ad.droid.bluetoothGetLeMaximumAdvertisingDataLength(
        )

        self.log.info("Will try to set data to maximum possible length")
        data_len = max_len - 4
        test_fill = '01' * data_len
        self.adv_ad.droid.bleAdvSetSetAdvertisingData(
            set_id, {"manufacturerData": [0x0123, test_fill]})
        data_set_evt = self.adv_ad.ed.pop_event(
            advertising_set_data_set.format(adv_callback),
            self.default_timeout)
        assert_equal(set_id, data_set_evt['data']['setId'])
        assert_equal(0, data_set_evt['data']['status'])
        self.log.info("Data changed successfully.")

        if max_len < 1650:
            self.log.info("Will try to set data to more than maximum length")
            data_len = max_len - 4 + 1
            test_fill = '01' * data_len
            self.adv_ad.droid.bleAdvSetSetAdvertisingData(
                set_id, {"manufacturerData": [0x0123, test_fill]})
            data_set_evt = self.adv_ad.ed.pop_event(
                advertising_set_data_set.format(adv_callback),
                self.default_timeout)
            assert_equal(set_id, data_set_evt['data']['setId'])
            #TODO(jpawlowski): make nicer error fot this case
            assert_true(data_set_evt['data']['status'] != 0,
                        "Setting data should fail because data too long.")

            self.log.info("Data change failed as expected.")

        self.adv_ad.droid.bleAdvSetStopAdvertisingSet(adv_callback)
        try:
            self.adv_ad.ed.pop_event(
                advertising_set_stopped.format(adv_callback),
                self.default_timeout)
        except Empty:
            self.log.error("Failed to find advertising set stopped event.")
            return False
        return True
