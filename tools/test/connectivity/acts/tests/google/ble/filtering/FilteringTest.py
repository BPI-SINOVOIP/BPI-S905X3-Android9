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

import itertools as it
import pprint
import time

from queue import Empty
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import ble_advertise_settings_modes
from acts.test_utils.bt.bt_constants import ble_advertise_settings_tx_powers
from acts.test_utils.bt.bt_constants import java_integer
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.bt_constants import small_timeout
from acts.test_utils.bt.bt_constants import adv_fail
from acts.test_utils.bt.bt_constants import adv_succ
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_constants import scan_result


class FilteringTest(BluetoothBaseTest):
    default_timeout = 30
    default_callback = 1
    default_is_connectable = True
    default_advertise_mode = 0
    default_tx_power_level = 2

    #Data constant variants
    manu_sepecific_data_small = [1]
    manu_sepecific_data_small_2 = [1, 2]
    manu_specific_data_small_3 = [127]
    manu_sepecific_data_large = [14, 0, 54, 0, 0, 0, 0, 0]
    manu_sepecific_data_mask_small = [1]
    manu_specific_data_id_1 = 1
    manu_specific_data_id_2 = 2
    manu_specific_data_id_3 = 65535

    service_data_small = [13]
    service_data_small_2 = [127]
    service_data_medium = [11, 14, 50]
    service_data_large = [
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 26, 17, 18, 19, 20,
        21, 22, 23, 24
    ]

    service_mask_1 = "00000000-0000-1000-8000-00805f9b34fb"
    service_uuid_1 = "00000000-0000-1000-8000-00805f9b34fb"
    service_uuid_2 = "FFFFFFFF-0000-1000-8000-00805f9b34fb"
    service_uuid_3 = "3846D7A0-69C8-11E4-BA00-0002A5D5C51B"

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.scn_ad = self.android_devices[0]
        self.adv_ad = self.android_devices[1]
        self.log.info("Scanner device model: {}".format(
            self.scn_ad.droid.getBuildModel()))
        self.log.info("Advertiser device model: {}".format(
            self.adv_ad.droid.getBuildModel()))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='be72fc18-e7e9-41cf-80b5-e31babd763f6')
    def test_filter_combo_0(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='92e11460-1877-4dd1-998b-8f78354dd776')
    def test_filter_combo_1(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9cdb7ad3-9f1e-4cbc-ae3f-af27d9833ae3')
    def test_filter_combo_2(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ae06ece8-28ae-4c2f-a768-d0e1e60cc253')
    def test_filter_combo_3(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7a7c40fb-1398-4659-af46-ba01ca23ba7f')
    def test_filter_combo_4(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='85cab0b7-4ba2-408c-b78b-c45d0cad1d1e')
    def test_filter_combo_5(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='31e7a496-6626-4d73-8337-b250f7386ab6')
    def test_filter_combo_6(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='465f5426-1157-4a6f-8c33-a266ee7439bc')
    def test_filter_combo_7(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='77552584-74c7-4a1b-a98e-8863e91f4e74')
    def test_filter_combo_8(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='fb2b5f08-53cd-400b-98a0-bbd96093e466')
    def test_filter_combo_9(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='edacb609-9508-4394-9c94-9ed13a4205b5')
    def test_filter_combo_10(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='73a86198-3213-43c5-b083-0a37089b8e44')
    def test_filter_combo_11(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9ca92075-d22b-4e82-9e7b-495060f3af45')
    def test_filter_combo_12(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a0689f97-c616-49a5-b690-00b6193ac822')
    def test_filter_combo_13(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='dbdf3a68-c79a-43a6-89a7-5269a1fad9a5')
    def test_filter_combo_14(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='93e45b16-dff0-4067-9c14-7adf32a0f484')
    def test_filter_combo_15(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='dace8a1c-e71a-4668-9e8f-b1cb19071087')
    def test_filter_combo_16(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='192528e2-4a67-4984-9c68-f9d716470d5b')
    def test_filter_combo_17(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2a9ffd92-f02d-45bc-81f5-f398e2572f14')
    def test_filter_combo_18(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ab98e5e5-ac35-4ebe-8b37-780b0ab56b82')
    def test_filter_combo_19(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0b4ca831-dbf6-44da-84b6-9425b7f50577')
    def test_filter_combo_20(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f615206a-16bf-4481-be31-7b2a28d8009b')
    def test_filter_combo_21(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e9a0e69e-bc5c-479e-a716-cbb88180e719')
    def test_filter_combo_22(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='16c2949d-e7c8-4fa1-a781-3ced2c902c4c')
    def test_filter_combo_23(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ffd7a3a8-b9b5-4bf0-84c1-ed3823b8a52c')
    def test_filter_combo_24(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='457b5dee-1034-4973-88c1-bde0a6ef700c')
    def test_filter_combo_25(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b848e90c-37ed-4ecb-8c49-601f3b66a4cc')
    def test_filter_combo_26(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c03df405-b7aa-42cf-b282-adf5c228e513')
    def test_filter_combo_27(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='83de681d-89fb-45e1-b8e0-0488e43b3248')
    def test_filter_combo_28(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='712bf6b2-0cdc-4782-b593-17a846fd1c65')
    def test_filter_combo_29(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='725c37e5-046b-4234-a7eb-ad8836531a74')
    def test_filter_combo_30(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='665344f9-c246-4b08-aff6-73f7ff35431b')
    def test_filter_combo_31(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6994ceff-fed8-42e4-a3cb-be6ed3a9a5c9')
    def test_filter_combo_32(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2fb756c0-8b72-403a-a769-d22d31376037')
    def test_filter_combo_33(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='146203bb-04cc-4b3d-b372-66e1b8da3e08')
    def test_filter_combo_34(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1e123df5-db37-4e8d-ac1f-9399fe8487f9')
    def test_filter_combo_35(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='886f0978-a2df-4005-810b-5e2cc0c2a5a4')
    def test_filter_combo_36(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='94f61b07-e90a-42e3-b97b-07afc73755e6')
    def test_filter_combo_37(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1dbb67ed-2f9e-464d-8ba8-dd7ac668d765')
    def test_filter_combo_38(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='3a3e5aa9-a5cc-4e99-aeb4-b32357186e1d')
    def test_filter_combo_39(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2c51245a-7be3-4dfb-87c5-7c4530ab5908')
    def test_filter_combo_40(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9d33bae5-0a5f-4d2c-96fc-fc1ec8107814')
    def test_filter_combo_41(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='05b5ee9e-9a64-4bf8-91ab-a7762358d25e')
    def test_filter_combo_42(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='589f4d3f-c644-4981-a0f8-cd9bcf4d5142')
    def test_filter_combo_43(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='10ce4d36-081f-4353-a484-2c7988e7cda8')
    def test_filter_combo_44(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6f52f24d-adda-4e2d-b52e-1b24b978c343')
    def test_filter_combo_45(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5aacbec9-4a8b-4c76-9684-590a29f73854')
    def test_filter_combo_46(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e1fa3728-9acb-47e9-bea4-3ac886c68a22')
    def test_filter_combo_47(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9ce98edd-5f94-456c-8083-3dd37eefe086')
    def test_filter_combo_48(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cb93cdab-6443-4946-a7f6-9c34e0b21272')
    def test_filter_combo_49(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d9069a4d-8635-4b91-9a0f-31a64586a216')
    def test_filter_combo_50(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='559f4f49-bd6a-4490-b8b3-da13ef57eb83')
    def test_filter_combo_51(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d769aa3c-c039-45f3-8ef7-f91ccbbcdfaf')
    def test_filter_combo_52(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4c6adf11-7c79-4a97-b507-cc8044d2c7c6')
    def test_filter_combo_53(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b99f7238-197b-4fb0-80a9-a51a20c00093')
    def test_filter_combo_54(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f1e3e036-b611-4325-81e2-114ad777d00e')
    def test_filter_combo_55(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9f786760-8a33-4076-b33e-38acc6689b5c')
    def test_filter_combo_56(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9e6466c3-ce73-471e-8b4a-dce1a1c9d046')
    def test_filter_combo_57(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b44f6d43-07cb-477d-bcc8-460cc2094475')
    def test_filter_combo_58(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='38dcb64b-6564-4116-8abb-3a8e8ed530a9')
    def test_filter_combo_59(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b0918c1a-1291-482c-9ecb-2df085ec036f')
    def test_filter_combo_60(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6c317053-6fdc-45e1-9109-bd2726b2490f')
    def test_filter_combo_61(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='af057157-3ef5-48af-918d-53ba6b2e8405')
    def test_filter_combo_62(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4693bb43-a4b6-4595-a75b-ff18c4be50c7')
    def test_filter_combo_63(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6e2037c1-5e99-4dc7-8950-5fd3df29fa08')
    def test_filter_combo_64(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a85acfbb-e6d2-42f4-b917-6b0bac26e457')
    def test_filter_combo_65(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d05f3aaa-833c-40a1-b3a0-c69756919218')
    def test_filter_combo_66(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1dd2b27b-f9fe-41e3-b884-3500d6bf9a38')
    def test_filter_combo_67(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e31e7d9d-878b-442e-9ae9-b07d5e903df3')
    def test_filter_combo_68(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='05c05a71-27a4-4620-940b-ce3747d4e6c5')
    def test_filter_combo_69(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c4bb2251-1246-466b-a6bb-76ae13089101')
    def test_filter_combo_70(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b284008a-81be-42b6-8176-906a780f92a2')
    def test_filter_combo_71(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='01bc025f-4696-4e80-a590-ec7b0eeea1a3')
    def test_filter_combo_72(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ef674d1e-f3b1-43fc-a037-718ffe650d12')
    def test_filter_combo_73(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cbc29a50-76fe-40b8-93fa-b274605660b2')
    def test_filter_combo_74(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0674b703-2571-4bcf-91f2-a34a323e179b')
    def test_filter_combo_75(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a65046b3-4aed-47f3-86cd-838155dfd309')
    def test_filter_combo_76(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a379dfdd-8924-4e62-95ac-14fe3ae358da')
    def test_filter_combo_77(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='3ed6b73f-23fb-4ef2-8bd5-e59a34f362cd')
    def test_filter_combo_78(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9d3fc46a-07b7-48ad-9a31-fcdba259c670')
    def test_filter_combo_79(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9ba5e905-634f-485b-829c-1ef79fa5f116')
    def test_filter_combo_80(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b275ff76-eec5-467b-b12d-7440ff588cec')
    def test_filter_combo_81(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='009ecb1c-2860-4a4e-867b-c712569ddfd1')
    def test_filter_combo_82(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='61c7da36-6b19-49d2-9981-120bb0b76372')
    def test_filter_combo_83(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4c22688a-4d03-4145-aa2f-f989832f8086')
    def test_filter_combo_84(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cc159f43-5619-46fe-b8ad-209a446f10c0')
    def test_filter_combo_85(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9a81d52e-cd46-4e2b-9ac1-ecebcc04d788')
    def test_filter_combo_86(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='938c404f-8dd8-46a5-afe4-f87559bb2c9d')
    def test_filter_combo_87(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4bc6a2db-e845-435d-8d8e-a990f4b1fcdc')
    def test_filter_combo_88(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e245fb6a-35fc-488f-ada6-393fe4a09134')
    def test_filter_combo_89(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4dd70ebf-ec85-4e95-a9f5-a10e1791293c')
    def test_filter_combo_90(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b0085042-0fd6-4ff3-af69-156f270953b1')
    def test_filter_combo_91(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6b9a539b-b6cc-46b1-a9a5-ef20808f5e74')
    def test_filter_combo_92(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1f18a94c-a72e-4912-a91a-0be96e708be4')
    def test_filter_combo_93(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9f2d3923-a932-40c8-b527-8baedcf3254c')
    def test_filter_combo_94(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='edf61fa9-b51f-41fd-b3ca-0035ee7dbd65')
    def test_filter_combo_95(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8b8adcf5-adb9-4a48-8570-4e1d2e6b47c6')
    def test_filter_combo_96(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cc7857e0-5a5b-468f-bf5e-dc1478716715')
    def test_filter_combo_97(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b4c9e01f-944c-4d8e-9a3f-49efaa22887c')
    def test_filter_combo_98(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='951e19cf-c138-4d8e-92e6-b42410b8114f')
    def test_filter_combo_99(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ad50f0c0-c19e-45b8-8fb2-95afe81f7620')
    def test_filter_combo_100(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a7fd36d6-77ec-453e-a67c-0c2fc78e572a')
    def test_filter_combo_101(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d661aafd-005d-4a31-88b0-a238e328b16d')
    def test_filter_combo_102(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7fe951d2-28c5-43a9-af79-c0fbf3a3388f')
    def test_filter_combo_103(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d802f38b-830f-4cd2-af2c-a44ba625a401')
    def test_filter_combo_104(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e1a30f67-1577-4cfb-9a0d-c07493a341b2')
    def test_filter_combo_105(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='774a6bf9-cfd6-40ef-8b91-3576f23eb01b')
    def test_filter_combo_106(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b85d0c78-69bc-42e3-ac78-61ad8176a1d0')
    def test_filter_combo_107(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='bd93c530-4ab0-4d9b-b202-ea6dd1c8a27d')
    def test_filter_combo_108(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4590bfbb-006f-46be-bd03-5afe8b81ac52')
    def test_filter_combo_109(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2626f60e-cb01-45a1-a23e-f1eaa85ac9ce')
    def test_filter_combo_110(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='24cf16ac-10a6-4a02-9b72-84c280fa77a2')
    def test_filter_combo_111(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6242aadb-028b-4932-9024-8b6d2148c458')
    def test_filter_combo_112(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5918fef7-0578-4999-b331-d2948e62e720')
    def test_filter_combo_113(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='63160fc2-306f-46a4-bf1f-b512642478c4')
    def test_filter_combo_114(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='849749ae-e5f3-4029-be92-66a1353ba165')
    def test_filter_combo_115(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5f150448-94f6-4f0b-a8da-0c4a78541a4f')
    def test_filter_combo_116(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='39af4eca-990a-4b3b-bcf2-1a840e8a9308')
    def test_filter_combo_117(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a59cb1fa-eb7d-4161-84a9-cda157b6b8c5')
    def test_filter_combo_118(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cfaf02e5-76e4-4593-849c-b63de4907638')
    def test_filter_combo_119(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9eb40c09-89ea-44e9-8514-e58cdce91779')
    def test_filter_combo_120(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e2d2a8d5-0554-49cc-9cc9-66e97378d260')
    def test_filter_combo_121(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cb72c86a-a7c6-4bf9-9eec-53f7d190a9f1')
    def test_filter_combo_122(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cb75e56e-a029-478d-8031-8de12f5fbebf')
    def test_filter_combo_123(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='277a98c4-4b1f-428d-8c10-8697a3fe1f0f')
    def test_filter_combo_124(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2d884bf2-c678-429c-8aee-3be78b3176ff')
    def test_filter_combo_125(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e6b5fcff-8a6e-4eb7-9070-74caf9e18349')
    def test_filter_combo_126(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='682ffa88-2d13-4d21-878e-c2a8a510cf71')
    def test_filter_combo_127(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a27d8f58-7523-404a-bf99-744afdb52aba')
    def test_filter_combo_128(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f2c77cf7-dc52-471d-b66d-54e72f7f7ea0')
    def test_filter_combo_129(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='3e21ad66-88fc-48ee-a698-6c475f478a86')
    def test_filter_combo_130(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='af046c81-524e-4016-b6a8-459538f320c2')
    def test_filter_combo_131(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f3aad5f8-6214-4c67-9a84-2da7171fb111')
    def test_filter_combo_132(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='16ab8d79-15ca-4ab3-b004-834edb4da37b')
    def test_filter_combo_133(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cb37c6a3-496f-49a6-b02a-552b8260205e')
    def test_filter_combo_134(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9e3ad4d0-4fab-4d85-9543-5e2c2fea79ec')
    def test_filter_combo_135(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='37f93abf-237e-4917-91a6-afa2629b5f98')
    def test_filter_combo_136(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1131c908-ddf2-4cdd-b1a2-9b73990e72c3')
    def test_filter_combo_137(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1b283fa0-485b-4f45-a353-36f9cdd6c123')
    def test_filter_combo_138(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e4bdc84e-413c-4a2b-9049-f5b04e32b5b7')
    def test_filter_combo_139(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8791e036-3f30-4a44-b3a8-23371da893a6')
    def test_filter_combo_140(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='27201535-9537-4e11-a1d7-1b1f5f01e213')
    def test_filter_combo_141(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e40e7b8f-44f0-4f87-8206-fea14d0fef52')
    def test_filter_combo_142(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0f369e78-c5ae-4cbc-8511-597cdc38b1ae')
    def test_filter_combo_143(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='174418c1-6938-4319-9d8b-361df3fc28f3')
    def test_filter_combo_144(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6bc94d17-b532-413b-86fc-185c194b430c')
    def test_filter_combo_145(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='3c47822e-5e74-4270-bcb4-72e3995bd5c5')
    def test_filter_combo_146(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='19515def-b28d-4ef7-bae7-c4f64940879a')
    def test_filter_combo_147(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a45abd7c-24ca-400c-b2d5-233431b07522')
    def test_filter_combo_148(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4492a245-c91f-4df1-a55b-57541ce410c8')
    def test_filter_combo_149(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e4c485af-66a0-413b-b70e-3396e130fffb')
    def test_filter_combo_150(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='31369cd6-feb7-47f3-9022-2d619c961ba7')
    def test_filter_combo_151(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5cf0da7f-a515-4f67-bae4-956d86275423')
    def test_filter_combo_152(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a78293f0-aee5-40d1-9c97-3fdda3ddd43e')
    def test_filter_combo_153(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='3fd7d0cb-6d98-4ca8-9a14-8ca23b6dae07')
    def test_filter_combo_154(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='19434f33-5bc5-427f-b332-36f85c997fe3')
    def test_filter_combo_155(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4195c9e1-b87c-4fa1-8039-ec0f2652e216')
    def test_filter_combo_156(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0536e50e-f33c-4772-b078-4f95231c3de6')
    def test_filter_combo_157(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='116dcfef-caae-496f-abfa-0863f2968f6f')
    def test_filter_combo_158(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='00f37533-9ca5-4c58-adb3-d3d709c7b215')
    def test_filter_combo_159(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d86e57da-29b5-445e-bf75-3e2843b9b739')
    def test_filter_combo_160(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='792736ce-8f43-4d21-b9b9-30d3bfd66b6a')
    def test_filter_combo_161(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': False,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ec387a8a-e7b2-4df7-9580-b09362c3dc4d')
    def test_filter_combo_162(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='93a8f3b0-0fb0-47bd-88fb-6dc847ac14e4')
    def test_filter_combo_163(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='283356e9-58ac-4edc-bf08-0bc9c7313053')
    def test_filter_combo_164(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ab4f0d84-58fd-4a2a-b3ed-128231f3e22f')
    def test_filter_combo_165(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='02bbbdd0-3c57-41c5-ab32-28185f33802c')
    def test_filter_combo_166(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f6c67a80-bede-4186-b7a1-09756b4c1a68')
    def test_filter_combo_167(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4fc73f9c-4826-4ff2-bba0-bc64cc469f3a')
    def test_filter_combo_168(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='58a6fafc-bbc5-466b-a586-310d9dfc14c1')
    def test_filter_combo_169(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='38de3927-212c-4948-bd46-cca1d09ead90')
    def test_filter_combo_170(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b03a34cf-c3e1-4954-9cb6-b5f1a59e94e9')
    def test_filter_combo_171(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='455ead9d-1e50-46e4-907c-c5b9bbbdcc9c')
    def test_filter_combo_172(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a7320afc-affb-4fa5-877d-7eb8bd1f8558')
    def test_filter_combo_173(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='163c5c85-bef7-4da6-8e8a-89b0656b71d0')
    def test_filter_combo_174(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='31147187-c5a9-4c2e-8be6-b79ff71cdaf3')
    def test_filter_combo_175(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='783b5756-ca16-4a17-b1f0-8a16ddc009c4')
    def test_filter_combo_176(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='924a107b-fe1c-4b9d-b29b-47c3b2df1de3')
    def test_filter_combo_177(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='096a6596-fd8c-4d8c-88c6-45903047fe2c')
    def test_filter_combo_178(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e37b2083-a9d0-4337-aa11-d9205c15f456')
    def test_filter_combo_179(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='3de8087b-7f25-4cda-8f07-fa9326524deb')
    def test_filter_combo_180(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f218bf14-0a6e-4c5f-b151-3ac9719ca1a2')
    def test_filter_combo_181(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1a13171d-b9a9-4b42-8cec-5c5841c4f3a5')
    def test_filter_combo_182(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='53311ac1-239f-4033-aaa6-084523916fc6')
    def test_filter_combo_183(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f8bb89b2-2dae-4d41-9d19-6c9af0fe6da8')
    def test_filter_combo_184(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4747ab09-8d62-4866-80e6-c9b8e4cf5061')
    def test_filter_combo_185(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='be6db894-57dd-452a-8f08-3ce462ac9417')
    def test_filter_combo_186(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2070329b-b7c8-4958-af9c-2e1044b71564')
    def test_filter_combo_187(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1a428d9e-46fd-4bd2-a12c-25c89ead74b1')
    def test_filter_combo_188(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': True,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f07220c2-c0a9-471a-871d-a87931feb278')
    def test_filter_combo_189(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b3af8fb0-cd93-4ab0-b8f3-4111969c7cbb')
    def test_filter_combo_190(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b630cc10-4cda-487d-ab84-599963c172d7')
    def test_filter_combo_191(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9cdbe1df-e3e1-45e4-b816-96b8a6efb90f')
    def test_filter_combo_192(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='10cd2757-a182-419f-9512-8b536539a134')
    def test_filter_combo_193(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5c7af5b2-8a2c-4c2d-911f-dad8216d849f')
    def test_filter_combo_194(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0acb37c7-0ece-4f5b-9294-014dd7fcb3ed')
    def test_filter_combo_195(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='14f07972-6403-49be-8eed-ce7294e33d32')
    def test_filter_combo_196(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f2db7e80-d3ea-4beb-8e09-24ae33904716')
    def test_filter_combo_197(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='30c1303f-956f-4b2d-af6a-3570aa4567fd')
    def test_filter_combo_198(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8e210748-f842-4fa1-ac40-2bfd291f08a1')
    def test_filter_combo_199(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='756c687d-8183-4bc9-90dc-4e46a5579fca')
    def test_filter_combo_200(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8f904946-a9c7-4f7a-af96-0d22c5592709')
    def test_filter_combo_201(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='abdfce9f-3529-435b-8fdc-9dd7bf0fc01c')
    def test_filter_combo_202(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='71289aa2-f74d-44b7-ad18-515a3c438a15')
    def test_filter_combo_203(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='223db6ca-7190-4f3f-89cc-92dcc2dcd109')
    def test_filter_combo_204(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8a61a11e-fc25-4e28-928d-e2be2d95af63')
    def test_filter_combo_205(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8c62bc17-6998-4ec8-923b-e29fe1693ae3')
    def test_filter_combo_206(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': [1, 2],
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='85f168fa-a868-4940-aef1-de063a497083')
    def test_filter_combo_207(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7697ca45-fc34-4f8c-9b61-58cc7d4f3321')
    def test_filter_combo_208(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f5221c06-c006-4363-ab9c-90b9d8c31b43')
    def test_filter_combo_209(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b7119a2d-6269-408c-ae69-39ebad1e4192')
    def test_filter_combo_210(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2025037f-433b-4167-a0c9-3265a53fe6ba')
    def test_filter_combo_211(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ecf2c84b-88b0-48d9-8a4f-3660f039cd97')
    def test_filter_combo_212(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_2,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='844a6bcb-cd5a-4023-9af7-cab68ed2e847')
    def test_filter_combo_213(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_medium
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b7e5ff94-93e0-45c9-a160-33628f5fcf9e')
    def test_filter_combo_214(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b9781df2-59e4-44ba-9a98-1d35670a6f63')
    def test_filter_combo_215(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_3,
            'service_data_uuid': self.service_uuid_1,
            'manufacturer_specific_data': self.manu_specific_data_small_3,
            'include_tx_power_level': False,
            'include_device_name': True,
            'service_data': self.service_data_small_2
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4ad156f8-8d80-4635-b6d8-7bca07d8a899')
    def test_filter_combo_216(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5b68c344-bfd1-44cb-9add-c81122d6b04f')
    def test_filter_combo_217(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='355ca57c-998c-4e7e-b0d2-66854b5192bb')
    def test_filter_combo_218(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a9ec0f76-6b8f-42e0-8310-07c02de49d9d')
    def test_filter_combo_219(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='73f448ff-e9f6-4608-80ba-92131485234f')
    def test_filter_combo_220(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='55ec509d-8cdd-4ab5-8e57-2ccadd5f8c0d')
    def test_filter_combo_221(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1f6835fd-b33b-4be3-b133-b77f6c9872c8')
    def test_filter_combo_222(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='62d483c0-7d08-4b7c-9b1f-3b0324006554')
    def test_filter_combo_223(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='01bcb867-3f39-4aef-baf5-50b439768b43')
    def test_filter_combo_224(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='faab6211-4408-4272-93d9-7b09b8c3b8cd')
    def test_filter_combo_225(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a0f9ad8d-c00a-4420-9205-eeb081bf2b35')
    def test_filter_combo_226(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7a2e8186-e8b0-4956-b8bc-fb2ba91b8f67')
    def test_filter_combo_227(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='414b5464-b135-453b-acf3-aebc728d0366')
    def test_filter_combo_228(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='580e3ff8-4648-402e-a531-ddb85bbf4c89')
    def test_filter_combo_229(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7fe0c829-94b5-4e88-aa92-47159c1eb232')
    def test_filter_combo_230(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='93c1747d-4b76-4faf-9efc-4ca40e751f08')
    def test_filter_combo_231(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='96f7e662-6f74-407a-b1d8-e29ac3405ff4')
    def test_filter_combo_232(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cde6415b-1138-4913-ab4f-542d4057542d')
    def test_filter_combo_233(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4ca75288-0af8-462b-9146-022b9f915b1f')
    def test_filter_combo_234(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4db5dae2-f974-4208-9c01-84ca050f8fc3')
    def test_filter_combo_235(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c9f7abf0-b333-4500-9fd0-e4678574cf18')
    def test_filter_combo_236(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b8176ca4-478c-49c6-a638-4f53d7d2720c')
    def test_filter_combo_237(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7e84c371-f28e-4995-86a9-bb99a4a29d0c')
    def test_filter_combo_238(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='3b8eb500-6885-4273-9c53-f7930896e895')
    def test_filter_combo_239(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f80ef69f-d71c-4c94-893d-363cf5a658f6')
    def test_filter_combo_240(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='dca1ab9d-7923-4917-8a82-1917dbad4923')
    def test_filter_combo_241(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='36faab09-d874-4f24-b7ea-8985d60dc4c3')
    def test_filter_combo_242(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c94b7e3b-064c-4885-a4aa-899e83d0e754')
    def test_filter_combo_243(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f6680820-8843-47e4-b4fb-0ee1b76d51f8')
    def test_filter_combo_244(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4b1bf9a9-7761-435a-8c6c-511b20312c04')
    def test_filter_combo_245(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5c5b0147-cacd-46f0-a6b7-ff8b37cf985b')
    def test_filter_combo_246(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c31e26f2-2e82-40bc-a9a5-5ae059b702d8')
    def test_filter_combo_247(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': True,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5a1a4438-6bb3-4acc-85ab-b48432c340db')
    def test_filter_combo_248(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0dd54bff-d170-441f-81ae-bc11f7c8491b')
    def test_filter_combo_249(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='98d878cf-1548-4e79-9527-8741e5b523d0')
    def test_filter_combo_250(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ec0c9a9b-3df3-4cba-84fc-9a17a11b1be7')
    def test_filter_combo_251(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='eabddddc-29c7-4804-93a8-c259957538ae')
    def test_filter_combo_252(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ee0abe8f-254e-4802-b808-4aa8e0306203')
    def test_filter_combo_253(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9482cda2-a6f5-4dff-8809-6dfabaaf9f71')
    def test_filter_combo_254(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='279f54ee-975b-4edc-a6c8-f018e45846c3')
    def test_filter_combo_255(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a3508588-ca01-4063-ae7e-c845ac4a595b')
    def test_filter_combo_256(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8bde1746-dec8-4b17-93ba-90448addcb13')
    def test_filter_combo_257(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9ae7e798-0981-4501-9302-54553c76a54c')
    def test_filter_combo_258(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='28e1efdc-1c5f-44d8-8650-02720db32048')
    def test_filter_combo_259(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a309d9d0-bf5e-4878-b6bb-89d3c388d5b2')
    def test_filter_combo_260(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b0db2d76-8039-4257-bed0-e5e154a5874f')
    def test_filter_combo_261(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='06ecd167-4dbc-4a8c-9c1c-2daea87f7a51')
    def test_filter_combo_262(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='12f6c002-8627-4477-8e5a-6d7b5335ed60')
    def test_filter_combo_263(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['balanced']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ccd43a09-cb39-4d84-8ffc-99ad8449783b')
    def test_filter_combo_264(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f9d9abad-e543-4996-b369-09fddd9c4965')
    def test_filter_combo_265(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2a14e619-23a6-4a49-acd6-e712b026d75b')
    def test_filter_combo_266(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2ed25b96-54fd-4a81-b8d1-732b959aff8d')
    def test_filter_combo_267(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='0ef9198b-78ac-4fa6-afe2-cc87007c2c0d')
    def test_filter_combo_268(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='43d97df2-07d7-4c45-bb83-908746e60923')
    def test_filter_combo_269(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='55262e57-7b47-45a3-8926-18cea480c2b2')
    def test_filter_combo_270(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5ece141d-43ad-448c-900c-500666cb0e1c')
    def test_filter_combo_271(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['opportunistic'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='66d3c0de-7e3b-4108-9ab0-3e101c6a14cd')
    def test_filter_combo_272(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9703d50a-8b23-4d42-8ed3-9b0704dac9d2')
    def test_filter_combo_273(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a5739a21-0e1b-4ba7-b259-acb7b38a8e09')
    def test_filter_combo_274(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c399011c-54c0-47a1-9e05-a52c2190f89d')
    def test_filter_combo_275(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['balanced'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7854fdcc-5771-463a-91da-5b394484b065')
    def test_filter_combo_276(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['high'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='fa0fe141-c99f-4228-b249-96232194e740')
    def test_filter_combo_277(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d2143fe1-bbec-429a-8241-19f39361b490')
    def test_filter_combo_278(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['ultra_low'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='21d025ef-2f89-49fd-bb31-2130dbe83c5c')
    def test_filter_combo_279(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_tx_powers['medium'],
            'is_connectable': False,
            'scan_mode': ble_scan_settings_modes['low_power'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='19c5f91d-e10a-43af-8727-c66ee43187f2')
    def test_filter_combo_280(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_tx_power_level': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7bda5df3-2644-46ca-b6de-e3d5557395cf')
    def test_filter_combo_281(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'filter_device_address': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a80f6a40-9a60-4d68-b5e1-66d6e157cdd8')
    def test_filter_combo_282(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='503bfb94-cfb8-4194-b451-23f19aff7b8e')
    def test_filter_combo_283(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'manufacturer_specific_data': self.manu_sepecific_data_large
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9bae0612-559b-460f-9723-fac896974835')
    def test_filter_combo_284(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'manufacturer_specific_data_mask': [1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f1ad0e3a-17cd-4e06-a395-7e5dde2268b4')
    def test_filter_combo_285(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '0000110A-0000-1000-8000-00805F9B34FB',
            'service_data': self.service_data_large
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='84f29360-9219-4f39-8ead-b43779c65504')
    def test_filter_combo_286(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '0000110B-0000-1000-8000-00805F9B34FB',
            'service_data': [13]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='caece017-8379-46a3-913b-a21d3057e096')
    def test_filter_combo_287(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '0000110C-0000-1000-8000-00805F9B34FB',
            'service_data': [11, 14, 50]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='37d63f4e-ed0c-4003-8044-f7032238a449')
    def test_filter_combo_288(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '0000110D-0000-1000-8000-00805F9B34FB',
            'service_data': [16, 22, 11]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7a57c0d7-1b8d-44e7-b407-7a6c58095058')
    def test_filter_combo_289(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '0000110E-0000-1000-8000-00805F9B34FB',
            'service_data': [2, 9, 54]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='38f80b83-2aba-40f4-9238-7e108acea1e4')
    def test_filter_combo_290(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '0000110F-0000-1000-8000-00805F9B34FB',
            'service_data': [69, 11, 50]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='adec5d6d-c1f2-46d0-8b05-2c46c02435a6')
    def test_filter_combo_291(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '00001101-0000-1000-8000-00805F9B34FB',
            'service_data': [12, 11, 21]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='cd3cbc57-80a6-43d8-8042-9f163beda73a')
    def test_filter_combo_292(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '00001102-0000-1000-8000-00805F9B34FB',
            'service_data': [12, 12, 44]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='35d9ab80-1ceb-4b45-ae9e-304c413f9273')
    def test_filter_combo_293(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '00001103-0000-1000-8000-00805F9B34FB',
            'service_data': [4, 54, 1]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='07cecc9f-6e72-407e-a11d-c982f92c1834')
    def test_filter_combo_294(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '00001104-0000-1000-8000-00805F9B34FB',
            'service_data': [33, 22, 44]
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8c0de318-4c57-47c3-8068-d1b0fde7f448')
    def test_filter_combo_295(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_uuid': '00000000-0000-1000-8000-00805f9b34fb',
            'service_mask': '00000000-0000-1000-8000-00805f9b34fb'
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8fbd96a9-5844-4714-8f63-5b92432d23d1')
    def test_filter_combo_296(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_uuid': 'FFFFFFFF-0000-1000-8000-00805f9b34fb',
            'service_mask': '00000000-0000-1000-8000-00805f9b34fb'
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d127b973-46ca-4a9f-a1e1-5cda6affaa53')
    def test_filter_combo_297(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_uuid': '3846D7A0-69C8-11E4-BA00-0002A5D5C51B',
            'service_mask': '00000000-0000-1000-8000-00805f9b34fb'
        }
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='efaad273-f953-43ca-b4f6-f9eba10d3ba5')
    def test_filter_combo_298(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='373ba3e8-01e8-4c26-ad7f-7b7ba69d1a70')
    def test_filter_combo_299(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_tx_power_level': True}
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e1848bba-b9a6-473b-bceb-101b14b4ccc1')
    def test_filter_combo_300(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'filter_device_address': True}
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='3a6068a5-0dd1-4503-b25a-79bc0f4a7006')
    def test_filter_combo_301(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c54e14e7-c5f6-4c16-9900-2b8ac9ee96a5')
    def test_filter_combo_302(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'manufacturer_specific_data': self.manu_sepecific_data_large
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='bb188de9-8c63-4eba-96ab-b8577001412d')
    def test_filter_combo_303(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'manufacturer_specific_data_id': self.manu_specific_data_id_1,
            'manufacturer_specific_data': self.manu_sepecific_data_small,
            'manufacturer_specific_data_mask': [1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4e42416b-fe86-41e7-99cd-3ea0ab61a027')
    def test_filter_combo_304(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '0000110A-0000-1000-8000-00805F9B34FB',
            'service_data': self.service_data_large
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a519609b-cd95-4017-adac-86954153669e')
    def test_filter_combo_305(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '0000110B-0000-1000-8000-00805F9B34FB',
            'service_data': [13]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ad1f5bdd-b532-482c-8f62-cc6804f0f8a2')
    def test_filter_combo_306(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '0000110C-0000-1000-8000-00805F9B34FB',
            'service_data': [11, 14, 50]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='a44af1a3-f5ac-419b-a11b-a72734b57fa7')
    def test_filter_combo_307(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '0000110D-0000-1000-8000-00805F9B34FB',
            'service_data': [16, 22, 11]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1b2b17e7-5a1a-4795-974d-3a239c7fccc8')
    def test_filter_combo_308(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '0000110E-0000-1000-8000-00805F9B34FB',
            'service_data': [2, 9, 54]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9e9944cc-a85c-4077-9129-ca348a6c0286')
    def test_filter_combo_309(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '0000110F-0000-1000-8000-00805F9B34FB',
            'service_data': [69, 11, 50]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e0bb52ea-ac8f-4951-bd00-5322d0e72fd2')
    def test_filter_combo_310(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '00001101-0000-1000-8000-00805F9B34FB',
            'service_data': [12, 11, 21]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='719d258d-6556-47b6-92d6-224c691b5dfd')
    def test_filter_combo_311(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '00001102-0000-1000-8000-00805F9B34FB',
            'service_data': [12, 12, 44]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1ab27561-6e2d-4da8-b2b1-dc4bd2c42f97')
    def test_filter_combo_312(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '00001103-0000-1000-8000-00805F9B34FB',
            'service_data': [4, 54, 1]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5b460a48-f6d6-469c-9553-11817171dacb')
    def test_filter_combo_313(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_data_uuid': '00001104-0000-1000-8000-00805F9B34FB',
            'service_data': [33, 22, 44]
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e0439501-b72d-43ac-a51f-c44b4d0c86d9')
    def test_filter_combo_314(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_uuid': '00000000-0000-1000-8000-00805f9b34fb',
            'service_mask': '00000000-0000-1000-8000-00805f9b34fb'
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='3a7f4527-2a77-4172-8402-78d90fbc5a8a')
    def test_filter_combo_315(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_uuid': 'FFFFFFFF-0000-1000-8000-00805f9b34fb',
            'service_mask': '00000000-0000-1000-8000-00805f9b34fb'
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='c6661021-33ad-4628-99f0-1a3b4b4a8263')
    def test_filter_combo_316(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {
            'service_uuid': '3846D7A0-69C8-11E4-BA00-0002A5D5C51B',
            'service_mask': '00000000-0000-1000-8000-00805f9b34fb'
        }
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='3a633941-1716-4bf6-a8d7-8a4ad0be24aa')
    def test_filter_combo_317(self):
        """Test a combination scan filter and advertisement

        Test that an advertisement is found and matches corresponding
        settings.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {'include_device_name': True}
        settings_in_effect = {
            'scan_mode': ble_scan_settings_modes['low_latency'],
            'mode': ble_advertise_settings_modes['low_latency']
        }
        return self._magic((filters, settings_in_effect))

    def _blescan_verify_onscanresult_event(self, event, filters):
        test_result = True
        self.log.debug("Verifying onScanResult event: {}".format(event))
        callback_type = event['data']['CallbackType']
        if 'callback_type' in filters.keys():
            if filters['callback_type'] != callback_type:
                self.log.error("Expected callback type: {}, Found callback "
                               "type: {}".format(filters['callback_type'],
                                                 callback_type))
            test_result = False
        elif self.default_callback != callback_type:
            self.log.error("Expected callback type: {}, Found callback type: "
                           "{}".format(self.default_callback, callback_type))
            test_result = False
        if 'include_device_name' in filters.keys() and filters[
                'include_device_name'] is not False:
            if event['data']['Result']['deviceName'] != filters[
                    'include_device_name']:
                self.log.error(
                    "Expected device name: {}, Found device name: {}"
                    .format(filters['include_device_name'], event['data'][
                        'Result']['deviceName']))

                test_result = False
        elif 'deviceName' in event['data']['Result'].keys():
            self.log.error(
                "Device name was found when it wasn't meant to be included.")
            test_result = False
        if ('include_tx_power_level' in filters.keys() and
                filters['include_tx_power_level'] is not False):
            if not event['data']['Result']['txPowerLevel']:
                self.log.error(
                    "Expected to find tx power level in event but found none.")
                test_result = False
        if not event['data']['Result']['rssi']:
            self.log.error("Expected rssi in the advertisement, found none.")
            test_result = False
        if not event['data']['Result']['timestampNanos']:
            self.log.error("Expected rssi in the advertisement, found none.")
            test_result = False
        return test_result

    def _bleadvertise_verify_onsuccess(self, event, settings_in_effect):
        self.log.debug("Verifying {} event".format(adv_succ))
        test_result = True
        if 'is_connectable' in settings_in_effect.keys():
            if (event['data']['SettingsInEffect']['isConnectable'] !=
                    settings_in_effect['is_connectable']):
                self.log.error("Expected is connectable value: {}, Actual is "
                               "connectable value:".format(settings_in_effect[
                                   'is_connectable'], event['data'][
                                       'SettingsInEffect']['isConnectable']))
                test_result = False
        elif (event['data']['SettingsInEffect']['isConnectable'] !=
              self.default_is_connectable):
            self.log.error(
                "Default value for isConnectable did not match what was found.")
            test_result = False
        if 'mode' in settings_in_effect.keys():
            if (event['data']['SettingsInEffect']['mode'] !=
                    settings_in_effect['mode']):
                self.log.error("Expected mode value: {}, Actual mode value: {}"
                               .format(settings_in_effect['mode'], event[
                                   'data']['SettingsInEffect']['mode']))
                test_result = False
        elif (event['data']['SettingsInEffect']['mode'] !=
              self.default_advertise_mode):
            self.log.error(
                "Default value for filtering mode did not match what was "
                "found.")
            test_result = False
        if 'tx_power_level' in settings_in_effect.keys():
            if (event['data']['SettingsInEffect']['txPowerLevel'] ==
                    java_integer['min']):
                self.log.error("Expected tx power level was not meant to be: "
                               "{}".format(java_integer['min']))
                test_result = False
        elif (event['data']['SettingsInEffect']['txPowerLevel'] !=
              self.default_tx_power_level):
            self.log.error(
                "Default value for tx power level did not match what"
                " was found.")
            test_result = False
        return test_result

    def _magic(self, params):
        (filters, settings_in_effect) = params
        test_result = True

        self.log.debug("Settings in effect: {}".format(
            pprint.pformat(settings_in_effect)))
        self.log.debug("Filters:".format(pprint.pformat(filters)))
        if 'is_connectable' in settings_in_effect.keys():
            self.log.debug("Setting advertisement is_connectable to {}".format(
                settings_in_effect['is_connectable']))
            self.adv_ad.droid.bleSetAdvertiseSettingsIsConnectable(
                settings_in_effect['is_connectable'])
        if 'mode' in settings_in_effect.keys():
            self.log.debug("Setting advertisement mode to {}"
                           .format(settings_in_effect['mode']))
            self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
                settings_in_effect['mode'])
        if 'tx_power_level' in settings_in_effect.keys():
            self.log.debug("Setting advertisement tx_power_level to {}".format(
                settings_in_effect['tx_power_level']))
            self.adv_ad.droid.bleSetAdvertiseSettingsTxPowerLevel(
                settings_in_effect['tx_power_level'])
        filter_list = self.scn_ad.droid.bleGenFilterList()
        if ('include_device_name' in filters.keys() and
                filters['include_device_name'] is not False):

            self.log.debug("Setting advertisement include_device_name to {}"
                           .format(filters['include_device_name']))
            self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
            filters['include_device_name'] = (
                self.adv_ad.droid.bluetoothGetLocalName())
            self.log.debug("Setting scanner include_device_name to {}".format(
                filters['include_device_name']))
            self.scn_ad.droid.bleSetScanFilterDeviceName(filters[
                'include_device_name'])
        else:
            self.log.debug(
                "Setting advertisement include_device_name to False")
            self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(False)
        if ('include_tx_power_level' in filters.keys() and
                filters['include_tx_power_level'] is not False):
            self.log.debug(
                "Setting advertisement include_tx_power_level to True")
            self.adv_ad.droid.bleSetAdvertiseDataIncludeTxPowerLevel(True)
        if 'manufacturer_specific_data_id' in filters.keys():
            if 'manufacturer_specific_data_mask' in filters.keys():
                self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(
                    filters['manufacturer_specific_data_id'],
                    filters['manufacturer_specific_data'])
                self.scn_ad.droid.bleSetScanFilterManufacturerData(
                    filters['manufacturer_specific_data_id'],
                    filters['manufacturer_specific_data'],
                    filters['manufacturer_specific_data_mask'])
            else:
                self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(
                    filters['manufacturer_specific_data_id'],
                    filters['manufacturer_specific_data'])
                self.scn_ad.droid.bleSetScanFilterManufacturerData(
                    filters['manufacturer_specific_data_id'],
                    filters['manufacturer_specific_data'])
        if 'service_data' in filters.keys():
            self.adv_ad.droid.bleAddAdvertiseDataServiceData(
                filters['service_data_uuid'], filters['service_data'])
            self.scn_ad.droid.bleSetScanFilterServiceData(
                filters['service_data_uuid'], filters['service_data'])
        if 'manufacturer_specific_data_list' in filters.keys():
            for pair in filters['manufacturer_specific_data_list']:
                (manu_id, manu_data) = pair
                self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(manu_id,
                                                                    manu_data)
        if 'service_mask' in filters.keys():
            self.scn_ad.droid.bleSetScanFilterServiceUuid(
                filters['service_uuid'].upper(), filters['service_mask'])
            self.adv_ad.droid.bleSetAdvertiseDataSetServiceUuids(
                [filters['service_uuid'].upper()])
        elif 'service_uuid' in filters.keys():
            self.scn_ad.droid.bleSetScanFilterServiceUuid(filters[
                'service_uuid'])
            self.adv_ad.droid.bleSetAdvertiseDataSetServiceUuids(
                [filters['service_uuid']])
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        if ('scan_mode' in settings_in_effect and
                settings_in_effect['scan_mode'] !=
                ble_scan_settings_modes['opportunistic']):
            self.scn_ad.droid.bleSetScanSettingsScanMode(settings_in_effect[
                'scan_mode'])
        else:
            self.scn_ad.droid.bleSetScanSettingsScanMode(
                ble_scan_settings_modes['low_latency'])
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback = self.scn_ad.droid.bleGenScanCallback()
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        opportunistic = False
        scan_settings2, scan_callback2 = None, None
        if ('scan_mode' in settings_in_effect and
                settings_in_effect['scan_mode'] ==
                ble_scan_settings_modes['opportunistic']):
            opportunistic = True
            scan_settings2 = self.scn_ad.droid.bleBuildScanSetting()
            scan_callback2 = self.scn_ad.droid.bleGenScanCallback()
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings2,
                                              scan_callback2)
            self.scn_ad.droid.bleSetScanSettingsScanMode(
                ble_scan_settings_modes['opportunistic'])
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        regex = "(" + adv_succ.format(
            advertise_callback) + "|" + adv_fail.format(
                advertise_callback) + ")"
        self.log.debug(regex)
        try:
            event = self.adv_ad.ed.pop_events(regex, self.default_timeout,
                                              small_timeout)
        except Empty:
            self.adv_ad.log.error("Failed to get success or failed event.")
            return False
        if event[0]["name"] == adv_succ.format(advertise_callback):
            if not self._bleadvertise_verify_onsuccess(event[0],
                                                       settings_in_effect):
                return False
            else:
                self.adv_ad.log.info("Advertisement started successfully.")
        else:
            self.adv_ad.log.error("Failed to start advertisement: {}".format(
                event[0]["data"]["Error"]))
        expected_scan_event_name = scan_result.format(scan_callback)
        try:
            event = self.scn_ad.ed.pop_event(expected_scan_event_name,
                                             self.default_timeout)
        except Empty:
            self.log.error("Scan event not found: {}".format(
                expected_scan_event_name))
            return False
        if not self._blescan_verify_onscanresult_event(event, filters):
            return False
        if opportunistic:
            expected_scan_event_name = scan_result.format(scan_callback2)
            try:
                event = self.scn_ad.ed.pop_event(expected_scan_event_name,
                                                 self.default_timeout)
            except Empty:
                self.log.error("Opportunistic scan event not found: {}".format(
                    expected_scan_event_name))
                return False
            if not self._blescan_verify_onscanresult_event(event, filters):
                return False
            self.scn_ad.droid.bleStopBleScan(scan_callback2)
        self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        return test_result
