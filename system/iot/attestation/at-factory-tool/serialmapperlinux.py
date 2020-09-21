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

"""This module provides the serial number to USB location map on Linux."""
import os


class SerialMapper(object):
  """Maps serial number to its USB physical location.

  This class should run under Linux environment and use sysfs to enumerate the
  USB devices. Use the serial file's content to create the map.
  """

  USB_DEVICES_PATH = '/sys/bus/usb/devices/'

  def __init__(self):
    self.serial_map = {}

  def refresh_serial_map(self):
    """Refresh the serial_number -> USB location map.
    """
    serial_to_location_map = {}
    # check if sysfs is mounted.
    if not os.path.exists(self.USB_DEVICES_PATH):
      return

    for device_folder_name in os.listdir(self.USB_DEVICES_PATH):
      device_folder = os.path.join(self.USB_DEVICES_PATH, device_folder_name)
      if os.path.isdir(device_folder):
        # The format of folder name should be either:
        # USB1, USB2... which are controllers (ignored).
        # bus-port[.port.port] which are devices.
        # bus-port[.port.port]:config.interface which are interfaces (ignored).
        if ':' not in device_folder and '-' in device_folder:
          serial_path = os.path.join(device_folder, 'serial')
          if os.path.isfile(serial_path):
            with open(serial_path) as f:
              serial = f.readline().rstrip('\n').lower()
              serial_to_location_map[serial] = device_folder_name

    self.serial_map = serial_to_location_map

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
