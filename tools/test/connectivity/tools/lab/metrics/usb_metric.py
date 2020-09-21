#!/usr/bin/env python
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

import io
import os
import subprocess
import logging
import sys

from metrics.metric import Metric
from utils import job
from utils import time_limit


def _get_output(stdout):
    if sys.version_info[0] == 2:
        return iter(stdout.readline, '')
    else:
        return io.TextIOWrapper(stdout, encoding="utf-8")


class UsbMetric(Metric):
    """Class to determine all USB Device traffic over a timeframe."""
    USB_IO_COMMAND = 'cat /sys/kernel/debug/usb/usbmon/0u | grep -v \'S Ci\''
    USBMON_CHECK_COMMAND = 'grep usbmon /proc/modules'
    USBMON_INSTALL_COMMAND = 'modprobe usbmon'
    DEVICES = 'devices'

    def is_privileged(self):
        """Checks if this module is being ran as the necessary root user.

        Returns:
            T if being run as root, F if not.
        """

        return os.getuid() == 0

    def check_usbmon(self):
        """Checks if the kernel module 'usbmon' is installed.

        Runs the command using shell.py.

        Raises:
            job.Error: When the module could not be loaded.
        """
        try:
            self._shell.run(self.USBMON_CHECK_COMMAND)
        except job.Error:
            logging.info('Kernel module not loaded, attempting to load usbmon')
            try:
                self._shell.run(self.USBMON_INSTALL_COMMAND)
            except job.Error as error:
                raise job.Error('Cannot load usbmon: %s' % error.result.stderr)

    def get_bytes(self, time=5):
        """Gathers data about USB Busses in a given timeframe.

        When ran, must have super user privileges as well as having the module
        'usbmon' installed. Since .../0u is a stream-file, we must read it in
        as a stream in the off chance of reading in too much data to buffer.

        Args:
            time: The amount of time data will be gathered in seconds.

        Returns:
            A dictionary where the key is the device's bus and device number,
            and value is the amount of bytes transferred in the timeframe.
        """
        bytes_sent = {}
        with time_limit.TimeLimit(time):
            # Lines matching 'S Ci' do not match output, and only 4 bytes/sec
            process = subprocess.Popen(
                self.USB_IO_COMMAND,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                shell=True)

            for line in _get_output(process.stdout):
                spl_line = line.split(' ')
                # Example line                  spl_line[3]   " "[5]
                # ffff88080bb00780 2452973093 C Ii:2:003:1 0:8 8 = 00000000

                # Splits from whole line, into Ii:2:003:1, and then cuts it
                # down to 2:003, this is for consistency as keys in dicts.
                dev_id = ':'.join(spl_line[3].split(':')[1:3])
                if dev_id in bytes_sent:
                    # spl_line[5] is the number of bytes transferred from a
                    # device, in the example line, spl_line[5] == 8
                    bytes_sent[dev_id] += int(spl_line[5])
                else:
                    bytes_sent[dev_id] = int(spl_line[5])
        return bytes_sent

    def match_device_id(self):
        """ Matches a device's id with its name according to lsusb.

        Returns:
            A dictionary with the devices 'bus:device' as key, and name of the
            device as a string. 'bus:device', the bus number is stripped of
            leading 0's because that is how 'usbmon' formats it.
        """
        devices = {}
        result = self._shell.run('lsusb').stdout

        if result:
            # Example line
            # Bus 003 Device 048: ID 18d1:4ee7 Device Name
            for line in result.split('\n'):
                line_list = line.split(' ')
                # Gets bus number, strips leading 0's, adds a ':', and then adds
                # the device, without its ':'. Example line output: 3:048
                dev_id = line_list[1].lstrip('0') + ':' + line_list[3].strip(
                    ':')
                # Parses the device name, example line output: 'Device Name'
                dev_name = ' '.join(line_list[6:])
                devices[dev_id] = dev_name
        return devices

    def gen_output(self, dev_name_dict, dev_byte_dict):
        """ Combines all information about device for returning.

        Args:
            dev_name_dict: A dictionary with the key as 'bus:device', leading
            0's stripped from bus, and value as the device's name.
            dev_byte_dict: A dictionary with the key as 'bus:device', leading
            0's stripped from bus, and value as the number of bytes transferred.
        Returns:
            List of populated Device objects.
        """
        devices = []
        for dev in dev_name_dict:
            if dev in dev_byte_dict:
                devices.append(
                    Device(dev, dev_byte_dict[dev], dev_name_dict[dev]))
            else:
                devices.append(Device(dev, 0, dev_name_dict[dev]))
        return devices

    def gather_metric(self):
        """ Gathers the usb bus metric

        Returns:
            A dictionary, with a single entry, 'devices', and the value of a
            list of Device objects. This is to fit with the formatting of other
            metrics.
        """
        if self.is_privileged():
            self.check_usbmon()
            dev_byte_dict = self.get_bytes()
            dev_name_dict = self.match_device_id()
            return {
                self.DEVICES: self.gen_output(dev_name_dict, dev_byte_dict)
            }
        else:
            return {self.DEVICES: None}


class Device:
    """USB Device Information

    Contains information about bytes transferred in timeframe for a device.

    Attributes:
        dev_id: The device id, usuall in form BUS:DEVICE
        trans_bytes: The number of bytes transferred in timeframe.
        name: The device's name according to lsusb.
    """

    def __init__(self, dev_id, trans_bytes, name):
        self.dev_id = dev_id
        self.trans_bytes = trans_bytes
        self.name = name

    def __eq__(self, other):
        return isinstance(other, Device) and \
               self.dev_id == other.dev_id and \
               self.trans_bytes == other.trans_bytes and \
               self.name == other.name
