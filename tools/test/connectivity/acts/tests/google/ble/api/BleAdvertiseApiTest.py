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
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See thea
# License for the specific language governing permissions and limitations under
# the License.
"""
Test script to exercise Ble Advertisement Api's. This exercises all getters and
setters. This is important since there is a builder object that is immutable
after you set all attributes of each object. If this test suite doesn't pass,
then other test suites utilising Ble Advertisements will also fail.
"""

from acts.controllers.sl4a_lib import rpc_client
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import adv_fail
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_constants import ble_advertise_settings_modes
from acts.test_utils.bt.bt_constants import ble_advertise_settings_tx_powers
from acts.test_utils.bt.bt_constants import java_integer


class BleAdvertiseVerificationError(Exception):
    """Error in fetsching BleScanner Advertise result."""


class BleAdvertiseApiTest(BluetoothBaseTest):
    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.ad_dut = self.android_devices[0]

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d6d8d0a6-7b3e-4e4b-a5d0-bcfd6e207474')
    def test_adv_settings_defaults(self):
        """Tests the default advertisement settings.

        This builder object should have a proper "get" expectation for each
        attribute of the builder object once it's built.

        Steps:
        1. Build a new advertise settings object.
        2. Get the attributes of the advertise settings object.
        3. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 0
        """
        test_result = True
        droid = self.ad_dut.droid
        adv_settings = droid.bleBuildAdvertiseSettings()
        adv_mode = droid.bleGetAdvertiseSettingsMode(adv_settings)
        tx_power_level = droid.bleGetAdvertiseSettingsTxPowerLevel(
            adv_settings)
        is_connectable = droid.bleGetAdvertiseSettingsIsConnectable(
            adv_settings)

        exp_adv_mode = ble_advertise_settings_modes['low_power']
        exp_tx_power_level = ble_advertise_settings_tx_powers['medium']
        exp_is_connectable = True
        if adv_mode != exp_adv_mode:
            test_result = False
            self.log.debug("exp filtering mode: {},"
                           " found filtering mode: {}".format(
                               exp_adv_mode, adv_mode))
        if tx_power_level != exp_tx_power_level:
            test_result = False
            self.log.debug("exp tx power level: {},"
                           " found filtering tx power level: {}".format(
                               exp_tx_power_level, tx_power_level))
        if exp_is_connectable != is_connectable:
            test_result = False
            self.log.debug("exp is connectable: {},"
                           " found filtering is connectable: {}".format(
                               exp_is_connectable, is_connectable))
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f2a276ae-1436-43e4-aba7-1ede787200ee')
    def test_adv_data_defaults(self):
        """Tests the default advertisement data.

        This builder object should have a proper "get" expectation for each
        attribute of the builder object once it's built.

        Steps:
        1. Build a new AdvertiseData object.
        2. Get the attributes of the advertise settings object.
        3. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 0
        """
        test_result = True
        droid = self.ad_dut.droid
        adv_data = droid.bleBuildAdvertiseData()
        service_uuids = droid.bleGetAdvertiseDataServiceUuids(adv_data)
        include_tx_power_level = droid.bleGetAdvertiseDataIncludeTxPowerLevel(
            adv_data)
        include_device_name = droid.bleGetAdvertiseDataIncludeDeviceName(
            adv_data)

        exp_service_uuids = []
        exp_include_tx_power_level = False
        exp_include_device_name = False
        self.log.debug("Step 4: Verify all defaults match exp values.")
        if service_uuids != exp_service_uuids:
            test_result = False
            self.log.debug("exp filtering service uuids: {},"
                           " found filtering service uuids: {}".format(
                               exp_service_uuids, service_uuids))
        if include_tx_power_level != exp_include_tx_power_level:
            test_result = False
            self.log.debug(
                "exp filtering include tx power level:: {},"
                " found filtering include tx power level: {}".format(
                    exp_include_tx_power_level, include_tx_power_level))
        if include_device_name != exp_include_device_name:
            test_result = False
            self.log.debug(
                "exp filtering include tx power level: {},"
                " found filtering include tx power level: {}".format(
                    exp_include_device_name, include_device_name))
        if not test_result:
            self.log.debug("Some values didn't match the defaults.")
        else:
            self.log.debug("All default values passed.")
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8d462e60-6b4e-49f3-9ef4-5a8b612d285d')
    def test_adv_settings_set_adv_mode_balanced(self):
        """Tests advertise settings balanced mode.

        This advertisement settings from "set" advertisement mode should match
        the corresponding "get" function.

        Steps:
        1. Build a new advertise settings object.
        2. Set the advertise mode attribute to balanced.
        3. Get the attributes of the advertise settings object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_adv_mode = ble_advertise_settings_modes['balanced']
        self.log.debug(
            "Step 2: Set the filtering settings object's value to {}".format(
                exp_adv_mode))
        return self.verify_adv_settings_adv_mode(droid, exp_adv_mode)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='334fefeb-365f-4ee3-9be0-42b1fabe3178')
    def test_adv_settings_set_adv_mode_low_power(self):
        """Tests advertise settings low power mode.

        This advertisement settings from "set" advertisement mode should match
        the corresponding "get" function.

        Steps:
        1. Build a new advertise settings object.
        2. Set the advertise mode attribute to low power mode.
        3. Get the attributes of the advertise settings object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_adv_mode = ble_advertise_settings_modes['low_power']
        self.log.debug(
            "Step 2: Set the filtering settings object's value to {}".format(
                exp_adv_mode))
        return self.verify_adv_settings_adv_mode(droid, exp_adv_mode)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ce087782-1535-4694-944a-e962c22638ed')
    def test_adv_settings_set_adv_mode_low_latency(self):
        """Tests advertise settings low latency mode.

        This advertisement settings from "set" advertisement mode should match
        the corresponding "get" function.

        Steps:
        1. Build a new advertise settings object.
        2. Set the advertise mode attribute to low latency mode.
        3. Get the attributes of the advertise settings object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_adv_mode = ble_advertise_settings_modes['low_latency']
        self.log.debug(
            "Step 2: Set the filtering settings object's value to {}".format(
                exp_adv_mode))
        return self.verify_adv_settings_adv_mode(droid, exp_adv_mode)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='59b52be9-d38b-4814-af08-c68aa8910a16')
    def test_adv_settings_set_invalid_adv_mode(self):
        """Tests advertise settings invalid advertising mode.

        This advertisement settings from "set" advertisement mode should fail
        when setting an invalid advertisement.

        Steps:
        1. Build a new advertise settings object.
        2. Set the advertise mode attribute to -1.

        Expected Result:
        Building the advertise settings should fail.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 2
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_adv_mode = -1
        self.log.debug("Step 2: Set the filtering mode to -1")
        return self.verify_invalid_adv_settings_adv_mode(droid, exp_adv_mode)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d8292633-831f-41c4-974a-ad267e9795e9')
    def test_adv_settings_set_adv_tx_power_level_high(self):
        """Tests advertise settings tx power level high.

        This advertisement settings from "set" advertisement tx power level
        should match the corresponding "get" function.

        Steps:
        1. Build a new advertise settings object.
        2. Set the advertise mode attribute to tx power level high.
        3. Get the attributes of the advertise settings object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_adv_tx_power = ble_advertise_settings_tx_powers['high']
        self.log.debug(
            "Step 2: Set the filtering settings object's value to {}".format(
                exp_adv_tx_power))
        return self.verify_adv_settings_tx_power_level(droid, exp_adv_tx_power)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d577de1f-4fd9-43d5-beff-c0696c5e0ea1')
    def test_adv_settings_set_adv_tx_power_level_medium(self):
        """Tests advertise settings tx power level medium.

        This advertisement settings from "set" advertisement tx power level
        should match the corresponding "get" function.

        Steps:
        1. Build a new advertise settings object.
        2. Set the advertise mode attribute to tx power level medium.
        3. Get the attributes of the advertise settings object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        test_result = True
        droid = self.ad_dut.droid
        exp_adv_tx_power = ble_advertise_settings_tx_powers['medium']
        self.log.debug(
            "Step 2: Set the filtering settings object's value to {}".format(
                exp_adv_tx_power))
        return self.verify_adv_settings_tx_power_level(droid, exp_adv_tx_power)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e972f8b5-a3cf-4b3f-9223-a8a74c0fd855')
    def test_adv_settings_set_adv_tx_power_level_low(self):
        """Tests advertise settings tx power level low.

        This advertisement settings from "set" advertisement tx power level
        should match the corresponding "get" function.

        Steps:
        1. Build a new advertise settings object.
        2. Set the advertise mode attribute to tx power level low.
        3. Get the attributes of the advertise settings object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_adv_tx_power = (ble_advertise_settings_tx_powers['low'])
        self.log.debug(
            "Step 2: Set the filtering settings object's value to ".format(
                exp_adv_tx_power))
        return self.verify_adv_settings_tx_power_level(droid, exp_adv_tx_power)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e972f8b5-a3cf-4b3f-9223-a8a74c0fd855')
    def test_adv_settings_set_adv_tx_power_level_ultra_low(self):
        """Tests advertise settings tx power level ultra low.

        This advertisement settings from "set" advertisement tx power level
        should match the corresponding "get" function.

        Steps:
        1. Build a new advertise settings object.
        2. Set the advertise mode attribute to tx power level ultra low.
        3. Get the attributes of the advertise settings object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_adv_tx_power = ble_advertise_settings_tx_powers['ultra_low']
        self.log.debug(
            "Step 2: Set the filtering settings object's value to ".format(
                exp_adv_tx_power))
        return self.verify_adv_settings_tx_power_level(droid, exp_adv_tx_power)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e972f8b5-a3cf-4b3f-9223-a8a74c0fd855')
    def test_adv_settings_set_invalid_adv_tx_power_level(self):
        """Tests advertise settings invalid advertising tx power level.

        This advertisement settings from "set" advertisement mode should fail
        when setting an invalid advertisement.

        Steps:
        1. Build a new advertise settings object.
        2. Set the advertise tx power level attribute to -1.

        Expected Result:
        Building the advertise settings should fail.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_adv_tx_power = -1
        self.log.debug("Step 2: Set the filtering mode to -1")
        return self.verify_invalid_adv_settings_tx_power_level(
            droid, exp_adv_tx_power)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1be71f77-64af-42b7-84cb-e06df0836966')
    def test_adv_settings_set_is_connectable_true(self):
        """Tests advertise settings is connectable true.

        This advertisement settings from "set" advertisement tx power level
        should match the corresponding "get" function.

        Steps:
        1. Build a new advertise settings object.
        2. Set the advertise is connectable to true.
        3. Get the attributes of the advertise settings object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_is_connectable = True
        self.log.debug(
            "Step 2: Set the filtering settings object's value to {}".format(
                exp_is_connectable))
        return self.verify_adv_settings_is_connectable(droid,
                                                       exp_is_connectable)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f9865333-9198-4385-938d-5fc641ee9968')
    def test_adv_settings_set_is_connectable_false(self):
        """Tests advertise settings is connectable false.

        This advertisement settings from "set" advertisement tx power level
        should match the corresponding "get" function.

        Steps:
        1. Build a new advertise settings object.
        2. Set the advertise is connectable to false.
        3. Get the attributes of the advertise settings object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_is_connectable = False
        self.log.debug("Step 2: Set the filtering settings object's value to "
                       + str(exp_is_connectable))
        return self.verify_adv_settings_is_connectable(droid,
                                                       exp_is_connectable)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a770ed7e-c6cd-4533-8876-e42e68f8b4fb')
    def test_adv_data_set_service_uuids_empty(self):
        """Tests advertisement data's service uuids to empty.

        This advertisement data from "set" advertisement service uuid
        should match the corresponding "get" function.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's service uuid to empty.
        3. Get the attributes of the AdvertiseData object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_service_uuids = []
        self.log.debug("Step 2: Set the filtering data object's value to " +
                       str(exp_service_uuids))
        return self.verify_adv_data_service_uuids(droid, exp_service_uuids)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='3da511db-d024-45c8-be3c-fe8e123129fa')
    def test_adv_data_set_service_uuids_single(self):
        """Tests advertisement data's service uuids to empty.

        This advertisement data from "set" advertisement service uuid
        should match the corresponding "get" function.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's service uuid to empty.
        3. Get the attributes of the AdvertiseData object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_service_uuids = ["00000000-0000-1000-8000-00805f9b34fb"]
        self.log.debug("Step 2: Set the filtering data object's value to " +
                       str(exp_service_uuids))
        return self.verify_adv_data_service_uuids(droid, exp_service_uuids)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='15073359-d607-4a76-b60a-00a4b34f9a85')
    def test_adv_data_set_service_uuids_multiple(self):
        """Tests advertisement data's service uuids to multiple uuids.

        This advertisement data from "set" advertisement service uuid
        should match the corresponding "get" function.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's service uuid to multiple uuids.
        3. Get the attributes of the AdvertiseData object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_service_uuids = [
            "00000000-0000-1000-8000-00805f9b34fb",
            "00000000-0000-1000-8000-00805f9b34fb"
        ]
        self.log.debug("Step 2: Set the filtering data object's value to " +
                       str(exp_service_uuids))
        return self.verify_adv_data_service_uuids(droid, exp_service_uuids)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='af783a71-ef80-4974-a077-16a4ed8f0114')
    def test_adv_data_set_service_uuids_invalid_uuid(self):
        """Tests advertisement data's service uuids to an invalid uuid.

        This advertisement data from "set" advertisement service uuid
        should fail when there is an invalid service uuid.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's service uuid to an invalid uuid.

        Expected Result:
        Building the AdvertiseData should fail.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_service_uuids = ["0"]
        self.log.debug("Step 2: Set the filtering data service uuids to " +
                       str(exp_service_uuids))
        return self.verify_invalid_adv_data_service_uuids(
            droid, exp_service_uuids)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='51d634e7-6271-4cc0-a57b-3c1b632a7db6')
    def test_adv_data_set_service_data(self):
        """Tests advertisement data's service data.

        This advertisement data from "set" advertisement service data
        should match the corresponding "get" function.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's service data.
        3. Get the attributes of the AdvertiseData object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_service_data_uuid = "00000000-0000-1000-8000-00805f9b34fb"
        exp_service_data = [1, 2, 3]
        self.log.debug(
            "Step 2: Set the filtering data object's service data uuid to: {}, "
            "service data: {}".format(exp_service_data_uuid, exp_service_data))
        return self.verify_adv_data_service_data(droid, exp_service_data_uuid,
                                                 exp_service_data)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='aa18b0af-2a41-4b2a-af64-ea639961d561')
    def test_adv_data_set_service_data_invalid_service_data(self):
        """Tests advertisement data's invalid service data.

        This advertisement data from "set" advertisement service data
        should fail on an invalid value.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's service data to an invalid value.
        3. Get the attributes of the AdvertiseData object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_service_data_uuid = "00000000-0000-1000-8000-00805f9b34fb"
        exp_service_data = "helloworld"
        self.log.debug(
            "Step 2: Set the filtering data object's service data uuid to: {}, "
            "service data: {}".format(exp_service_data_uuid, exp_service_data))
        return self.verify_invalid_adv_data_service_data(
            droid, exp_service_data_uuid, exp_service_data)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='13a75a47-eff4-429f-a436-d244bbfe4496')
    def test_adv_data_set_service_data_invalid_service_data_uuid(self):
        """Tests advertisement data's invalid service data and uuid.

        This advertisement data from "set" advertisement service data
        should fail on an invalid value.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's service data and uuid to an invalid value.
        3. Get the attributes of the AdvertiseData object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_service_data_uuid = "0"
        exp_service_data = "1,2,3"
        self.log.debug(
            "Step 2: Set the filtering data object's service data uuid to: {}, "
            "service data: {}".format(exp_service_data_uuid, exp_service_data))
        return self.verify_invalid_adv_data_service_data(
            droid, exp_service_data_uuid, exp_service_data)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='386024e2-212e-4eed-8ef3-43d0c0239ea5')
    def test_adv_data_set_manu_id(self):
        """Tests advertisement data's manufacturers data and id.

        This advertisement data from "set" advertisement manufacturers data
        should match the corresponding "get" function.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's manufacturers data and id.
        3. Get the attributes of the AdvertiseData object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_manu_id = 0
        exp_manu_specific_data = [1, 2, 3]
        self.log.debug(
            "Step 2: Set the filtering data object's service data manu id: {}"
            ", manu specific data: {}".format(exp_manu_id,
                                              exp_manu_specific_data))
        return self.verify_adv_data_manu_id(droid, exp_manu_id,
                                            exp_manu_specific_data)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='650ceff2-2760-4a34-90fb-e637a2c5ebb5')
    def test_adv_data_set_manu_id_invalid_manu_id(self):
        """Tests advertisement data's manufacturers invalid id.

        This advertisement data from "set" advertisement manufacturers data
        should not be successful on an invalid id.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's manufacturers id to -1.
        3. Build the advertisement data.

        Expected Result:
        Building the advertisement data should fail.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_manu_id = -1
        exp_manu_specific_data = [1, 2, 3]
        self.log.debug(
            "Step 2: Set the filtering data object's service data manu id: {}"
            ", manu specific data: {}".format(exp_manu_id,
                                              exp_manu_specific_data))
        return self.verify_invalid_adv_data_manu_id(droid, exp_manu_id,
                                                    exp_manu_specific_data)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='27ccd7d7-9cd2-4e95-8198-ef6ca746d1cc')
    def test_adv_data_set_manu_id_invalid_manu_specific_data(self):
        """Tests advertisement data's manufacturers invalid specific data.

        This advertisement data from "set" advertisement manufacturers data
        should not be successful on an invalid specific data.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's manufacturers specific data to helloworld.
        3. Build the advertisement data.

        Expected Result:
        Building the advertisement data should fail.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_manu_id = 0
        exp_manu_specific_data = ['helloworld']
        self.log.debug(
            "Step 2: Set the filtering data object's service data manu id: {}"
            ", manu specific data: {}".format(exp_manu_id,
                                              exp_manu_specific_data))
        return self.verify_invalid_adv_data_manu_id(droid, exp_manu_id,
                                                    exp_manu_specific_data)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8e78d444-cd25-4c17-9532-53972a6f0ffe')
    def test_adv_data_set_manu_id_max(self):
        """Tests advertisement data's manufacturers id to the max size.

        This advertisement data from "set" advertisement manufacturers data
        should match the corresponding "get" function.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's manufacturers id to JavaInterger.MAX value.
        3. Get the attributes of the AdvertiseData object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 3
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_manu_id = java_integer['max']
        exp_manu_specific_data = [1, 2, 3]
        self.log.debug(
            "Step 2: Set the filtering data object's service data manu id: {}"
            ", manu specific data: {}".format(exp_manu_id,
                                              exp_manu_specific_data))
        return self.verify_adv_data_manu_id(droid, exp_manu_id,
                                            exp_manu_specific_data)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6c4866b7-ddf5-44ef-a231-0af683c6db80')
    def test_adv_data_set_include_tx_power_level_true(self):
        """Tests advertisement data's include tx power level to True.

        This advertisement data from "set" advertisement manufacturers data
        should match the corresponding "get" function.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's include tx power level to True.
        3. Get the attributes of the AdvertiseData object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_include_tx_power_level = True
        self.log.debug(
            "Step 2: Set the filtering data object's include tx power level: "
            "{}".format(exp_include_tx_power_level))
        return self.verify_adv_data_include_tx_power_level(
            droid, exp_include_tx_power_level)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='db06cc5f-60cf-4f04-b0fe-0c354f987541')
    def test_adv_data_set_include_tx_power_level_false(self):
        """Tests advertisement data's include tx power level to False.

        This advertisement data from "set" advertisement manufacturers data
        should match the corresponding "get" function.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's include tx power level to False.
        3. Get the attributes of the AdvertiseData object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_include_tx_power_level = False
        self.log.debug(
            "Step 2: Set the filtering data object's include tx power level: {}"
            .format(exp_include_tx_power_level))
        return self.verify_adv_data_include_tx_power_level(
            droid, exp_include_tx_power_level)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e99480c4-fd37-4791-a8d0-7eb8f8f72d62')
    def test_adv_data_set_include_device_name_true(self):
        """Tests advertisement data's include device name to True.

        This advertisement data from "set" advertisement manufacturers data
        should match the corresponding "get" function.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's include device name to True.
        3. Get the attributes of the AdvertiseData object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        droid = self.ad_dut.droid
        exp_include_device_name = True
        self.log.debug(
            "Step 2: Set the filtering data object's include device name: {}"
            .format(exp_include_device_name))
        return self.verify_adv_data_include_device_name(
            droid, exp_include_device_name)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b89ed642-c426-4777-8217-7bb8c2058592')
    def test_adv_data_set_include_device_name_false(self):
        """Tests advertisement data's include device name to False.

        This advertisement data from "set" advertisement manufacturers data
        should match the corresponding "get" function.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's include device name to False.
        3. Get the attributes of the AdvertiseData object.
        4. Compare the attributes found against the attributes exp.

        Expected Result:
        Found attributes should match expected attributes.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        self.log.debug("Step 1: Setup environment.")
        test_result = True
        droid = self.ad_dut.droid
        exp_include_device_name = False
        self.log.debug(
            "Step 2: Set the filtering data object's include device name: {}".
            format(exp_include_device_name))
        return self.verify_adv_data_include_device_name(
            droid, exp_include_device_name)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5033bcf5-a841-4b8b-af35-92c7237c7b36')
    def test_advertisement_greater_than_31_bytes(self):
        """Tests advertisement data's size to be greater than 31 bytes.

        This advertisement data from "set" advertisement manufacturers data
        should match the corresponding "get" function.

        Steps:
        1. Build a new AdvertiseData object.
        2. Set the AdvertiseData's size to be greater than 31 bytes
        3. Build the advertisement data.

        Expected Result:
        Api fails to build the AdvertiseData.

        Returns:
          True is pass
          False if fail

        TAGS: LE, Advertising
        Priority: 1
        """
        test_result = True
        droid = self.ad_dut.droid
        ed = self.android_devices[0].ed
        service_data = []
        for i in range(25):
            service_data.append(i)
        droid.bleAddAdvertiseDataServiceData(
            "0000110D-0000-1000-8000-00805F9B34FB", service_data)
        advcallback, adv_data, adv_settings = generate_ble_advertise_objects(
            droid)
        droid.bleStartBleAdvertising(advcallback, adv_data, adv_settings)
        try:
            ed.pop_event(adv_fail.format(advcallback))
        except rpc_client.Sl4aApiError:
            self.log.info("{} event was not found.".format(
                adv_fail.format(advcallback)))
            return False
        return test_result

    def verify_adv_settings_adv_mode(self, droid, exp_adv_mode):
        try:
            droid.bleSetAdvertiseSettingsAdvertiseMode(exp_adv_mode)
        except BleAdvertiseVerificationError as error:
            self.log.debug(str(error))
            return False
        self.log.debug("Step 3: Get a filtering settings object's index.")
        settings_index = droid.bleBuildAdvertiseSettings()
        self.log.debug("Step 4: Get the filtering setting's filtering mode.")
        adv_mode = droid.bleGetAdvertiseSettingsMode(settings_index)
        if exp_adv_mode is not adv_mode:
            self.log.debug("exp value: {}, Actual value: {}".format(
                exp_adv_mode, adv_mode))
            return False
        self.log.debug("Advertise Setting's filtering mode {} value "
                       "test Passed.".format(exp_adv_mode))
        return True

    def verify_adv_settings_tx_power_level(self, droid, exp_adv_tx_power):
        try:
            droid.bleSetAdvertiseSettingsTxPowerLevel(exp_adv_tx_power)
        except BleAdvertiseVerificationError as error:
            self.log.debug(str(error))
            return False
        self.log.debug("Step 3: Get a filtering settings object's index.")
        settings_index = droid.bleBuildAdvertiseSettings()
        self.log.debug("Step 4: Get the filtering setting's tx power level.")
        adv_tx_power_level = droid.bleGetAdvertiseSettingsTxPowerLevel(
            settings_index)
        if exp_adv_tx_power is not adv_tx_power_level:
            self.log.debug("exp value: {}, Actual value: {}".format(
                exp_adv_tx_power, adv_tx_power_level))
            return False
        self.log.debug("Advertise Setting's tx power level {}"
                       "  value test Passed.".format(exp_adv_tx_power))
        return True

    def verify_adv_settings_is_connectable(self, droid, exp_is_connectable):
        try:
            droid.bleSetAdvertiseSettingsIsConnectable(exp_is_connectable)
        except BleAdvertiseVerificationError as error:
            self.log.debug(str(error))
            return False
        self.log.debug("Step 3: Get a filtering settings object's index.")
        settings_index = droid.bleBuildAdvertiseSettings()
        self.log.debug(
            "Step 4: Get the filtering setting's is connectable value.")
        is_connectable = droid.bleGetAdvertiseSettingsIsConnectable(
            settings_index)
        if exp_is_connectable is not is_connectable:
            self.log.debug("exp value: {}, Actual value: {}".format(
                exp_is_connectable, is_connectable))
            return False
        self.log.debug("Advertise Setting's is connectable {}"
                       " value test Passed.".format(exp_is_connectable))
        return True

    def verify_adv_data_service_uuids(self, droid, exp_service_uuids):
        try:
            droid.bleSetAdvertiseDataSetServiceUuids(exp_service_uuids)
        except BleAdvertiseVerificationError as error:
            self.log.debug(str(error))
            return False
        self.log.debug("Step 3: Get a filtering data object's index.")
        data_index = droid.bleBuildAdvertiseData()
        self.log.debug("Step 4: Get the filtering data's service uuids.")
        service_uuids = droid.bleGetAdvertiseDataServiceUuids(data_index)
        if exp_service_uuids != service_uuids:
            self.log.debug("exp value: {}, Actual value: {}".format(
                exp_service_uuids, service_uuids))
            return False
        self.log.debug(
            "Advertise Data's service uuids {}, value test Passed.".format(
                exp_service_uuids))
        return True

    def verify_adv_data_service_data(self, droid, exp_service_data_uuid,
                                     exp_service_data):
        try:
            droid.bleAddAdvertiseDataServiceData(exp_service_data_uuid,
                                                 exp_service_data)
        except BleAdvertiseVerificationError as error:
            self.log.debug(str(error))
            return False
        self.log.debug("Step 3: Get a filtering data object's index.")
        data_index = droid.bleBuildAdvertiseData()
        self.log.debug("Step 4: Get the filtering data's service data.")
        service_data = droid.bleGetAdvertiseDataServiceData(
            data_index, exp_service_data_uuid)
        if exp_service_data != service_data:
            self.log.debug("exp value: {}, Actual value: {}".format(
                exp_service_data, service_data))
            return False
        self.log.debug("Advertise Data's service data uuid: {}, service data: "
                       "{}, value test Passed.".format(exp_service_data_uuid,
                                                       exp_service_data))
        return True

    def verify_adv_data_manu_id(self, droid, exp_manu_id,
                                exp_manu_specific_data):
        try:
            droid.bleAddAdvertiseDataManufacturerId(exp_manu_id,
                                                    exp_manu_specific_data)
        except BleAdvertiseVerificationError as error:
            self.log.debug(str(error))
            return False
        self.log.debug("Step 3: Get a filtering data object's index.")
        data_index = droid.bleBuildAdvertiseData()
        self.log.debug("Step 5: Get the filtering data's manu specific data.")
        manu_specific_data = droid.bleGetAdvertiseDataManufacturerSpecificData(
            data_index, exp_manu_id)
        if exp_manu_specific_data != manu_specific_data:
            self.log.debug("exp value: " + str(exp_manu_specific_data) +
                           ", Actual value: " + str(manu_specific_data))
            return False
        self.log.debug("Advertise Data's manu id: " + str(exp_manu_id) +
                       ", manu's specific data: " +
                       str(exp_manu_specific_data) + "  value test Passed.")
        return True

    def verify_adv_data_include_tx_power_level(self, droid,
                                               exp_include_tx_power_level):
        try:
            droid.bleSetAdvertiseDataIncludeTxPowerLevel(
                exp_include_tx_power_level)
        except BleAdvertiseVerificationError as error:
            self.log.debug(str(error))
            return False
        self.log.debug("Step 3: Get a filtering settings object's index.")
        data_index = droid.bleBuildAdvertiseData()
        self.log.debug(
            "Step 4: Get the filtering data's include tx power level.")
        include_tx_power_level = droid.bleGetAdvertiseDataIncludeTxPowerLevel(
            data_index)
        if exp_include_tx_power_level is not include_tx_power_level:
            self.log.debug("exp value: " + str(exp_include_tx_power_level) +
                           ", Actual value: " + str(include_tx_power_level))
            return False
        self.log.debug("Advertise Setting's include tx power level " + str(
            exp_include_tx_power_level) + "  value test Passed.")
        return True

    def verify_adv_data_include_device_name(self, droid,
                                            exp_include_device_name):
        try:
            droid.bleSetAdvertiseDataIncludeDeviceName(exp_include_device_name)
        except BleAdvertiseVerificationError as error:
            self.log.debug(str(error))
            return False
        self.log.debug("Step 3: Get a filtering settings object's index.")
        data_index = droid.bleBuildAdvertiseData()
        self.log.debug("Step 4: Get the filtering data's include device name.")
        include_device_name = droid.bleGetAdvertiseDataIncludeDeviceName(
            data_index)
        if exp_include_device_name is not include_device_name:
            self.log.debug("exp value: {}, Actual value: {}".format(
                exp_include_device_name, include_device_name))
            return False
        self.log.debug("Advertise Setting's include device name {}"
                       " value test Passed.".format(exp_include_device_name))
        return True

    def verify_invalid_adv_settings_adv_mode(self, droid, exp_adv_mode):
        try:
            droid.bleSetAdvertiseSettingsAdvertiseMode(exp_adv_mode)
            droid.bleBuildAdvertiseSettings()
            self.log.debug("Set Advertise settings invalid filtering mode "
                           "passed with input as {}".format(exp_adv_mode))
            return False
        except rpc_client.Sl4aApiError:
            self.log.debug(
                "Set Advertise settings invalid filtering mode "
                "failed successfully with input as {}".format(exp_adv_mode))
            return True

    def verify_invalid_adv_settings_tx_power_level(self, droid,
                                                   exp_adv_tx_power):
        try:
            droid.bleSetAdvertiseSettingsTxPowerLevel(exp_adv_tx_power)
            droid.bleBuildAdvertiseSettings()
            self.log.debug("Set Advertise settings invalid tx power level " +
                           " with input as {}".format(exp_adv_tx_power))
            return False
        except rpc_client.Sl4aApiError:
            self.log.debug(
                "Set Advertise settings invalid tx power level "
                "failed successfullywith input as {}".format(exp_adv_tx_power))
            return True

    def verify_invalid_adv_data_service_uuids(self, droid, exp_service_uuids):
        try:
            droid.bleSetAdvertiseDataSetServiceUuids(exp_service_uuids)
            droid.bleBuildAdvertiseData()
            self.log.debug("Set Advertise Data service uuids " +
                           " with input as {}".format(exp_service_uuids))
            return False
        except rpc_client.Sl4aApiError:
            self.log.debug(
                "Set Advertise Data invalid service uuids failed "
                "successfully with input as {}".format(exp_service_uuids))
            return True

    def verify_invalid_adv_data_service_data(
            self, droid, exp_service_data_uuid, exp_service_data):
        try:
            droid.bleAddAdvertiseDataServiceData(exp_service_data_uuid,
                                                 exp_service_data)
            droid.bleBuildAdvertiseData()
            self.log.debug("Set Advertise Data service data uuid: {},"
                           ", service data: {}".format(exp_service_data_uuid,
                                                       exp_service_data))
            return False
        except rpc_client.Sl4aApiError:
            self.log.debug("Set Advertise Data service data uuid: " +
                           str(exp_service_data_uuid) + ", service data: " +
                           str(exp_service_data) + " failed successfully.")
            return True

    def verify_invalid_adv_data_manu_id(self, droid, exp_manu_id,
                                        exp_manu_specific_data):
        try:
            droid.bleAddAdvertiseDataManufacturerId(exp_manu_id,
                                                    exp_manu_specific_data)
            droid.bleBuildAdvertiseData()
            self.log.debug(
                "Set Advertise Data manu id: " + str(exp_manu_id) +
                ", manu specific data: " + str(exp_manu_specific_data))
            return False
        except rpc_client.Sl4aApiError:
            self.log.debug("Set Advertise Data manu id: {},"
                           " manu specific data: {},".format(
                               exp_manu_id, exp_manu_specific_data))
            return True
