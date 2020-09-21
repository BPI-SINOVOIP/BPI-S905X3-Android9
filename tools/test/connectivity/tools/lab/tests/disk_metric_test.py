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

from metrics import disk_metric
from tests import fake


class DiskMetricTest(unittest.TestCase):
    """Class for testing DiskMetric."""

    def test_return_total_used_avail_percent(self):
        # Create sample stdout string ShellCommand.run() would return
        stdout_string = '/dev/sda 57542652 18358676 ' '36237928  34% /'
        FAKE_RESULT = fake.FakeResult(stdout=stdout_string)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = disk_metric.DiskMetric(shell=fake_shell)

        expected_result = {
            disk_metric.DiskMetric.TOTAL: 57542652,
            disk_metric.DiskMetric.USED: 18358676,
            disk_metric.DiskMetric.AVAIL: 36237928,
            disk_metric.DiskMetric.PERCENT_USED: 34
        }
        self.assertEqual(expected_result, metric_obj.gather_metric())


if __name__ == '__main__':
    unittest.main()
