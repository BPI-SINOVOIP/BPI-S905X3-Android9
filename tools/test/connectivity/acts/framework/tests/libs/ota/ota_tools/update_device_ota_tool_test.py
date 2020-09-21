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
from acts.libs.ota.ota_tools import update_device_ota_tool


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


class UpdateDeviceOtaToolTest(unittest.TestCase):
    """Tests for UpdateDeviceOtaTool."""

    def setUp(self):
        self.sl4a_service_setup_time = ota_runner.SL4A_SERVICE_SETUP_TIME
        ota_runner.SL4A_SERVICE_SETUP_TIME = 0

    def tearDown(self):
        ota_runner.SL4A_SERVICE_SETUP_TIME = self.sl4a_service_setup_time

    def test_update(self):
        with mock.patch('tempfile.mkdtemp') as mkdtemp, (
                mock.patch('shutil.rmtree')) as rmtree, (
                    mock.patch('acts.utils.unzip_maintain_permissions')):
            mkdtemp.return_value = ''
            rmtree.return_value = ''
            device = get_mock_android_device()
            tool = update_device_ota_tool.UpdateDeviceOtaTool('')
            runner = mock.Mock(
                ota_runner.SingleUseOtaRunner(tool, device, '', ''))
            runner.return_value.android_device = device
            with mock.patch('acts.libs.proc.job.run'):
                tool.update(runner)
            del tool

    def test_del(self):
        with mock.patch('tempfile.mkdtemp') as mkdtemp, (
                mock.patch('shutil.rmtree')) as rmtree, (
                    mock.patch('acts.utils.unzip_maintain_permissions')):
            mkdtemp.return_value = ''
            rmtree.return_value = ''
            tool = update_device_ota_tool.UpdateDeviceOtaTool('')
            del tool
            self.assertTrue(mkdtemp.called)
            self.assertTrue(rmtree.called)
