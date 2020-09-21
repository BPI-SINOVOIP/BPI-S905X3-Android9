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
    Test Script for Telephony Locked SIM Emergency Call Test
"""

import time
from acts.base_test import BaseTestClass
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import DEFAULT_DEVICE_PASSWORD
from acts.test_utils.tel.tel_test_utils import abort_all_tests
from acts.test_utils.tel.tel_test_utils import fastboot_wipe
from acts.test_utils.tel.tel_test_utils import is_sim_locked
from acts.test_utils.tel.tel_test_utils import is_sim_ready_by_adb
from acts.test_utils.tel.tel_test_utils import reset_device_password
from acts.test_utils.tel.tel_test_utils import refresh_sl4a_session
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode_by_adb
from acts.test_utils.tel.tel_test_utils import unlocking_device
from acts.test_utils.tel.tel_test_utils import unlock_sim
from acts.test_utils.tel.tel_test_utils import STORY_LINE
from TelLiveEmergencyTest import TelLiveEmergencyTest

EXPECTED_CALL_TEST_RESULT = False


class TelLiveLockedSimTest(TelLiveEmergencyTest):
    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        self.logger_sessions = []
        fake_number = self.user_params.get("fake_emergency_number", STORY_LINE)
        self.fake_emergency_number = fake_number.strip("+").replace("-", "")
        for ad in self.android_devices:
            if not is_sim_locked(ad):
                ad.log.info("SIM is not locked")
            else:
                ad.log.info("SIM is locked")
                self.dut = ad
                return
        #if there is no locked SIM, reboot the device and check again
        for ad in self.android_devices:
            reset_device_password(ad, None)
            ad.reboot(stop_at_lock_screen=True)
            for _ in range(10):
                if is_sim_ready_by_adb(self.log, ad):
                    ad.log.info("SIM is not locked")
                    break
                elif is_sim_locked(ad):
                    ad.log.info("SIM is locked")
                    self.dut = ad
                    ad.ensure_screen_on()
                    ad.start_services(ad.skip_sl4a)
                    return
                else:
                    time.sleep(5)
        self.log.error("There is no locked SIM in this testbed")
        abort_all_tests(self.log, "There is no locked SIM")

    def setup_class(self):
        self.android_devices = [self.dut]
        pass

    def setup_test(self):
        self.expected_call_result = False
        unlocking_device(self.dut)
        refresh_sl4a_session(self.dut)
        unlock_sim(self.dut)

    """ Tests Begin """

    @test_tracker_info(uuid="fd7fb69c-6fd4-4874-a4ca-769353b9db25")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_by_emergency_dialer(self):
        """Test emergency call with emergency dialer in user account.

        Enable SIM lock on the SIM. Reboot device to SIM pin request page.
        Add storyline number to system emergency number list.
        Use the emergency dialer to call "611".
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        self.expected_call_result = True
        toggle_airplane_mode_by_adb(self.log, self.dut, False)
        return self.fake_emergency_call_test()

    @test_tracker_info(uuid="669cf1d9-9513-4f90-b0fd-2f0e8f1cc941")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_by_dialer(self):
        """Test emergency call with dialer.

        Enable SIM lock on the SIM. Reboot device to SIM pin request page.
        Add system emergency number list with storyline number.
        Call storyline by dialer.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        self.expected_call_result = True
        toggle_airplane_mode_by_adb(self.log, self.dut, False)
        return self.fake_emergency_call_test(by_emergency_dialer=True)

    @test_tracker_info(uuid="1990f166-66a7-4092-b448-c179a9194371")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_in_apm(self):
        """Test emergency call with emergency dialer in airplane mode.

        Enable airplane mode.
        Enable SIM lock on the SIM. Reboot device to SIM pin request page.
        Add system emergency number list with storyline number.
        Use the emergency dialer to call storyline.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        self.expected_call_result = True
        try:
            toggle_airplane_mode_by_adb(self.log, self.dut, True)
            if self.fake_emergency_call_test():
                return True
            else:
                return False
        finally:
            toggle_airplane_mode_by_adb(self.log, self.dut, False)

    @test_tracker_info(uuid="7ffdad34-b8fb-41b0-b0fd-2def5adc67bc")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_in_screen_lock(self):
        """Test emergency call with emergency dialer in screen lock phase.

        Enable SIM lock on the SIM.
        Enable device password and then reboot upto password and pin query stage.
        Add system emergency number list with storyline number.
        Use the emergency dialer to call storyline.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        self.dut.log.info("Turn off airplane mode")
        toggle_airplane_mode_by_adb(self.log, self.dut, False)
        self.dut.log.info("Set screen lock pin")
        reset_device_password(self.dut, DEFAULT_DEVICE_PASSWORD)
        self.dut.log.info("Reboot device to screen lock screen")
        self.dut.reboot(stop_at_lock_screen=True)
        if self.fake_emergency_call_test():
            return True
        else:
            return False

    @test_tracker_info(uuid="12dc1eb6-50ed-4ad9-b195-5d96c6b6952e")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_in_screen_lock_apm(self):
        """Test emergency call with emergency dialer in screen lock phase.

        Enable device password and airplane mode
        Enable SIM lock on the SIM.
        Reboot upto pin query window.
        Add system emergency number list with story line.
        Use the emergency dialer to call story line.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        toggle_airplane_mode_by_adb(self.log, self.dut, True)
        self.dut.log.info("Set screen lock pin")
        reset_device_password(self.dut, DEFAULT_DEVICE_PASSWORD)
        self.dut.log.info("Reboot device to screen lock screen")
        self.dut.reboot(stop_at_lock_screen=True)
        if self.fake_emergency_call_test():
            return True
        else:
            return False

    @test_tracker_info(uuid="1e01927a-a077-466d-8bf8-52dca87ab87c")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_in_setupwizard(self):
        """Test emergency call with emergency dialer in setupwizard.

        Enable SIM lock on the SIM.
        Wipe the device and then reboot upto setupwizard.
        Add system emergency number list with story line.
        Use the emergency dialer to call story line.
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
