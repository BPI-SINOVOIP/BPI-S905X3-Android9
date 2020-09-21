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

from vts.runners.host import const
from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device


class VtsKernelSyscallExistenceTest(base_test.BaseTestClass):
    """Tests to verify kernel syscall interface."""

    def setUpClass(self):
        self.dut = self.android_devices[0]
        if "arm" in self.dut.cpu_abi:
            self.ARCH64__NR_name_to_handle_at = 264
            self.ARCH64__NR_open_by_handle_at = 265
            self.ARCH64__NR_uselib = 1077
        elif "x86" in self.dut.cpu_abi:
            self.ARCH64__NR_name_to_handle_at = 303
            self.ARCH64__NR_open_by_handle_at = 304
            self.ARCH64__NR_uselib = 134
        else:
            asserts.fail("Unknown CPU ABI: %s" % self.dut.cpu_abi)

    def tearDown(self):
        results = self.dut.shell.Execute("which ls")
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]), 1)
        asserts.assertEqual(results[const.STDOUT][0].strip(), "/system/bin/ls")
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)

    def testSyscall_name_to_handle_at(self):
        """Testcase to verify syscall [name_to_handle_at] is disabled."""
        if self.dut.is64Bit:
            logging.info("testing syscall: name_to_handle_at [%d]",
                         self.ARCH64__NR_name_to_handle_at)
            asserts.assertTrue(self.SyscallDisabled(self.ARCH64__NR_name_to_handle_at),
                               "syscall [name_to_handle_at] should be disabled")
        else:
            asserts.skip("32-bit not supported")

    def testSyscall_open_by_handle_at(self):
        """Testcase to verify syscall [open_by_handle_at] is disabled."""
        if self.dut.is64Bit:
            logging.info("testing syscall: open_by_handle_at [%d]",
                         self.ARCH64__NR_open_by_handle_at)
            asserts.assertTrue(self.SyscallDisabled(self.ARCH64__NR_open_by_handle_at),
                               "syscall [open_by_handle_at] should be disabled")
        else:
            asserts.skip("32-bit not supported")

    def testSyscall_uselib(self):
        """Testcase to verify syscall [uselib] is disabled."""
        if self.dut.is64Bit:
            logging.info("testing syscall: uselib [%d]",
                         self.ARCH64__NR_uselib)
            asserts.assertTrue(self.SyscallDisabled(self.ARCH64__NR_uselib),
                               "syscall [uselib] should be disabled")
        else:
            asserts.skip("32-bit not supported")

    def SyscallDisabled(self, syscallid):
        """Helper function to check if a syscall is disabled."""
        target = "/data/local/tmp/64/vts_test_binary_syscall_exists"
        results = self.dut.shell.Execute([
            "chmod 755 %s" % target,
            "%s %d" % (target, syscallid)
        ])
        return len(results[const.STDOUT]) == 2 and results[const.STDOUT][1].strip() == "n"


if __name__ == "__main__":
    test_runner.main()
