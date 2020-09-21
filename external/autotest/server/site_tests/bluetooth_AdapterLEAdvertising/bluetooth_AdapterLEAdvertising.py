# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Server side bluetooth tests on adapter ble advertising.

The Mnemonics describing the test cases:
    CD: check advertising duration and intervals
    RA: register advertisements
    UA: unregister advertisements
    SI: set advertising intervals
    RS: reset advertising
    FRA: fail to register extra advertisements when max ones
         have been registered.
    FSI: fail to set advertising intervals beyond legitimate range
         of [20 ms, 10,240 ms].
    PC: power cycle the bluetooth adapter (controller).
    SR: suspend and resume the DUT (chromebook)

A test represents a component of a test case which comprises a
sequence of tests. A test case usually requires a tester (user)
to perform a sequence of actions and make a sequence of
observations if the test case is performed manually.

A test consists of an optional action such as "register n
advertisements" and a number of test criteria such as "verifying
if min advertising interval is set to an expected value" or
"verifying if advertising is disabled".

"""

import copy
import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.bluetooth import bluetooth_adapter_tests
from autotest_lib.server.cros.multimedia import bluetooth_le_facade_adapter


test_case_log = bluetooth_adapter_tests.test_case_log
UNSUPPORTED_KERNEL = "3.8.11"


class bluetooth_AdapterLEAdvertising(
        bluetooth_adapter_tests.BluetoothAdapterTests):
    """Server side bluetooth adapter advertising Test.

    This class comprises a number of test cases to verify
    bluetooth low-energy advertising.

    Refer to BluetoothAdapterTests for the implementation of the tests
    performed in this autotest test.

    Refer to the design doc for more details:
    "Test Cases for Bluetooth Low-Energy Advertising".

    """

    @staticmethod
    def get_instance_ids(advertisements):
        """Get the list of instace IDs starting at 1.

        @param advertisements: a list of advertisements.

        """
        return range(1, len(advertisements) + 1)


    def register_advertisements(self, advertisements, min_adv_interval_ms,
                                max_adv_interval_ms, instance_ids=None):
        """Register multiple advertisements continuously.

        @param advertisements: a list of advertisement instances.
        @param min_adv_interval_ms: min_adv_interval in milliseconds.
        @param max_adv_interval_ms: max_adv_interval in milliseconds.
        @param instance_ids: the list of instance IDs to register.

        """
        if instance_ids is None:
            instance_ids = self.get_instance_ids(advertisements)

        for instance_id, advertisement in zip(instance_ids, advertisements):
            self.test_register_advertisement(advertisement,
                                             instance_id,
                                             min_adv_interval_ms,
                                             max_adv_interval_ms)


    def unregister_advertisements(self, advertisements, instance_ids=None):
        """Register multiple advertisements.

        @param advertisements: a list of advertisement instances.
        @param min_adv_interval_ms: min_adv_interval in milliseconds.
        @param max_adv_interval_ms: max_adv_interval in milliseconds.
        @param instance_ids: the list of instance IDs to unregister.

        """
        if instance_ids is None:
            instance_ids = self.get_instance_ids(advertisements)

        count = 0
        number_advs = len(advertisements)
        for instance_id, advertisement in zip(instance_ids, advertisements):
            # Advertising is only disabled at the removal of the
            # last advertisement.
            count += 1
            advertising_disabled = count == number_advs
            self.test_unregister_advertisement(advertisement,
                                               instance_id,
                                               advertising_disabled)

    def get_kernel_version(self, host):
        """Get the kernel version of the DUT.

        @param host: DUT host

        @returns: kernel version
        """
        kernel_command = "uname -r"
        kernel_version = self.host.run(kernel_command).stdout.strip()
        return kernel_version


    # ---------------------------------------------------------------
    # Definitions of all test cases
    # ---------------------------------------------------------------


    @test_case_log
    def test_case_SI200_RA3_CD_UA3(self):
        """Test Case: SI(200) - RA(3) - CD - UA(3)"""
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = self.three_advertisements

        self.test_reset_advertising()

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.register_advertisements(advertisements, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        self.unregister_advertisements(advertisements)


    @test_case_log
    def test_case_SI200_RA3_CD_RA1_CD_UA1_CD_UA3(self):
        """Test Case: SI(200) - RA(3) - CD - RA(1) - CD - UA(1) - CD - UA(3)"""
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        # Make a copy of advertisements since we are going to modify it.
        advertisements = copy.copy(self.three_advertisements)
        number_advs = len(advertisements)
        one_more_advertisement = [self.sixth_advertisement]

        self.test_reset_advertising()

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.register_advertisements(advertisements, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        # Register one more advertisement.
        # The instance ID to register is len(advertisements) + 1 = 4
        self.register_advertisements(one_more_advertisement,
                                     new_min_adv_interval_ms,
                                     new_max_adv_interval_ms,
                                     instance_ids=[number_advs + 1])

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs + 1)

        # Unregister the 3rd advertisement.
        # After removing the advertisement, the remaining instance IDs
        # would be [1, 2, 4]
        instance_id = 3
        self.test_unregister_advertisement(advertisements.pop(instance_id - 1),
                                           instance_id,
                                           advertising_disabled=False)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        # Unregister all existing advertisements which are [1, 2, 4]
        # since adv 3 was removed in the previous step.
        self.unregister_advertisements(advertisements + one_more_advertisement,
                                       instance_ids=[1, 2, 4])


    @test_case_log
    def test_case_SI200_RA3_CD_RS(self):
        """Test Case: SI(200) - RA(3) - CD - RS"""
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = self.three_advertisements
        number_advs = len(advertisements)

        self.test_reset_advertising()

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.register_advertisements(advertisements, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        self.test_reset_advertising(self.get_instance_ids(advertisements))


    @test_case_log
    def test_case_SI200_RA3_CD_UA1_CD_RS(self):
        """Test Case: SI(200) - RA(3) - CD - UA(1) - CD - RS"""
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        # Make a copy of advertisements since we are going to modify it.
        advertisements = copy.copy(self.three_advertisements)

        self.test_reset_advertising()

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.register_advertisements(advertisements, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        # Unregister the 1st advertisement.
        # After removing the advertisement, the remaining instance IDs
        # would be [2, 3]
        instance_id = 1
        self.test_unregister_advertisement(advertisements.pop(instance_id - 1),
                                           instance_id,
                                           advertising_disabled=False)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements) - 1)

        self.test_reset_advertising([2, 3])


    @test_case_log
    def test_case_SI200_RA3_CD_UA1_CD_RA2_CD_UA4(self):
        """Test Case: SI(200) - RA(3) - CD - UA(1) - CD - RA(2) - CD - UA(4)"""
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        # Make a copy of three_advertisements since we are going to modify it.
        advertisements1 = copy.copy(self.three_advertisements)
        advertisements2 = self.two_advertisements
        number_advs1 = len(advertisements1)
        number_advs2 = len(advertisements2)

        self.test_reset_advertising()

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.register_advertisements(advertisements1, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs1)

        # Unregister the 2nd advertisement.
        # After removing the 2nd advertisement, the remaining instance IDs
        # would be [1, 3]
        instance_id = 2
        self.test_unregister_advertisement(advertisements1.pop(instance_id - 1),
                                           instance_id,
                                           advertising_disabled=False)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs1 - 1)

        # Register two more advertisements.
        # The instance IDs to register would be [2, 4]
        self.register_advertisements(advertisements2, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms,
                                     instance_ids=[2, 4])

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs1 + number_advs2 - 1)

        # Unregister all advertisements.
        # The instance_ids of advertisements1 is [1, 3].
        # The instance_ids of advertisements2 is [2, 4].
        self.unregister_advertisements(advertisements1 + advertisements2,
                                       instance_ids=[1, 3, 2, 4])


    @test_case_log
    def test_case_SI200_RA5_CD_FRA1_CD_UA5(self):
        """Test Case: SI(200) - RA(5) - CD - FRA(1) - CD - UA(5)"""
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = self.five_advertisements
        extra_advertisement = self.sixth_advertisement
        number_advs = len(advertisements)

        self.test_reset_advertising()

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.register_advertisements(advertisements, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        self.test_fail_to_register_advertisement(extra_advertisement,
                                                 new_min_adv_interval_ms,
                                                 new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        self.unregister_advertisements(advertisements)


    @test_case_log
    def test_case_SI200_RA3_CD_PC_CD_UA3(self):
        """Test Case: SI(200) - RA(3) - CD - PC - CD - UA(3)"""
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = self.three_advertisements

        self.test_reset_advertising()

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.register_advertisements(advertisements, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        # Turn off and then turn on the adapter.
        self.test_power_off_adapter()
        time.sleep(1)
        self.test_power_on_adapter()

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        self.unregister_advertisements(advertisements)


    @test_case_log
    def test_case_SI200_RA3_CD_SR_CD_UA3(self):
        """Test Case: SI(200) - RA(3) - CD - SR - CD - UA(3)"""
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = self.three_advertisements

        self.test_reset_advertising()

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.register_advertisements(advertisements, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        # Suspend for a while and resume.
        self.suspend_resume()

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        self.unregister_advertisements(advertisements)


    @test_case_log
    def test_case_RA3_CD_SI200_CD_UA3(self):
        """Test Case: RA(3) - CD - SI(200) - CD - UA(3)"""
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = self.three_advertisements
        number_advs = len(advertisements)

        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               number_advs)

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        self.unregister_advertisements(advertisements)


    @test_case_log
    def test_case_RA3_CD_SI200_CD_RS(self):
        """Test Case: RA(3) - CD - SI(200) - CD - RS"""
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = self.three_advertisements
        number_advs = len(advertisements)

        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               number_advs)

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        self.test_reset_advertising(self.get_instance_ids(advertisements))



    @test_case_log
    def test_case_RA3_CD_SI200_CD_UA1_CD_RS(self):
        """Test Case: RA(3) - CD - SI(200) - CD - UA(1) - CD - RS"""
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = self.three_advertisements
        number_advs = len(advertisements)

        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               number_advs)

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        # Unregister the 2nd advertisement.
        instance_id = 2
        self.test_unregister_advertisement(advertisements[instance_id - 1],
                                           instance_id,
                                           advertising_disabled=False)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs - 1)

        # Test if advertising is reset correctly.Only instances [1, 3] are left.
        self.test_reset_advertising([1, 3])


    @test_case_log
    def test_case_RA3_CD_SI200_CD_SI2000_CD_UA3(self):
        """Test Case: RA(3) - CD - SI(200) - CD - SI(2000) - CD - UA(3)"""
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_small_min_adv_interval_ms = 200
        new_small_max_adv_interval_ms = 200
        new_large_min_adv_interval_ms = 2000
        new_large_max_adv_interval_ms = 2000
        advertisements = self.three_advertisements
        number_advs = len(advertisements)

        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               number_advs)

        self.test_set_advertising_intervals(new_small_min_adv_interval_ms,
                                            new_small_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_small_min_adv_interval_ms,
                                               new_small_max_adv_interval_ms,
                                               number_advs)

        self.test_set_advertising_intervals(new_large_min_adv_interval_ms,
                                            new_large_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_large_min_adv_interval_ms,
                                               new_large_max_adv_interval_ms,
                                               number_advs)

        self.unregister_advertisements(advertisements)


    @test_case_log
    def test_case_RA5_CD_SI200_CD_FRA1_CD_UA5(self):
        """Test Case: RA(5) - CD - SI(200) - CD - FRA(1) - CD - UA(5)"""
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = self.five_advertisements
        extra_advertisement = self.sixth_advertisement
        number_advs = len(advertisements)

        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               number_advs)

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        self.test_fail_to_register_advertisement(extra_advertisement,
                                                 new_min_adv_interval_ms,
                                                 new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        self.unregister_advertisements(advertisements)


    @test_case_log
    def test_case_RA3_CD_SI200_CD_FSI10_CD_FSI20000_CD_UA3(self):
        """Test Case: RA(3) - CD - SI(200) - CD - FSI(10) - CD - FSI(20000) - CD
        - UA(3)
        """
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        invalid_small_min_adv_interval_ms = 10
        invalid_small_max_adv_interval_ms = 10
        invalid_large_min_adv_interval_ms = 20000
        invalid_large_max_adv_interval_ms = 20000
        advertisements = self.three_advertisements
        number_advs = len(advertisements)

        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               number_advs)

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        # Fails to set intervals that are too small. Intervals remain the same.
        self.test_fail_to_set_advertising_intervals(
                invalid_small_min_adv_interval_ms,
                invalid_small_max_adv_interval_ms,
                new_min_adv_interval_ms, new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        # Fails to set intervals that are too large. Intervals remain the same.
        self.test_fail_to_set_advertising_intervals(
                invalid_large_min_adv_interval_ms,
                invalid_large_max_adv_interval_ms,
                new_min_adv_interval_ms, new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        # Unregister all advertisements.
        self.unregister_advertisements(advertisements)


    @test_case_log
    def test_case_RA3_CD_SI200_CD_PC_CD_UA3(self):
        """Test Case: RA(3) - CD - SI(200) - CD - PC - CD - UA(3)"""
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = self.three_advertisements
        number_advs = len(advertisements)

        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               number_advs)

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        # Turn off and then turn on the adapter.
        self.test_power_off_adapter()
        time.sleep(1)
        self.test_power_on_adapter()

        # Check if the advertising durations remain the same after resume.
        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        # Unregister all advertisements.
        self.unregister_advertisements(advertisements)


    @test_case_log
    def test_case_RA3_CD_SI200_CD_SR_CD_UA3(self):
        """Test Case: RA(3) - CD - SI(200) - CD - SR - CD - UA(3)"""
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = self.three_advertisements
        number_advs = len(advertisements)

        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               number_advs)

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        # Suspend for a while and resume.
        self.suspend_resume()

        # Check if the advertising durations remain the same after resume.
        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               number_advs)

        # Unregister all advertisements.
        self.unregister_advertisements(advertisements)

    # SINGLE TEST CASES
    @test_case_log
    def test_case_SI200_RA1_CD_UA1(self):
        """Test Case: SI(200) - RA(1) - CD - UA(1)"""
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = [self.sixth_advertisement]

        self.test_reset_advertising()

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.register_advertisements(advertisements, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        self.unregister_advertisements(advertisements)


    @test_case_log
    def test_case_SI200_RA1_CD_RS(self):
        """Test Case: SI(200) - RA(1) - CD - RS"""
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = [self.first_advertisement]

        self.test_reset_advertising()
        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.register_advertisements(advertisements, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        self.test_reset_advertising()


    @test_case_log
    def test_case_SI200_RA1_CD_SR_CD_UA1(self):
        """Test Case: SI(200) - RA(1) - CD - SR - CD - UA(1)"""
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = [self.sixth_advertisement]

        self.test_reset_advertising()

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.register_advertisements(advertisements, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))
        self.suspend_resume()

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        self.unregister_advertisements(advertisements)


    @test_case_log
    def test_case_RA1_CD_SI200_CD_UA1(self):
        """Test Case: RA(1) - CD - SI(200) - CD - UA(1)"""
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = [self.first_advertisement]

        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               len(advertisements))

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)


        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        self.unregister_advertisements(advertisements)


    @test_case_log
    def test_case_RA1_CD_SI200_CD_RS(self):
        """Test Case: RA(1) - CD - SI(200) - CD - RS"""
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = [self.sixth_advertisement]
        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               len(advertisements))

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))
        self.test_reset_advertising()


    @test_case_log
    def test_case_RA1_CD_SI200_CD_FSI10_UA1_RA1_CD_UA1(self):
        """Test Case:  RA(1) - CD - SI(200) - CD - FSI(10) - UA(1)
         - RA(1) - CD - UA(1)"""
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        invalid_small_min_adv_interval_ms = 10
        invalid_small_max_adv_interval_ms = 10
        advertisements = [self.three_advertisements[1]]
        new_advertisement = [self.three_advertisements[2]]

        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               len(advertisements))

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        # Fails to set intervals that are too small. Intervals remain the same.
        self.test_fail_to_set_advertising_intervals(
                invalid_small_min_adv_interval_ms,
                invalid_small_max_adv_interval_ms,
                new_min_adv_interval_ms, new_max_adv_interval_ms)

        self.unregister_advertisements(advertisements)

        # Register a new advertisement in order to avoid kernel caching.
        self.register_advertisements(new_advertisement, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        self.unregister_advertisements(new_advertisement)


    @test_case_log
    def test_case_RA1_CD_SI200_CD_FSI20000_UA1_RA1_CD_UA1(self):
        """Test Case:  RA(1) - CD - SI(200) - CD - FSI(20000) - UA(1)
         - RA(1) - CD - UA(1)"""
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        invalid_large_min_adv_interval_ms = 20000
        invalid_large_max_adv_interval_ms = 20000
        advertisements = [self.three_advertisements[1]]
        new_advertisement = [self.three_advertisements[2]]

        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               len(advertisements))

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))
        # Fails to set intervals that are too large. Intervals remain the same.
        self.test_fail_to_set_advertising_intervals(
                invalid_large_min_adv_interval_ms,
                invalid_large_max_adv_interval_ms,
                new_min_adv_interval_ms, new_max_adv_interval_ms)

        self.unregister_advertisements(advertisements)

        # Register a new advertisement in order to avoid kernel caching.
        self.register_advertisements(new_advertisement, new_min_adv_interval_ms,
                                     new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        self.unregister_advertisements(new_advertisement)


    @test_case_log
    def test_case_RA1_CD_SI200_CD_PC_CD_UA1(self):
        """Test Case: RA(1) - CD - SI(200) - CD - PC - CD - UA(1)"""
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = [self.sixth_advertisement]
        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               len(advertisements))

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        # Turn off and then turn on the adapter.
        self.test_power_off_adapter()
        time.sleep(1)
        self.test_power_on_adapter()

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        self.unregister_advertisements(advertisements)


    @test_case_log
    def test_case_RA1_CD_SI200_CD_SR_CD_UA1(self):
        """Test Case: RA(1) - CD - SI(200) - CD - SR - CD - UA(1)"""
        orig_min_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        orig_max_adv_interval_ms = self.DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS
        new_min_adv_interval_ms = 200
        new_max_adv_interval_ms = 200
        advertisements = [self.first_advertisement]
        self.test_reset_advertising()

        self.register_advertisements(advertisements, orig_min_adv_interval_ms,
                                     orig_max_adv_interval_ms)

        self.test_check_duration_and_intervals(orig_min_adv_interval_ms,
                                               orig_max_adv_interval_ms,
                                               len(advertisements))

        self.test_set_advertising_intervals(new_min_adv_interval_ms,
                                            new_max_adv_interval_ms)

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))
        self.suspend_resume()

        self.test_check_duration_and_intervals(new_min_adv_interval_ms,
                                               new_max_adv_interval_ms,
                                               len(advertisements))

        self.unregister_advertisements(advertisements)

    def run_once(self, host, advertisements, test_type, num_iterations=1):
        """Running Bluetooth adapter LE advertising autotest.

        @param host: device under test host.
        @param advertisements: a list of advertisement instances.
        @param test_type: indicating one of three test types: multi-advertising,
                          single-advertising, reboot (stress only), or
                          suspend_resume (stress only).

        @raises TestNAError: if DUT has low kernel version (<=3.8.11)

        """
        self.host = host
        self.kernel_version = self.get_kernel_version(self.host)
        if self.kernel_version == UNSUPPORTED_KERNEL:
            # NOTE: Due to crbug/729648, we cannot set advertising intervals
            # on kernels that are 3.8.11 and below, so we raise TestNAError.
            raise error.TestNAError('Test cannnot proceed on old kernel '
                                    ' versions.')

        self.advertisements = advertisements
        self.first_advertisement = advertisements[0]
        self.two_advertisements = advertisements[3:5]
        self.three_advertisements = advertisements[0:3]
        self.five_advertisements = advertisements[0:5]
        self.sixth_advertisement = advertisements[5]

        ble_adapter = bluetooth_le_facade_adapter.BluetoothLEFacadeRemoteAdapter
        self.bluetooth_le_facade = ble_adapter(self.host)
        self.bluetooth_facade = self.bluetooth_le_facade

        # Reset the adapter to forget previous stored data and turn it on.
        self.test_reset_on_adapter()

        if test_type == 'multi_advertising':
            # Run all test cases for multiple advertisements.
            self.test_case_SI200_RA3_CD_UA3()
            self.test_case_SI200_RA3_CD_RA1_CD_UA1_CD_UA3()
            self.test_case_SI200_RA3_CD_RS()
            self.test_case_SI200_RA3_CD_UA1_CD_RS()
            self.test_case_SI200_RA3_CD_UA1_CD_RA2_CD_UA4()
            self.test_case_SI200_RA5_CD_FRA1_CD_UA5()
            self.test_case_RA3_CD_SI200_CD_UA3()
            self.test_case_RA3_CD_SI200_CD_RS()
            self.test_case_RA3_CD_SI200_CD_UA1_CD_RS()
            self.test_case_RA3_CD_SI200_CD_SI2000_CD_UA3()
            self.test_case_RA5_CD_SI200_CD_FRA1_CD_UA5()
            self.test_case_RA3_CD_SI200_CD_FSI10_CD_FSI20000_CD_UA3()
            self.test_case_SI200_RA3_CD_SR_CD_UA3()
            self.test_case_RA3_CD_SI200_CD_SR_CD_UA3()
            self.test_case_SI200_RA3_CD_PC_CD_UA3()
            self.test_case_RA3_CD_SI200_CD_PC_CD_UA3()

        elif test_type == 'single_advertising':
            # Run all test cases for single advertisement.
            # Note: it is required to change the advertisement instance
            #       so that the advertisement data could be monitored by btmon.
            #       Otherwise, the advertisement data would be just cached and
            #       reused such that the data would not be visible in btmon.
            self.test_case_SI200_RA1_CD_UA1()
            self.test_case_SI200_RA1_CD_RS()
            self.test_case_RA1_CD_SI200_CD_UA1()
            self.test_case_RA1_CD_SI200_CD_RS()
            self.test_case_RA1_CD_SI200_CD_FSI10_UA1_RA1_CD_UA1()
            self.test_case_RA1_CD_SI200_CD_FSI20000_UA1_RA1_CD_UA1()
            self.test_case_SI200_RA1_CD_SR_CD_UA1()
            self.test_case_RA1_CD_SI200_CD_SR_CD_UA1()
            self.test_case_RA1_CD_SI200_CD_PC_CD_UA1()

        elif test_type == 'suspend_resume':
           # Run all test cases for suspend resume stress testing.
            for i in xrange(num_iterations):
               logging.info('Starting suspend resume loop #%d', i+1)
               self.test_case_SI200_RA3_CD_SR_CD_UA3()
               self.test_case_RA3_CD_SI200_CD_SR_CD_UA3()
               self.test_case_SI200_RA1_CD_SR_CD_UA1()
               self.test_case_RA1_CD_SI200_CD_SR_CD_UA1()

        elif test_type == 'reboot':
            # Run all test cases for reboot stress testing.
            for i in xrange(num_iterations):
                logging.info('Starting reboot loop #%d', i+1)
                self.test_case_SI200_RA3_CD_PC_CD_UA3()
                self.test_case_RA3_CD_SI200_CD_PC_CD_UA3()
                self.test_case_RA1_CD_SI200_CD_PC_CD_UA1()

        if self.fails:
            raise error.TestFail(self.fails)
