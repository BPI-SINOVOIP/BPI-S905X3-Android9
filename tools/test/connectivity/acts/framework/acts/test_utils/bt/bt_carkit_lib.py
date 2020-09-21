#/usr/bin/env python3.4
#
# Copyright (C) 2018 The Android Open Source Project
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

import time
import os

from acts.keys import Config
from acts.utils import rand_ascii_str
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_constants import logcat_strings
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import AUDIO_ROUTE_BLUETOOTH
from acts.test_utils.tel.tel_defines import AUDIO_ROUTE_EARPIECE
from acts.test_utils.tel.tel_defines import AUDIO_ROUTE_SPEAKER
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_test_utils import get_phone_number
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import initiate_call
from acts.test_utils.tel.tel_test_utils import num_active_calls
from acts.test_utils.tel.tel_test_utils import sms_send_receive_verify
from acts.test_utils.tel.tel_test_utils import wait_and_answer_call
from acts.test_utils.tel.tel_voice_utils import get_audio_route
from acts.test_utils.tel.tel_voice_utils import set_audio_route
from acts.test_utils.tel.tel_voice_utils import swap_calls
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.utils import exe_cmd
from acts.utils import get_current_epoch_time

KEYCODE_VOLUME_UP = "input keyevent 24"
KEYCODE_VOLUME_DOWN = "input keyevent 25"
KEYCODE_EVENT_PLAY_PAUSE = "input keyevent 85"
KEYCODE_MEDIA_STOP = "input keyevent 86"
KEYCODE_EVENT_NEXT = "input keyevent 87"
KEYCODE_EVENT_PREVIOUS = "input keyevent 88"
KEYCODE_MEDIA_REWIND = "input keyevent 89"
KEYCODE_MEDIA_FAST_FORWARD = "input keyevent 90"
KEYCODE_MUTE = "input keyevent 91"

default_timeout = 10


