#!/usr/bin/env python
#
# Copyright (C) 2016 The Android Open Source Project
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

from vts.runners.host import base_test
from vts.runners.host import test_runner


class LibBionicLibmFuzzTest(base_test.BaseTestClass):
    """A fuzz testcase for a libm shared library of bionic."""

    def setUpClass(self):
        self.dut = self.android_devices[0]
        self.dut.lib.InitSharedLib(
            target_type="bionic_libm",
            target_basepaths=["/system/lib64"],
            target_version=1.0,
            target_filename="libm.so",
            bits=64,
            handler_name="libm",
            target_package="lib.ndk.bionic")

    def testFabs(self):
        """A simple testcase which just calls the fabs function."""
        # TODO: support ability to test non-instrumented hals.
        logging.info("result %s", self.dut.lib.libm.fabs(5.0))


if __name__ == "__main__":
    test_runner.main()
