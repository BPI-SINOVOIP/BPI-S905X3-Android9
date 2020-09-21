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

from metrics import ram_metric
from tests import fake


class RamMetricTest(unittest.TestCase):
    """Class for testing RamMetric."""

    def test_correct_ram_output(self):
        # Create sample stdout string ShellCommand.run() would return
        stdout_string = ('             total       used       free     shared'
                         'buffers     cached\nMem:      64350   34633   '
                         '29717     309024    1744   24692\n-/+ '
                         'buffers/cache:    9116   56777\nSwap:     '
                         '67031          0   67031')

        FAKE_RESULT = fake.FakeResult(stdout=stdout_string)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = ram_metric.RamMetric(shell=fake_shell)

        expected_result = {
            ram_metric.RamMetric.TOTAL: 64350,
            ram_metric.RamMetric.USED: 34633,
            ram_metric.RamMetric.FREE: 29717,
            ram_metric.RamMetric.BUFFERS: 1744,
            ram_metric.RamMetric.CACHED: 24692
        }
        self.assertEqual(expected_result, metric_obj.gather_metric())


if __name__ == '__main__':
    unittest.main()
