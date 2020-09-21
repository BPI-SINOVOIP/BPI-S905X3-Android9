#!/usr/bin/env python3.4
#
#   Copyright 2017 - The Android Open Source Project
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

import time

from acts.base_test import BaseTestClass
from acts.test_decorators import test_tracker_info


class NfcBasicFunctionalityTest(BaseTestClass):
    nfc_on_event = "NfcStateOn"
    nfc_off_event = "NfcStateOff"
    timeout = 5

    def setup_class(self):
        self.dut = self.android_devices[0]
        self._ensure_nfc_enabled(self.dut)
        self.dut.droid.nfcStartTrackingStateChange()
        self.dut.adb.shell("setprop nfc.app_log_level 255")
        self.dut.adb.shell("setprop nfc.enable_protocol_log 255")
        self.dut.adb.shell("setprop nfc.nxp_log_level_global 5")
        self.dut.adb.shell("setprop nfc.nxp_log_level_extns 5")
        self.dut.adb.shell("setprop nfc.nxp_log_level_hal 5")
        self.dut.adb.shell("setprop nfc.nxp_log_level_nci 5")
        self.dut.adb.shell("setprop nfc.nxp_log_level_tml 5")
        self.dut.adb.shell("setprop nfc.nxp_log_level_dnld 5")
        self._ensure_nfc_disabled(self.dut)
        return True

    def _ensure_nfc_enabled(self, dut):
        end_time = time.time() + 10
        while end_time > time.time():
            try:
                dut.ed.pop_event(self.nfc_on_event, self.timeout)
                self.log.info("Event {} found".format(self.nfc_on_event))
                return True
            except Exception as err:
                self.log.debug(
                    "Event {} not yet found".format(self.nfc_on_event))
        return False

    def _ensure_nfc_disabled(self, dut):
        end_time = time.time() + 10
        while end_time > time.time():
            try:
                dut.ed.pop_event(self.nfc_off_event, self.timeout)
                self.log.info("Event {} found".format(self.nfc_off_event))
                return True
            except Exception as err:
                self.log.debug(
                    "Event {} not yet found".format(self.nfc_off_event))
        return False

    def setup_test(self):
        # Every test starts with the assumption that NFC is enabled
        if not self.dut.droid.nfcIsEnabled():
            self.dut.droid.nfcEnable()
        else:
            return True
        if not self._ensure_nfc_enabled(self.dut):
            self.log.error("Failed to toggle NFC on")
            return False
        return True

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)

    @test_tracker_info(uuid='d57fcdd8-c56c-4ab0-81fb-e2218b100de9')
    def test_nfc_toggle_state_100_iterations(self):
        """Test toggling NFC state 100 times.

        Verify that NFC toggling works. Test assums NFC is on.

        Steps:
        1. Toggle NFC off
        2. Toggle NFC on
        3. Repeat steps 1-2 100 times.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: NFC
        Priority: 1
        """
        iterations = 100
        for i in range(iterations):
            self.log.info("Starting iteration {}".format(i + 1))
            self.dut.droid.nfcDisable()
            if not self._ensure_nfc_disabled(self.dut):
                return False
            self.dut.droid.nfcEnable()
            if not self._ensure_nfc_enabled(self.dut):
                return False
        return True
