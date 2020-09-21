# !/usr/bin/python
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

"""Fastboot Interface Implementation using subprocess library."""
import os
import subprocess
import sys
import threading

import fastboot_exceptions

CREATE_NO_WINDOW = 0x08000000


def _GetCurrentPath():
  if getattr(sys, 'frozen', False):
    # we are running in a bundle
    path = sys._MEIPASS  # pylint: disable=protected-access
  else:
    # we are running in a normal Python environment
    path = os.path.dirname(os.path.abspath(__file__))
  return path


class FastbootDevice(object):
  """An abstracted fastboot device object.

  Attributes:
    serial_number: The serial number of the fastboot device.
  """

  current_path = _GetCurrentPath()
  fastboot_command = os.path.join(current_path, 'fastboot.exe')
  HOST_OS = 'Windows'

  @staticmethod
  def ListDevices():
    """List all fastboot devices.

    Returns:
      A list of serial numbers for all the fastboot devices.
    """
    try:
      out = subprocess.check_output(
          [FastbootDevice.fastboot_command, 'devices'],
          creationflags=CREATE_NO_WINDOW)
      device_serial_numbers = (out.replace('\tfastboot', '')
                               .rstrip().splitlines())
      # filter out empty string
      return filter(None, device_serial_numbers)
    except subprocess.CalledProcessError as e:
      raise fastboot_exceptions.FastbootFailure(e.output)

  def __init__(self, serial_number):
    """Initiate the fastboot device object.

    Args:
      serial_number: The serial number of the fastboot device.
    """
    self.serial_number = serial_number
    # Lock to make sure only one fastboot command can be issued to one device
    # at one time.
    self._lock = threading.Lock()

  def Reboot(self):
    """Reboot the device into fastboot mode.

    Returns:
      The command output.
    """
    try:
      self._lock.acquire()
      out = subprocess.check_output(
          [FastbootDevice.fastboot_command, '-s', self.serial_number,
           'reboot-bootloader'], creationflags=CREATE_NO_WINDOW)
      return out
    except subprocess.CalledProcessError as e:
      raise fastboot_exceptions.FastbootFailure(e.output)
    finally:
      self._lock.release()

  def Oem(self, oem_command, err_to_out):
    """"Run an OEM command.

    Args:
      oem_command: The OEM command to run.
      err_to_out: Whether to redirect stderr to stdout.
    Returns:
      The result message for the OEM command.
    Raises:
      FastbootFailure: If failure happens during the command.
    """
    try:
      self._lock.acquire()
      # We need to redirect the output no matter err_to_out is set
      # So that FastbootFailure can catch the right error.
      return subprocess.check_output(
          [
              FastbootDevice.fastboot_command, '-s', self.serial_number,
              'oem', oem_command
          ],
          stderr=subprocess.STDOUT,
          creationflags=CREATE_NO_WINDOW)
    except subprocess.CalledProcessError as e:
      raise fastboot_exceptions.FastbootFailure(e.output)
    finally:
      self._lock.release()

  def Flash(self, partition, file_path):
    """Flash a file to a partition.

    Args:
      file_path: The partition file to be flashed.
      partition: The partition to be flashed.
    Returns:
      The output for the fastboot command required.
    Raises:
      FastbootFailure: If failure happens during the command.
    """
    try:
      self._lock.acquire()
      return subprocess.check_output(
          [
              FastbootDevice.fastboot_command, '-s', self.serial_number,
              'flash', partition, file_path
          ],
          creationflags=CREATE_NO_WINDOW)
    except subprocess.CalledProcessError as e:
      raise fastboot_exceptions.FastbootFailure(e.output)
    finally:
      self._lock.release()

  def Upload(self, file_path):
    """Pulls a file from the fastboot device to the local file system.

    Args:
      file_path: The file path of the file system
        that the remote file would be pulled to.
    Returns:
      The output for the fastboot command required.
    Raises:
      FastbootFailure: If failure happens during the command.
    """
    try:
      self._lock.acquire()
      return subprocess.check_output(
          [
              FastbootDevice.fastboot_command, '-s', self.serial_number,
              'get_staged', file_path
          ],
          creationflags=CREATE_NO_WINDOW)
    except subprocess.CalledProcessError as e:
      raise fastboot_exceptions.FastbootFailure(e.output)
    finally:
      self._lock.release()

  def Download(self, file_path):
    """Push a file from the file system to the fastboot device.

    Args:
      file_path: The file path of the file on the local file system
        that would be pushed to fastboot device.
    Returns:
      The output for the fastboot command required.
    Raises:
      FastbootFailure: If failure happens during the command.
    """
    try:
      self._lock.acquire()
      return subprocess.check_output(
          [
              FastbootDevice.fastboot_command, '-s', self.serial_number,
              'stage', file_path
          ],
          creationflags=CREATE_NO_WINDOW)
    except subprocess.CalledProcessError as e:
      raise fastboot_exceptions.FastbootFailure(e.output)
    finally:
      self._lock.release()

  def GetVar(self, var):
    """Get a variable from the device.

    Note that the return value is in stderr instead of stdout.
    Args:
      var: The name of the variable.
    Returns:
      The value for the variable.
    Raises:
      FastbootFailure: If failure happens during the command.
    """
    try:
      # Fastboot getvar command's output would be in stderr instead of stdout.
      # Need to redirect stderr to stdout.
      self._lock.acquire()

      # Need the shell=True flag for windows, otherwise it hangs.
      out = subprocess.check_output(
          [
              FastbootDevice.fastboot_command, '-s', self.serial_number,
              'getvar', var
          ],
          stderr=subprocess.STDOUT,
          shell=True,
          creationflags=CREATE_NO_WINDOW)
    except subprocess.CalledProcessError as e:
      # Since we redirected stderr, we should print stdout here.
      raise fastboot_exceptions.FastbootFailure(e.output)
    finally:
      self._lock.release()
    if var == 'at-vboot-state':
      # For the result of vboot-state, it does not follow the standard.
      return out
    lines = out.splitlines()
    for line in lines:
      if line.startswith(var + ': '):
        value = line.replace(var + ': ', '').replace('\r', '')
    return value

  @staticmethod
  def GetHostOs():
    return FastbootDevice.HOST_OS

  def Disconnect(self):
    """Disconnect from the fastboot device."""
    pass

  def __del__(self):
    self.Disconnect()
