#!/usr/bin/env python
#
# Copyright (C) 2018 The Android Open Source Project
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

import unittest
import vts.utils.python.controllers.android_device as android_device


class AndroidDeviceTest(unittest.TestCase):
    '''Test methods inside android_device module.'''

    def setUp(self):
        """SetUp tasks"""
        available_serials = android_device.list_adb_devices()
        self.assertGreater(len(available_serials), 0, 'no device available.')
        self.dut = android_device.AndroidDevice(available_serials[0])

    def tearDown(self):
        """TearDown tasks"""
        pass

    def testFrameworkStatusChange(self):
        '''Test AndroidDevice class startRuntime related functions.'''
        err_msg = 'Runtime status is wrong'
        print('step 1 start runtime')
        self.dut.start()

        print('step 2 check runtime status')
        self.assertTrue(self.dut.isFrameworkRunning(), err_msg)

        print('step 3 stop runtime')
        self.dut.stop()

        print('step 4 check runtime status')
        self.assertFalse(self.dut.isFrameworkRunning(), err_msg)

        print('step 5 start runtime')
        self.dut.start()

        print('step 6 check runtime status')
        self.assertTrue(self.dut.isFrameworkRunning(), err_msg)


if __name__ == "__main__":
    unittest.main()
