#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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
from tradefed_py import base_test
from tradefed_py import tf_main

class HelloWorldTest(base_test._TradefedTestClass):
    """An example showing a possible implementation of python test"""

    def test_hello(self):
        self.assertEqual('hello'.upper(), 'HELLO')

    def test_world_failed(self):
        self.assertEqual('world'.upper(), 'WORLD2')

    @unittest.skip('demonstrating skipping')
    def test_skipped(self):
        self.fail('should have been skipped')

    @unittest.expectedFailure
    def test_expectation(self):
        self.fail('failed')

    @unittest.expectedFailure
    def test_failedExpectation(self):
        pass

    def test_device(self):
        """If a serial was provided this test will check that we can query the
        device. It will throw if the serial is invalid.
        """
        if self.serial is not None:
            res = self.android_device.executeShellCommand('id')
            self.assertTrue('uid' in res)

if __name__ == '__main__':
    tf_main.main()
