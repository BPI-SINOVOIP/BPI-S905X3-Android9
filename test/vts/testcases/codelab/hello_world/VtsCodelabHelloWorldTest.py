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
from vts.runners.host import const
from vts.runners.host import test_runner


class VtsCodelabHelloWorldTest(base_test.BaseTestClass):
    """Two hello world test cases which use the shell driver."""

    def setUpClass(self):
        self.dut = self.android_devices[0]
        self.shell = self.dut.shell

    def testEcho1(self):
        """A simple testcase which sends a command."""
        results = self.shell.Execute(
            "echo hello_world")  # runs a shell command.
        logging.info(str(results[const.STDOUT]))  # prints the stdout
        asserts.assertEqual(results[const.STDOUT][0].strip(),
                            "hello_world")  # checks the stdout
        asserts.assertEqual(results[const.EXIT_CODE][0],
                            0)  # checks the exit code

    def testEcho2(self):
        """A simple testcase which sends two commands."""
        results = self.shell.Execute(["echo hello", "echo world"])
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]),
                            2)  # check the number of processed commands
        asserts.assertEqual(results[const.STDOUT][0].strip(), "hello")
        asserts.assertEqual(results[const.STDOUT][1].strip(), "world")
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)
        asserts.assertEqual(results[const.EXIT_CODE][1], 0)


if __name__ == "__main__":
    test_runner.main()
