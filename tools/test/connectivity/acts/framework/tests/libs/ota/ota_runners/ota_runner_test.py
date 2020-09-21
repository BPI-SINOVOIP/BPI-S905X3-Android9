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
import mock

from acts.libs.ota.ota_tools import ota_tool
from acts.libs.ota.ota_runners import ota_runner
from acts.controllers import android_device


class MockOtaTool(ota_tool.OtaTool):
    def __init__(self, command):
        super(MockOtaTool, self).__init__(command)
        self.update_call_count = 0
        self.cleanup_call_count = 0

    def update(self, unused):
        self.update_call_count += 1

    def cleanup(self, unused):
        self.cleanup_call_count += 1

    def reset_count(self):
        self.update_call_count = 0
        self.cleanup_call_count = 0

    def assert_calls_equal(self, test, number_of_expected_calls):
        test.assertEqual(number_of_expected_calls, self.update_call_count)
        test.assertEqual(number_of_expected_calls, self.cleanup_call_count)


class OtaRunnerImpl(ota_runner.OtaRunner):
    """Sets properties to return an empty string to allow OtaRunner tests."""

    def get_sl4a_apk(self):
        return ''

    def get_ota_package(self):
        return ''


class OtaRunnerTest(unittest.TestCase):
    """Tests the OtaRunner class."""

    def setUp(self):
        self.prev_sl4a_service_setup_time = ota_runner.SL4A_SERVICE_SETUP_TIME
        ota_runner.SL4A_SERVICE_SETUP_TIME = 0

    def tearDown(self):
        ota_runner.SL4A_SERVICE_SETUP_TIME = self.prev_sl4a_service_setup_time

    def test_update(self):
        device = mock.MagicMock()
        tool = MockOtaTool('mock_command')
        runner = OtaRunnerImpl(tool, device)
        runner.android_device.adb.getprop = mock.Mock(side_effect=['a', 'b'])
        runner._update()
        device.stop_services.assert_called()
        device.wait_for_boot_completion.assert_called()
        device.start_services.assert_called()
        device.adb.install.assert_called()
        tool.assert_calls_equal(self, 1)

    def test_update_fail_on_no_change_to_build(self):
        device = mock.MagicMock()
        tool = MockOtaTool('mock_command')
        runner = OtaRunnerImpl(tool, device)
        runner.android_device.adb.getprop = mock.Mock(side_effect=['a', 'a'])
        try:
            runner._update()
            self.fail('Matching build fingerprints did not throw an error!')
        except ota_runner.OtaError:
            pass

    def test_init(self):
        device = mock.MagicMock()
        tool = MockOtaTool('mock_command')
        runner = ota_runner.OtaRunner(tool, device)

        self.assertEqual(runner.ota_tool, tool)
        self.assertEqual(runner.android_device, device)
        self.assertEqual(runner.serial, device.serial)


class SingleUseOtaRunnerTest(unittest.TestCase):
    """Tests the SingleUseOtaRunner class."""

    def setUp(self):
        self.device = mock.MagicMock()
        self.tool = MockOtaTool('mock_command')

    def test_update_first_update_runs(self):
        runner = ota_runner.SingleUseOtaRunner(self.tool, self.device, '', '')
        try:
            with mock.patch.object(ota_runner.OtaRunner, '_update'):
                runner.update()
        except ota_runner.OtaError:
            self.fail('SingleUseOtaRunner threw an exception on the first '
                      'update call.')

    def test_update_second_update_raises_error(self):
        runner = ota_runner.SingleUseOtaRunner(self.tool, self.device, '', '')
        with mock.patch.object(ota_runner.OtaRunner, '_update'):
            runner.update()
            try:
                runner.update()
            except ota_runner.OtaError:
                return
        self.fail('SingleUseOtaRunner did not throw an exception on the second'
                  'update call.')

    def test_can_update_no_updates_called(self):
        runner = ota_runner.SingleUseOtaRunner(self.tool, self.device, '', '')
        self.assertEqual(True, runner.can_update())

    def test_can_update_has_updated_already(self):
        runner = ota_runner.SingleUseOtaRunner(self.tool, self.device, '', '')
        with mock.patch.object(ota_runner.OtaRunner, '_update'):
            runner.update()
        self.assertEqual(False, runner.can_update())

    def test_get_ota_package(self):
        runner = ota_runner.SingleUseOtaRunner(self.tool, self.device, 'a',
                                               'b')
        self.assertEqual(runner.get_ota_package(), 'a')

    def test_get_sl4a_apk(self):
        runner = ota_runner.SingleUseOtaRunner(self.tool, self.device, 'a',
                                               'b')
        self.assertEqual(runner.get_sl4a_apk(), 'b')


