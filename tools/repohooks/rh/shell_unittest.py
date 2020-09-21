#!/usr/bin/python
# -*- coding:utf-8 -*-
# Copyright 2016 The Android Open Source Project
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

"""Unittests for the shell module."""

from __future__ import print_function

import difflib
import os
import sys
import unittest

_path = os.path.realpath(__file__ + '/../..')
if sys.path[0] != _path:
    sys.path.insert(0, _path)
del _path

# We have to import our local modules after the sys.path tweak.  We can't use
# relative imports because this is an executable program, not a module.
# pylint: disable=wrong-import-position
import rh.shell


class DiffTestCase(unittest.TestCase):
    """Helper that includes diff output when failing."""

    def setUp(self):
        self.differ = difflib.Differ()

    def _assertEqual(self, func, test_input, test_output, result):
        """Like assertEqual but with built in diff support."""
        diff = '\n'.join(list(self.differ.compare([test_output], [result])))
        msg = ('Expected %s to translate %r to %r, but got %r\n%s' %
               (func, test_input, test_output, result, diff))
        self.assertEqual(test_output, result, msg)

    def _testData(self, functor, tests, check_type=True):
        """Process a dict of test data."""
        for test_output, test_input in tests.iteritems():
            result = functor(test_input)
            self._assertEqual(functor.__name__, test_input, test_output, result)

            if check_type:
                # Also make sure the result is a string, otherwise the %r
                # output will include a "u" prefix and that is not good for
                # logging.
                self.assertEqual(type(test_output), str)


class ShellQuoteTest(DiffTestCase):
    """Test the shell_quote & shell_unquote functions."""

    def testShellQuote(self):
        """Basic ShellQuote tests."""
        # Dict of expected output strings to input lists.
        tests_quote = {
            "''": '',
            'a': unicode('a'),
            "'a b c'": unicode('a b c'),
            "'a\tb'": 'a\tb',
            "'/a$file'": '/a$file',
            "'/a#file'": '/a#file',
            """'b"c'""": 'b"c',
            "'a@()b'": 'a@()b',
            'j%k': 'j%k',
            r'''"s'a\$va\\rs"''': r"s'a$va\rs",
            r'''"\\'\\\""''': r'''\'\"''',
            r'''"'\\\$"''': r"""'\$""",
        }

        # Expected input output specific to ShellUnquote.  This string cannot
        # be produced by ShellQuote but is still a valid bash escaped string.
        tests_unquote = {
            r'''\$''': r'''"\\$"''',
        }

        def aux(s):
            return rh.shell.shell_unquote(rh.shell.shell_quote(s))

        self._testData(rh.shell.shell_quote, tests_quote)
        self._testData(rh.shell.shell_unquote, tests_unquote)

        # Test that the operations are reversible.
        self._testData(aux, {k: k for k in tests_quote.values()}, False)
        self._testData(aux, {k: k for k in tests_quote.keys()}, False)


class CmdToStrTest(DiffTestCase):
    """Test the cmd_to_str function."""

    def testCmdToStr(self):
        # Dict of expected output strings to input lists.
        tests = {
            r"a b": ['a', 'b'],
            r"'a b' c": ['a b', 'c'],
            r'''a "b'c"''': ['a', "b'c"],
            r'''a "/'\$b" 'a b c' "xy'z"''':
                [unicode('a'), "/'$b", 'a b c', "xy'z"],
            '': [],
        }
        self._testData(rh.shell.cmd_to_str, tests)


class BooleanShellTest(unittest.TestCase):
    """Test the boolean_shell_value function."""

    def testFull(self):
        """Verify nputs work as expected"""
        for v in (None,):
            self.assertTrue(rh.shell.boolean_shell_value(v, True))
            self.assertFalse(rh.shell.boolean_shell_value(v, False))

        for v in (1234, '', 'akldjsf', '"'):
            self.assertRaises(ValueError, rh.shell.boolean_shell_value, v, True)

        for v in ('yes', 'YES', 'YeS', 'y', 'Y', '1', 'true', 'True', 'TRUE',):
            self.assertTrue(rh.shell.boolean_shell_value(v, True))
            self.assertTrue(rh.shell.boolean_shell_value(v, False))

        for v in ('no', 'NO', 'nO', 'n', 'N', '0', 'false', 'False', 'FALSE',):
            self.assertFalse(rh.shell.boolean_shell_value(v, True))
            self.assertFalse(rh.shell.boolean_shell_value(v, False))


if __name__ == '__main__':
    unittest.main()
