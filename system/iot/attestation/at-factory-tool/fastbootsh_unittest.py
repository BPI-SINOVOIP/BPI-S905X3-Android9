# Copyright 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Unit test for fastboot interface using sh library."""
import unittest

import fastboot_exceptions
import fastbootsh
from mock import patch
import sh


class FastbootShTest(unittest.TestCase):

  ATFA_TEST_SERIAL = 'ATFA_TEST_SERIAL'
  TEST_MESSAGE_FAILURE = 'FAIL: TEST MESSAGE'
  TEST_MESSAGE_SUCCESS = 'OKAY: TEST MESSAGE'
  TEST_SERIAL = 'TEST_SERIAL'

  TEST_VAR = 'VAR1'

  class TestError(sh.ErrorReturnCode):

    def __init__(self):
      pass

  def setUp(self):
    pass

  # Test FastbootDevice.ListDevices
  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testListDevicesOneDevice(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = self.TEST_SERIAL + '\tfastboot'
    device_serial_numbers = fastbootsh.FastbootDevice.ListDevices()
    mock_fastboot_commands.assert_called_once_with('devices')
    self.assertEqual(1, len(device_serial_numbers))
    self.assertEqual(self.TEST_SERIAL, device_serial_numbers[0])

  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testListDevicesTwoDevices(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = (self.TEST_SERIAL + '\tfastboot\n' +
                                           self.ATFA_TEST_SERIAL + '\tfastboot')
    device_serial_numbers = fastbootsh.FastbootDevice.ListDevices()
    mock_fastboot_commands.assert_called_once_with('devices')
    self.assertEqual(2, len(device_serial_numbers))
    self.assertEqual(self.TEST_SERIAL, device_serial_numbers[0])
    self.assertEqual(self.ATFA_TEST_SERIAL, device_serial_numbers[1])

  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testListDevicesMultiDevices(self, mock_fastboot_commands):
    one_device = self.TEST_SERIAL + '\tfastboot'
    result = one_device
    for _ in range(0, 9):
      result += '\n' + one_device
    mock_fastboot_commands.return_value = result
    device_serial_numbers = fastbootsh.FastbootDevice.ListDevices()
    mock_fastboot_commands.assert_called_once_with('devices')
    self.assertEqual(10, len(device_serial_numbers))

  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testListDevicesNone(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = ''
    device_serial_numbers = fastbootsh.FastbootDevice.ListDevices()
    mock_fastboot_commands.assert_called_once_with('devices')
    self.assertEqual(0, len(device_serial_numbers))

  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testListDevicesFailure(self, mock_fastboot_commands):
    mock_error = self.TestError()
    mock_error.stderr = self.TEST_MESSAGE_FAILURE
    mock_fastboot_commands.side_effect = mock_error
    with self.assertRaises(fastboot_exceptions.FastbootFailure) as e:
      fastbootsh.FastbootDevice.ListDevices()
    self.assertEqual(self.TEST_MESSAGE_FAILURE, str(e.exception))

  # Test FastbootDevice.Oem
  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testOem(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = self.TEST_MESSAGE_SUCCESS
    command = 'TEST COMMAND'
    device = fastbootsh.FastbootDevice(self.TEST_SERIAL)
    message = device.Oem(command, False)
    mock_fastboot_commands.assert_called_once_with('-s',
                                                   self.TEST_SERIAL,
                                                   'oem',
                                                   command,
                                                   _err_to_out=False)
    self.assertEqual(self.TEST_MESSAGE_SUCCESS, message)

  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testOemErrToOut(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = self.TEST_MESSAGE_SUCCESS
    command = 'TEST COMMAND'
    device = fastbootsh.FastbootDevice(self.TEST_SERIAL)
    message = device.Oem(command, True)
    mock_fastboot_commands.assert_called_once_with('-s',
                                                   self.TEST_SERIAL,
                                                   'oem',
                                                   command,
                                                   _err_to_out=True)
    self.assertEqual(self.TEST_MESSAGE_SUCCESS, message)

  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testOemFailure(self, mock_fastboot_commands):
    mock_error = self.TestError()
    mock_error.stderr = self.TEST_MESSAGE_FAILURE
    mock_fastboot_commands.side_effect = mock_error
    command = 'TEST COMMAND'
    device = fastbootsh.FastbootDevice(self.TEST_SERIAL)
    with self.assertRaises(fastboot_exceptions.FastbootFailure) as e:
      device.Oem(command, False)
    self.assertEqual(self.TEST_MESSAGE_FAILURE, str(e.exception))

  # Test FastbootDevice.Upload
  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testUpload(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = self.TEST_MESSAGE_SUCCESS
    command = 'TEST COMMAND'
    device = fastbootsh.FastbootDevice(self.TEST_SERIAL)
    message = device.Upload(command)
    mock_fastboot_commands.assert_called_once_with('-s',
                                                   self.TEST_SERIAL,
                                                   'get_staged',
                                                   command)
    self.assertEqual(self.TEST_MESSAGE_SUCCESS, message)

  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testUploadFailure(self, mock_fastboot_commands):
    mock_error = self.TestError()
    mock_error.stderr = self.TEST_MESSAGE_FAILURE
    mock_fastboot_commands.side_effect = mock_error
    command = 'TEST COMMAND'
    device = fastbootsh.FastbootDevice(self.TEST_SERIAL)
    with self.assertRaises(fastboot_exceptions.FastbootFailure) as e:
      device.Upload(command)
    self.assertEqual(self.TEST_MESSAGE_FAILURE, str(e.exception))

  # Test FastbootDevice.Download
  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testDownload(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = self.TEST_MESSAGE_SUCCESS
    command = 'TEST COMMAND'
    device = fastbootsh.FastbootDevice(self.TEST_SERIAL)
    message = device.Download(command)
    mock_fastboot_commands.assert_called_once_with('-s',
                                                   self.TEST_SERIAL,
                                                   'stage',
                                                   command)
    self.assertEqual(self.TEST_MESSAGE_SUCCESS, message)

  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testDownloadFailure(self, mock_fastboot_commands):
    mock_error = self.TestError()
    mock_error.stderr = self.TEST_MESSAGE_FAILURE
    mock_fastboot_commands.side_effect = mock_error
    command = 'TEST COMMAND'
    device = fastbootsh.FastbootDevice(self.TEST_SERIAL)
    with self.assertRaises(fastboot_exceptions.FastbootFailure) as e:
      device.Download(command)
    self.assertEqual(self.TEST_MESSAGE_FAILURE, str(e.exception))

  # Test FastbootDevice.GetVar
  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testGetVar(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = self.TEST_VAR + ': ' + 'abcd'
    device = fastbootsh.FastbootDevice(self.TEST_SERIAL)
    message = device.GetVar(self.TEST_VAR)
    mock_fastboot_commands.assert_called_once_with('-s',
                                                   self.TEST_SERIAL,
                                                   'getvar',
                                                   self.TEST_VAR,
                                                   _err_to_out=True)
    self.assertEqual('abcd', message)

  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testGetVarFailure(self, mock_fastboot_commands):
    mock_error = self.TestError()
    mock_error.stdout = self.TEST_MESSAGE_FAILURE
    mock_fastboot_commands.side_effect = mock_error
    device = fastbootsh.FastbootDevice(self.TEST_SERIAL)
    with self.assertRaises(fastboot_exceptions.FastbootFailure) as e:
      device.GetVar(self.TEST_VAR)
    self.assertEqual(self.TEST_MESSAGE_FAILURE, str(e.exception))

  # Test FastbootDevice.Reboot
  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testReboot(self, mock_fastboot_commands):
    device = fastbootsh.FastbootDevice(self.TEST_SERIAL)
    device.Reboot()
    mock_fastboot_commands.assert_called_once_with('-s',
                                                   self.TEST_SERIAL,
                                                   'reboot-bootloader')

  @patch('fastbootsh.FastbootDevice.fastboot_command', create=True)
  def testRebootFailure(self, mock_fastboot_commands):
    mock_error = self.TestError()
    mock_error.stderr = self.TEST_MESSAGE_FAILURE
    mock_fastboot_commands.side_effect = mock_error
    device = fastbootsh.FastbootDevice(self.TEST_SERIAL)
    with self.assertRaises(fastboot_exceptions.FastbootFailure) as e:
      device.Reboot()
    self.assertEqual(self.TEST_MESSAGE_FAILURE, str(e.exception))

if __name__ == '__main__':
  unittest.main()
