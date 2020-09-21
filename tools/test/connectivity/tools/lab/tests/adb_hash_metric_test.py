#!/usr/bin/env python
#
#   copyright 2017 - the android open source project
#
#   licensed under the apache license, version 2.0 (the "license");
#   you may not use this file except in compliance with the license.
#   you may obtain a copy of the license at
#
#       http://www.apache.org/licenses/license-2.0
#
#   unless required by applicable law or agreed to in writing, software
#   distributed under the license is distributed on an "as is" basis,
#   without warranties or conditions of any kind, either express or implied.
#   see the license for the specific language governing permissions and
#   limitations under the license.

import unittest

from metrics import adb_hash_metric
import mock

from tests import fake


class HashMetricTest(unittest.TestCase):
    @mock.patch('os.environ', {'ADB_VENDOR_KEYS': '/root/adb/'})
    def test_gather_metric_env_set(self):
        # Create sample stdout string ShellCommand.run() would return
        stdout_string = ('12345abcdef_hash')
        FAKE_RESULT = fake.FakeResult(stdout=stdout_string)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = adb_hash_metric.AdbHashMetric(shell=fake_shell)

        expected_result = {
            'hash': '12345abcdef_hash',
            'keys_path': '/root/adb/'
        }
        self.assertEqual(expected_result, metric_obj.gather_metric())

    @mock.patch('os.environ', {})
    def test_gather_metric_env_not_set(self):
        # Create sample stdout string ShellCommand.run() would return
        stdout_string = ('12345abcdef_hash')
        FAKE_RESULT = fake.FakeResult(stdout=stdout_string)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = adb_hash_metric.AdbHashMetric(shell=fake_shell)

        expected_result = {'hash': None, 'keys_path': None}
        self.assertEqual(expected_result, metric_obj.gather_metric())

    @mock.patch('os.environ', {'ADB_VENDOR_KEYS': '/root/adb/'})
    def test_verify_env_set(self):
        self.assertEquals(adb_hash_metric.AdbHashMetric()._verify_env(),
                          '/root/adb/')

    @mock.patch('os.environ', {})
    def test_verify_env_not_set(self):
        self.assertEquals(adb_hash_metric.AdbHashMetric()._verify_env(), None)


if __name__ == '__main__':
    unittest.main()