class E2eBtCarkitLib():

    android_devices = []
    short_timeout = 3
    active_call_id = None
    hold_call_id = None
    log = None
    mac_address = None

    def __init__(self, log, target_mac_address=None):
        self.log = log
        self.target_mac_address = target_mac_address

    def connect_hsp_helper(self, ad):
        end_time = time.time() + default_timeout + 10
        connected_hsp_devices = len(ad.droid.bluetoothHspGetConnectedDevices())
        while connected_hsp_devices != 1 and time.time() < end_time:
            try:
                ad.droid.bluetoothHspConnect(self.target_mac_address)
                time.sleep(3)
                if len(ad.droid.bluetoothHspGetConnectedDevices() == 1):
                    break
            except Exception:
                self.log.debug("Failed to connect hsp trying again...")
            try:
                ad.droid.bluetoothConnectBonded(self.target_mac_address)
            except Exception:
                self.log.info("Failed to connect to bonded device...")
            connected_hsp_devices = len(
                ad.droid.bluetoothHspGetConnectedDevices())
        if connected_hsp_devices != 1:
            self.log.error("Failed to reconnect to HSP service...")
            return False
        self.log.info("Connected to HSP service...")
        return True

    def setup_multi_call(self, caller0, caller1, callee):
        outgoing_num = get_phone_number(self.log, callee)
        if not initiate_call(self.log, caller0, outgoing_num):
            self.log.error("Failed to initiate call")
            return False
        if not wait_and_answer_call(self.log, callee):
            self.log.error("Failed to answer call.")
            return False
        time.sleep(self.short_timeout)
        if not initiate_call(self.log, caller1, outgoing_num):
            self.log.error("Failed to initiate call")
            return False
        if not wait_and_answer_call(self.log, callee):
            self.log.error("Failed to answer call.")
            return False
        return True

    def process_tests(self, tests):
        for test in tests:
            try:
                test()
            except Exception as err:
                self.log.error(err)

    def run_suite_hfp_tests(self):
        tests = [
            self.outgoing_call_private_number,
            self.outgoing_call_unknown_contact,
            self.incomming_call_private_number,
            self.incomming_call_unknown_contact,
            self.outgoing_call_multiple_iterations,
            self.outgoing_call_hsp_disabled_then_enabled_during_call,
            self.call_audio_routes,
            self.sms_during_incomming_call,
            self.multi_incomming_call,
            self.multi_call_audio_routing,
            self.multi_call_swap_multiple_times,
            self.outgoing_call_a2dp_play_before_and_after,
        ]
        _process_tests(tests)

    def run_suite_hfp_conf_tests(self):
        tests = [
            self.multi_call_join_conference_call,
            self.multi_call_join_conference_call_hangup_conf_call,
            self.outgoing_multi_call_join_conference_call,
            self.multi_call_join_conference_call_audio_routes,
        ]
        _process_tests(tests)

    def run_suite_map_tests(self):
        tests = [
            self.sms_receive_different_sizes,
            self.sms_receive_multiple,
            self.sms_send_outgoing_texts,
        ]
        _process_tests(tests)

    def run_suite_avrcp_tests(self):
        tests = [
            self.avrcp_play_pause,
            self.avrcp_next_previous_song,
            self.avrcp_next_previous,
            self.avrcp_next_repetative,
        ]
        _process_tests(tests)

    def disconnect_reconnect_multiple_iterations(self, pri_dut):
        iteration_count = 5
        self.log.info(
            "Test disconnect-reconnect scenario from phone {} times.".format(
                iteration_count))
        self.log.info(
            "This test will prompt for user interaction after each reconnect.")
        input("Press enter to execute this testcase...")
        #Assumes only one devices connected
        grace_timeout = 4  #disconnect and reconnect timeout
        for n in range(iteration_count):
            self.log.info("Test iteration {}.".format(n + 1))
            self.log.info("Disconnecting device {}...".format(
                self.target_mac_address))
            pri_dut.droid.bluetoothDisconnectConnected(self.target_mac_address)
            # May have to do a longer sleep for carkits.... need to test
            time.sleep(grace_timeout)
            self.log.info("Connecting device {}...".format(
                self.target_mac_address))
            pri_dut.droid.bluetoothConnectBonded(self.target_mac_address)
            if not self.connect_hsp_helper(pri_dut):
                return False
            start_time = time.time()
            connected_devices = pri_dut.droid.bluetoothGetConnectedDevices()
            self.log.info(
                "Waiting up to 10 seconds for device to reconnect...")
            while time.time() < start_time + 10 and len(connected_devices) != 1:
                connected_devices = pri_dut.droid.bluetoothGetConnectedDevices(
                )
                time.sleep(1)
            if len(connected_devices) != 1:
                self.log.error(
                    "Failed to reconnect at iteration {}... continuing".format(
                        n))
                return False
            input("Continue to next iteration?")
        return True

    def disconnect_a2dp_only_then_reconnect(self, pri_dut):
        self.log.info(
            "Test disconnect-reconnect a2dp only scenario from phone.")
        input("Press enter to execute this testcase...")
        if not pri_dut.droid.bluetoothA2dpDisconnect(self.target_mac_address):
            self.log.error("Failed to disconnect A2DP service...")
            return False
        time.sleep(self.short_timeout)
        result = input("Confirm A2DP disconnected? (Y/n) ")
        if result == "n":
            self.log.error(
                "Tester confirmed that A2DP did not disconnect. Failing test.")
            return False
        if len(pri_dut.droid.bluetoothA2dpGetConnectedDevices()) != 0:
            self.log.error("Failed to disconnect from A2DP service")
            return False
        pri_dut.droid.bluetoothA2dpConnect(self.target_mac_address)
        time.sleep(self.short_timeout)
        if len(pri_dut.droid.bluetoothA2dpGetConnectedDevices()) != 1:
            self.log.error("Failed to reconnect to A2DP service...")
            return False
        return True

    def disconnect_hsp_only_then_reconnect(self, pri_dut):
        self.log.info(
            "Test disconnect-reconnect hsp only scenario from phone.")
        input("Press enter to execute this testcase...")
        if not pri_dut.droid.bluetoothHspDisconnect(self.target_mac_address):
            self.log.error("Failed to disconnect HSP service...")
            return False
        time.sleep(self.short_timeout)
        result = input("Confirm HFP disconnected? (Y/n) ")
        pri_dut.droid.bluetoothHspConnect(self.target_mac_address)
        time.sleep(self.short_timeout)
        if len(pri_dut.droid.bluetoothHspGetConnectedDevices()) != 1:
            self.log.error("Failed to connect from HSP service")
            return False
        return True

    def disconnect_both_hsp_and_a2dp_then_reconnect(self, pri_dut):
        self.log.info(
            "Test disconnect-reconnect hsp and a2dp scenario from phone.")
        input("Press enter to execute this testcase...")
        if not pri_dut.droid.bluetoothA2dpDisconnect(self.target_mac_address):
            self.log.error("Failed to disconnect A2DP service...")
            return False
        if not pri_dut.droid.bluetoothHspDisconnect(self.target_mac_address):
            self.log.error("Failed to disconnect HSP service...")
            return False
        time.sleep(self.short_timeout)
        if len(pri_dut.droid.bluetoothA2dpGetConnectedDevices()) != 0:
            self.log.error("Failed to disconnect from A2DP service")
            return False
        if len(pri_dut.droid.bluetoothHspGetConnectedDevices()) != 0:
            self.log.error("Failed to disconnect from HSP service")
            return False
        result = input("Confirm HFP and A2DP disconnected? (Y/n) ")
        pri_dut.droid.bluetoothConnectBonded(self.target_mac_address)
        time.sleep(self.short_timeout)
        if len(pri_dut.droid.bluetoothA2dpGetConnectedDevices()) != 1:
            self.log.error("Failed to reconnect to A2DP service...")
            return False
        if not self.connect_hsp_helper(pri_dut):
            return False
        return True

    def outgoing_call_private_number(self, pri_dut, ter_dut):
        self.log.info(
            "Test outgoing call scenario from phone to private number")
        input("Press enter to execute this testcase...")
        outgoing_num = "*67" + get_phone_number(self.log, ter_dut)
        if not initiate_call(self.log, pri_dut, outgoing_num):
            self.log.error("Failed to initiate call")
            return False
        if not wait_and_answer_call(self.log, ter_dut):
            self.log.error("Failed to answer call.")
            return False
        time.sleep(self.short_timeout)
        input("Press enter to hangup call...")
        if not hangup_call(self.log, pri_dut):
            self.log.error("Failed to hangup call")
            return False
        return True

    def outgoing_call_a2dp_play_before_and_after(self, pri_dut, sec_dut):
        self.log.info(
            "Test outgoing call scenario while playing music. Music should resume after call."
        )
        pri_dut.adb.shell(KEYCODE_EVENT_PREVIOUS)
        input(
            "Press enter to execute this testcase when music is in a play state..."
        )
        outgoing_num = get_phone_number(self.log, sec_dut)
        if not initiate_call(self.log, pri_dut, outgoing_num):
            self.log.error("Failed to initiate call")
            return False
        if not wait_and_answer_call(self.log, sec_dut):
            self.log.error("Failed to answer call.")
            return False
        time.sleep(self.short_timeout)
        input("Press enter to hangup call...")
        if not hangup_call(self.log, pri_dut):
            self.log.error("Failed to hangup call")
            return False
        input("Press enter when music continues to play.")
        self.log.info("Pausing Music...")
        pri_dut.adb.shell(KEYCODE_EVENT_PLAY_PAUSE)
        return True

    def outgoing_call_unknown_contact(self, pri_dut, ter_dut):
        self.log.info(
            "Test outgoing call scenario from phone to unknow contact")
        input("Press enter to execute this testcase...")
        outgoing_num = get_phone_number(self.log, ter_dut)
        if not initiate_call(self.log, pri_dut, outgoing_num):
            self.log.error("Failed to initiate call")
            return False
        if not wait_and_answer_call(self.log, ter_dut):
            self.log.error("Failed to answer call.")
            return False
        time.sleep(self.short_timeout)
        input("Press enter to hangup call...")
        if not hangup_call(self.log, pri_dut):
            self.log.error("Failed to hangup call")
            return False
        return True

    def incomming_call_private_number(self, pri_dut, ter_dut):
        self.log.info(
            "Test incomming call scenario to phone from private number")
        input("Press enter to execute this testcase...")
        outgoing_num = "*67" + get_phone_number(self.log, pri_dut)
        if not initiate_call(self.log, ter_dut, outgoing_num):
            self.log.error("Failed to initiate call")
            return False
        if not wait_and_answer_call(self.log, pri_dut):
            self.log.error("Failed to answer call.")
            return False
        time.sleep(self.short_timeout)
        input("Press enter to hangup call...")
        if not hangup_call(self.log, ter_dut):
            self.log.error("Failed to hangup call")
            return False

        return True

    def incomming_call_unknown_contact(self, pri_dut, ter_dut):
        self.log.info(
            "Test incomming call scenario to phone from unknown contact")
        input("Press enter to execute this testcase...")
        outgoing_num = get_phone_number(self.log, pri_dut)
        if not initiate_call(self.log, ter_dut, outgoing_num):
            self.log.error("Failed to initiate call")
            return False
        if not wait_and_answer_call(self.log, pri_dut):
            self.log.error("Failed to answer call.")
            return False
        time.sleep(self.short_timeout)
        input("Press enter to hangup call...")
        if not hangup_call(self.log, ter_dut):
            self.log.error("Failed to hangup call")
            return False
        return True

    def outgoing_call_multiple_iterations(self, pri_dut, sec_dut):
        iteration_count = 3
        self.log.info(
            "Test outgoing call scenario from phone {} times from known contact".
            format(iteration_count))
        input("Press enter to execute this testcase...")
        outgoing_num = get_phone_number(self.log, sec_dut)
        for _ in range(iteration_count):
            if not initiate_call(self.log, pri_dut, outgoing_num):
                self.log.error("Failed to initiate call")
                return False
            if not wait_and_answer_call(self.log, sec_dut):
                self.log.error("Failed to answer call.")
                return False
            time.sleep(self.short_timeout)
            if not hangup_call(self.log, pri_dut):
                self.log.error("Failed to hangup call")
                return False
        return True

    def outgoing_call_hsp_disabled_then_enabled_during_call(
            self, pri_dut, sec_dut):
        self.log.info(
            "Test outgoing call hsp disabled then enable during call.")
        input("Press enter to execute this testcase...")
        outgoing_num = get_phone_number(self.log, sec_dut)
        if not pri_dut.droid.bluetoothHspDisconnect(self.target_mac_address):
            self.log.error("Failed to disconnect HSP service...")
            return False
        time.sleep(self.short_timeout)
        if len(pri_dut.droid.bluetoothHspGetConnectedDevices()) != 0:
            self.log.error("Failed to disconnect from HSP service")
            return False
        if not initiate_call(self.log, pri_dut, outgoing_num):
            self.log.error("Failed to initiate call")
            return False
        time.sleep(default_timeout)
        pri_dut.droid.bluetoothConnectBonded(self.target_mac_address)
        time.sleep(self.short_timeout)
        test_result = True
        if len(pri_dut.droid.bluetoothHspGetConnectedDevices()) != 1:
            self.log.error("Failed to reconnect to HSP service...")
            return
        if not hangup_call(self.log, pri_dut):
            self.log.error("Failed to hangup call")
            return False
        return test_result

    def call_audio_routes(self, pri_dut, sec_dut):
        self.log.info("Test various audio routes scenario from phone.")
        input("Press enter to execute this testcase...")
        outgoing_num = get_phone_number(self.log, sec_dut)
        if not initiate_call(self.log, pri_dut, outgoing_num):
            self.log.error("Failed to initiate call")
            return False
        if not wait_and_answer_call(self.log, sec_dut):
            self.log.error("Failed to answer call.")
            return False
        time.sleep(self.short_timeout)
        call_id = pri_dut.droid.telecomCallGetCallIds()[0]
        pri_dut.droid.telecomCallPlayDtmfTone(call_id, "9")
        input("Press enter to switch to speaker...")
        self.log.info("Switching to speaker.")
        set_audio_route(self.log, pri_dut, AUDIO_ROUTE_SPEAKER)
        time.sleep(self.short_timeout)
        if get_audio_route(self.log, pri_dut) != AUDIO_ROUTE_SPEAKER:
            self.log.error(
                "Audio Route not set to {}".format(AUDIO_ROUTE_SPEAKER))
            return False
        input("Press enter to switch to earpiece...")
        self.log.info("Switching to earpiece.")
        set_audio_route(self.log, pri_dut, AUDIO_ROUTE_EARPIECE)
        time.sleep(self.short_timeout)
        if get_audio_route(self.log, pri_dut) != AUDIO_ROUTE_EARPIECE:
            self.log.error(
                "Audio Route not set to {}".format(AUDIO_ROUTE_EARPIECE))
            return False
        input("Press enter to switch to Bluetooth...")
        self.log.info("Switching to Bluetooth...")
        set_audio_route(self.log, pri_dut, AUDIO_ROUTE_BLUETOOTH)
        time.sleep(self.short_timeout)
        if get_audio_route(self.log, pri_dut) != AUDIO_ROUTE_BLUETOOTH:
            self.log.error(
                "Audio Route not set to {}".format(AUDIO_ROUTE_BLUETOOTH))
            return False
        input("Press enter to hangup call...")
        self.log.info("Hanging up call...")
        pri_dut.droid.telecomCallStopDtmfTone(call_id)
        if not hangup_call(self.log, pri_dut):
            self.log.error("Failed to hangup call")
            return False
        return True

    def sms_receive_different_sizes(self, pri_dut, sec_dut):
        self.log.info("Test recieve sms.")
        input("Press enter to execute this testcase...")
        msg = [rand_ascii_str(50), rand_ascii_str(1), rand_ascii_str(500)]
        if not sms_send_receive_verify(self.log, sec_dut, pri_dut, msg):
            return False
        else:
            self.log.info("Successfully sent sms. Please verify on carkit.")
        return True

    def sms_receive_multiple(self, pri_dut, sec_dut):
        text_count = 10
        self.log.info(
            "Test sending {} sms messages to phone.".format(text_count))
        input("Press enter to execute this testcase...")
        for _ in range(text_count):
            msg = [rand_ascii_str(50)]
            if not sms_send_receive_verify(self.log, sec_dut, pri_dut, msg):
                return False
            else:
                self.log.info(
                    "Successfully sent sms. Please verify on carkit.")
        return True

    def sms_send_outgoing_texts(self, pri_dut, sec_dut):
        self.log.info("Test send sms of different sizes.")
        input("Press enter to execute this testcase...")
        msg = [rand_ascii_str(50), rand_ascii_str(1), rand_ascii_str(500)]
        if not sms_send_receive_verify(self.log, pri_dut, sec_dut, msg):
            return False
        else:
            self.log.info("Successfully sent sms. Please verify on carkit.")
        return True

    def sms_during_incomming_call(self, pri_dut, sec_dut):
        self.log.info(
            "Test incomming call scenario to phone from unknown contact")
        input("Press enter to execute this testcase...")
        outgoing_num = get_phone_number(self.log, pri_dut)
        if not initiate_call(self.log, sec_dut, outgoing_num):
            self.log.error("Failed to initiate call")
            return False
        if not wait_and_answer_call(self.log, pri_dut):
            self.log.error("Failed to answer call.")
            return False
        time.sleep(self.short_timeout)
        msg = [rand_ascii_str(10)]
        if not sms_send_receive_verify(self.log, sec_dut, pri_dut, msg):
            return False
        else:
            self.log.info("Successfully sent sms. Please verify on carkit.")
        input("Press enter to hangup call...")
        if not hangup_call(self.log, sec_dut):
            self.log.error("Failed to hangup call")
            return False
        return True

    def multi_incomming_call(self, pri_dut, sec_dut, ter_dut):
        self.log.info("Test 2 incomming calls scenario to phone.")
        input("Press enter to execute this testcase...")
        if not self.setup_multi_call(sec_dut, ter_dut, pri_dut):
            return False
        input("Press enter to hangup call 1...")
        if not hangup_call(self.log, sec_dut):
            self.log.error("Failed to hangup call")
            return False
        input("Press enter to hangup call 2...")
        if not hangup_call(self.log, ter_dut):
            self.log.error("Failed to hangup call")
            return False
        return True

    def multi_call_audio_routing(self, pri_dut, sec_dut, ter_dut):
        self.log.info(
            "Test 2 incomming calls scenario to phone, then test audio routing."
        )
        input("Press enter to execute this testcase...")
        if not self.setup_multi_call(sec_dut, ter_dut, pri_dut):
            return False
        input("Press enter to switch to earpiece...")
        self.log.info("Switching to earpiece.")
        set_audio_route(self.log, pri_dut, AUDIO_ROUTE_EARPIECE)
        time.sleep(self.short_timeout)
        if get_audio_route(self.log, pri_dut) != AUDIO_ROUTE_EARPIECE:
            self.log.error(
                "Audio Route not set to {}".format(AUDIO_ROUTE_EARPIECE))
            return False
        input("Press enter to switch to Bluetooth...")
        self.log.info("Switching to Bluetooth...")
        set_audio_route(self.log, pri_dut, AUDIO_ROUTE_BLUETOOTH)
        time.sleep(self.short_timeout)
        if get_audio_route(self.log, pri_dut) != AUDIO_ROUTE_BLUETOOTH:
            self.log.error(
                "Audio Route not set to {}".format(AUDIO_ROUTE_BLUETOOTH))
            return False
        input("Press enter to hangup call 1...")
        if not hangup_call(self.log, sec_dut):
            self.log.error("Failed to hangup call")
            return False
        input("Press enter to hangup call 2...")
        if not hangup_call(self.log, ter_dut):
            self.log.error("Failed to hangup call")
            return False
        return True

    def multi_call_swap_multiple_times(self, pri_dut, sec_dut, ter_dut):
        self.log.info(
            "Test 2 incomming calls scenario to phone, then test audio routing."
        )
        input("Press enter to execute this testcase...")
        if not self.setup_multi_call(sec_dut, ter_dut, pri_dut):
            return False
        input("Press enter to swap active calls...")
        calls = pri_dut.droid.telecomCallGetCallIds()
        if not swap_calls(self.log, [pri_dut, sec_dut, ter_dut], calls[0],
                          calls[1], 5):
            return False
        input("Press enter to hangup call 1...")
        if not hangup_call(self.log, sec_dut):
            self.log.error("Failed to hangup call")
            return False
        input("Press enter to hangup call 2...")
        if not hangup_call(self.log, ter_dut):
            self.log.error("Failed to hangup call")
            return False
        return True

    def multi_call_join_conference_call(self, pri_dut, sec_dut, ter_dut):
        self.log.info(
            "Test 2 incomming calls scenario to phone then join the calls.")
        input("Press enter to execute this testcase...")
        if not self.setup_multi_call(sec_dut, ter_dut, pri_dut):
            return False
        input("Press enter to join active calls...")
        calls = pri_dut.droid.telecomCallGetCallIds()
        pri_dut.droid.telecomCallJoinCallsInConf(calls[0], calls[1])
        time.sleep(WAIT_TIME_IN_CALL)
        if num_active_calls(self.log, pri_dut) != 4:
            self.log.error("Total number of call ids in {} is not 4.".format(
                pri_dut.serial))
            return False
        input("Press enter to hangup call 1...")
        if not hangup_call(self.log, sec_dut):
            self.log.error("Failed to hangup call")
            return False
        input("Press enter to hangup call 2...")
        if not hangup_call(self.log, ter_dut):
            self.log.error("Failed to hangup call")
            return False
        return True

    def multi_call_join_conference_call_hangup_conf_call(
            self, pri_dut, sec_dut, ter_dut):
        self.log.info(
            "Test 2 incomming calls scenario to phone then join the calls, then terminate the call from the primary dut."
        )
        input("Press enter to execute this testcase...")
        if not self.setup_multi_call(sec_dut, ter_dut, pri_dut):
            return False
        input("Press enter to join active calls...")
        calls = pri_dut.droid.telecomCallGetCallIds()
        pri_dut.droid.telecomCallJoinCallsInConf(calls[0], calls[1])
        time.sleep(WAIT_TIME_IN_CALL)
        if num_active_calls(self.log, pri_dut) != 4:
            self.log.error("Total number of call ids in {} is not 4.".format(
                pri_dut.serial))
            return False
        input("Press enter to hangup conf call...")
        if not hangup_call(self.log, pri_dut):
            self.log.error("Failed to hangup call")
            return False
        return True

    def outgoing_multi_call_join_conference_call(self, pri_dut, sec_dut,
                                                 ter_dut):
        self.log.info(
            "Test 2 outgoing calls scenario from phone then join the calls.")
        input("Press enter to execute this testcase...")
        outgoing_num = get_phone_number(self.log, sec_dut)
        if not initiate_call(self.log, pri_dut, outgoing_num):
            self.log.error("Failed to initiate call")
            return False
        if not wait_and_answer_call(self.log, sec_dut):
            self.log.error("Failed to answer call.")
            return False
        time.sleep(self.short_timeout)
        outgoing_num = get_phone_number(self.log, ter_dut)
        if not initiate_call(self.log, pri_dut, outgoing_num):
            self.log.error("Failed to initiate call")
            return False
        if not wait_and_answer_call(self.log, ter_dut):
            self.log.error("Failed to answer call.")
            return False
        input("Press enter to join active calls...")
        calls = pri_dut.droid.telecomCallGetCallIds()
        pri_dut.droid.telecomCallJoinCallsInConf(calls[0], calls[1])
        time.sleep(WAIT_TIME_IN_CALL)
        if num_active_calls(self.log, pri_dut) != 4:
            self.log.error("Total number of call ids in {} is not 4.".format(
                pri_dut.serial))
            return False
        input("Press enter to hangup call 1...")
        if not hangup_call(self.log, sec_dut):
            self.log.error("Failed to hangup call")
            return False
        input("Press enter to hangup call 2...")
        if not hangup_call(self.log, ter_dut):
            self.log.error("Failed to hangup call")
            return False
        return True

    def multi_call_join_conference_call_audio_routes(self, pri_dut, sec_dut,
                                                     ter_dut):
        self.log.info(
            "Test 2 incomming calls scenario to phone then join the calls, then test different audio routes."
        )
        input("Press enter to execute this testcase...")
        if not self.setup_multi_call(sec_dut, ter_dut, pri_dut):
            return False
        input("Press enter to join active calls...")
        calls = pri_dut.droid.telecomCallGetCallIds()
        pri_dut.droid.telecomCallJoinCallsInConf(calls[0], calls[1])
        time.sleep(WAIT_TIME_IN_CALL)
        if num_active_calls(self.log, pri_dut) != 4:
            self.log.error("Total number of call ids in {} is not 4.".format(
                pri_dut.serial))
            return False
        input("Press enter to switch to phone speaker...")
        self.log.info("Switching to earpiece.")
        set_audio_route(self.log, pri_dut, AUDIO_ROUTE_EARPIECE)
        time.sleep(self.short_timeout)
        if get_audio_route(self.log, pri_dut) != AUDIO_ROUTE_EARPIECE:
            self.log.error(
                "Audio Route not set to {}".format(AUDIO_ROUTE_EARPIECE))
            return False
        input("Press enter to switch to Bluetooth...")
        self.log.info("Switching to Bluetooth...")
        set_audio_route(self.log, pri_dut, AUDIO_ROUTE_BLUETOOTH)
        time.sleep(self.short_timeout)
        if get_audio_route(self.log, pri_dut) != AUDIO_ROUTE_BLUETOOTH:
            self.log.error(
                "Audio Route not set to {}".format(AUDIO_ROUTE_BLUETOOTH))
            return False
        input("Press enter to hangup conf call...")
        if not hangup_call(self.log, pri_dut):
            self.log.error("Failed to hangup call")
            return False
        return True

    def avrcp_play_pause(self, pri_dut):
        play_pause_count = 5
        self.log.info(
            "Test AVRCP play/pause {} times.".format(play_pause_count))
        pri_dut.adb.shell(KEYCODE_EVENT_PREVIOUS)
        input(
            "Press enter to execute this testcase when music is in the play state..."
        )
        for i in range(play_pause_count):
            input("Execute iteration {}?".format(i + 1))
            pri_dut.adb.shell(KEYCODE_EVENT_PLAY_PAUSE)
        self.log.info("Test should end in a paused state.")
        return True

    def avrcp_next_previous_song(self, pri_dut):
        self.log.info("Test AVRCP go to the next song then the previous song.")
        pri_dut.adb.shell(KEYCODE_EVENT_PREVIOUS)
        input(
            "Press enter to execute this testcase when music is in the play state..."
        )
        self.log.info("Hitting Next input event...")
        pri_dut.adb.shell(KEYCODE_EVENT_NEXT)
        input("Press enter to go to the previous song")
        pri_dut.adb.shell(KEYCODE_EVENT_PREVIOUS)
        pri_dut.adb.shell(KEYCODE_EVENT_PREVIOUS)
        self.log.info("Test should end on original song.")
        return True

    def avrcp_next_previous(self, pri_dut):
        self.log.info(
            "Test AVRCP go to the next song then the press previous after a few seconds."
        )
        pri_dut.adb.shell(KEYCODE_EVENT_PREVIOUS)
        input(
            "Press enter to execute this testcase when music is in the play state..."
        )
        self.log.info("Hitting Next input event...")
        pri_dut.adb.shell(KEYCODE_EVENT_NEXT)
        time.sleep(5)
        self.log.info("Hitting Previous input event...")
        pri_dut.adb.shell(KEYCODE_EVENT_PREVIOUS)
        self.log.info("Test should end on \"next\" song.")
        return True

    def avrcp_next_repetative(self, pri_dut):
        iterations = 10
        self.log.info("Test AVRCP go to the next {} times".format(iterations))
        pri_dut.adb.shell(KEYCODE_EVENT_PREVIOUS)
        input(
            "Press enter to execute this testcase when music is in the play state..."
        )
        for i in range(iterations):
            self.log.info(
                "Hitting Next input event, iteration {}...".format(i + 1))
            pri_dut.adb.shell(KEYCODE_EVENT_NEXT)
            # Allow time for the carkit to update.
            time.sleep(1)
        return True

    def _cycle_aboslute_volume_control_helper(self, volume_step,
                                              android_volume_steps, pri_dut):
        begin_time = get_current_epoch_time()
        pri_dut.droid.setMediaVolume(volume_step)
        percentage_to_set = int((volume_step / android_volume_steps) * 100)
        self.log.info("Setting phone volume to {}%".format(percentage_to_set))
        volume_info_logcat = pri_dut.search_logcat(
            logcat_strings['media_playback_vol_changed'], begin_time)
        if len(volume_info_logcat) > 1:
            self.log.info("Instant response detected.")
            carkit_response = volume_info_logcat[-1]['log_message'].split(',')
            for item in carkit_response:
                if " volume=" in item:
                    carkit_vol_response = int((
                        int(item.split("=")[-1]) / android_volume_steps) * 100)
                    self.log.info(
                        "Carkit set volume to {}%".format(carkit_vol_response))
        result = input(
            "Did volume change reflect properly on carkit and phone? (Y/n) "
        ).lower()

    def cycle_absolute_volume_control(self, pri_dut):
        result = input(
            "Does carkit support Absolute Volume Control? (Y/n) ").lower()
        if result is "n":
            return True
        android_volume_steps = 25
        for i in range(android_volume_steps):
            self._cycle_aboslute_volume_control_helper(i, android_volume_steps,
                                                       pri_dut)
        for i in reversed(range(android_volume_steps)):
            self._cycle_aboslute_volume_control_helper(i, android_volume_steps,
                                                       pri_dut)
        return True

    def cycle_battery_level(self, pri_dut):
        for i in range(11):
            level = i * 10
            pri_dut.shell.set_battery_level(level)
            question = "Phone battery level {}. Has the carkit indicator " \
                "changed? (Y/n) "
            result = input(question.format(level)).lower()

    def test_voice_recognition_from_phone(self, pri_dut):
        result = input(
            "Does carkit support voice recognition (BVRA)? (Y/n) ").lower()
        if result is "n":
            return True
        input("Press enter to start voice recognition from phone.")
        self.pri_dut.droid.bluetoothHspStartVoiceRecognition(
            self.target_mac_address)
        input("Press enter to stop voice recognition from phone.")
        self.pri_dut.droid.bluetoothHspStopVoiceRecognition(
            self.target_mac_address)

    def test_audio_and_voice_recognition_from_phone(self, pri_dut):
        result = input(
            "Does carkit support voice recognition (BVRA)? (Y/n) ").lower()
        if result is "n":
            return True
        # Start playing music here
        input("Press enter to start voice recognition from phone.")
        self.pri_dut.droid.bluetoothHspStartVoiceRecognition(
            self.target_mac_address)
        input("Press enter to stop voice recognition from phone.")
        self.pri_dut.droid.bluetoothHspStopVoiceRecognition(
            self.target_mac_address)
        time.sleep(2)
        result = input("Did carkit continue music playback after? (Y/n) ")
