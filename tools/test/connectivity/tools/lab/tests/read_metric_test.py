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

import mock
from metrics import read_metric
from tests import fake


class ReadMetricTest(unittest.TestCase):
    """Class for testing ReadMetric."""

    @mock.patch('os.getuid')
    def test_return_total_used_avail_percent_with_permiss(self, getuid_func):
        getuid_func.return_value = 0
        # Create sample stdout string ShellCommand.run() would return
        stdout_string = ('/dev/sda:\n'
                         ' Timing cached reads: '
                         '18192 MB in  2.00 seconds = 9117.49 MB/sec\n'
                         ' Timing buffered disk reads: '
                         '414 MB in 3.07 seconds = 134.80 MB/sec\n'
                         '\n/dev/sda:\n'
                         ' Timing cached reads:   '
                         '18100 MB in  2.00 seconds = 9071.00 MB/sec\n'
                         ' Timing buffered disk reads: '
                         '380 MB in  3.01 seconds = 126.35 MB/sec\n'
                         '\n/dev/sda:\n'
                         ' Timing cached reads:   '
                         '18092 MB in  2.00 seconds = 9067.15 MB/sec\n'
                         ' Timing buffered disk reads: '
                         '416 MB in  3.01 seconds = 138.39 MB/sec')
        FAKE_RESULT = fake.FakeResult(stdout=stdout_string)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = read_metric.ReadMetric(shell=fake_shell)

        exp_res = {
            read_metric.ReadMetric.CACHED_READ_RATE:
            27255.64 / read_metric.ReadMetric.NUM_RUNS,
            read_metric.ReadMetric.BUFFERED_READ_RATE:
            399.54 / read_metric.ReadMetric.NUM_RUNS
        }
        result = metric_obj.gather_metric()
        self.assertAlmostEqual(
            exp_res[read_metric.ReadMetric.CACHED_READ_RATE],
            result[metric_obj.CACHED_READ_RATE])
        self.assertAlmostEqual(
            exp_res[read_metric.ReadMetric.BUFFERED_READ_RATE],
            result[metric_obj.BUFFERED_READ_RATE])

    @mock.patch('os.getuid')
    def test_return_total_used_avail_percent_with_no_permiss(
            self, getuid_func):
        getuid_func.return_value = 1
        exp_res = {
            read_metric.ReadMetric.CACHED_READ_RATE: None,
            read_metric.ReadMetric.BUFFERED_READ_RATE: None
        }
        result = read_metric.ReadMetric().gather_metric()
        self.assertEquals(result, exp_res)


if __name__ == '__main__':
    unittest.main()
