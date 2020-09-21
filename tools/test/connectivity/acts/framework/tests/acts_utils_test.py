#!/usr/bin/env python3
#
#   Copyright 2016 - The Android Open Source Project
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
import unittest
import enum

from acts import utils
from acts.controllers.adb import AdbError


class ActsUtilsTest(unittest.TestCase):
    """This test class has unit tests for the implementation of everything
    under acts.utils.
    """

    def test_start_standing_subproc(self):
        with self.assertRaisesRegexp(utils.ActsUtilsError,
                                     "Process .* has terminated"):
            utils.start_standing_subprocess("sleep 0", check_health_delay=0.1)

    def test_stop_standing_subproc(self):
        p = utils.start_standing_subprocess("sleep 0")
        time.sleep(0.1)
        with self.assertRaisesRegexp(utils.ActsUtilsError,
                                     "Process .* has terminated"):
            utils.stop_standing_subprocess(p)

    def test_bypass_setup_wizard_no_complications(self):
        ad = MockAd()
        ad.adb.return_state = BypassSetupWizardReturn.NO_COMPLICATIONS
        self.assertTrue(utils.bypass_setup_wizard(ad, 0))
        self.assertFalse(ad.adb.root_adb_called)

    def test_bypass_setup_wizard_unrecognized_error(self):
        ad = MockAd()
        ad.adb.return_state = BypassSetupWizardReturn.UNRECOGNIZED_ERR
        with self.assertRaises(AdbError):
            utils.bypass_setup_wizard(ad, 0)
        self.assertFalse(ad.adb.root_adb_called)

    def test_bypass_setup_wizard_need_root_access(self):
        ad = MockAd()
        ad.adb.return_state = BypassSetupWizardReturn.ROOT_ADB_NO_COMP
        self.assertTrue(utils.bypass_setup_wizard(ad, 0))
        self.assertTrue(ad.adb.root_adb_called)

    def test_bypass_setup_wizard_need_root_already_skipped(self):
        ad = MockAd()
        ad.adb.return_state = BypassSetupWizardReturn.ROOT_ADB_SKIPPED
        self.assertTrue(utils.bypass_setup_wizard(ad, 0))
        self.assertTrue(ad.adb.root_adb_called)

    def test_bypass_setup_wizard_root_access_still_fails(self):
        ad = MockAd()
        ad.adb.return_state = BypassSetupWizardReturn.ROOT_ADB_FAILS
        with self.assertRaises(AdbError):
            utils.bypass_setup_wizard(ad, 0)
        self.assertTrue(ad.adb.root_adb_called)


class BypassSetupWizardReturn:
    # No complications. Bypass works the first time without issues.
    NO_COMPLICATIONS = AdbError("", "", "", 1)
    # Fail with doesn't need to be skipped/was skipped already.
    ALREADY_BYPASSED = AdbError("", "ADB_CMD_OUTPUT:0", "Error type 3\n"
                                "Error: Activity class", 1)
    # Fail with different error.
    UNRECOGNIZED_ERR = AdbError("", "ADB_CMD_OUTPUT:0", "Error type 4\n"
                                "Error: Activity class", 0)
    # Fail, get root access, then no complications arise.
    ROOT_ADB_NO_COMP = AdbError("", "ADB_CMD_OUTPUT:255",
                                "Security exception: Permission Denial: "
                                "starting Intent { flg=0x10000000 "
                                "cmp=com.google.android.setupwizard/"
                                ".SetupWizardExitActivity } from null "
                                "(pid=5045, uid=2000) not exported from uid "
                                "10000", 0)
    # Even with root access, the bypass setup wizard doesn't need to be skipped.
    ROOT_ADB_SKIPPED = AdbError("", "ADB_CMD_OUTPUT:255",
                                "Security exception: Permission Denial: "
                                "starting Intent { flg=0x10000000 "
                                "cmp=com.google.android.setupwizard/"
                                ".SetupWizardExitActivity } from null "
                                "(pid=5045, uid=2000) not exported from "
                                "uid 10000", 0)
    # Even with root access, the bypass setup wizard fails
    ROOT_ADB_FAILS = AdbError(
        "", "ADB_CMD_OUTPUT:255", "Security exception: Permission Denial: "
        "starting Intent { flg=0x10000000 "
        "cmp=com.google.android.setupwizard/"
        ".SetupWizardExitActivity } from null (pid=5045, "
        "uid=2000) not exported from uid 10000", 0)


class MockAd:
    def __init__(self):
        self.adb = MockAdb()


class MockAdb:
    def __init__(self):
        self.return_state = BypassSetupWizardReturn.NO_COMPLICATIONS
        self.root_adb_called = False

    def shell(self, string):
        if string == "settings get global device_provisioned":
            return self.return_state.ret_code
        raise self.return_state

    def root_adb(self):
        self.root_adb_called = True
        if self.return_state is BypassSetupWizardReturn.ROOT_ADB_FAILS:
            self.return_state = BypassSetupWizardReturn.UNRECOGNIZED_ERR
        elif self.return_state is not BypassSetupWizardReturn.ROOT_ADB_SKIPPED:
            self.return_state = BypassSetupWizardReturn.ALREADY_BYPASSED
        else:
            self.return_state = BypassSetupWizardReturn.NO_COMPLICATIONS


if __name__ == "__main__":
    unittest.main()
