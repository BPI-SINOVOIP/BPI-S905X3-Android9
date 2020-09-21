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

from vts.runners.host import test_runner
from vts.testcases.vts_selftest.test_framework.base_test import VtsSelfTestBaseTestFilter


class VtsSelfTestBaseTestFilterExclude(
        VtsSelfTestBaseTestFilter.VtsSelfTestBaseTestFilter):
    '''Filter test class for exclude filter.

    Attributes:
        SHOULD_PASS_FILTER: list of string, test names that should pass
                            the internal test filter configured by user
        SHOULD_NOT_PASS_FILTER: list of string, test names that should pass
                                the internal test filter configured by user
    '''

    SHOULD_PASS_FILTER = [
        'suite1.test1_16bit',
        'suite1.test2',
        'suite1.test2_64bit',
        'suite2.test1_32bit',
        'suite2.test1_64bit',
        'suite3.test2_64bit',
        # Since include_filter is empty, any pattern not matching the ones in
        # exclude filter should pass
        'other.test_names',
    ]

    SHOULD_NOT_PASS_FILTER = [
        'suite1.test1',
        'suite1.test2_32bit',
        'suite2.test2_32bit',
        'suite2.test2_64bit',
        'suite3.test1',
        'suite3.test1_32bit',
        'suite3.test1_64bit',
        'suite3.test2_32bit',
        # The following test is added to exclude filter through
        # filter's add_to_exclude_filter method when expand_bitness is True
        'added.test1_32bit',
        # The following test is added to include filter with negative pattern by
        # filter's add_to_exclude_filter method when expand_bitness is True
        'added.test2_64bit',
    ]

    # Override
    def setUpClass(self):
        super(VtsSelfTestBaseTestFilterExclude, self).setUpClass()
        self.test_filter.add_to_exclude_filter('added.test1')
        self.test_filter.add_to_include_filter('-added.test2')

if __name__ == "__main__":
    test_runner.main()
