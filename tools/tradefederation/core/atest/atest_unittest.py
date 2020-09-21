#!/usr/bin/env python
#
# Copyright 2017, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Unittests for atest."""

import unittest
import mock

import atest

#pylint: disable=protected-access
class AtestUnittests(unittest.TestCase):
    """Unit tests for atest.py"""

    @mock.patch('os.environ.get', return_value=None)
    def test_missing_environment_variables_uninitialized(self, _):
        """Test _has_environment_variables when no env vars."""
        self.assertTrue(atest._missing_environment_variables())

    @mock.patch('os.environ.get', return_value='out/testcases/')
    def test_missing_environment_variables_initialized(self, _):
        """Test _has_environment_variables when env vars."""
        self.assertFalse(atest._missing_environment_variables())

    def test_parse_args(self):
        """Test _parse_args parses command line args."""
        test_one = 'test_name_one'
        test_two = 'test_name_two'
        custom_arg = '--custom_arg'
        custom_arg_val = 'custom_arg_val'
        pos_custom_arg = 'pos_custom_arg'

        # Test out test and custom args are properly retrieved.
        args = [test_one, test_two, '--', custom_arg, custom_arg_val]
        parsed_args = atest._parse_args(args)
        self.assertEqual(parsed_args.tests, [test_one, test_two])
        self.assertEqual(parsed_args.custom_args, [custom_arg, custom_arg_val])

        # Test out custom positional args with no test args.
        args = ['--', pos_custom_arg, custom_arg_val]
        parsed_args = atest._parse_args(args)
        self.assertEqual(parsed_args.tests, [])
        self.assertEqual(parsed_args.custom_args, [pos_custom_arg,
                                                   custom_arg_val])


if __name__ == '__main__':
    unittest.main()
