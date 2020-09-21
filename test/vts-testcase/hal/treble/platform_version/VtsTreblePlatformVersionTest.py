#!/usr/bin/env python3.4
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.android import api


class VtsTreblePlatformVersionTest(base_test.BaseTestClass):
    """VTS should run on devices launched with O or later."""

    def setUpClass(self):
        self.dut = self.android_devices[0]
        self.dut.shell.InvokeTerminal("VtsTreblePlatformVersionTest")

    def getProp(self, prop, required=True):
        """Helper to retrieve a property from device."""

        results = self.dut.shell.Execute("getprop " + prop)
        if required:
            asserts.assertEqual(results[const.EXIT_CODE][0], 0,
                "getprop must succeed")
            asserts.assertTrue(len(results[const.STDOUT][0].strip()) > 0,
                "getprop must return a value")
        else:
            if (results[const.EXIT_CODE][0] != 0 or
                len(results[const.STDOUT][0].strip()) == 0):
                logging.info("sysprop %s undefined", prop)
                return None

        result = results[const.STDOUT][0].strip()

        logging.info("getprop {}={}".format(prop, result))

        return result

    def getEnv(self, env):
        """Helper to retrieve an environment varable from device."""

        results = self.dut.shell.Execute("printenv " + env)
        if (results[const.EXIT_CODE][0] != 0 or
            len(results[const.STDOUT][0].strip()) == 0):
            logging.info("environment variable %s undefined", env)
            return None

        result = results[const.STDOUT][0].strip()

        logging.info("printenv {}:{}".format(env, result))

        return result

    def testFirstApiLevel(self):
        """Test that device launched with O or later."""
        firstApiLevel = self.dut.getLaunchApiLevel()
        asserts.assertTrue(firstApiLevel >= api.PLATFORM_API_LEVEL_O,
            "VTS can only be run for new launches in O or above")

    def testTrebleEnabled(self):
        """Test that device has Treble enabled."""
        trebleIsEnabledStr = self.getProp("ro.treble.enabled")
        asserts.assertEqual(trebleIsEnabledStr, "true",
            "VTS can only be run for Treble enabled devices")

    def testSdkVersion(self):
        """Test that SDK version >= O (26)."""
        try:
            sdkVersion = int(self.getProp("ro.build.version.sdk"))
            asserts.assertTrue(sdkVersion >= api.PLATFORM_API_LEVEL_O,
                "VTS is for devices launching in O or above")
        except ValueError as e:
            asserts.fail("Unexpected value returned from getprop: %s" % e)

    def testVndkVersion(self):
        """Test that VNDK version is specified.

        If ro.vndk.version is not defined on boot, GSI sets LD_CONFIG_FILE to
        temporary configuration file and ro.vndk.version to default value.
        """

        vndkVersion = self.getProp("ro.vndk.version")
        if vndkVersion is None:
            asserts.fail("VNDK version is not defined")

        firstApiLevel = self.dut.getLaunchApiLevel()
        if firstApiLevel > api.PLATFORM_API_LEVEL_O_MR1:
            vndkLite = self.getProp("ro.vndk.lite", False)
            if vndkLite is not None:
                asserts.fail("ro.vndk.lite is defined as %s" % vndkLite)
            envLdConfigFile = self.getEnv("LD_CONFIG_FILE")
            if envLdConfigFile is not None:
                asserts.fail("LD_CONFIG_FILE is defined as %s" % envLdConfigFile)

if __name__ == "__main__":
    test_runner.main()
