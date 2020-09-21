# /usr/bin/env python3.4
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
Test the HFP profile for conference calling functionality.
"""

import time

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BluetoothCarHfpBaseTest import BluetoothCarHfpBaseTest
from acts.test_utils.bt import BtEnum
from acts.test_utils.bt import bt_test_utils
from acts.test_utils.car import car_telecom_utils
from acts.test_utils.tel import tel_defines
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import initiate_call
from acts.test_utils.tel.tel_test_utils import wait_and_answer_call
from acts.test_utils.tel.tel_test_utils import wait_for_ringing_call
from acts.test_utils.tel.tel_voice_utils import get_audio_route
from acts.test_utils.tel.tel_voice_utils import set_audio_route
from acts.test_utils.tel.tel_voice_utils import swap_calls

BLUETOOTH_PKG_NAME = "com.android.bluetooth"
CALL_TYPE_OUTGOING = "CALL_TYPE_OUTGOING"
CALL_TYPE_INCOMING = "CALL_TYPE_INCOMING"
SHORT_TIMEOUT = 3


class BtCarHfpConferenceTest(BluetoothCarHfpBaseTest):
    def setup_class(self):
        if not super(BtCarHfpConferenceTest, self).setup_class():
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
        return connected

    @test_tracker_info(uuid='a9657693-b534-4625-bf91-69a1d1b9a943')
    @BluetoothBaseTest.bt_test_wrap
    def test_multi_way_call_accept(self):
        """
        Tests if we can have a 3-way calling between re, RE2 and AG/HF.

        Precondition:
        1. Devices are connected over HFP.

        Steps:
        1. Make a call from re to AG
        2. Wait for dialing on re and ringing on HF/AG.
        3. Accept the call on HF
        4. Make a call on RE2 to AG
        5. Wait for dialing on re and ringing on HF/AG.
        6. Accept the call on HF.
        7. See that HF/AG have one active and one held call.
        8. Merge the call on HF.
        9. Verify that we have a conference call on HF/AG.
        10. Hangup the call on HF.
        11. Wait for all devices to go back into stable state.

        Returns:
          Pass if True
          Fail if False

        Priority: 0
        """
        timeout_for_state_updates = 3
        # Dial AG from re
        if not initiate_call(self.log, self.re, self.ag_phone_number):
            self.log.error("Failed to initiate call from re.")
            return False

        # Wait for dialing/ringing
        ret = True
        ret &= wait_for_ringing_call(self.log, self.ag)
        ret &= car_telecom_utils.wait_for_ringing(self.log, self.hf)

        if not ret:
            self.log.error("Failed to dial incoming number from")
            return False

        # Give time for state to update due to carrier limitations
        time.sleep(SHORT_TIMEOUT)
        # Extract the call.
        call_1 = car_telecom_utils.get_calls_in_states(
            self.log, self.hf, [tel_defines.CALL_STATE_RINGING])
        if len(call_1) != 1:
            self.hf.log.error("Call State in ringing failed {}".format(call_1))
            return False

        # Accept the call on HF
        if not car_telecom_utils.accept_call(self.log, self.hf, call_1[0]):
            self.hf.log.error("Accepting call failed {}".format(
                self.hf.serial))
            return False

        # Dial another call from RE2
        if not initiate_call(self.log, self.re2, self.ag_phone_number):
            self.re2.log.error("Failed to initiate call from re.")
            return False

        # Wait for dialing/ringing
        ret &= wait_for_ringing_call(self.log, self.ag)
        ret &= car_telecom_utils.wait_for_ringing(self.log, self.hf)

        if not ret:
            self.log.error("AG and HF not in ringing state.")
            return False

        # Give time for state to update due to carrier limitations
        time.sleep(SHORT_TIMEOUT)
        # Extract the call.
        # input("Continue?")
        call_2 = car_telecom_utils.get_calls_in_states(
            self.log, self.hf, [tel_defines.CALL_STATE_RINGING])
        if len(call_2) != 1:
            self.hf.log.info("Call State in ringing failed {}".format(call_2))
            return False

        # Accept the call on HF
        if not car_telecom_utils.accept_call(self.log, self.hf, call_2[0]):
            self.hf.log.info("Accepting call failed {}".format(self.hf.serial))
            return False

        # Merge the calls now.
        self.hf.droid.telecomCallJoinCallsInConf(call_1[0], call_2[0])

        # Check if we are in conference with call_1 and call_2
        conf_call_id = car_telecom_utils.wait_for_conference(
            self.log, self.hf, [call_1[0], call_2[0]])
        if conf_call_id == None:
            self.hf.log.error("Did not get the conference setup correctly")
            return False

        # Now hangup the conference call.
        if not car_telecom_utils.hangup_conf(self.log, self.hf, conf_call_id):
            self.hf.log.error("Could not hangup conference call {}!".format(
                conf_call_id))
            return False

        return True
