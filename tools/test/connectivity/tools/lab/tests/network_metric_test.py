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

from metrics.network_metric import NetworkMetric
from tests import fake


class NetworkMetricTest(unittest.TestCase):
    def test_hostname_prefix(self):
        mock_stdout = 'android'
        FAKE_RESULT = fake.FakeResult(stdout=mock_stdout)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = NetworkMetric(['8.8.8.8'], shell=fake_shell)
        expected_result = 'android'
        self.assertEqual(metric_obj.get_prefix_hostname(), expected_result)

    def test_connected_empty(self):
        mock_result = fake.FakeResult(exit_status=0, stdout="connected")
        metric_obj = NetworkMetric(shell=fake.MockShellCommand(
            fake_result=mock_result))
        exp_out = {'8.8.8.8': True, '8.8.4.4': True}
        self.assertEquals(metric_obj.check_connected(), exp_out)

    def test_connected_false(self):
        mock_result = fake.FakeResult(exit_status=1, stdout="not connected")
        metric_obj = NetworkMetric(shell=fake.MockShellCommand(
            fake_result=mock_result))
        exp_out = {'8.8.8.8': False, '8.8.4.4': False}
        self.assertEquals(metric_obj.check_connected(), exp_out)

    def test_connected_true_passed_in(self):
        mock_result = fake.FakeResult(exit_status=0, stdout="connected")
        metric_obj = NetworkMetric(
            ['8.8.8.8'], shell=fake.MockShellCommand(fake_result=mock_result))
        self.assertEquals(
            metric_obj.check_connected(metric_obj.ip_list), {'8.8.8.8': True})


if __name__ == '__main__':
    unittest.main()
