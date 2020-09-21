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
from vts.runners.host import test_runner

from vts.testcases.template.gtest_binary_test import gtest_binary_test


class VtsKernelLibcutilsTest(gtest_binary_test.GtestBinaryTest):
    '''Test runner script for kernel libctils test.

    Attributes:
        include_test_suite: list of string, gtest test suite names to include.
    '''

    include_test_suite = ["AshmemTest"]

    # Override
    def filterOneTest(self, test_name):
        """Check test filters for a test name.

        The first layer of filter is user defined test filters:
        if a include filter is not empty, only tests in include filter will
        be executed regardless whether they are also in exclude filter. Else
        if include filter is empty, only tests not in exclude filter will be
        executed.

        The second layer of filter is checking abi bitness:
        if a test has a suffix indicating the intended architecture bitness,
        and the current abi bitness information is available, non matching tests
        will be skipped. By our convention, this function will look for bitness in suffix
        formated as "32bit", "32Bit", "32BIT", or 64 bit equivalents.

        This method assumes  const.SUFFIX_32BIT and const.SUFFIX_64BIT are in lower cases.

        Args:
            test_name: string, name of a test

        Raises:
            signals.TestSilent if a test should not be executed
        """
        super(VtsKernelLibcutilsTest, self).filterOneTest(test_name)
        asserts.skipIf(
            test_name.split('.')[0] not in self.include_test_suite,
            'Test case not selected.')


if __name__ == "__main__":
    test_runner.main()
