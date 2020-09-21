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
Test the HFP profile for advanced functionality and try to create race
conditions by executing actions quickly.
"""

import time

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BluetoothCarHfpBaseTest import BluetoothCarHfpBaseTest
from acts.test_utils.bt import BtEnum
from acts.test_utils.bt import bt_test_utils
from acts.test_utils.car import car_telecom_utils
from acts.test_utils.tel import tel_defines

STABILIZATION_DELAY_SEC = 5


class BtCarHfpFuzzTest(BluetoothCarHfpBaseTest):
    def setup_class(self):
        if not super(BtCarHfpFuzzTest, self).setup_class():
            return False

        # Connect the devices now, try twice.
        attempts = 2
        connected = False
        while attempts > 0 and not connected:
            connected = bt_test_utils.connect_pri_to_sec(
                self.hf, self.ag,
                set([BtEnum.BluetoothProfile.HEADSET_CLIENT.value]))
            self.log.info("Connected {}".format(connected))
            attempts -= 1

        if not connected:
            self.log.error("Failed to connect")
            return False

        # Delay set contains the delay between dial and hangup for a call.
        # We keep very small delays to significantly large ones to stress test
        # various kind of timing issues.
        self.delay_set = [
            0.1,
            0.2,
            0.3,
            0.4,
            0.5,  # Very short delays
            1.0,
            2.0,
            3.0,
            4.0,
            5.0,  # Med delays
            10.0
        ]  # Large delays

    def dial_a_hangup_b_quick(self, a, b, delay=0, ph=""):
        """
        This test does a quick succession of dial and hangup. We make the test
        case sleep for delay amount between each dial and hangup. This is
        different from other test cases where we give enough time for devices
        to get into a calling state before trying to tear down connection.
        """
        if ph == "": ph = self.re_phone_number

        # Dial from A now.
        self.log.info("Dialing at droid {}".format(a.droid.getBuildDisplay()))
        a.droid.telecomCallTelUri(ph)

        # Wait for delay millis.
        time.sleep(delay)

        # Cancel the call at B. Use end call in this scenario so that we do not
        # wait for looking up the call!
        self.log.info("Hanging up at droid {}".format(b.droid.getBuildDisplay(
        )))
        b.droid.telecomEndCall()

        # Check/Wait that we are clear before executing the test.
        for d in self.android_devices:
            if not car_telecom_utils.wait_for_not_in_call(self.log, d):
                self.log.warn(
                    "dial_a_hangup_quick wait_for_not_in_call failed {}".
                    format(d.serial))
                return False

        return True

    def stabilize_and_check_sanity(self):
        # Since we dial and hangup very very quickly we may end up in a state
        # where we need to wait to see the results. For instance if the delay is
        # 0.1 sec it may take upto 2 seconds for the platform to respond to a
        # dial() and hence even if we hangup 0.1 sec later we will not see its
        # result immidiately (this may be a false positive on test).
        time.sleep(STABILIZATION_DELAY_SEC)

        # First check if HF is in dialing state, we can send an actual hangup if
        # that is the case and then wait for devices to come back to normal.
        if self.hf.droid.telecomIsInCall():
            self.log.info("HF still in call, send hangup")
            self.hf.droid.telecomEndCall()

        # Wait for devices to go back to normal.
        for d in self.android_devices:
            if not car_telecom_utils.wait_for_not_in_call(self.log, d):
                self.log.warning(
                    "stabilize_and_check_sanity wait_for_not_in_call failed {}".
                    format(d.serial))
                return False

        return True

    @test_tracker_info(uuid='32022c74-fdf3-44c4-9e82-e518bdcce667')
    @BluetoothBaseTest.bt_test_wrap
    def test_fuzz_outgoing_hf(self):
        """
        Test calling and hangup from HF with varied delays as defined in
        self.delay_set

        Precondition:
        1. Devices are paired and connected

        Steps:
        For each delay do the following:
        a) Call HF
        b) Wait for delay seconds
        c) Hangup HF
        d) Check if all devices are in stable state, if not wait for stabilizing
        e) If (d) fails then we fail otherwise we go back to (a)
        f) Once all delays are done we do a final check for sanity as pass
        scenario

        Returns:
          Pass if True
          Fail if False

        Priority: 1
        """

        for delay in self.delay_set:
            self.log.info("test_fuzz outgoing_hf: {}".format(delay))
            # Make the call and hangup, we do a light check inside to see if the
            # phones are in a clean state -- if not, we let them stabilize
            # before continuing.
            if not self.dial_a_hangup_b_quick(self.hf, self.hf, delay):
                if not self.stabilize_and_check_sanity():
                    self.log.info("Devices not able to stabilize!")
                    return False

        # Final sanity check (if we never called stabilize_and_check_sanity
        # above).
        return self.stabilize_and_check_sanity()

    @test_tracker_info(uuid='bc6d52b2-4acc-461e-ad55-fad5a5ecb091')
    @BluetoothBaseTest.bt_test_wrap
    def test_fuzz_outgoing_ag(self):
        """
        Test calling and hangup from AG with varied delays as defined in
        self.delay_set

        Precondition:
        1. Devices are paired and connected

        Steps:
        For each delay do the following:
        a) Call AG
        b) Wait for delay seconds
        c) Hangup AG
        d) Check if all devices are in stable state, if not wait for stabilizing
        e) If (d) fails then we fail otherwise we go back to (a)
        f) Once all delays are done we do a final check for sanity as pass
        scenario

        Returns:
          Pass if True
          Fail if False

        Priority: 1
        """

        for delay in self.delay_set:
            self.log.info("test_fuzz outgoing_ag: {}".format(delay))
            # Make the call and hangup, we do a light check inside to see if the
            # phones are in a clean state -- if not, we let them stabilize
            # before continuing.
            if not self.dial_a_hangup_b_quick(self.ag, self.ag, delay):
                if not self.stabilize_and_check_sanity():
                    self.log.error("Devices not able to stabilize!")
                    return False

        # Final sanity check (if we never called stabilize_and_check_sanity
        # above).
        return self.stabilize_and_check_sanity()

    @test_tracker_info(uuid='d834384a-38d5-4260-bfd5-98f8207c04f5')
    @BluetoothBaseTest.bt_test_wrap
    def test_fuzz_dial_hf_hangup_ag(self):
        """
        Test calling and hangup from HF and AG resp. with varied delays as defined in
        self.delay_set

        Precondition:
        1. Devices are paired and connected

        Steps:
        For each delay do the following:
        a) Call HF
        b) Wait for delay seconds
        c) Hangup AG
        d) Check if all devices are in stable state, if not wait for stabilizing
        e) If (d) fails then we fail otherwise we go back to (a)
        f) Once all delays are done we do a final check for sanity as pass
        scenario

        Returns:
          Pass if True
          Fail if False

        Priority: 1
        """

        for delay in self.delay_set:
            self.log.info("test_fuzz dial_hf hangup_ag: {}".format(delay))
            # Make the call and hangup, we do a light check inside to see if the
            # phones are in a clean state -- if not, we let them stabilize
            # before continuing.
            if not self.dial_a_hangup_b_quick(self.hf, self.ag, delay):
                if not self.stabilize_and_check_sanity():
                    self.log.info("Devices not able to stabilize!")
                    return False

        # Final sanity check (if we never called stabilize_and_check_sanity
        # above).
        return self.stabilize_and_check_sanity()

    @test_tracker_info(uuid='6de1a8ab-3cb0-4594-a9bb-d882a3414836')
    @BluetoothBaseTest.bt_test_wrap
    def test_fuzz_dial_ag_hangup_hf(self):
        """
        Test calling and hangup from HF and AG resp. with varied delays as defined in
        self.delay_set

        Precondition:
        1. Devices are paired and connected

        Steps:
        For each delay do the following:
        a) Call AG
        b) Wait for delay seconds
        c) Hangup HF
        d) Check if all devices are in stable state, if not wait for stabilizing
        e) If (d) fails then we fail otherwise we go back to (a)
        f) Once all delays are done we do a final check for sanity as pass
        scenario

        Returns:
          Pass if True
          Fail if False

        Priority: 1
        """

        for delay in self.delay_set:
            self.log.info("test_fuzz dial_ag hangup_hf: {}".format(delay))
            # Make the call and hangup, we do a light check inside to see if the
            # phones are in a clean state -- if not, we let them stabilize
            # before continuing.
            if not self.dial_a_hangup_b_quick(self.ag, self.hf, delay):
                if not self.stabilize_and_check_sanity():
                    self.log.info("Devices not able to stabilize!")
                    return False

        # Final sanity check (if we never called stabilize_and_check_sanity
        # above).
        return self.stabilize_and_check_sanity()
