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

"""This module provides the USB device serial number to USB location map on Win.

This module uses Windows APIs to get USB device information. Ctypes are used to
support calling C type functions.
"""
# pylint: disable=invalid-name
import ctypes
from ctypes.wintypes import BYTE
from ctypes.wintypes import DWORD
from ctypes.wintypes import ULONG
from ctypes.wintypes import WORD

NULL = None
DIGCF_ALLCLASSES = 0x4
DIGCF_PRESENT = 0x2
ULONG_PTR = ctypes.POINTER(ULONG)
SPDRP_HARDWAREID = 1
SPDRP_LOCATION_INFORMATION = 0xD
INVALID_HANDLE_VALUE = -1
ERROR_NO_MORE_ITEMS = 0x103
BUFFER_SIZE = 1024


class GUID(ctypes.Structure):
  _fields_ = [
      ('Data1', DWORD),
      ('Data2', WORD),
      ('Data3', WORD),
      ('Data4', BYTE*8),
  ]


class SP_DEVINFO_DATA(ctypes.Structure):
  _fields_ = [
      ('cbSize', DWORD),
      ('ClassGuid', GUID),
      ('DevInst', DWORD),
      ('Reserved', ULONG_PTR),
  ]


class SerialMapper(object):
  """Maps serial number to its USB physical location.

  This class should run under Windows environment. It uses windows setupapi DLL
  to get USB device information. This class is just a wrapper around windows C++
  library.
  """

  def __init__(self):
    self.setupapi = ctypes.WinDLL('setupapi')
    self.serial_map = {}

  def refresh_serial_map(self):
    """Refresh the serial_number -> USB location map.
    """
    serial_map = {}
    device_inf_set = None
    SetupDiGetClassDevs = self.setupapi.SetupDiGetClassDevsA
    SetupDiEnumDeviceInfo = self.setupapi.SetupDiEnumDeviceInfo
    SetupDiGetDeviceRegistryProperty = (
        self.setupapi.SetupDiGetDeviceRegistryPropertyA)
    SetupDiGetDeviceInstanceId = self.setupapi.SetupDiGetDeviceInstanceIdA
    SetupDiDestroyDeviceInfoList = self.setupapi.SetupDiDestroyDeviceInfoList

    # Get the device information set for all the present USB devices
    flags = DIGCF_ALLCLASSES | DIGCF_PRESENT
    device_inf_set = SetupDiGetClassDevs(NULL,
                                         ctypes.c_char_p('USB'),
                                         NULL,
                                         flags)
    if device_inf_set == INVALID_HANDLE_VALUE:
      raise ctypes.WinError()

    devinfo = SP_DEVINFO_DATA()
    p_dev_info = ctypes.byref(devinfo)
    # cbsize is the size of SP_DEVINFO_DATA, need to be set
    devinfo.cbSize = ctypes.sizeof(devinfo)
    i = 0
    while True:
      # Enumerate through the device information set until ERROR_NO_MORE_ITEMS
      # i is the index

      # Fill the devinfo
      result = SetupDiEnumDeviceInfo(device_inf_set, i, ctypes.byref(devinfo))
      if not result and (ctypes.GetLastError() == ERROR_NO_MORE_ITEMS):
        # If we reach the last device.
        break
      location_buffer = ctypes.create_string_buffer(BUFFER_SIZE)
      p_location_buffer = ctypes.byref(location_buffer)
      result = SetupDiGetDeviceRegistryProperty(device_inf_set,
                                                p_dev_info,
                                                SPDRP_LOCATION_INFORMATION,
                                                NULL,
                                                p_location_buffer,
                                                BUFFER_SIZE,
                                                NULL)
      if not result:
        i += 1
        continue
      location = location_buffer.value
      device_instance_id_buffer = ctypes.create_string_buffer(BUFFER_SIZE)
      p_id_buffer = ctypes.byref(device_instance_id_buffer)
      result = SetupDiGetDeviceInstanceId(device_inf_set,
                                          p_dev_info,
                                          p_id_buffer,
                                          BUFFER_SIZE,
                                          NULL)
      if not result:
        i += 1
        continue

      # device instance id contains a serial number in the format of
      # [XXX]\[SERIAL]
      instance_id = device_instance_id_buffer.value
      instance_parts = instance_id.split('\\')
      if instance_parts:
        serial = instance_parts.pop().lower()
        serial_map[serial] = location
      i += 1

    # Destroy the device information set
    if device_inf_set is not None:
      SetupDiDestroyDeviceInfoList(device_inf_set)
    self.serial_map = serial_map

  def get_location(self, serial):
    """Get the USB location according to the serial number.

    Args:
      serial: The serial number for the device.
    Returns:
      The USB physical location for the device.
    """
    serial_lower = serial.lower()
    if serial_lower in self.serial_map:
      return self.serial_map[serial_lower]
    return None

