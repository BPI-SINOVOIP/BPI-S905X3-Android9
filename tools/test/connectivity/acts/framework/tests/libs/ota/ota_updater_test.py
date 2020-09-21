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

import mock
import unittest
from acts.libs.ota import ota_updater
from acts.libs.ota.ota_runners import ota_runner


class MockAndroidDevice(object):
    def __init__(self, serial):
        self.serial = serial
        self.log = mock.Mock()
        self.take_bug_report = mock.MagicMock()


class MockOtaRunner(object):
    def __init__(self):
        self.call_count = 0
        self.should_fail = False
        self.can_update_value = 'CAN_UPDATE_CALLED'

    def set_failure(self, should_fail=True):
        self.should_fail = should_fail

    def update(self):
        self.call_count += 1
        if self.should_fail:
            raise ota_runner.OtaError

    def can_update(self):
        return self.can_update_value


class OtaUpdaterTests(unittest.TestCase):
    """Tests the methods in the ota_updater module."""

    def test_initialize(self):
        user_params = {'a': 1, 'b': 2, 'c': 3}
        android_devices = ['x', 'y', 'z']
        with mock.patch('acts.libs.ota.ota_runners.ota_runner_factory.'
                        'create_from_configs') as fn:
            ota_updater.initialize(user_params, android_devices)
            for i in range(len(android_devices)):
                fn.assert_has_call(mock.call(user_params, android_devices[i]))
        self.assertSetEqual(
            set(android_devices), set(ota_updater.ota_runners.keys()))

    def test_check_initialization_is_initialized(self):
        device = MockAndroidDevice('serial')
        ota_updater.ota_runners = {
            device: ota_runner.OtaRunner('tool', device)
        }
        try:
            ota_updater._check_initialization(device)
        except ota_runner.OtaError:
            self.fail('_check_initialization raised for initialized runner!')

    def test_check_initialization_is_not_initialized(self):
        device = MockAndroidDevice('serial')
        ota_updater.ota_runners = {}
        with self.assertRaises(KeyError):
            ota_updater._check_initialization(device)

    def test_update_do_not_ignore_failures_and_failures_occur(self):
        device = MockAndroidDevice('serial')
        runner = MockOtaRunner()
        runner.set_failure(True)
        ota_updater.ota_runners = {device: runner}
        with self.assertRaises(ota_runner.OtaError):
            ota_updater.update(device)

    def test_update_ignore_failures_and_failures_occur(self):
        device = MockAndroidDevice('serial')
        runner = MockOtaRunner()
        runner.set_failure(True)
        ota_updater.ota_runners = {device: runner}
        try:
            ota_updater.update(device, ignore_update_errors=True)
        except ota_runner.OtaError:
            self.fail('OtaError was raised when errors are to be ignored!')

    def test_can_update(self):
        device = MockAndroidDevice('serial')
        runner = MockOtaRunner()
        ota_updater.ota_runners = {device: runner}
        self.assertEqual(ota_updater.can_update(device), 'CAN_UPDATE_CALLED')
