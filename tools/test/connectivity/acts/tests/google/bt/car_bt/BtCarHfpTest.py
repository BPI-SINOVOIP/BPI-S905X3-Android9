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
Test the HFP profile for basic calling functionality.
"""

import time
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BluetoothCarHfpBaseTest import BluetoothCarHfpBaseTest
from acts.test_utils.bt import BtEnum
from acts.test_utils.bt import bt_test_utils
from acts.test_utils.car import car_telecom_utils
from acts.test_utils.car import tel_telecom_utils
from acts.test_utils.tel import tel_defines

BLUETOOTH_PKG_NAME = "com.android.bluetooth"
CALL_TYPE_OUTGOING = "CALL_TYPE_OUTGOING"
CALL_TYPE_INCOMING = "CALL_TYPE_INCOMING"
AUDIO_STATE_DISCONNECTED = 0
AUDIO_STATE_ROUTED = 2
SHORT_TIMEOUT = 5


class BtCarHfpTest(BluetoothCarHfpBaseTest):
    def setup_class(self):
        if not super(BtCarHfpTest, self).setup_class():
            return False
        # Disable the A2DP profile.
        bt_test_utils.set_profile_priority(self.hf, self.ag, [
            BtEnum.BluetoothProfile.PBAP_CLIENT.value,
            BtEnum.BluetoothProfile.A2DP_SINK.value
        ], BtEnum.BluetoothPriorityLevel.PRIORITY_OFF)
        bt_test_utils.set_profile_priority(
            self.hf, self.ag, [BtEnum.BluetoothProfile.HEADSET_CLIENT.value],
            BtEnum.BluetoothPriorityLevel.PRIORITY_ON)

        if not bt_test_utils.connect_pri_to_sec(
                self.hf, self.ag,
                set([BtEnum.BluetoothProfile.HEADSET_CLIENT.value])):
            self.log.error("Failed to connect.")
            return False
        return True

    @test_tracker_info(uuid='4ce2195a-b70a-4584-912e-cbd20d20e19d')
    @BluetoothBaseTest.bt_test_wrap
    def test_default_calling_account(self):
        """
        Tests if the default calling account is coming from the
        bluetooth pacakge.

        Precondition:
        1. Devices are connected.

        Steps:
        1. Check if the default calling account is via Bluetooth package.

        Returns:
          Pass if True
          Fail if False

        Priority: 0
        """
        selected_acc = \
            self.hf.droid.telecomGetUserSelectedOutgoingPhoneAccount()
        if not selected_acc:
            self.hf.log.error("No default account found.")
            return False

        # Check if the default account is from the Bluetooth package. This is a
        # light weight check.
        try:
            acc_component_id = selected_acc['ComponentName']
        except KeyError:
            self.hf.log.error("No component name for account {}".format(
                selected_acc))
            return False
        if not acc_component_id.startswith(BLUETOOTH_PKG_NAME):
            self.hf.log.error("Component name does not start with pkg name {}".
                              format(selected_acc))
            return False
        return True

    @test_tracker_info(uuid='e579009d-05f3-4236-a698-5de8c11d73a9')
    @BluetoothBaseTest.bt_test_wrap
    def test_outgoing_call_hf(self):
        """
        Tests if we can make a phone call from HF role and disconnect from HF
        role.

        Precondition:
        1. Devices are connected.

        Steps:
        1. Make a call from HF role.
        2. Wait for the HF, AG to be dialing and RE to see the call ringing.
        3. Hangup the call on HF role.
        4. Wait for all devices to hangup the call.

        Returns:
          Pass if True
          Fail if False

        Priority: 0
        """
        return self.dial_a_hangup_b(self.hf, self.hf)

    @test_tracker_info(uuid='c9d5f9cd-f275-4adf-b212-c2e9a70d4cac')
    @BluetoothBaseTest.bt_test_wrap
    def test_outgoing_call_ag(self):
        """
        Tests if we can make a phone call from AG role and disconnect from AG
        role.

        Precondition:
        1. Devices are connected.

        Steps:
        1. Make a call from AG role.
        2. Wait for the HF, AG to be in dialing and RE to see the call ringing.
        3. Hangup the call on AG role.
        4. Wait for all devices to hangup the call.

        Returns:
          Pass if True
          Fail if False

        Priority: 0
        """
        return self.dial_a_hangup_b(self.ag, self.ag)

    @test_tracker_info(uuid='908c199b-ca65-4694-821d-1b864ee3fe69')
    @BluetoothBaseTest.bt_test_wrap
    def test_outgoing_dial_ag_hangup_hf(self):
        """
        Tests if we can make a phone call from AG role and disconnect from HF
        role.

        Precondition:
        1. Devices are connected.

        Steps:
        1. Make a call from AG role.
        2. Wait for the HF, AG to show dialing and RE to see the call ringing.
        3. Hangup the call on HF role.
        4. Wait for all devices to hangup the call.

        Returns:
          Pass if True
          Fail if False

        Priority: 0
        """
        return self.dial_a_hangup_b(self.ag, self.hf)

    @test_tracker_info(uuid='5d1d52c7-51d8-4c82-b437-2e91a6220db3')
    @BluetoothBaseTest.bt_test_wrap
    def test_outgoing_dial_hf_hangup_ag(self):
        """
        Tests if we can make a phone call from HF role and disconnect from AG
        role.

        Precondition:
        1. Devices are connected.

        Steps:
        1. Make a call from HF role.
        2. Wait for the HF, AG to show dialing and RE to see the call ringing.
        3. Hangup the call on AG role.
        4. Wait for all devices to hangup the call.

        Returns:
          Pass if True
          Fail if False

        Priority: 0
        """
        return self.dial_a_hangup_b(self.hf, self.ag)

    @test_tracker_info(uuid='a718e238-7e31-40c9-a45b-72081210cc73')
    @BluetoothBaseTest.bt_test_wrap
    def test_incoming_dial_re_hangup_re(self):
        """
        Tests if we can make a phone call from remote and disconnect from
        remote.

        Precondition:
        1. Devices are connected.

        Steps:
        1. Make a call from RE role.
        2. Wait for the HF, AG to show ringing and RE to see the call dialing.
        3. Hangup the call on RE role.
        4. Wait for all devices to hangup the call.

        Returns:
          Pass if True
          Fail if False

        Priority: 0
        """
        return self.dial_a_hangup_b(self.re, self.re, self.ag_phone_number)

    def test_bluetooth_voice_recognition_assistant(self):
        """
        Tests if we can initate a remote Voice Recognition session.

        Precondition:
        1. Devices are connected.

        Steps:
        1. Verify that audio isn't routed between the HF and AG.
        2. From the HF send a BVRA command.
        3. Verify that audio is routed from the HF to AG.
        4. From the HF send a BVRA command to stop the session.
        5. Verify that audio is no longer routed from the HF to AG.

        Returns:
          Pass if True
          Fail if False

        Priority: 0
        """
        audio_state = self.hf.droid.bluetoothHfpClientGetAudioState(
            self.ag.droid.bluetoothGetLocalAddress())
        if (audio_state != AUDIO_STATE_DISCONNECTED):
            self.log.info(
                "Audio connected before test started, current state {}.".
                format(str(audio_state)))
            return False
        bvra_started = self.hf.droid.bluetoothHfpClientStartVoiceRecognition(
            self.ag.droid.bluetoothGetLocalAddress())
        if (bvra_started != True):
            self.log.info("BVRA Failed to start.")
            return False
        time.sleep(SHORT_TIMEOUT)
        audio_state = self.hf.droid.bluetoothHfpClientGetAudioState(
            self.ag.droid.bluetoothGetLocalAddress())
        if (audio_state != AUDIO_STATE_ROUTED):
            self.log.info("Audio didn't route, current state {}.".format(
                str(audio_state)))
            return False
        bvra_stopped = self.hf.droid.bluetoothHfpClientStopVoiceRecognition(
            self.ag.droid.bluetoothGetLocalAddress())
        if (bvra_stopped != True):
            self.log.info("BVRA Failed to stop.")
            return False
        time.sleep(SHORT_TIMEOUT)
        audio_state = self.hf.droid.bluetoothHfpClientGetAudioState(
            self.ag.droid.bluetoothGetLocalAddress())
        if (audio_state != AUDIO_STATE_DISCONNECTED):
            self.log.info("Audio didn't cleanup, current state {}.".format(
                str(audio_state)))
            return False
        return True

    def dial_a_hangup_b(self, caller, callee, ph=""):
        """
        a, b and c can be either of AG, HF or Remote.
        1. Make a call from 'a' on a fixed number.
        2. Wait for the call to get connected (check on both 'a' and 'b')
           Check that 'c' is in ringing state.
        3. Hangup the call on 'b'.
        4. Wait for call to get completely disconnected
        (check on both 'a' and 'b')
        It is assumed that scenarios will not go into voice mail.
        """
        if ph == "": ph = self.re_phone_number

        # Determine if this is outgoing or incoming call.
        call_type = None
        if caller == self.ag or caller == self.hf:
            call_type = CALL_TYPE_OUTGOING
            if callee != self.ag and callee != self.hf:
                self.log.info("outgoing call should terminate at AG or HF")
                return False
        elif caller == self.re:
            call_type = CALL_TYPE_INCOMING
            if callee != self.re:
                self.log.info("Incoming call should terminate at Re")
                return False

        self.log.info("Call type is {}".format(call_type))

        # make a call on 'caller'
        if not tel_telecom_utils.dial_number(self.log, caller, ph):
            return False

        # Give time for state to update due to carrier limitations
        time.sleep(SHORT_TIMEOUT)
        # Check that everyone is in dialing/ringing state.
        ret = True
        if call_type == CALL_TYPE_OUTGOING:
            ret &= tel_telecom_utils.wait_for_dialing(self.log, self.hf)
            ret &= tel_telecom_utils.wait_for_dialing(self.log, self.ag)
            ret &= tel_telecom_utils.wait_for_ringing(self.log, self.re)
        else:
            ret &= tel_telecom_utils.wait_for_ringing(self.log, self.hf)
            ret &= tel_telecom_utils.wait_for_ringing(self.log, self.ag)
            ret &= tel_telecom_utils.wait_for_dialing(self.log, self.re)
        if not ret:
            return False

        # Give time for state to update due to carrier limitations
        time.sleep(SHORT_TIMEOUT)
        # Check if we have any calls with dialing or active state on 'b'.
        # We assume we never disconnect from 'ringing' state since it will lead
        # to voicemail.
        call_state_dialing_or_active = \
            [tel_defines.CALL_STATE_CONNECTING,
             tel_defines.CALL_STATE_DIALING,
             tel_defines.CALL_STATE_ACTIVE]

        calls_in_dialing_or_active = tel_telecom_utils.get_calls_in_states(
            self.log, callee, call_state_dialing_or_active)

        # Make sure there is only one!
        if len(calls_in_dialing_or_active) != 1:
            self.log.info("Call State in dialing or active failed {}".format(
                calls_in_dialing_or_active))
            return False

        # Hangup the *only* call on 'callee'
        if not car_telecom_utils.hangup_call(self.log, callee,
                                             calls_in_dialing_or_active[0]):
            return False

        time.sleep(SHORT_TIMEOUT)
        # Make sure everyone got out of in call state.
        for d in self.android_devices:
            ret &= tel_telecom_utils.wait_for_not_in_call(self.log, d)
        return ret
