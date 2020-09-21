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

import logging
import mock

from acts.controllers import android_device
from acts.libs.ota.ota_runners import ota_runner
from acts.libs.ota.ota_runners import ota_runner_factory
from acts import config_parser


class OtaRunnerFactoryTests(unittest.TestCase):
    """Tests all of the functions in the ota_runner_factory module."""

    def setUp(self):
        self.device = mock.MagicMock()
        self.device.serial = 'fake_serial'

    def test_get_ota_value_from_config_no_map_key_missing(self):
        acts_config = {}
        with self.assertRaises(config_parser.ActsConfigError):
            ota_runner_factory.get_ota_value_from_config(
                acts_config, 'ota_tool', self.device)

    def test_get_ota_value_from_config_with_map_key_missing(self):
        acts_config = {'ota_map': {'fake_serial': 'MockOtaTool'}}
        with self.assertRaises(config_parser.ActsConfigError):
            ota_runner_factory.get_ota_value_from_config(
                acts_config, 'ota_tool', self.device)

    def test_get_ota_value_from_config_with_map_key_found(self):
        expected_value = '/path/to/tool'
        acts_config = {
            'ota_map': {
                'fake_serial': 'MockOtaTool'
            },
            'ota_tool_MockOtaTool': expected_value
        }
        ret = ota_runner_factory.get_ota_value_from_config(
            acts_config, 'ota_tool', self.device)
        self.assertEqual(expected_value, ret)

    def test_create_from_configs_raise_when_non_default_tool_path_missing(
            self):
        acts_config = {
            'ota_tool': 'FakeTool',
        }
        try:
            ota_runner_factory.create_from_configs(acts_config, self.device)
        except config_parser.ActsConfigError:
            return
        self.fail('create_from_configs did not throw an error when a tool was'
                  'specified without a tool path.')

    def test_create_from_configs_without_map_makes_proper_calls(self):
        acts_config = {
            'ota_package': 'jkl;',
            'ota_sl4a': 'qaz',
            'ota_tool': 'FakeTool',
            'FakeTool': 'qwerty'
        }
        function_path = 'acts.libs.ota.ota_runners.ota_runner_factory.create'
        with mock.patch(function_path) as mocked_function:
            ota_runner_factory.create_from_configs(acts_config, self.device)
            mocked_function.assert_called_with('jkl;', 'qaz', self.device,
                                               'FakeTool', 'qwerty')

    def test_create_from_configs_with_map_makes_proper_calls(self):
        acts_config = {
            'ota_map': {
                'fake_serial': "hardwareA"
            },
            'ota_package_hardwareA': 'jkl;',
            'ota_sl4a_hardwareA': 'qaz',
            'ota_tool_hardwareA': 'FakeTool',
            'FakeTool': 'qwerty'
        }
        function_path = 'acts.libs.ota.ota_runners.ota_runner_factory.create'
        with mock.patch(function_path) as mocked_function:
            ota_runner_factory.create_from_configs(acts_config, self.device)
            mocked_function.assert_called_with('jkl;', 'qaz', self.device,
                                               'FakeTool', 'qwerty')

    def test_create_raise_on_ota_pkg_and_sl4a_fields_have_different_types(
            self):
        with mock.patch('acts.libs.ota.ota_tools.ota_tool_factory.create'):
            with self.assertRaises(TypeError):
                ota_runner_factory.create('ota_package', ['ota_sl4a'],
                                          self.device)

    def test_create_raise_on_ota_package_not_a_list_or_string(self):
        with mock.patch('acts.libs.ota.ota_tools.ota_tool_factory.create'):
            with self.assertRaises(TypeError):
                ota_runner_factory.create({
                    'ota': 'pkg'
                }, {'ota': 'sl4a'}, self.device)

    def test_create_returns_single_ota_runner_on_ota_package_being_a_str(self):
        with mock.patch('acts.libs.ota.ota_tools.ota_tool_factory.create'):
            ret = ota_runner_factory.create('', '', self.device)
            self.assertEqual(type(ret), ota_runner.SingleUseOtaRunner)

    def test_create_returns_multi_ota_runner_on_ota_package_being_a_list(self):
        with mock.patch('acts.libs.ota.ota_tools.ota_tool_factory.create'):
            ret = ota_runner_factory.create([], [], self.device)
            self.assertEqual(type(ret), ota_runner.MultiUseOtaRunner)

    def test_create_returns_bound_ota_runner_on_second_request(self):
        with mock.patch('acts.libs.ota.ota_tools.ota_tool_factory.create'):
            first_return = ota_runner_factory.create([], [], self.device)
            logging.disable(logging.WARNING)
            second_return = ota_runner_factory.create([], [], self.device)
            logging.disable(logging.NOTSET)
            self.assertEqual(first_return, second_return)

    def test_create_returns_different_ota_runner_on_second_request(self):
        with mock.patch('acts.libs.ota.ota_tools.ota_tool_factory.create'):
            first_return = ota_runner_factory.create(
                [], [], self.device, use_cached_runners=False)
            second_return = ota_runner_factory.create(
                [], [], self.device, use_cached_runners=False)
            self.assertNotEqual(first_return, second_return)
