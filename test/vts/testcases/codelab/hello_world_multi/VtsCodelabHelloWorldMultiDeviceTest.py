#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner


class VtsCodelabHelloWorldTest(base_test.BaseTestClass):
    '''Two hello world test cases which use the shell driver.'''

    def setUpClass(self):
        logging.info('number of device: %s', self.android_devices)

        asserts.assertEqual(
            len(self.android_devices), 2, 'number of device is wrong.')

        self.dut1 = self.android_devices[0]
        self.dut2 = self.android_devices[1]
        self.shell1 = self.dut1.shell
        self.shell2 = self.dut2.shell

    def testHelloWorld(self):
        '''A simple testcase which sends a hello world command.'''
        command = 'echo Hello World!'

        res1 = self.shell1.Execute(command)
        res2 = self.shell2.Execute(command)

        asserts.assertFalse(
            any(res1[const.EXIT_CODE]),
            'command for device 1 failed: %s' % res1)  # checks the exit code
        asserts.assertFalse(
            any(res2[const.EXIT_CODE]),
            'command for device 2 failed: %s' % res2)  # checks the exit code

    def testSerialNotEqual(self):
        '''Checks serial number from two device not being equal.'''
        command = 'getprop | grep ro.serial'

        res1 = self.shell1.Execute(command)
        res2 = self.shell2.Execute(command)

        asserts.assertFalse(
            any(res1[const.EXIT_CODE]),
            'command for device 1 failed: %s' % res1)  # checks the exit code
        asserts.assertFalse(
            any(res2[const.EXIT_CODE]),
            'command for device 2 failed: %s' % res2)  # checks the exit code

        def getSerial(output):
            '''Get serial from getprop query'''
            return output.strip().split(' ')[-1][1:-1]

        serial1 = getSerial(res1[const.STDOUT][0])
        serial2 = getSerial(res2[const.STDOUT][0])

        logging.info('Serial number of device 1: %s', serial1)
        logging.info('Serial number of device 2: %s', serial2)

        asserts.assertNotEqual(
            serial1, serial2,
            'serials from two devices should not be the same')


if __name__ == '__main__':
    test_runner.main()
