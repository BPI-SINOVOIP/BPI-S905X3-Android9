#!/usr/bin/env python
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

import os
import sys
import unittest

try:
    from unittest import mock
except ImportError:
    import mock

from host_controller.build import build_flasher


class BuildFlasherTest(unittest.TestCase):
    """Tests for Build Flasher"""

    @mock.patch(
        "host_controller.build.build_flasher.android_device")
    @mock.patch("host_controller.build.build_flasher.os")
    def testFlashGSIBadPath(self, mock_os, mock_class):
        flasher = build_flasher.BuildFlasher("thisismyserial")
        mock_os.path.exists.return_value = False
        with self.assertRaises(ValueError) as cm:
            flasher.FlashGSI("notexists.img")
        self.assertEqual("Couldn't find system image at notexists.img",
                         str(cm.exception))

    @mock.patch(
        "host_controller.build.build_flasher.android_device")
    @mock.patch("host_controller.build.build_flasher.os")
    def testFlashGSISystemOnly(self, mock_os, mock_class):
        mock_device = mock.Mock()
        mock_class.AndroidDevice.return_value = mock_device
        flasher = build_flasher.BuildFlasher("thisismyserial")
        mock_os.path.exists.return_value = True
        flasher.FlashGSI("exists.img")
        mock_device.fastboot.erase.assert_any_call('system')
        mock_device.fastboot.flash.assert_any_call('system', 'exists.img')
        mock_device.fastboot.erase.assert_any_call('metadata')

    @mock.patch(
        "host_controller.build.build_flasher.android_device")
    def testFlashall(self, mock_class):
        mock_device = mock.Mock()
        mock_class.AndroidDevice.return_value = mock_device
        flasher = build_flasher.BuildFlasher("thisismyserial")
        flasher.Flashall("path/to/dir")
        mock_device.fastboot.flashall.assert_called_with()

    @mock.patch(
        "host_controller.build.build_flasher.android_device")
    def testEmptySerial(self, mock_class):
        mock_class.list_adb_devices.return_value = ['oneserial']
        flasher = build_flasher.BuildFlasher(serial="")
        mock_class.AndroidDevice.assert_called_with(
            "oneserial", device_callback_port=-1)

    @mock.patch(
        "host_controller.build.build_flasher.android_device")
    def testFlashUsingCustomBinary(self, mock_class):
        """Test for FlashUsingCustomBinary().

            Tests if the method passes right args to customflasher
            and the execution path of the method.
        """
        mock_device = mock.Mock()
        mock_class.AndroidDevice.return_value = mock_device
        flasher = build_flasher.BuildFlasher("mySerial", "myCustomBinary")

        mock_device.isBootloaderMode = False
        device_images = {"img": "my/image/path"}
        flasher.FlashUsingCustomBinary(device_images, "reboottowhatever",
                                       ["myarg"])
        mock_device.adb.reboot.assert_called_with("reboottowhatever")
        mock_device.customflasher.ExecCustomFlasherCmd.assert_called_with(
            "myarg", "my/image/path")

        mock_device.reset_mock()
        mock_device.isBootloaderMode = True
        device_images["img"] = "my/other/image/path"
        flasher.FlashUsingCustomBinary(device_images, "reboottowhatever",
                                       ["myarg"])
        mock_device.adb.reboot.assert_not_called()
        mock_device.customflasher.ExecCustomFlasherCmd.assert_called_with(
            "myarg", "my/other/image/path")

    @mock.patch("host_controller.build.build_flasher.android_device")
    @mock.patch("host_controller.build.build_flasher.logging")
    @mock.patch("host_controller.build.build_flasher.cmd_utils")
    @mock.patch("host_controller.build.build_flasher.os")
    @mock.patch("host_controller.build.build_flasher.open",
                new_callable=mock.mock_open)
    def testRepackageArtifacts(self, mock_open, mock_os, mock_cmd_utils,
                               mock_logger, mock_class):
        """Test for RepackageArtifacts().

            Tests if the method executes in correct path regarding
            given arguments.
        """
        mock_device = mock.Mock()
        mock_class.AndroidDevice.return_value = mock_device
        flasher = build_flasher.BuildFlasher("serial")
        device_images = {
            "system.img": "/my/tmp/path/system.img",
            "vendor.img": "/my/tmp/path/vendor.img"
        }
        mock_cmd_utils.ExecuteOneShellCommand.return_value = "", "", 0
        ret = flasher.RepackageArtifacts(device_images, "tar.md5")
        self.assertEqual(ret, True)
        self.assertIsNotNone(device_images["img"])

        ret = flasher.RepackageArtifacts(device_images, "incorrect")
        self.assertFalse(ret)
        mock_logger.error.assert_called_with(
            "Please specify correct repackage form: --repackage=incorrect")


if __name__ == "__main__":
    unittest.main()
