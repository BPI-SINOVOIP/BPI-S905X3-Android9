#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import power_cycle_usb_util


class PowerCycleUsbUtilTest(unittest.TestCase):
    """Unittest for the parse functions within power_cycle_usb_util.py."""

    VID = '0001'
    PID = '0001'
    BUS = 1
    DEV = 2

    LSUSB_DEVICE_OUTPUT = 'Bus 001 Device 002:  ID 0001:0001\n'
    LSUSB_DEVICE_OUTPUT_NONE = ''
    LSUSB_DEVICE_OUTPUT_MULTI = ('Bus 001 Device 002:  ID 0001:0001\n'
                                 'Bus 001 Device 002:  ID 0001:0001\n')

    LSUSB_TREE_OUTPUT = \
    ('/:  Bus 02.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/4p, 5000M\n'
     '    |__ Port 3: Dev 2, If 0, Class=Hub, Driver=hub/4p, 5000M\n'
     '/:  Bus 01.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/11p, 480M\n'
     '    |__ Port 2: Dev 52, If 0, Class=Hub, Driver=hub/4p, 480M\n'
     '        |__ Port 1: Dev 2, If 0, Class=Human Interface Device,'
     'Driver=usbhid, 12M\n'
     '        |__ Port 3: Dev 54, If 0, Class=Vendor Specific Class,'
     'Driver=udl, 480M\n'
     '    |__ Port 3: Dev 3, If 0, Class=Hub, Driver=hub/4p, 480M\n'
     '    |__ Port 4: Dev 4, If 0, Class=Wireless, Driver=btusb, 12M\n'
     '    |__ Port 4: Dev 4, If 1, Class=Wireless, Driver=btusb, 12M\n')

    def test_get_bus_dev_id(self):
        want = (self.BUS, self.DEV)
        want_none = (None, None)
        want_multi = (None, None)

        bus, dev = power_cycle_usb_util.get_bus_dev_id(
            self.LSUSB_DEVICE_OUTPUT, self.VID, self.PID)
        self.assertEqual((bus, dev), want)
        bus, dev = power_cycle_usb_util.get_bus_dev_id(
            self.LSUSB_DEVICE_OUTPUT_NONE, self.VID, self.PID)
        self.assertEqual((bus, dev), want_none)
        bus, dev = power_cycle_usb_util.get_bus_dev_id(
            self.LSUSB_DEVICE_OUTPUT_MULTI, self.VID, self.PID)
        self.assertEqual((bus, dev), want_multi)

    def test_get_port_number(self):
        want = 2

        port = power_cycle_usb_util.get_port_number(
            self.LSUSB_TREE_OUTPUT, self.BUS, self.DEV)
        self.assertEqual(port, want)


if __name__ == '__main__':
  unittest.main()
