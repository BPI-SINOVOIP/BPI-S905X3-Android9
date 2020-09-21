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

# TODO(hsinyichen): delete this file when BINDER_IPC_32BIT is unsupported
import gzip
import logging
import os
import tempfile

from vts.runners.host import asserts
from vts.runners.host import test_runner
from vts.testcases.template.gtest_binary_test import gtest_binary_test
from vts.utils.python.controllers import adb
from vts.utils.python.os import path_utils


class VtsKernelBinderTest(gtest_binary_test.GtestBinaryTest):
    """Tests to verify kernel binder.

    Attributes:
        _dut: AndroidDevice, the device under test.
        _is_32: boolean, whether CONFIG_ANDROID_BINDER_IPC_32BIT=y.
        _BINDER_IPC_32BIT_LINE: the line enabling 32-bit binder interface in
                                kernel config.
        _TAG_IPC32: the tag on 32-bit binder interface tests.
        _TAG_IPC64: the tag on 64-bit binder interface tests.
    """
    _BINDER_IPC_32BIT_LINE = "CONFIG_ANDROID_BINDER_IPC_32BIT=y\n"
    _TAG_IPC32 = "_IPC32_"
    _TAG_IPC64 = "_IPC64_"

    # @Override
    def setUpClass(self):
        """Checks if binder interface is 32-bit."""
        super(VtsKernelBinderTest, self).setUpClass()
        self._is_32 = None
        with tempfile.NamedTemporaryFile(delete=False) as temp_file:
            config_path = temp_file.name
        try:
            logging.info("Pull config.gz to %s", config_path)
            self._dut.adb.pull("/proc/config.gz", config_path)
            with gzip.GzipFile(config_path) as config_file:
                self._is_32 = (self._BINDER_IPC_32BIT_LINE in config_file)
        except (adb.AdbError, IOError) as e:
            logging.exception("Cannot read kernel config. Both 32 and 64-bit "
                              "binder interface tests will be executed: %s", e)
        finally:
            os.remove(config_path)
        logging.info("BINDER_IPC_32BIT = %s", self._is_32)

    # @Override
    def RunTestCase(self, test_case):
        """Runs the test case corresponding to binder bitness.

        Args:
            test_case: GtestTestCase object
        """
        asserts.skipIf((self._TAG_IPC32 in test_case.tag and
                        self._is_32 == False),
                       "Skip tests for 32-bit binder interface.")
        asserts.skipIf(self._TAG_IPC64 in test_case.tag and self._is_32,
                       "Skip tests for 64-bit binder interface.")
        super(VtsKernelBinderTest, self).RunTestCase(test_case)


if __name__ == "__main__":
    test_runner.main()
