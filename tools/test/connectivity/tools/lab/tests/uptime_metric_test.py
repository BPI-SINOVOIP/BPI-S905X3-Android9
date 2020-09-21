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

from metrics import uptime_metric
from tests import fake


class UptimeMetricTest(unittest.TestCase):
    """Class for testing UptimeMetric."""

    def test_get_seconds(self):
        stdout_string = '358350.70'
        FAKE_RESULT = fake.FakeResult(stdout=stdout_string)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = uptime_metric.UptimeMetric(shell=fake_shell)

        expected_result = '358350.70'
        self.assertEqual(expected_result, metric_obj.get_seconds())

    def test_get_readable(self):
        stdout_string = '2 days, 8 hours, 19 minutes'
        FAKE_RESULT = fake.FakeResult(stdout=stdout_string)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = uptime_metric.UptimeMetric(shell=fake_shell)

        expected_result = '2 days, 8 hours, 19 minutes'
        self.assertEqual(expected_result, metric_obj.get_readable())


if __name__ == '__main__':
    unittest.main()
