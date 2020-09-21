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
Test script to exercise Ble Scan Api's. This exercises all getters and
setters. This is important since there is a builder object that is immutable
after you set all attributes of each object. If this test suite doesn't pass,
then other test suites utilising Ble Scanner will also fail.
"""

from acts.controllers.sl4a_lib import rpc_client
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import ble_scan_settings_callback_types
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_constants import ble_scan_settings_result_types
from acts.test_utils.bt.bt_constants import ble_scan_settings_report_delay_milli_seconds
from acts.test_utils.bt.bt_constants import ble_uuids


class BleScanResultsError(Exception):
    """Error in getting scan results"""


class BleScanVerificationError(Exception):
    """Error in comparing BleScan results"""


class BleSetScanSettingsError(Exception):
    """Error in setting Ble Scan Settings"""


class BleSetScanFilterError(Exception):
    """Error in setting Ble Scan Settings"""


class BleScanApiTest(BluetoothBaseTest):
    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.ad_dut = self.android_devices[0]

    def _format_defaults(self, input):
        """
        Creates a dictionary of default ScanSetting and ScanFilter Values.
        :return: input: dict
        """
        if 'ScanSettings' not in input.keys():
            input['ScanSettings'] = (
                ble_scan_settings_callback_types['all_matches'], 0,
                ble_scan_settings_modes['low_power'],
                ble_scan_settings_result_types['full'])
        if 'ScanFilterManufacturerDataId' not in input.keys():
            input['ScanFilterManufacturerDataId'] = -1
        if 'ScanFilterDeviceName' not in input.keys():
            input['ScanFilterDeviceName'] = None
        if 'ScanFilterDeviceAddress' not in input.keys():
            input['ScanFilterDeviceAddress'] = None
        if 'ScanFilterManufacturerData' not in input.keys():
            input['ScanFilterManufacturerData'] = None
        return input

    def validate_scan_settings_helper(self, input, droid):
        """
        Validates each input of the scan settings object that is matches what
        was set or not set such that it matches the defaults.
        :return: False at any point something doesn't match. True if everything
        matches.
        """
        filter_list = droid.bleGenFilterList()
        if 'ScanSettings' in input.keys():
            try:
                droid.bleSetScanSettingsCallbackType(input['ScanSettings'][0])
                droid.bleSetScanSettingsReportDelayMillis(
                    input['ScanSettings'][1])
                droid.bleSetScanSettingsScanMode(input['ScanSettings'][2])
                droid.bleSetScanSettingsResultType(input['ScanSettings'][3])
            except rpc_client.Sl4aApiError as error:
                self.log.debug("Set Scan Settings failed with: ".format(error))
                return False
        if 'ScanFilterDeviceName' in input.keys():
            try:
                droid.bleSetScanFilterDeviceName(input['ScanFilterDeviceName'])
            except rpc_client.Sl4aApiError as error:
                self.log.debug("Set Scan Filter Device Name failed with: {}"
                               .format(error))
                return False
        if 'ScanFilterDeviceAddress' in input.keys():
            try:
                droid.bleSetScanFilterDeviceAddress(
                    input['ScanFilterDeviceAddress'])
            except rpc_client.Sl4aApiError as error:
                self.log.debug("Set Scan Filter Device Address failed with: {}"
                               .format(error))
                return False
        if ('ScanFilterManufacturerDataId' in input.keys()
                and 'ScanFilterManufacturerDataMask' in input.keys()):
            try:
                droid.bleSetScanFilterManufacturerData(
                    input['ScanFilterManufacturerDataId'],
                    input['ScanFilterManufacturerData'],
                    input['ScanFilterManufacturerDataMask'])
            except rpc_client.Sl4aApiError as error:
                self.log.debug("Set Scan Filter Manufacturer info with data "
                               "mask failed with: {}".format(error))
                return False
        if ('ScanFilterManufacturerDataId' in input.keys()
                and 'ScanFilterManufacturerData' in input.keys()
                and 'ScanFilterManufacturerDataMask' not in input.keys()):
            try:
                droid.bleSetScanFilterManufacturerData(
                    input['ScanFilterManufacturerDataId'],
                    input['ScanFilterManufacturerData'])
            except rpc_client.Sl4aApiError as error:
                self.log.debug(
                    "Set Scan Filter Manufacturer info failed with: "
                    "{}".format(error))
                return False
        if ('ScanFilterServiceUuid' in input.keys()
                and 'ScanFilterServiceMask' in input.keys()):

            droid.bleSetScanFilterServiceUuid(input['ScanFilterServiceUuid'],
                                              input['ScanFilterServiceMask'])

        input = self._format_defaults(input)
        scan_settings_index = droid.bleBuildScanSetting()
        scan_settings = (
            droid.bleGetScanSettingsCallbackType(scan_settings_index),
            droid.bleGetScanSettingsReportDelayMillis(scan_settings_index),
            droid.bleGetScanSettingsScanMode(scan_settings_index),
            droid.bleGetScanSettingsScanResultType(scan_settings_index))

        scan_filter_index = droid.bleBuildScanFilter(filter_list)
        device_name_filter = droid.bleGetScanFilterDeviceName(
            filter_list, scan_filter_index)
        device_address_filter = droid.bleGetScanFilterDeviceAddress(
            filter_list, scan_filter_index)
        manufacturer_id = droid.bleGetScanFilterManufacturerId(
            filter_list, scan_filter_index)
        manufacturer_data = droid.bleGetScanFilterManufacturerData(
            filter_list, scan_filter_index)

        if scan_settings != input['ScanSettings']:
            self.log.debug("Scan Settings did not match. expected: {}, found: "
                           "{}".format(input['ScanSettings'], scan_settings))
            return False
        if device_name_filter != input['ScanFilterDeviceName']:
            self.log.debug("Scan Filter device name did not match. expected: "
                           "{}, found {}".format(input['ScanFilterDeviceName'],
                                                 device_name_filter))
            return False
        if device_address_filter != input['ScanFilterDeviceAddress']:
            self.log.debug(
                "Scan Filter address name did not match. expected: "
                "{}, found: {}".format(input['ScanFilterDeviceAddress'],
                                       device_address_filter))
            return False
        if manufacturer_id != input['ScanFilterManufacturerDataId']:
            self.log.debug("Scan Filter manufacturer data id did not match. "
                           "expected: {}, found: {}".format(
                               input['ScanFilterManufacturerDataId'],
                               manufacturer_id))
            return False
        if manufacturer_data != input['ScanFilterManufacturerData']:
            self.log.debug("Scan Filter manufacturer data did not match. "
                           "expected: {}, found: {}".format(
                               input['ScanFilterManufacturerData'],
                               manufacturer_data))
            return False
        if 'ScanFilterManufacturerDataMask' in input.keys():
            manufacturer_data_mask = droid.bleGetScanFilterManufacturerDataMask(
                filter_list, scan_filter_index)
            if manufacturer_data_mask != input['ScanFilterManufacturerDataMask']:
                self.log.debug(
                    "Manufacturer data mask did not match. expected:"
                    " {}, found: {}".format(
                        input['ScanFilterManufacturerDataMask'],
                        manufacturer_data_mask))

                return False
        if ('ScanFilterServiceUuid' in input.keys()
                and 'ScanFilterServiceMask' in input.keys()):

            expected_service_uuid = input['ScanFilterServiceUuid']
            expected_service_mask = input['ScanFilterServiceMask']
            service_uuid = droid.bleGetScanFilterServiceUuid(
                filter_list, scan_filter_index)
            service_mask = droid.bleGetScanFilterServiceUuidMask(
                filter_list, scan_filter_index)
            if service_uuid != expected_service_uuid.lower():
                self.log.debug("Service uuid did not match. expected: {}, "
                               "found {}".format(expected_service_uuid,
                                                 service_uuid))
                return False
            if service_mask != expected_service_mask.lower():
                self.log.debug("Service mask did not match. expected: {}, "
                               "found {}".format(expected_service_mask,
                                                 service_mask))
                return False
        self.scan_settings_index = scan_settings_index
        self.filter_list = filter_list
        self.scan_callback = droid.bleGenScanCallback()
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5ffc9f7b-c261-4bf0-9a6b-7fda182b6c97')
    def test_start_ble_scan_with_default_settings(self):
        """Test LE scan with default settings.

        Test to validate all default scan settings values.

        Steps:
        1. Create LE scan objects.
        2. Start LE scan.

        Expected Result:
        Scan starts successfully and matches expected settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='88c3b0cb-b4d4-45d1-be25-9855290a6d03')
    def test_stop_ble_scan_default_settings(self):
        """Test stopping an LE scan.

        Test default scan settings on an actual scan. Verify it can also stop
        the scan.

        Steps:
        1. Validate default scan settings.
        2. Start ble scan.
        3. Stop ble scan.

        Expected Result:
        LE scan is stopped successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 0
        """
        input = {}
        test_result = self.validate_scan_settings_helper(
            input, self.ad_dut.droid)
        if not test_result:
            self.log.error("Could not setup ble scanner.")
            return test_result
        self.ad_dut.droid.bleStartBleScan(
            self.filter_list, self.scan_settings_index, self.scan_callback)
        try:
            self.ad_dut.droid.bleStopBleScan(self.scan_callback)
        except BleScanResultsError as error:
            self.log.error(str(error))
            test_result = False
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5aa7a4c2-0b7d-4000-a980-f00c9329a7b9')
    def test_scan_settings_callback_type_all_matches(self):
        """Test LE scan settings callback type all matches.

        Test scan settings callback type all matches.

        Steps:
        1. Validate the scan settings callback type with all other settings set
        to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['all_matches'], 0,
            ble_scan_settings_modes['low_power'],
            ble_scan_settings_result_types['full'])
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='fd764861-aa76-480e-b2d2-5d55a888d123')
    def test_scan_settings_set_callback_type_first_match(self):
        """Test LE scan settings callback type first match

        Test scan settings callback type first match.

        Steps:
        1. Validate the scan settings callback type with all other settings set
        to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['first_match'], 0,
            ble_scan_settings_modes['low_power'],
            ble_scan_settings_result_types['full'])
        test_result = self.validate_scan_settings_helper(
            input, self.ad_dut.droid)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='52e4626e-199c-4755-b9f1-8b38ecb30896')
    def test_scan_settings_set_callback_type_match_lost(self):
        """Test LE scan settings callback type match lost.

        Test scan settings callback type match lost.

        Steps:
        1. Validate the scan settings callback type with all other settings set
        to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['match_lost'], 0,
            ble_scan_settings_modes['low_power'],
            ble_scan_settings_result_types['full'])
        test_result = self.validate_scan_settings_helper(
            input, self.ad_dut.droid)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='57476b3c-ba7a-4342-86f6-1b56b2c00181')
    def test_scan_settings_set_invalid_callback_type(self):
        """Test LE scan settings invalid callback type.

        Test scan settings invalid callback type -1.

        Steps:
        1. Build a LE ScanSettings object with an invalid callback type.

        Expected Result:
        Api should fail to build object.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        input = {}
        input["ScanSettings"] = (-1, 0, ble_scan_settings_modes['low_power'],
                                 ble_scan_settings_result_types['full'])
        test_result = self.validate_scan_settings_helper(
            input, self.ad_dut.droid)
        return not test_result

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='52c80f0c-4f26-4cda-8a6b-291ac52f673a')
    def test_scan_settings_set_scan_mode_low_power(self):
        """Test LE scan settings scan mode low power mode.

        Test scan settings scan mode low power.

        Steps:
        1. Validate the scan settings scan mode with all other settings set to
        their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['all_matches'], 0,
            ble_scan_settings_modes['low_power'],
            ble_scan_settings_result_types['full'])
        test_result = self.validate_scan_settings_helper(
            input, self.ad_dut.droid)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='20f4513c-44a7-435d-be4e-03420093297a')
    def test_scan_settings_set_scan_mode_balanced(self):
        """Test LE scan settings scan mode balanced.

        Test scan settings scan mode balanced.

        Steps:
        1. Validate the scan settings scan mode with all other settings set to
        their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['all_matches'], 0,
            ble_scan_settings_modes['balanced'],
            ble_scan_settings_result_types['full'])
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='bf14e7fd-853b-4833-8fef-8c4bd629374b')
    def test_scan_settings_set_scan_mode_low_latency(self):
        """Test LE scan settings scan mode low latency.

        Test scan settings scan mode low latency.

        Steps:
        1. Validate the scan settings scan mode with all other settings set to
        their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['all_matches'], 0,
            ble_scan_settings_modes['low_latency'],
            ble_scan_settings_result_types['full'])
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9f3b2e10-98f8-4d6a-b6b6-e8dee87063f0')
    def test_scan_settings_set_invalid_scan_mode(self):
        """Test LE scan settings scan mode as an invalid value.
        Test scan settings invalid scan mode -2.
        Steps:
        1. Set the scan settings scan mode to -2.

        Expected Result:
        Building the ScanSettings object should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['all_matches'], 0, -2,
            ble_scan_settings_result_types['full'])
        return not self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cb246be7-4fef-4313-964d-5fb6dbe558c8')
    def test_scan_settings_set_report_delay_millis_min(self):
        """Test scan settings report delay millis as min value

        Test scan settings report delay millis min acceptable value.

        Steps:
        1. Validate the scan settings report delay millis with all other
        settings set to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['all_matches'],
            ble_scan_settings_report_delay_milli_seconds['min'],
            ble_scan_settings_modes['low_power'],
            ble_scan_settings_result_types['full'])
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='db1ea8f6-503d-4e9a-b61a-01210508c5a2')
    def test_scan_settings_set_report_delay_millis_min_plus_one(self):
        """Test scan settings report delay millis as min value plus one.

        Test scan settings report delay millis as min value plus one.

        Steps:
        1. Validate the scan settings report delay millis with all other
        settings set to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 4
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['all_matches'],
            ble_scan_settings_report_delay_milli_seconds['min'] + 1,
            ble_scan_settings_modes['low_power'],
            ble_scan_settings_result_types['full'])
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='54e83ff8-92b0-473e-839a-1ff1c7dcea83')
    def test_scan_settings_set_report_delay_millis_max(self):
        """Test scan settings report delay millis as max value.

        Test scan settings report delay millis max value.

        Steps:
        1. Validate the scan settings report delay millis with all other
        settings set to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 3
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['all_matches'],
            ble_scan_settings_report_delay_milli_seconds['max'],
            ble_scan_settings_modes['low_power'],
            ble_scan_settings_result_types['full'])
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='45d918ec-7e43-463b-8f07-f009f8808903')
    def test_scan_settings_set_report_delay_millis_max_minus_one(self):
        """Test scan settings report delay millis as max value minus one.

        Test scan settings report delay millis max value - 1.

        Steps:
        1. Validate the scan settings report delay millis with all other
        settings set to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 3
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['all_matches'],
            ble_scan_settings_report_delay_milli_seconds['max'] - 1,
            ble_scan_settings_modes['low_power'],
            ble_scan_settings_result_types['full'])
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='eb94b5ee-f2e7-4322-b3df-7bdd3a250262')
    def test_scan_settings_set_invalid_report_delay_millis_min_minus_one(self):
        """Test scan settings report delay millis as an invalid value.

        Test scan settings invalid report delay millis min value - 1.

        Steps:
        1. Set scan settings report delay millis to min value -1.
        2. Build scan settings object.

        Expected Result:
        Building scan settings object should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        droid = self.ad_dut.droid
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['all_matches'],
            ble_scan_settings_report_delay_milli_seconds['min'] - 1,
            ble_scan_settings_modes['low_power'],
            ble_scan_settings_result_types['full'])
        return not self.validate_scan_settings_helper(input, droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8f5a2bd0-6037-4ac6-a962-f11e7fc13920')
    def test_scan_settings_set_scan_result_type_full(self):
        """Test scan settings result type full.

        Test scan settings result type full.

        Steps:
        1. Validate the scan settings result type with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['all_matches'], 0,
            ble_scan_settings_modes['low_power'],
            ble_scan_settings_result_types['full'])
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='610fe301-600e-443e-a28b-cd722cc8a4c1')
    def test_scan_settings_set_scan_result_type_abbreviated(self):
        """Test scan settings result type abbreviated.

        Test scan settings result type abbreviated.

        Steps:
        1. Validate the scan settings result type with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['all_matches'], 0,
            ble_scan_settings_modes['low_power'],
            ble_scan_settings_result_types['abbreviated'])
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ed58430b-8180-472f-a118-64f5fce5e84c')
    def test_scan_settings_set_invalid_scan_result_type(self):
        """Test scan settings result type as an invalid value.

        Test scan settings invalid result type -1.

        Steps:
        1. Set scan settings result type as an invalid value.
        2. Build scan settings object.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        input = {}
        input["ScanSettings"] = (
            ble_scan_settings_callback_types['all_matches'], 0,
            ble_scan_settings_modes['low_power'], -1)
        return not self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6489665f-313d-4b1b-bd7f-f0fdeeaad335')
    def test_scan_filter_set_device_name(self):
        """Test scan filter set valid device name.

        Test scan filter device name sl4atest.

        Steps:
        1. Validate the scan filter device name with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input['ScanFilterDeviceName'] = "sl4atest"
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='76021a9a-14ca-4a2f-a908-96ab90db39ce')
    def test_scan_filter_set_device_name_blank(self):
        """Test scan filter set blank device name.

        Test scan filter device name blank.

        Steps:
        1. Validate the scan filter device name with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        droid = self.ad_dut.droid
        input = {}
        input['ScanFilterDeviceName'] = ""
        return self.validate_scan_settings_helper(input, droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d77c3d81-43a9-4572-a99b-87969117ede5')
    def test_scan_filter_set_device_name_special_chars(self):
        """Test scan filter set device name as special chars.

        Test scan filter device name special characters.

        Steps:
        1. Validate the scan filter device name with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input['ScanFilterDeviceName'] = "!@#$%^&*()\":<>/"
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1697004e-76ab-444b-9419-0437e30444ad')
    def test_scan_filter_set_device_address(self):
        """Test scan filter set valid device address.

        Test scan filter device address valid.

        Steps:
        1. Validate the scan filter device address with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input['ScanFilterDeviceAddress'] = "01:02:03:AB:CD:EF"
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='eab0409c-7fc5-4d1f-8fbe-5ee2bb743f7e')
    def test_scan_filter_set_invalid_device_address_lower_case(self):
        """Test scan filter set invalid device address.

        Test scan filter device address lower case.

        Steps:
        1. Set the scan filter address to an invalid, lowercase mac address

        Expected Result:
        Api to build scan filter should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        input = {}
        input['ScanFilterDeviceAddress'] = "01:02:03:ab:cd:ef"
        return not self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0ec491ac-d273-468e-bbfe-e36a290aeb2a')
    def test_scan_filter_set_invalid_device_address_blank(self):
        """Test scan filter set invalid device address.

        Test scan filter invalid device address blank.

        Steps:
        1. Set the scan filter address to an invalid, blank mac address

        Expected Result:
        Api to build scan filter should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        input = {}
        input['ScanFilterDeviceAddress'] = ""
        test_result = self.validate_scan_settings_helper(
            input, self.ad_dut.droid)
        return not test_result

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5cebc454-091c-4e46-b200-1e52c8dffbec')
    def test_scan_filter_set_invalid_device_address_bad_format(self):
        """Test scan filter set badly formatted device address.

        Test scan filter badly formatted device address.

        Steps:
        1. Set the scan filter address to an invalid, blank mac address

        Expected Result:
        Api to build scan filter should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        input = {}
        input['ScanFilterDeviceAddress'] = "10.10.10.10.10"
        return not self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d5249d10-1486-4c38-a22d-1f1b077926db')
    def test_scan_filter_set_invalid_device_address_bad_address(self):
        """Test scan filter device address as an invalid value.

        Test scan filter invalid device address invalid characters.

        Steps:
        1. Set a scan filter's device address as ZZ:ZZ:ZZ:ZZ:ZZ:ZZ

        Expected Result:
        Api to build the scan filter should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input['ScanFilterDeviceAddress'] = "ZZ:ZZ:ZZ:ZZ:ZZ:ZZ"
        return not self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='65c62d50-69f6-4a0b-bd74-2340e0ce32ca')
    def test_scan_filter_set_manufacturer_id_data(self):
        """Test scan filter manufacturer data.

        Test scan filter manufacturer data with a valid input.

        Steps:
        1. Validate the scan filter manufacturer id with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        expected_manufacturer_id = 0
        expected_manufacturer_data = [1, 2, 1, 3, 4, 5, 6]
        input = {}
        input['ScanFilterManufacturerDataId'] = expected_manufacturer_id
        input['ScanFilterManufacturerData'] = expected_manufacturer_data
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='12807021-9f66-4784-b34a-80859cf4a32f')
    def test_scan_filter_set_manufacturer_id_data_mask(self):
        """Test scan filter manufacturer data mask.

        Test scan filter manufacturer data with a valid data mask.

        Steps:
        1. Validate the scan filter manufacturer id with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        expected_manufacturer_id = 1
        expected_manufacturer_data = [1]
        expected_manufacturer_data_mask = [1, 2, 1, 3, 4, 5, 6]
        input = {}
        input['ScanFilterManufacturerDataId'] = expected_manufacturer_id
        input['ScanFilterManufacturerData'] = expected_manufacturer_data
        input[
            'ScanFilterManufacturerDataMask'] = expected_manufacturer_data_mask
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='980e5ab6-5381-4471-8e5b-0b716665a9b8')
    def test_scan_filter_set_manufacturer_max_id(self):
        """Test scan filter manufacturer data id.

        Test scan filter manufacturer data max id.

        Steps:
        1. Validate the scan filter manufacturer id with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        expected_manufacturer_id = 2147483647
        expected_manufacturer_data = [1, 2, 1, 3, 4, 5, 6]
        input = {}
        input['ScanFilterManufacturerDataId'] = expected_manufacturer_id
        input['ScanFilterManufacturerData'] = expected_manufacturer_data
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cf0efe38-8621-4288-be26-742719da2f6c')
    def test_scan_filter_set_manufacturer_data_empty(self):
        """Test scan filter empty manufacturer data.

        Test scan filter manufacturer data as empty but valid manufacturer data.

        Steps:
        1. Validate the scan filter manufacturer id with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        expected_manufacturer_id = 1
        expected_manufacturer_data = []
        input = {}
        input['ScanFilterManufacturerDataId'] = expected_manufacturer_id
        input['ScanFilterManufacturerData'] = expected_manufacturer_data
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7ea0e82e-e92a-469c-8432-8f21978508cb')
    def test_scan_filter_set_manufacturer_data_mask_empty(self):
        """Test scan filter empty manufacturer data mask.

        Test scan filter manufacturer mask empty.

        Steps:
        1. Validate the scan filter manufacturer id with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        expected_manufacturer_id = 1
        expected_manufacturer_data = [1, 2, 1, 3, 4, 5, 6]
        expected_manufacturer_data_mask = []
        input = {}
        input['ScanFilterManufacturerDataId'] = expected_manufacturer_id
        input['ScanFilterManufacturerData'] = expected_manufacturer_data
        input[
            'ScanFilterManufacturerDataMask'] = expected_manufacturer_data_mask
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='88e4a9b8-afae-48cb-873a-fd6b4ef84116')
    def test_scan_filter_set_invalid_manufacturer_min_id_minus_one(self):
        """Test scan filter invalid manufacturer data.

        Test scan filter invalid manufacturer id min value - 1.

        Steps:
        1. Set the scan filters manufacturer id to -1.
        2. Build the scan filter.

        Expected Result:
        Api to build the scan filter should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        expected_manufacturer_id = -1
        expected_manufacturer_data = [1, 2, 1, 3, 4, 5, 6]
        input = {}
        input['ScanFilterManufacturerDataId'] = expected_manufacturer_id
        input['ScanFilterManufacturerData'] = expected_manufacturer_data
        return not self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2e8438dc-29cd-4f72-8747-4a161974d4d3')
    def test_scan_filter_set_service_uuid(self):
        """Test scan filter set valid service uuid.

        Test scan filter service uuid.

        Steps:
        1. Validate the scan filter service uuid with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        expected_service_uuid = "00000000-0000-1000-8000-00805F9B34FB"
        expected_service_mask = "00000000-0000-1000-8000-00805F9B34FB"
        input = {}
        input['ScanFilterServiceUuid'] = expected_service_uuid
        input['ScanFilterServiceMask'] = expected_service_mask
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e07b9985-44b6-4dc4-b570-0833b5d2893c')
    def test_scan_filter_service_uuid_p_service(self):
        """Test scan filter service uuid.

        Test scan filter service uuid p service

        Steps:
        1. Validate the scan filter service uuid with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        expected_service_uuid = ble_uuids['p_service']
        expected_service_mask = "00000000-0000-1000-8000-00805F9B34FB"
        self.log.debug("Step 1: Setup environment.")

        input = {}
        input['ScanFilterServiceUuid'] = expected_service_uuid
        input['ScanFilterServiceMask'] = expected_service_mask
        return self.validate_scan_settings_helper(input, self.ad_dut.droid)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0467af19-6e9a-4cfe-9e10-878b0c224df2')
    def test_classic_ble_scan_with_service_uuids_p(self):
        """Test classic LE scan with valid service uuid.

        Test classic ble scan with scan filter service uuid p service uuids.

        Steps:
        1. Validate the scan filter service uuid with all other settings
        set to their respective defaults.
        2. Start classic ble scan.
        3. Stop classic ble scan

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """

        droid = self.ad_dut.droid
        service_uuid_list = [ble_uuids['p_service']]
        scan_callback = droid.bleGenLeScanCallback()
        return self.verify_classic_ble_scan_with_service_uuids(
            droid, scan_callback, service_uuid_list)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='516c295f-a2df-44f6-b2ad-54451af43ce8')
    def test_classic_ble_scan_with_service_uuids_hr(self):
        """Test classic LE scan with valid service uuid.

        Test classic ble scan with scan filter service uuid hr service

        Steps:
        1. Validate the scan filter service uuid with all other settings
        set to their respective defaults.
        2. Start classic ble scan.
        3. Stop classic ble scan

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        droid = self.ad_dut.droid
        service_uuid_list = [ble_uuids['hr_service']]
        scan_callback = droid.bleGenLeScanCallback()
        return self.verify_classic_ble_scan_with_service_uuids(
            droid, scan_callback, service_uuid_list)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0458b5e0-bb0b-4d6e-ab79-e21169d3256b')
    def test_classic_ble_scan_with_service_uuids_empty_uuid_list(self):
        """Test classic LE scan with empty but valid uuid list.

        Test classic ble scan with service uuids as empty list.

        Steps:
        1. Validate the scan filter service uuid with all other settings
        set to their respective defaults.
        2. Start classic ble scan.
        3. Stop classic ble scan

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        droid = self.ad_dut.droid
        service_uuid_list = []
        scan_callback = droid.bleGenLeScanCallback()
        return self.verify_classic_ble_scan_with_service_uuids(
            droid, scan_callback, service_uuid_list)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c0d84a37-c86c-43c4-9dc7-d16959fdbc2a')
    def test_classic_ble_scan_with_service_uuids_hr_and_p(self):
        """Test classic LE scan with multiple service uuids.

        Test classic ble scan with service uuids a list of hr and p service.

        Steps:
        1. Validate the scan filter service uuid with all other settings
        set to their respective defaults.
        2. Start classic ble scan.
        3. Stop classic ble scan

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        droid = self.ad_dut.droid
        service_uuid_list = [ble_uuids['hr_service'], ble_uuids['p_service']]
        scan_callback = droid.bleGenLeScanCallback()
        return self.verify_classic_ble_scan_with_service_uuids(
            droid, scan_callback, service_uuid_list)

    def verify_classic_ble_scan_with_service_uuids(self, droid, scan_callback,
                                                   service_uuid_list):

        test_result = True
        try:
            test_result = droid.bleStartClassicBleScanWithServiceUuids(
                scan_callback, service_uuid_list)
        except BleScanResultsError as error:
            self.log.error(str(error))
            return False
        droid.bleStopClassicBleScan(scan_callback)
        if not test_result:
            self.log.error(
                "Start classic ble scan with service uuids return false "
                "boolean value.")
            return False
        return True
