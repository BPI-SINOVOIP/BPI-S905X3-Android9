#!/usr/bin/env python3.4
#
#   Copyright 2017 - Google
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
"""
    Test Script for Telephony Pre Check In Sanity
"""

from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import DEFAULT_DEVICE_PASSWORD
from acts.test_utils.tel.tel_defines import SIM_STATE_ABSENT
from acts.test_utils.tel.tel_test_utils import fastboot_wipe
from acts.test_utils.tel.tel_test_utils import reset_device_password
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode_by_adb
from TelLiveEmergencyTest import TelLiveEmergencyTest


class TelLiveNoSimTest(TelLiveEmergencyTest):
    def setup_class(self):
        for ad in self.android_devices:
            if ad.adb.getprop("gsm.sim.state") == SIM_STATE_ABSENT:
                ad.log.info("Device has no SIM in it, set as DUT")
                self.dut = ad
                return True
        self.log.error("No device meets no SIM requirement")
        return False

    def setup_test(self):
        self.expected_call_result = False
        self.android_devices = [self.dut]

    """ Tests Begin """

    @test_tracker_info(uuid="91bc0c02-c1f2-4112-a7f8-c91617bff53e")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_by_emergency_dialer(self):
        """Test emergency call with emergency dialer in user account.

        Add storyline number to system emergency number list.
        Use the emergency dialer to call storyline.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        toggle_airplane_mode_by_adb(self.log, self.dut, False)
        return self.fake_emergency_call_test()

    @test_tracker_info(uuid="cdf7ddad-480f-4757-83bd-a74321b799f7")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_by_dialer(self):
        """Test emergency call with dialer.

        Add storyline number to system emergency number list.
        Call storyline by dialer.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        toggle_airplane_mode_by_adb(self.log, self.dut, False)
        return self.fake_emergency_call_test(by_emergency_dialer=False)

    @test_tracker_info(uuid="e147960a-4227-41e2-bd06-65001ad5e0cd")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_in_apm(self):
        """Test emergency call with emergency dialer in airplane mode.

        Enable airplane mode.
        Add storyline number to system emergency number list.
        Use the emergency dialer to call storyline.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        try:
            toggle_airplane_mode_by_adb(self.log, self.dut, True)
            if self.fake_emergency_call_test():
                return True
            else:
                return False
        finally:
            toggle_airplane_mode_by_adb(self.log, self.dut, False)

    @test_tracker_info(uuid="34068bc8-bfa0-4c7b-9450-e189a0b93c8a")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_in_screen_lock(self):
        """Test emergency call with emergency dialer in screen lock phase.

        Enable device password and then reboot upto password query window.
        Add storyline number to system emergency number list.
        Use the emergency dialer to call storyline.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        toggle_airplane_mode_by_adb(self.log, self.dut, False)
        reset_device_password(self.dut, DEFAULT_DEVICE_PASSWORD)
        self.dut.reboot(stop_at_lock_screen=True)
        if self.fake_emergency_call_test():
            return True
        else:
            return False

    @test_tracker_info(uuid="1ef97f8a-eb3d-45b7-b947-ac409bb70587")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_in_screen_lock_apm(self):
        """Test emergency call with emergency dialer in screen lock phase.

        Enable device password and then reboot upto password query window.
        Add storyline number to system emergency number list.
        Use the emergency dialer to call storyline.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        try:
            toggle_airplane_mode_by_adb(self.log, self.dut, True)
            reset_device_password(self.dut, DEFAULT_DEVICE_PASSWORD)
            self.dut.reboot(stop_at_lock_screen=True)
            if self.fake_emergency_call_test():
                return True
            else:
                return False
        finally:
            toggle_airplane_mode_by_adb(self.log, self.dut, False)

    @test_tracker_info(uuid="50f8b3d9-b126-4419-b5e5-b37b850deb8e")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_in_setupwizard(self):
        """Test emergency call with emergency dialer in setupwizard.

        Wipe the device and then reboot upto setupwizard.
        Add storyline number to system emergency number list.
        Use the emergency dialer to call storyline.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        try:
            if not fastboot_wipe(self.dut, skip_setup_wizard=False):
                return False
            if self.fake_emergency_call_test():
                return True
            else:
                return False
        finally:
            self.dut.exit_setup_wizard()


""" Tests End """
