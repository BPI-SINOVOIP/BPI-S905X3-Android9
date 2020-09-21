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


class VtsSelfTestBaseTestFilterIncludeExclude(
        VtsSelfTestBaseTestFilter.VtsSelfTestBaseTestFilter):
    '''Filter test class for include and exclude filter overlapping.

    Attributes:
        SHOULD_PASS_FILTER: list of string, test names that should pass
                            the internal test filter configured by user
        SHOULD_NOT_PASS_FILTER: list of string, test names that should pass
                                the internal test filter configured by user
    '''

    SHOULD_PASS_FILTER = [
        'suite1.test1_32bit',
        'any_32bit',
        'r(fake.regex)',
    ]

    SHOULD_NOT_PASS_FILTER = [
        'suite1.test1',
        'suite1.test1_64bit',
        'any.other',
    ]


if __name__ == "__main__":
    test_runner.main()
