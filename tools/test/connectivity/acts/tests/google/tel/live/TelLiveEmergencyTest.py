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

import time
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import DEFAULT_DEVICE_PASSWORD
from acts.test_utils.tel.tel_test_utils import abort_all_tests
from acts.test_utils.tel.tel_test_utils import dumpsys_telecom_call_info
from acts.test_utils.tel.tel_test_utils import get_service_state_by_adb
from acts.test_utils.tel.tel_test_utils import fastboot_wipe
from acts.test_utils.tel.tel_test_utils import hangup_call_by_adb
from acts.test_utils.tel.tel_test_utils import initiate_call
from acts.test_utils.tel.tel_test_utils import initiate_emergency_dialer_call_by_adb
from acts.test_utils.tel.tel_test_utils import reset_device_password
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode_by_adb
from acts.test_utils.tel.tel_test_utils import unlock_sim
from acts.test_utils.tel.tel_test_utils import wait_for_sim_ready_by_adb


class TelLiveEmergencyTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)

        self.dut = self.android_devices[0]
        self.number_of_devices = 1
        fake_number = self.user_params.get("fake_emergency_number", "800")
        self.fake_emergency_number = fake_number.strip("+").replace("-", "")
        self.wifi_network_ssid = self.user_params.get(
            "wifi_network_ssid") or self.user_params.get(
                "wifi_network_ssid_2g")
        self.wifi_network_pass = self.user_params.get(
            "wifi_network_pass") or self.user_params.get(
                "wifi_network_pass_2g")

    def setup_test(self):
        if not unlock_sim(self.dut):
            abort_all_tests(self.dut.log, "unable to unlock SIM")
        self.expected_call_result = True

    def teardown_test(self):
        self.dut.ensure_screen_on()
        reset_device_password(self.dut, None)

    def change_emergency_number_list(self):
        for _ in range(5):
            existing = self.dut.adb.getprop("ril.ecclist")
            self.dut.log.info("Existing ril.ecclist is: %s", existing)
            if self.fake_emergency_number in existing:
                return True
            emergency_numbers = "%s,%s" % (existing,
                                           self.fake_emergency_number)
            cmd = "setprop ril.ecclist %s" % emergency_numbers
            self.dut.log.info(cmd)
            self.dut.adb.shell(cmd)
            # After some system events, ril.ecclist might change
            # wait sometime for it to settle
            time.sleep(10)
            if self.fake_emergency_number in existing:
                return True
        return False

    def change_qcril_emergency_source_mcc_table(self):
        # This will add the fake number into emergency number list for a mcc
        # in qcril. Please note, the fake number will be send as an emergency
        # number by modem and reach the real 911 by this
        qcril_database_path = self.dut.adb.shell("find /data -iname  qcril.db")
        if not qcril_database_path: return
        mcc = self.dut.droid.telephonyGetNetworkOperator()
        mcc = mcc[:3]
        self.dut.log.info("Add %s mcc %s in qcril_emergency_source_mcc_table")
        self.dut.adb.shell(
            "sqlite3 %s \"INSERT INTO qcril_emergency_source_mcc_table VALUES('%s','%s','','')\""
            % (qcril_database_path, mcc, self.fake_emergency_number))

    def fake_emergency_call_test(self, by_emergency_dialer=True, attemps=3):
        self.dut.log.info("ServiceState is in %s",
                          get_service_state_by_adb(self.log, self.dut))
        if by_emergency_dialer:
            dialing_func = initiate_emergency_dialer_call_by_adb
            callee = self.fake_emergency_number
        else:
            dialing_func = initiate_call
            # Initiate_call method has to have "+" in front
            # otherwise the number will be in dialer without dial out
            # with sl4a fascade. Need further investigation
            callee = "+%s" % self.fake_emergency_number
        for i in range(attemps):
            result = True
            if not self.change_emergency_number_list():
                self.dut.log.error("Unable to add number to ril.ecclist")
                return False
            time.sleep(1)
            call_numbers = len(dumpsys_telecom_call_info(self.dut))
            dial_result = dialing_func(self.log, self.dut, callee)
            hangup_call_by_adb(self.dut)
            self.dut.send_keycode("BACK")
            self.dut.send_keycode("BACK")
            calls_info = dumpsys_telecom_call_info(self.dut)
            if len(calls_info) <= call_numbers:
                self.dut.log.error("New call is not in sysdump telecom")
                result = False
            else:
                self.dut.log.info("New call info = %s", calls_info[-1])

            if dial_result == self.expected_call_result:
                self.dut.log.info("Call to %s returns %s as expected", callee,
                                  self.expected_call_result)
            else:
                self.dut.log.info("Call to %s returns %s", callee,
                                  not self.expected_call_result)
                result = False
            if result:
                return True
            ecclist = self.dut.adb.getprop("ril.ecclist")
            self.dut.log.info("ril.ecclist = %s", ecclist)
            if self.fake_emergency_number in ecclist:
                if i == attemps - 1:
                    self.dut.log.error("%s is in ril-ecclist, but call failed",
                                       self.fake_emergency_number)
                else:
                    self.dut.log.warning(
                        "%s is in ril-ecclist, but call failed, try again",
                        self.fake_emergency_number)
            else:
                if i == attemps - 1:
                    self.dut.log.error("Fail to write %s to ril-ecclist",
                                       self.fake_emergency_number)
                else:
                    self.dut.log.info("%s is not in ril-ecclist",
                                      self.fake_emergency_number)
        self.dut.log.info("fake_emergency_call_test result is %s", result)
        import pdb
        pdb.set_trace()
        return result

    """ Tests Begin """

    @test_tracker_info(uuid="fe75ba2c-e4ea-4fc1-881b-97e7a9a7f48e")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_by_emergency_dialer(self):
        """Test emergency call with emergency dialer in user account.

        Add system emergency number list with storyline number.
        Use the emergency dialer to call storyline.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        return self.fake_emergency_call_test()

    @test_tracker_info(uuid="8a0978a8-d93e-4f6a-99fe-d0e28bf1be2a")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_by_dialer(self):
        """Test emergency call with dialer.

        Add system emergency number list with storyline number.
        Call storyline by dialer.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        return self.fake_emergency_call_test(by_emergency_dialer=False)

    @test_tracker_info(uuid="2e6fcc75-ff9e-47b1-9ae8-ed6f9966d0f5")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_in_apm(self):
        """Test emergency call with emergency dialer in airplane mode.

        Enable airplane mode.
        Add system emergency number list with storyline number.
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

    @test_tracker_info(uuid="469bfa60-6e8f-4159-af1f-ab6244073079")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_in_screen_lock(self):
        """Test emergency call with emergency dialer in screen lock phase.

        Enable device password and then reboot upto password query window.
        Add system emergency number list with storyline.
        Use the emergency dialer to call storyline.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        toggle_airplane_mode_by_adb(self.log, self.dut, False)
        reset_device_password(self.dut, DEFAULT_DEVICE_PASSWORD)
        if not wait_for_sim_ready_by_adb(self.log, self.dut):
            self.dut.log.error("SIM is not ready")
            return False
        self.dut.reboot(stop_at_lock_screen=True)
        if self.fake_emergency_call_test():
            return True
        else:
            return False

    @test_tracker_info(uuid="17401c57-0dc2-49b5-b954-a94dbb2d5ad0")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_in_screen_lock_apm(self):
        """Test emergency call with emergency dialer in screen lock phase.

        Enable device password and then reboot upto password query window.
        Add system emergency number list with storyline.
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
            if not wait_for_sim_ready_by_adb(self.log, self.dut):
                self.dut.log.error("SIM is not ready")
                return False
            if self.fake_emergency_call_test():
                return True
            else:
                return False
        finally:
            toggle_airplane_mode_by_adb(self.log, self.dut, False)

    @test_tracker_info(uuid="ccea13ae-6951-4790-a5f7-b5b7a2451c6c")
    @TelephonyBaseTest.tel_test_wrap
    def test_fake_emergency_call_in_setupwizard(self):
        """Test emergency call with emergency dialer in setupwizard.

        Wipe the device and then reboot upto setupwizard.
        Add system emergency number list with storyline number.
        Use the emergency dialer to call storyline.
        Verify DUT has in call activity.

        Returns:
            True if success.
            False if failed.
        """
        try:
            if not fastboot_wipe(self.dut, skip_setup_wizard=False):
                return False
            if not wait_for_sim_ready_by_adb(self.log, self.dut):
                self.dut.log.error("SIM is not ready")
                return False
            if self.fake_emergency_call_test():
                return True
            else:
                return False
        finally:
            self.dut.exit_setup_wizard()


""" Tests End """
