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

from metrics import verify_metric
from tests import fake


class VerifyMetricTest(unittest.TestCase):
    def test_offline(self):
        mock_output = '00serial01\toffline'
        FAKE_RESULT = fake.FakeResult(stdout=mock_output)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = verify_metric.VerifyMetric(shell=fake_shell)

        expected_result = {
            verify_metric.VerifyMetric.TOTAL_UNHEALTHY: 1,
            verify_metric.VerifyMetric.UNAUTHORIZED: [],
            verify_metric.VerifyMetric.RECOVERY: [],
            verify_metric.VerifyMetric.OFFLINE: ['00serial01'],
            verify_metric.VerifyMetric.QUESTION: [],
            verify_metric.VerifyMetric.DEVICE: []
        }
        self.assertEquals(metric_obj.gather_metric(), expected_result)

    def test_unauth(self):
        mock_output = '00serial01\tunauthorized'
        FAKE_RESULT = fake.FakeResult(stdout=mock_output)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = verify_metric.VerifyMetric(shell=fake_shell)

        expected_result = {
            verify_metric.VerifyMetric.TOTAL_UNHEALTHY: 1,
            verify_metric.VerifyMetric.UNAUTHORIZED: ['00serial01'],
            verify_metric.VerifyMetric.RECOVERY: [],
            verify_metric.VerifyMetric.OFFLINE: [],
            verify_metric.VerifyMetric.QUESTION: [],
            verify_metric.VerifyMetric.DEVICE: []
        }
        self.assertEquals(metric_obj.gather_metric(), expected_result)

    def test_device(self):
        mock_output = '00serial01\tdevice'
        FAKE_RESULT = fake.FakeResult(stdout=mock_output)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = verify_metric.VerifyMetric(shell=fake_shell)

        expected_result = {
            verify_metric.VerifyMetric.TOTAL_UNHEALTHY: 0,
            verify_metric.VerifyMetric.UNAUTHORIZED: [],
            verify_metric.VerifyMetric.RECOVERY: [],
            verify_metric.VerifyMetric.OFFLINE: [],
            verify_metric.VerifyMetric.QUESTION: [],
            verify_metric.VerifyMetric.DEVICE: ['00serial01']
        }
        self.assertEquals(metric_obj.gather_metric(), expected_result)

    def test_both(self):
        mock_output = '00serial01\toffline\n' \
                      '01serial00\tunauthorized\n' \
                      '0regan0\tdevice'
        FAKE_RESULT = fake.FakeResult(stdout=mock_output)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = verify_metric.VerifyMetric(shell=fake_shell)

        expected_result = {
            verify_metric.VerifyMetric.TOTAL_UNHEALTHY: 2,
            verify_metric.VerifyMetric.UNAUTHORIZED: ['01serial00'],
            verify_metric.VerifyMetric.RECOVERY: [],
            verify_metric.VerifyMetric.QUESTION: [],
            verify_metric.VerifyMetric.OFFLINE: ['00serial01'],
            verify_metric.VerifyMetric.DEVICE: ['0regan0']
        }

        self.assertEquals(metric_obj.gather_metric(), expected_result)

    def test_gather_device_empty(self):
        mock_output = ''
        FAKE_RESULT = fake.FakeResult(stdout=mock_output)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = verify_metric.VerifyMetric(shell=fake_shell)

        expected_result = {
            verify_metric.VerifyMetric.TOTAL_UNHEALTHY: 0,
            verify_metric.VerifyMetric.UNAUTHORIZED: [],
            verify_metric.VerifyMetric.RECOVERY: [],
            verify_metric.VerifyMetric.OFFLINE: [],
            verify_metric.VerifyMetric.QUESTION: [],
            verify_metric.VerifyMetric.DEVICE: []
        }
        self.assertEquals(metric_obj.gather_metric(), expected_result)

    def test_gather_device_question(self):
        mock_output = '00serial01\t???'
        FAKE_RESULT = fake.FakeResult(stdout=mock_output)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = verify_metric.VerifyMetric(shell=fake_shell)

        expected_result = {
            verify_metric.VerifyMetric.TOTAL_UNHEALTHY: 1,
            verify_metric.VerifyMetric.UNAUTHORIZED: [],
            verify_metric.VerifyMetric.RECOVERY: [],
            verify_metric.VerifyMetric.OFFLINE: [],
            verify_metric.VerifyMetric.QUESTION: ['00serial01'],
            verify_metric.VerifyMetric.DEVICE: []
        }
        self.assertEquals(metric_obj.gather_metric(), expected_result)


if __name__ == '__main__':
    unittest.main()
