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

from io import BytesIO
import unittest

from tests import fake
from metrics.usb_metric import Device
from metrics.usb_metric import UsbMetric
import mock


class UsbMetricTest(unittest.TestCase):
    def test_check_usbmon_install_t(self):
        pass

    def test_check_usbmon_install_f(self):
        pass

    def test_check_get_bytes_2(self):
        with mock.patch('subprocess.Popen') as mock_popen:
            mock_popen.return_value.stdout = BytesIO(
                b'x x C Ii:2:003:1 0:8 8 = x x\nx x S Ii:2:004:1 -115:8 8 <')

            self.assertEquals(UsbMetric().get_bytes(0),
                              {'2:003': 8,
                               '2:004': 8})

    def test_check_get_bytes_empty(self):
        with mock.patch('subprocess.Popen') as mock_popen:
            mock_popen.return_value.stdout = BytesIO(b'')
            self.assertEquals(UsbMetric().get_bytes(0), {})

    def test_match_device_id(self):
        mock_lsusb = ('Bus 003 Device 047: ID 18d1:d00d Device 0\n'
                      'Bus 003 Device 001: ID 1d6b:0002 Device 1')
        exp_res = {'3:047': 'Device 0', '3:001': 'Device 1'}
        fake_result = fake.FakeResult(stdout=mock_lsusb)
        fake_shell = fake.MockShellCommand(fake_result=fake_result)
        m = UsbMetric(shell=fake_shell)
        self.assertEquals(m.match_device_id(), exp_res)

    def test_match_device_id_empty(self):
        mock_lsusb = ''
        exp_res = {}
        fake_result = fake.FakeResult(stdout=mock_lsusb)
        fake_shell = fake.MockShellCommand(fake_result=fake_result)
        m = UsbMetric(shell=fake_shell)
        self.assertEquals(m.match_device_id(), exp_res)

    def test_gen_output(self):
        dev_name_dict = {
            '1:001': 'Device 1',
            '1:002': 'Device 2',
            '1:003': 'Device 3'
        }
        dev_byte_dict = {'1:001': 256, '1:002': 200}

        dev_1 = Device('1:002', 200, 'Device 2')
        dev_2 = Device('1:001', 256, 'Device 1')
        dev_3 = Device('1:003', 0, 'Device 3')

        act_out = UsbMetric().gen_output(dev_name_dict, dev_byte_dict)

        self.assertTrue(dev_1 in act_out and dev_2 in act_out and
                        dev_3 in act_out)
