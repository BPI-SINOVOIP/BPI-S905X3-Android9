#!/usr/bin/env python3
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

import mock
import unittest
from acts.controllers import android_device
from acts.libs.ota.ota_runners import ota_runner
from acts.libs.ota.ota_tools import ota_tool
from acts.libs.ota.ota_tools import adb_sideload_ota_tool


def get_mock_android_device(serial='', ssh_connection=None):
    """Returns a mocked AndroidDevice with a mocked adb/fastboot."""
    with mock.patch('acts.controllers.adb.AdbProxy') as adb_proxy, (
            mock.patch('acts.controllers.fastboot.FastbootProxy')) as fb_proxy:
        fb_proxy.return_value.devices.return_value = ""
        ret = mock.Mock(
            android_device.AndroidDevice(
                serial=serial, ssh_connection=ssh_connection))
        fb_proxy.reset_mock()
        return ret


class AdbSideloadOtaToolTest(unittest.TestCase):
    """Tests the OtaTool class."""

    def test_init(self):
        expected_value = 'commmand string'
        self.assertEqual(
            ota_tool.OtaTool(expected_value).command, expected_value)

    def setUp(self):
        self.sl4a_service_setup_time = ota_runner.SL4A_SERVICE_SETUP_TIME
        ota_runner.SL4A_SERVICE_SETUP_TIME = 0

    def tearDown(self):
        ota_runner.SL4A_SERVICE_SETUP_TIME = self.sl4a_service_setup_time

    @staticmethod
    def test_start():
        # This test could have a bunch of verify statements,
        # but its probably not worth it.
        device = get_mock_android_device()
        tool = adb_sideload_ota_tool.AdbSideloadOtaTool('')
        runner = ota_runner.SingleUseOtaRunner(tool, device, '', '')
        runner.android_device.adb.getprop = mock.Mock(side_effect=['a', 'b'])
        runner.update()
