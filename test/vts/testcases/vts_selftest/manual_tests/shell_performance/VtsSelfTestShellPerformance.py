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

import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
import time


class VtsSelfTestShellPerformance(base_test.BaseTestClass):
    '''A simple performance test to compare adb shell and VTS shell.'''

    def setUpClass(self):
        # Since we are running the actual test cases, run_as_vts_self_test
        # must be set to False.
        self.run_as_vts_self_test = False

        self.dut = self.android_devices[0]
        self.shell = self.dut.shell

    def VtsShell(self, cmd, n):
        '''Execute a command for n times via VTS shell.

        Args:
            cmd: string, command to execute
            n: int, number of repeated calls
        '''
        for i in range(n):
            self.shell.Execute(cmd)

    def AdbShell(self, cmd, n):
        '''Execute a command for n times via ADB shell.

        Args:
            cmd: string, command to execute
            n: int, number of repeated calls
        '''
        for i in range(n):
            self.dut.adb.shell(cmd)

    def VtsShellList(self, cmd, n):
        '''Execute a command for n times via VTS shell as a list.

        Args:
            cmd: string, command to execute
            n: int, number of repeated calls
        '''
        self.shell.Execute([cmd] * n)

    def Measure(self, func, *args):
        '''Measure time lapsed when executing a function.

        Args:
            func: function, function to execute
            *args: list of arguments for the functions
        '''
        start = time.time()
        func(*args)
        return time.time() - start

    def testPerformance(self):
        '''Run a empty test case on device for 100 times and log the times.'''

        cmd = "/data/local/tmp/zero_testcase"

        # First call to eliminate caching effects
        self.AdbShell(cmd, 1)
        self.VtsShell(cmd, 1)

        repeats = 100

        adb_time = self.Measure(self.AdbShell, cmd, repeats)
        vts_time = self.Measure(self.VtsShell, cmd, repeats)
        vts_list_time = self.Measure(self.VtsShellList, cmd, repeats)

        logging.info("adb shell for 100 times = %s", adb_time)
        logging.info("vts shell for 100 times = %s", vts_time)
        logging.info("vts shell with for 100 times = %s", vts_list_time)


if __name__ == "__main__":
    test_runner.main()
