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


class SampleShellTest(base_test.BaseTestClass):
    """A sample testcase for the shell driver."""

    REPEAT_COUNT = 10

    def setUpClass(self):
        self.dut = self.registerController(android_device)[0]

    def testOneCommand(self):
        """A simple testcase which just emulates a normal usage pattern."""
        self.dut.shell.InvokeTerminal("my_shell1")
        results = self.dut.shell.my_shell1.Execute("which ls")
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]), 1)
        asserts.assertEqual(results[const.STDOUT][0].strip(), "/system/bin/ls")
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)

    def testCommandList(self):
        """A simple testcase which just emulates a normal usage pattern."""
        self.dut.shell.InvokeTerminal("my_shell2")
        results = self.dut.shell.my_shell2.Execute(["which ls"] *
                                                   self.REPEAT_COUNT)
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]), self.REPEAT_COUNT)
        for index in range(self.REPEAT_COUNT):
            asserts.assertEqual(results[const.STDOUT][index].strip(),
                                "/system/bin/ls")
            asserts.assertEqual(results[const.EXIT_CODE][index], 0)

    def testMultipleCommands(self):
        """A simple testcase which just emulates a normal usage pattern."""
        self.dut.shell.InvokeTerminal("my_shell3")
        for _ in range(self.REPEAT_COUNT):
            results = self.dut.shell.my_shell3.Execute("which ls")
            logging.info(str(results[const.STDOUT]))
            asserts.assertEqual(len(results[const.STDOUT]), 1)
            asserts.assertEqual(results[const.STDOUT][0].strip(),
                                "/system/bin/ls")
            asserts.assertEqual(results[const.EXIT_CODE][0], 0)

    def testCommandSequenceCd(self):
        """A simple test case that emulates using cd bash command sequence
           connected by '&&' under normal usage pattern.
        """
        self.dut.shell.InvokeTerminal("command_sequence_cd")
        directory = "/data/local"
        commands = ["cd %s && pwd" % directory, "'cd' '%s' && 'pwd'" %
                    directory, "\"cd\" \"%s\" && \"pwd\"" % directory]
        for cmd in commands:
            results = self.dut.shell.command_sequence_cd.Execute(cmd)
            asserts.assertEqual(results[const.EXIT_CODE][0], 0)
            asserts.assertEqual(results[const.STDOUT][0].strip(), directory)

    def testCommandSequenceExport(self):
        """A simple test case that emulates using export bash command sequence
           connected by '&&' under normal usage pattern.
        """
        self.dut.shell.InvokeTerminal("command_sequence_export")
        var_value = "helloworld"
        results = self.dut.shell.command_sequence_export.Execute(
            "export {var_name}={var_value} && echo ${var_name}".format(
                var_name="TESTTMPVAR", var_value=var_value))
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)
        asserts.assertEqual(results[const.STDOUT][0].strip(), var_value)

    def testCommandSequenceMktemp(self):
        """A simple test case that emulates using mktemp bash command sequence
           connected by '&&' under normal usage pattern.
        """
        self.dut.shell.InvokeTerminal("command_sequence_mktemp")
        results = self.dut.shell.command_sequence_mktemp.Execute(
            "TMPFILE=`mktemp /data/local/tmp/test.XXXXXXXXXXXX` "
            "&& ls $TMPFILE")
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)

    def testMultipleShells(self):
        """A simple testcase which just emulates a normal usage pattern."""
        for index in range(self.REPEAT_COUNT):
            current_shell_name = "shell%s" % index
            self.dut.shell.InvokeTerminal(current_shell_name)
            current_shell = getattr(self.dut.shell, current_shell_name)
            results = current_shell.Execute("which ls")
            logging.info(str(results[const.STDOUT]))
            asserts.assertEqual(len(results[const.STDOUT]), 1)
            asserts.assertEqual(results[const.STDOUT][0].strip(),
                                "/system/bin/ls")
            asserts.assertEqual(results[const.EXIT_CODE][0], 0)


if __name__ == "__main__":
    test_runner.main()
