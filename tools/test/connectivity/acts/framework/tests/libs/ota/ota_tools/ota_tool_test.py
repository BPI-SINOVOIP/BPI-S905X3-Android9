#!/usr/bin/env python3
#
#   Copyright 2017 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import unittest
from acts.libs.ota.ota_tools import ota_tool


class OtaToolTests(unittest.TestCase):
    """Tests the OtaTool class."""

    def test_init(self):
        expected_value = 'commmand string'
        self.assertEqual(
            ota_tool.OtaTool(expected_value).command, expected_value)

    def test_start_throws_error_on_unimplemented(self):
        obj = 'some object'
        with self.assertRaises(NotImplementedError):
            ota_tool.OtaTool('').update(obj)

    def test_end_is_not_abstract(self):
        obj = 'some object'
        try:
            ota_tool.OtaTool('').cleanup(obj)
        except:
            self.fail('End is not required and should be a virtual function.')
