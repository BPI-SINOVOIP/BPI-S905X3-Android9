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

import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import signals
from vts.utils.python.common import filter_utils


class VtsSelfTestBaseTestFilter(base_test.BaseTestClass):
    '''Base class for filter tests.

    Attributes:
        SHOULD_PASS_FILTER: list of string, test names that should pass
                            the internal test filter configured by user
        SHOULD_NOT_PASS_FILTER: list of string, test names that should not pass
                                the internal test filter configured by user
    '''
    SHOULD_PASS_FILTER = []
    SHOULD_NOT_PASS_FILTER = []

    # Override
    def setUpClass(self):
        # Since we are running the actual test cases, run_as_vts_self_test
        # must be set to False.
        self.run_as_vts_self_test = False

        self.dut = self.android_devices[0]
        self.shell = self.dut.shell

    # Override
    def filterOneTest(self, test_name, test_filter=None):
        '''Filter a test case using the give test name.'''
        pass

    def CheckPassFilter(self, name):
        '''Check whether test filter accept a give name.

        Args:
            name: string, test name

        Returns:
            bool, True if accept, False otherwise
        '''
        try:
            super(VtsSelfTestBaseTestFilter,
                  self)._filterOneTestThroughTestFilter(name)
        except signals.TestSilent:
            return False
        else:
            return True

    def generatePassFilterTests(self):
        '''Generate test cases for filter passing test names.'''
        logging.info('generating tests for SHOULD_PASS_FILTER: %s',
                     self.SHOULD_PASS_FILTER)
        self.runGeneratedTests(
            test_func=lambda x: asserts.assertTrue(self.CheckPassFilter(x), 'Filter should accept test name: %s' % x),
            settings=self.SHOULD_PASS_FILTER,
            name_func=lambda x: 'filter_accept_%s' % x)

    def generateNonPassFilterTests(self):
        '''Generate test cases for filter non passing test names.'''
        logging.info('generating tests for SHOULD_NOT_PASS_FILTER: %s',
                     self.SHOULD_NOT_PASS_FILTER)
        self.runGeneratedTests(
            test_func=lambda x: asserts.assertFalse(self.CheckPassFilter(x), 'Filter should not accept test name: %s' % x),
            settings=self.SHOULD_NOT_PASS_FILTER,
            name_func=lambda x: 'filter_not_accept_%s' % x)
