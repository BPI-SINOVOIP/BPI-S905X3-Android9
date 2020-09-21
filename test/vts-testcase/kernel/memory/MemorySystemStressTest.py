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
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
import re


class MemorySystemStressTest(base_test.BaseTestClass):
    """Userspace Memory Stress Test."""
    _mem = 64
    _DURATION_SEC = 300
    _STRESSAPPTEST = '/data/local/tmp/32/stressapptest'

    def setUpClass(self):
        self.dut = self.android_devices[0]

        # Set executable bit on stressapptest binary
        self.shell = self.dut.shell
        cmd = ['chmod +x', str(self._STRESSAPPTEST)]
        self.shell.Execute(' '.join(cmd))

        # Read /proc/meminfo to read MemFree; use MemFree as total memory under
        # test
        meminfo_results = self.shell.Execute("cat /proc/meminfo")
        matched = re.search(r'MemFree:\s+(\d+)', str(meminfo_results[const.STDOUT]))
        if matched:
            self._mem = int(matched.groups()[0]) / 1024
            self._mem = max(self._mem, 64)   # Minimum 64M memory under test
            self._mem = min(self._mem, 1280) # Maximum 1280M memory under test

    def testMemory32bit(self):
        """Memory Test using stressapptest."""

        # Compose stressapptest command to execute for memory test
        cmd = [str(self._STRESSAPPTEST), '-W ' '-M', str(self._mem), '-s', str(self._DURATION_SEC)]
        logging.info(cmd)
        results = self.shell.Execute(' '.join(cmd))
        logging.info(str(results[const.STDOUT]))

        # Check for 'PASS' and exit code
        asserts.assertTrue('Status: PASS' in results[const.STDOUT][0].strip(), "Memory test failed.")

if __name__ == "__main__":
    test_runner.main()