class MultiUseOtaRunnerTest(unittest.TestCase):
    """Tests the MultiUseOtaRunner class."""

    def setUp(self):
        self.device = mock.MagicMock()
        self.tool = MockOtaTool('mock_command')

    def test_update_first_update_runs(self):
        runner = ota_runner.MultiUseOtaRunner(self.tool, self.device, [''],
                                              [''])
        try:
            with mock.patch.object(ota_runner.OtaRunner, '_update'):
                runner.update()
        except ota_runner.OtaError:
            self.fail('MultiUseOtaRunner threw an exception on the first '
                      'update call.')

    def test_update_multiple_updates_run(self):
        runner = ota_runner.MultiUseOtaRunner(self.tool, self.device,
                                              ['first_pkg', 'second_pkg'],
                                              ['first_apk', 'second_apk'])
        with mock.patch.object(ota_runner.OtaRunner, '_update'):
            runner.update()
            try:
                runner.update()
            except ota_runner.OtaError:
                self.fail('MultiUseOtaRunner threw an exception before '
                          'running out of update packages.')

    def test_update_too_many_update_calls_raises_error(self):
        runner = ota_runner.MultiUseOtaRunner(self.tool, self.device,
                                              ['first_pkg', 'second_pkg'],
                                              ['first_apk', 'second_apk'])
        with mock.patch.object(ota_runner.OtaRunner, '_update'):
            runner.update()
            runner.update()
            try:
                runner.update()
            except ota_runner.OtaError:
                return
        self.fail('MultiUseOtaRunner did not throw an exception after running '
                  'out of update packages.')

    def test_can_update_no_updates_called(self):
        runner = ota_runner.MultiUseOtaRunner(self.tool, self.device,
                                              ['first_pkg', 'second_pkg'],
                                              ['first_apk', 'second_apk'])
        self.assertEqual(True, runner.can_update())

    def test_can_update_has_more_updates_left(self):
        runner = ota_runner.MultiUseOtaRunner(self.tool, self.device,
                                              ['first_pkg', 'second_pkg'],
                                              ['first_apk', 'second_apk'])
        with mock.patch.object(ota_runner.OtaRunner, '_update'):
            runner.update()
        self.assertEqual(True, runner.can_update())

    def test_can_update_ran_out_of_updates(self):
        runner = ota_runner.MultiUseOtaRunner(self.tool, self.device,
                                              ['first_pkg', 'second_pkg'],
                                              ['first_apk', 'second_apk'])
        with mock.patch.object(ota_runner.OtaRunner, '_update'):
            runner.update()
            runner.update()
        self.assertEqual(False, runner.can_update())

    def test_get_ota_package(self):
        runner = ota_runner.MultiUseOtaRunner(self.tool, self.device,
                                              ['first_pkg', 'second_pkg'],
                                              ['first_apk', 'second_apk'])
        self.assertEqual(runner.get_ota_package(), 'first_pkg')

    def test_get_sl4a_apk(self):
        runner = ota_runner.MultiUseOtaRunner(self.tool, self.device,
                                              ['first_pkg', 'second_pkg'],
                                              ['first_apk', 'second_apk'])
        self.assertEqual(runner.get_sl4a_apk(), 'first_apk')
