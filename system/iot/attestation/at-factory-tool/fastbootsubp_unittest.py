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

"""Unit test for fastboot interface using subprocess library."""
import subprocess
import unittest

import fastboot_exceptions
import fastbootsubp
from mock import patch

CREATE_NO_WINDOW = 0x08000000


class TestError(subprocess.CalledProcessError):

  def __init__(self):
    pass


class FastbootSubpTest(unittest.TestCase):
  ATFA_TEST_SERIAL = 'ATFA_TEST_SERIAL'
  TEST_MESSAGE_FAILURE = 'FAIL: TEST MESSAGE'
  TEST_MESSAGE_SUCCESS = 'OKAY: TEST MESSAGE'
  TEST_SERIAL = 'TEST_SERIAL'

  TEST_VAR = 'VAR1'
  TEST_MESSAGE = 'TEST MESSAGE'

  def setUp(self):
    fastbootsubp.FastbootDevice.fastboot_command = 'fastboot'

  # Test FastbootDevice.ListDevices
  @patch('subprocess.check_output', create=True)
  def testListDevicesOneDevice(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = self.TEST_SERIAL + '\tfastboot'
    device_serial_numbers = fastbootsubp.FastbootDevice.ListDevices()
    mock_fastboot_commands.assert_called_once_with(
        ['fastboot', 'devices'], creationflags=CREATE_NO_WINDOW)
    self.assertEqual(1, len(device_serial_numbers))
    self.assertEqual(self.TEST_SERIAL, device_serial_numbers[0])

  @patch('subprocess.check_output', create=True)
  def testListDevicesTwoDevices(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = (self.TEST_SERIAL + '\tfastboot\n' +
                                           self.ATFA_TEST_SERIAL + '\tfastboot')
    device_serial_numbers = fastbootsubp.FastbootDevice.ListDevices()
    mock_fastboot_commands.assert_called_once_with(
        ['fastboot', 'devices'], creationflags=CREATE_NO_WINDOW)
    self.assertEqual(2, len(device_serial_numbers))
    self.assertEqual(self.TEST_SERIAL, device_serial_numbers[0])
    self.assertEqual(self.ATFA_TEST_SERIAL, device_serial_numbers[1])

  @patch('subprocess.check_output', create=True)
  def testListDevicesTwoDevicesCRLF(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = (self.TEST_SERIAL + '\tfastboot\r\n' +
                                           self.ATFA_TEST_SERIAL + '\tfastboot')
    device_serial_numbers = fastbootsubp.FastbootDevice.ListDevices()
    mock_fastboot_commands.assert_called_once_with(
        ['fastboot', 'devices'], creationflags=CREATE_NO_WINDOW)
    self.assertEqual(2, len(device_serial_numbers))
    self.assertEqual(self.TEST_SERIAL, device_serial_numbers[0])
    self.assertEqual(self.ATFA_TEST_SERIAL, device_serial_numbers[1])

  @patch('subprocess.check_output', create=True)
  def testListDevicesMultiDevices(self, mock_fastboot_commands):
    one_device = self.TEST_SERIAL + '\tfastboot'
    result = one_device
    for _ in range(0, 9):
      result += '\n' + one_device
    mock_fastboot_commands.return_value = result
    device_serial_numbers = fastbootsubp.FastbootDevice.ListDevices()
    mock_fastboot_commands.assert_called_once_with(
        ['fastboot', 'devices'], creationflags=CREATE_NO_WINDOW)
    self.assertEqual(10, len(device_serial_numbers))

  @patch('subprocess.check_output', create=True)
  def testListDevicesNone(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = ''
    device_serial_numbers = fastbootsubp.FastbootDevice.ListDevices()
    mock_fastboot_commands.assert_called_once_with(
        ['fastboot', 'devices'], creationflags=CREATE_NO_WINDOW)
    self.assertEqual(0, len(device_serial_numbers))

  @patch('subprocess.check_output', create=True)
  def testListDevicesFailure(self, mock_fastboot_commands):
    mock_error = TestError()
    mock_error.output = self.TEST_MESSAGE_FAILURE
    mock_fastboot_commands.side_effect = mock_error
    with self.assertRaises(fastboot_exceptions.FastbootFailure) as e:
      fastbootsubp.FastbootDevice.ListDevices()
    self.assertEqual(self.TEST_MESSAGE_FAILURE, str(e.exception))

  # Test FastbootDevice.Oem
  @patch('subprocess.check_output', create=True)
  def testOem(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = self.TEST_MESSAGE_SUCCESS
    command = 'TEST COMMAND'
    device = fastbootsubp.FastbootDevice(self.TEST_SERIAL)
    message = device.Oem(command, False)
    mock_fastboot_commands.assert_called_once_with(
        ['fastboot', '-s', self.TEST_SERIAL, 'oem', command],
        stderr=subprocess.STDOUT,
        creationflags=CREATE_NO_WINDOW)
    self.assertEqual(self.TEST_MESSAGE_SUCCESS, message)

  @patch('subprocess.check_output', create=True)
  def testOemErrToOut(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = self.TEST_MESSAGE_SUCCESS
    command = 'TEST COMMAND'
    device = fastbootsubp.FastbootDevice(self.TEST_SERIAL)
    message = device.Oem(command, True)
    mock_fastboot_commands.assert_called_once_with(
        ['fastboot', '-s', self.TEST_SERIAL, 'oem', command],
        stderr=subprocess.STDOUT,
        creationflags=CREATE_NO_WINDOW)
    self.assertEqual(self.TEST_MESSAGE_SUCCESS, message)

  @patch('subprocess.check_output', create=True)
  def testOemFailure(self, mock_fastboot_commands):
    mock_error = TestError()
    mock_error.output = self.TEST_MESSAGE_FAILURE
    mock_fastboot_commands.side_effect = mock_error
    command = 'TEST COMMAND'
    device = fastbootsubp.FastbootDevice(self.TEST_SERIAL)
    with self.assertRaises(fastboot_exceptions.FastbootFailure) as e:
      device.Oem(command, False)
    self.assertEqual(self.TEST_MESSAGE_FAILURE, str(e.exception))

  # Test FastbootDevice.Upload
  @patch('subprocess.check_output', create=True)
  def testUpload(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = self.TEST_MESSAGE_SUCCESS
    command = 'TEST COMMAND'
    device = fastbootsubp.FastbootDevice(self.TEST_SERIAL)
    message = device.Upload(command)
    mock_fastboot_commands.assert_called_once_with(
        ['fastboot', '-s', self.TEST_SERIAL, 'get_staged', command],
        creationflags=CREATE_NO_WINDOW)
    self.assertEqual(self.TEST_MESSAGE_SUCCESS, message)

  @patch('subprocess.check_output', create=True)
  def testUploadFailure(self, mock_fastboot_commands):
    mock_error = TestError()
    mock_error.output = self.TEST_MESSAGE_FAILURE
    mock_fastboot_commands.side_effect = mock_error
    command = 'TEST COMMAND'
    device = fastbootsubp.FastbootDevice(self.TEST_SERIAL)
    with self.assertRaises(fastboot_exceptions.FastbootFailure) as e:
      device.Upload(command)
    self.assertEqual(self.TEST_MESSAGE_FAILURE, str(e.exception))

  # Test FastbootDevice.Download
  @patch('subprocess.check_output', create=True)
  def testDownload(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = self.TEST_MESSAGE_SUCCESS
    command = 'TEST COMMAND'
    device = fastbootsubp.FastbootDevice(self.TEST_SERIAL)
    message = device.Download(command)
    mock_fastboot_commands.assert_called_once_with(
        ['fastboot', '-s', self.TEST_SERIAL, 'stage', command],
        creationflags=CREATE_NO_WINDOW)
    self.assertEqual(self.TEST_MESSAGE_SUCCESS, message)

  @patch('subprocess.check_output', create=True)
  def testDownloadFailure(self, mock_fastboot_commands):
    mock_error = TestError()
    mock_error.output = self.TEST_MESSAGE_FAILURE
    mock_fastboot_commands.side_effect = mock_error
    command = 'TEST COMMAND'
    device = fastbootsubp.FastbootDevice(self.TEST_SERIAL)
    with self.assertRaises(fastboot_exceptions.FastbootFailure) as e:
      device.Download(command)
    self.assertEqual(self.TEST_MESSAGE_FAILURE, str(e.exception))

  # Test FastbootDevice.GetVar
  @patch('subprocess.check_output', create=True)
  def testGetVar(self, mock_fastboot_commands):
    mock_fastboot_commands.return_value = (
        self.TEST_VAR + ': ' + self.TEST_MESSAGE)
    device = fastbootsubp.FastbootDevice(self.TEST_SERIAL)
    message = device.GetVar(self.TEST_VAR)
    mock_fastboot_commands.assert_called_once_with(
        ['fastboot', '-s', self.TEST_SERIAL, 'getvar', self.TEST_VAR],
        stderr=subprocess.STDOUT,
        shell=True,
        creationflags=CREATE_NO_WINDOW)
    self.assertEqual(self.TEST_MESSAGE, message)

  @patch('subprocess.check_output', create=True)
  def testGetVarFailure(self, mock_fastboot_commands):
    mock_error = TestError()
    mock_error.output = self.TEST_MESSAGE_FAILURE
    mock_fastboot_commands.side_effect = mock_error
    device = fastbootsubp.FastbootDevice(self.TEST_SERIAL)
    with self.assertRaises(fastboot_exceptions.FastbootFailure) as e:
      device.GetVar(self.TEST_VAR)
    self.assertEqual(self.TEST_MESSAGE_FAILURE, str(e.exception))

  # Test FastbootDevice.Reboot
  @patch('subprocess.check_output', create=True)
  def testGetVar(self, mock_fastboot_commands):
    device = fastbootsubp.FastbootDevice(self.TEST_SERIAL)
    message = device.Reboot()
    mock_fastboot_commands.assert_called_once_with(
        ['fastboot', '-s', self.TEST_SERIAL, 'reboot-bootloader'],
        creationflags=CREATE_NO_WINDOW)

  @patch('subprocess.check_output', create=True)
  def testGetVarFailure(self, mock_fastboot_commands):
    mock_error = TestError()
    mock_error.output = self.TEST_MESSAGE_FAILURE
    mock_fastboot_commands.side_effect = mock_error
    device = fastbootsubp.FastbootDevice(self.TEST_SERIAL)
    with self.assertRaises(fastboot_exceptions.FastbootFailure) as e:
      device.Reboot()
    self.assertEqual(self.TEST_MESSAGE_FAILURE, str(e.exception))

if __name__ == '__main__':
  unittest.main()
