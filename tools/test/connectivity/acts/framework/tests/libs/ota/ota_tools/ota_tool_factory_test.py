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
from acts.libs.ota.ota_tools import ota_tool_factory


class MockOtaTool(object):
    def __init__(self, command):
        self.command = command


class OtaToolFactoryTests(unittest.TestCase):
    def setUp(self):
        ota_tool_factory._constructed_tools = {}

    def test_create_constructor_exists(self):
        ota_tool_factory._CONSTRUCTORS = {
            MockOtaTool.__name__: lambda command: MockOtaTool(command),
        }
        ret = ota_tool_factory.create(MockOtaTool.__name__, 'command')
        self.assertEqual(type(ret), MockOtaTool)
        self.assertTrue(ret in ota_tool_factory._constructed_tools.values())

    def test_create_not_in_constructors(self):
        ota_tool_factory._CONSTRUCTORS = {}
        with self.assertRaises(KeyError):
            ota_tool_factory.create(MockOtaTool.__name__, 'command')

    def test_create_returns_cached_tool(self):
        ota_tool_factory._CONSTRUCTORS = {
            MockOtaTool.__name__: lambda command: MockOtaTool(command),
        }
        ret_a = ota_tool_factory.create(MockOtaTool.__name__, 'command')
        ret_b = ota_tool_factory.create(MockOtaTool.__name__, 'command')
        self.assertEqual(ret_a, ret_b)
