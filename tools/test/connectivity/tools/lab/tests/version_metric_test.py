#!/usr/bin/env python
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

from metrics import version_metric
from tests import fake


class VersionMetricTest(unittest.TestCase):
    """Class for testing VersionMetric."""

    def test_get_fastboot_version_error_message(self):
        stderr_str = version_metric.FastbootVersionMetric.FASTBOOT_ERROR_MESSAGE,
        FAKE_RESULT = fake.FakeResult(stderr=stderr_str)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = version_metric.FastbootVersionMetric(shell=fake_shell)

        expected_result = {
            version_metric.FastbootVersionMetric.FASTBOOT_VERSION:
            version_metric.FastbootVersionMetric.FASTBOOT_ERROR_MESSAGE
        }

        self.assertEqual(expected_result, metric_obj.gather_metric())

    def test_get_fastboot_version_one_line_input(self):
        stdout_str = 'fastboot version 5cf8bbd5b29d-android\n'

        FAKE_RESULT = fake.FakeResult(stdout=stdout_str)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = version_metric.FastbootVersionMetric(shell=fake_shell)

        expected_result = {
            version_metric.FastbootVersionMetric.FASTBOOT_VERSION:
            '5cf8bbd5b29d-android'
        }
        self.assertEqual(expected_result, metric_obj.gather_metric())

    def test_get_fastboot_version_two_line_input(self):
        stdout_str = ('fastboot version 5cf8bbd5b29d-android\n'
                      'Installed as /usr/bin/fastboot\n')

        FAKE_RESULT = fake.FakeResult(stdout=stdout_str)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = version_metric.FastbootVersionMetric(shell=fake_shell)

        expected_result = {
            version_metric.FastbootVersionMetric.FASTBOOT_VERSION:
            '5cf8bbd5b29d-android'
        }
        self.assertEqual(expected_result, metric_obj.gather_metric())

    def test_get_adb_version(self):
        stdout_str = ('Android Debug Bridge version 1.0.39\n'
                      'Revision 3db08f2c6889-android\n')

        FAKE_RESULT = fake.FakeResult(stdout=stdout_str)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = version_metric.AdbVersionMetric(shell=fake_shell)

        expected_result = {
            version_metric.AdbVersionMetric.ADB_VERSION: '1.0.39',
            version_metric.AdbVersionMetric.ADB_REVISION:
            '3db08f2c6889-android'
        }
        self.assertEqual(expected_result, metric_obj.gather_metric())

    def test_get_adb_revision_does_not_exist(self):
        stdout_str = ('Android Debug Bridge version 1.0.39\n')

        FAKE_RESULT = fake.FakeResult(stdout=stdout_str)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = version_metric.AdbVersionMetric(shell=fake_shell)

        expected_result = {
            version_metric.AdbVersionMetric.ADB_VERSION: '1.0.39',
            version_metric.AdbVersionMetric.ADB_REVISION: ''
        }
        self.assertEqual(expected_result, metric_obj.gather_metric())

    def test_get_python_version(self):
        stdout_str = 'Python 2.7.6'
        FAKE_RESULT = fake.FakeResult(stdout=stdout_str)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = version_metric.PythonVersionMetric(shell=fake_shell)

        expected_result = {
            version_metric.PythonVersionMetric.PYTHON_VERSION: '2.7.6'
        }
        self.assertEqual(expected_result, metric_obj.gather_metric())

    def test_get_kernel_release(self):
        stdout_str = '4.4.0-78-generic'

        FAKE_RESULT = fake.FakeResult(stdout=stdout_str)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = version_metric.KernelVersionMetric(shell=fake_shell)

        expected_result = {
            version_metric.KernelVersionMetric.KERNEL_RELEASE:
            '4.4.0-78-generic'
        }
        self.assertEqual(expected_result, metric_obj.gather_metric())


if __name__ == '__main__':
    unittest.main()
