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

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.runners.host import const


class ShellBinaryCrashTest(base_test.BaseTestClass):
    """A binary crash test case for the shell driver."""

    EXIT_CODE_CRASH = 133
    EXIT_CODE_SEGFAULT = 139

    def setUpClass(self):
        self.run_as_vts_self_test = False
        self.dut = self.registerController(android_device)[0]

    def testCrashBinary(self):
        """Tests whether the agent survives when a called binary crashes."""
        self.dut.shell.InvokeTerminal("my_shell1")
        target = "/data/local/tmp/vts_test_binary_crash_app"
        results = self.dut.shell.my_shell1.Execute(
            ["chmod 755 %s" % target, target])
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]), 2)
        asserts.assertEqual(results[const.STDOUT][1].strip(), "")
        # "crash_app: start" is also valid output.
        asserts.assertEqual(results[const.EXIT_CODE][1], self.EXIT_CODE_CRASH)

        self.CheckShellDriver("my_shell1")
        self.CheckShellDriver("my_shell2")

    def testSegmentFaultBinary(self):
        """Tests whether the agent survives when a binary leads to segfault."""
        self.dut.shell.InvokeTerminal("my_shell1")
        target = "/data/local/tmp/vts_test_binary_seg_fault"
        results = self.dut.shell.my_shell1.Execute(
            ["chmod 755 %s" % target, target])
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]), 2)
        asserts.assertEqual(results[const.STDOUT][1].strip(), "")
        # TODO: currently the agent doesn't return the stdout log emitted
        # before a failure.
        asserts.assertEqual(results[const.EXIT_CODE][1],
                            self.EXIT_CODE_SEGFAULT)

        self.CheckShellDriver("my_shell1")
        self.CheckShellDriver("my_shell2")

    def CheckShellDriver(self, shell_name):
        """Checks whether the shell driver sevice is available.

        Args:
            shell_name: string, the name of a shell service to create.
        """
        self.dut.shell.InvokeTerminal(shell_name)
        results = getattr(self.dut.shell, shell_name).Execute("which ls")
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]), 1)
        asserts.assertEqual(results[const.STDOUT][0].strip(),
                            "/system/bin/ls")
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)


if __name__ == "__main__":
    test_runner.main()
