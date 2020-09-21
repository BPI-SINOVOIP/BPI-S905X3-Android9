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
Test the HFP profile for calling and connection management.
"""

import time

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BluetoothCarHfpBaseTest import BluetoothCarHfpBaseTest
from acts.test_utils.bt import BtEnum
from acts.test_utils.bt import bt_test_utils
from acts.test_utils.car import car_bt_utils
from acts.test_utils.car import car_telecom_utils
from acts.test_utils.tel import tel_defines
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import initiate_call
from acts.test_utils.tel.tel_test_utils import wait_and_answer_call

BLUETOOTH_PKG_NAME = "com.android.bluetooth"
CALL_TYPE_OUTGOING = "CALL_TYPE_OUTGOING"
CALL_TYPE_INCOMING = "CALL_TYPE_INCOMING"
default_timeout = 20


class BtCarHfpConnectionTest(BluetoothCarHfpBaseTest):
    def setup_class(self):
        if not super(BtCarHfpConnectionTest, self).setup_class():
            return False

        # Disable all
        car_bt_utils.set_car_profile_priorities_off(self.hf, self.ag)

        # Enable A2DP
        bt_test_utils.set_profile_priority(
            self.hf, self.ag, [BtEnum.BluetoothProfile.HEADSET_CLIENT],
            BtEnum.BluetoothPriorityLevel.PRIORITY_ON)

        return True

    def setup_test(self):
        if not super(BtCarHfpConnectionTest, self).setup_test():
            return False
        self.hf.droid.bluetoothDisconnectConnected(
            self.ag.droid.bluetoothGetLocalAddress())

    @test_tracker_info(uuid='a6669f9b-fb49-4bd8-aa9c-9d6369e34442')
    @BluetoothBaseTest.bt_test_wrap
    def test_call_transfer_disconnect_connect(self):
        """
        Tests that after we connect when an active call is in progress,
        we show the call.

        Precondition:
        1. AG & HF are disconnected but paired.

        Steps:
        1. Make a call from AG role (since disconnected)
        2. Accept from RE role and transition the call to Active
        3. Connect AG & HF
        4. HF should transition into Active call state.

        Returns:
          Pass if True
          Fail if False

        Priority: 1
        """
        # make a call on AG
        if not initiate_call(self.log, self.ag, self.re_phone_number):
            self.ag.log.error("Failed to initiate call from ag.")
            return False
        if not wait_and_answer_call(self.log, self.re):
            self.re.log.error("Failed to accept call on re.")
            return False

        # Wait for AG, RE to go into an Active state.
        if not car_telecom_utils.wait_for_active(self.log, self.ag):
            self.ag.log.error("AG not in Active state.")
            return False
        if not car_telecom_utils.wait_for_active(self.log, self.re):
            self.re.log.error("RE not in Active state.")
            return False

        # Now connect the devices.
        if not bt_test_utils.connect_pri_to_sec(
                self.hf, self.ag,
                set([BtEnum.BluetoothProfile.HEADSET_CLIENT.value])):
            self.log.error("Could not connect HF and AG {} {}".format(
                self.hf.serial, self.ag.serial))
            return False

        # Check that HF is in active state
        if not car_telecom_utils.wait_for_active(self.log, self.hf):
            self.hf.log.error("HF not in Active state.")
            return False

        # Hangup the call and check all devices are clean
        self.hf.droid.telecomEndCall()
        ret = True
        ret &= car_telecom_utils.wait_for_not_in_call(self.log, self.hf)
        ret &= car_telecom_utils.wait_for_not_in_call(self.log, self.ag)
        ret &= car_telecom_utils.wait_for_not_in_call(self.log, self.re)

        return ret

    @test_tracker_info(uuid='97727b64-a590-4d84-a257-1facd8aafd16')
    @BluetoothBaseTest.bt_test_wrap
    def test_call_transfer_off_on(self):
        """
        Tests that after we turn adapter on when an active call is in
        progress, we show the call.

        Precondition:
        1. AG & HF are disconnected but paired.
        2. HF's adapter is OFF

        Steps:
        1. Make a call from AG role (since disconnected)
        2. Accept from RE role and transition the call to Active
        3. Turn HF's adapter ON
        4. HF should transition into Active call state.

        Returns:
          Pass if True
          Fail if False

        Priority: 1
        """
        # Connect HF & AG
        if not bt_test_utils.connect_pri_to_sec(
                self.hf, self.ag,
                set([BtEnum.BluetoothProfile.HEADSET_CLIENT.value])):
            self.log.error("Could not connect HF and AG {} {}".format(
                self.hf.serial, self.ag.serial))
            return False

        # make a call on AG
        if not initiate_call(self.log, self.ag, self.re_phone_number):
            self.ag.log.error("Failed to initiate call from ag.")
            return False

        # Wait for all HF
        if not car_telecom_utils.wait_for_dialing(self.log, self.hf):
            self.hf.log.error("HF not in ringing state.")
            return False

        # Accept the call on RE
        if not wait_and_answer_call(self.log, self.re):
            self.re.log.error("Failed to accept call on re.")
            return False
        # Wait for all HF, AG, RE to go into an Active state.
        if not car_telecom_utils.wait_for_active(self.log, self.hf):
            self.hf.log.error("HF not in Active state.")
            return False
        if not car_telecom_utils.wait_for_active(self.log, self.ag):
            self.ag.log.error("AG not in Active state.")
            return False
        if not car_telecom_utils.wait_for_active(self.log, self.re):
            self.re.log.error("RE not in Active state.")
            return False

        # Turn the adapter OFF on HF
        if not bt_test_utils.disable_bluetooth(self.hf.droid):
            self.hf.log.error("Failed to turn BT off on HF.")
            return False

        # Turn adapter ON on HF
        if not bt_test_utils.enable_bluetooth(self.hf.droid, self.hf.ed):
            self.hf.log.error("Failed to turn BT ON after call on HF.")
            return False

        # Check that HF is in active state
        if not car_telecom_utils.wait_for_active(self.log, self.hf):
            self.hf.log.error("HF not in Active state.")
            return False

        # Hangup the call and check all devices are clean
        self.hf.droid.telecomEndCall()
        ret = True
        ret &= car_telecom_utils.wait_for_not_in_call(self.log, self.hf)
        ret &= car_telecom_utils.wait_for_not_in_call(self.log, self.ag)
        ret &= car_telecom_utils.wait_for_not_in_call(self.log, self.re)

        return ret

    @test_tracker_info(uuid='95f76e2c-1cdd-4a7c-8e26-863b4c4242be')
    @BluetoothBaseTest.bt_test_wrap
    def test_call_transfer_connect_disconnect_connect(self):
        """
        Test that when we go from connect -> disconnect -> connect on an active
        call then the call is restored on HF.

        Precondition:
        1. AG & HF are paired

        Steps:
        0. Connect AG & HF
        1. Make a call from HF role
        2. Accept from RE role and transition the call to Active
        3. Disconnect AG & HF
        4. Verify that we don't have any calls on HF
        5. Connect AG & HF
        6. Verify that HF gets the call back.

        Returns:
          Pass if True
          Fail if False

        Priority: 1
        """
        # Now connect the devices.
        if not bt_test_utils.connect_pri_to_sec(
                self.hf, self.ag,
                set([BtEnum.BluetoothProfile.HEADSET_CLIENT.value])):
            self.log.error("Could not connect HF and AG {} {}".format(
                self.hf.serial, self.ag.serial))
            return False

        # make a call on HF
        if not car_telecom_utils.dial_number(self.log, self.hf,
                                             self.re_phone_number):
            self.hf.log.error("HF not in dialing state.")
            return False

        # Wait for HF, AG to be dialing and RE to be ringing
        ret = True
        ret &= car_telecom_utils.wait_for_dialing(self.log, self.hf)
        #uncomment once sl4a code has been merged.
        ret &= car_telecom_utils.wait_for_dialing(self.log, self.ag)
        ret &= car_telecom_utils.wait_for_ringing(self.log, self.re)

        if not ret:
            self.log.error("Outgoing call did not get established")
            return False

        # Accept call on RE.
        if not wait_and_answer_call(self.log, self.re):
            self.re.log.error("Failed to accept call on re.")
            return False

        ret &= car_telecom_utils.wait_for_active(self.log, self.hf)
        ret &= car_telecom_utils.wait_for_active(self.log, self.ag)
        ret &= car_telecom_utils.wait_for_active(self.log, self.re)

        if not ret:
            self.log.error("Outgoing call did not transition to active")
            return False

        # Disconnect HF & AG
        self.hf.droid.bluetoothDisconnectConnected(
            self.ag.droid.bluetoothGetLocalAddress())

        # We use the proxy of the Call going away as HF disconnected
        if not car_telecom_utils.wait_for_not_in_call(self.log, self.hf):
            self.hf.log.error("HF still in call after disconnection.")
            return False

        # Now connect the devices.
        if not bt_test_utils.connect_pri_to_sec(
                self.hf, self.ag,
                set([BtEnum.BluetoothProfile.HEADSET_CLIENT.value])):
            self.log.error("Could not connect HF and AG {} {}".format(
                self.hf.serial, self.ag.serial))
            # Additional profile connection check for b/
            if not bt_test_utils.is_hfp_client_device_connected(
                    self.hf, self.ag.droid.bluetoothGetLocalAddress()):
                self.hf.log.info(
                    "HFP Client connected even though connection state changed "
                    + " event not found")
                return False

        # Check that HF is in active state
        if not car_telecom_utils.wait_for_active(self.log, self.hf):
            self.hf.log.error("HF not in Active state.")
            return False

        # Hangup the call and check all devices are clean
        self.hf.droid.telecomEndCall()
        ret &= car_telecom_utils.wait_for_not_in_call(self.log, self.hf)
        ret &= car_telecom_utils.wait_for_not_in_call(self.log, self.ag)
        ret &= car_telecom_utils.wait_for_not_in_call(self.log, self.re)

        return ret
