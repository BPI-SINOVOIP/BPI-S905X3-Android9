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

from metrics import zombie_metric
from tests import fake


class ZombieMetricTest(unittest.TestCase):
    """Class for testing ZombieMetric."""

    def test_gather_metric_oob(self):
        stdout_string = '30888 Z+ adb -s'
        FAKE_RESULT = fake.FakeResult(stdout=stdout_string)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = zombie_metric.ZombieMetric(shell=fake_shell)

        expected_result = {
            zombie_metric.ZombieMetric.ADB_ZOMBIES: [None],
            zombie_metric.ZombieMetric.NUM_ADB_ZOMBIES: 1,
            zombie_metric.ZombieMetric.FASTBOOT_ZOMBIES: [],
            zombie_metric.ZombieMetric.NUM_FASTBOOT_ZOMBIES: 0,
            zombie_metric.ZombieMetric.OTHER_ZOMBIES: [],
            zombie_metric.ZombieMetric.NUM_OTHER_ZOMBIES: 0
        }
        self.assertEqual(expected_result, metric_obj.gather_metric())

    def test_gather_metric_no_serial(self):
        stdout_string = '30888 Z+ adb <defunct>'
        FAKE_RESULT = fake.FakeResult(stdout=stdout_string)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = zombie_metric.ZombieMetric(shell=fake_shell)

        expected_result = {
            zombie_metric.ZombieMetric.ADB_ZOMBIES: [None],
            zombie_metric.ZombieMetric.NUM_ADB_ZOMBIES: 1,
            zombie_metric.ZombieMetric.FASTBOOT_ZOMBIES: [],
            zombie_metric.ZombieMetric.NUM_FASTBOOT_ZOMBIES: 0,
            zombie_metric.ZombieMetric.OTHER_ZOMBIES: [],
            zombie_metric.ZombieMetric.NUM_OTHER_ZOMBIES: 0
        }
        self.assertEqual(expected_result, metric_obj.gather_metric())

    def test_gather_metric_with_serial(self):
        stdout_string = ('12345 Z+ fastboot -s M4RKY_M4RK\n'
                         '99999 Z+ adb -s OR3G4N0\n')
        FAKE_RESULT = fake.FakeResult(stdout=stdout_string)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = zombie_metric.ZombieMetric(shell=fake_shell)

        expected_result = {
            zombie_metric.ZombieMetric.ADB_ZOMBIES: ['OR3G4N0'],
            zombie_metric.ZombieMetric.NUM_ADB_ZOMBIES: 1,
            zombie_metric.ZombieMetric.FASTBOOT_ZOMBIES: ['M4RKY_M4RK'],
            zombie_metric.ZombieMetric.NUM_FASTBOOT_ZOMBIES: 1,
            zombie_metric.ZombieMetric.OTHER_ZOMBIES: [],
            zombie_metric.ZombieMetric.NUM_OTHER_ZOMBIES: 0
        }
        self.assertEquals(metric_obj.gather_metric(), expected_result)

    def test_gather_metric_adb_fastboot_no_s(self):
        stdout_string = ('12345 Z+ fastboot\n' '99999 Z+ adb\n')
        FAKE_RESULT = fake.FakeResult(stdout=stdout_string)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = zombie_metric.ZombieMetric(shell=fake_shell)

        expected_result = {
            zombie_metric.ZombieMetric.ADB_ZOMBIES: [None],
            zombie_metric.ZombieMetric.NUM_ADB_ZOMBIES: 1,
            zombie_metric.ZombieMetric.FASTBOOT_ZOMBIES: [None],
            zombie_metric.ZombieMetric.NUM_FASTBOOT_ZOMBIES: 1,
            zombie_metric.ZombieMetric.OTHER_ZOMBIES: [],
            zombie_metric.ZombieMetric.NUM_OTHER_ZOMBIES: 0
        }
        self.assertEquals(metric_obj.gather_metric(), expected_result)

    def test_gather_metric_no_adb_fastboot(self):
        stdout_string = '12345 Z+ otters'
        FAKE_RESULT = fake.FakeResult(stdout=stdout_string)
        fake_shell = fake.MockShellCommand(fake_result=FAKE_RESULT)
        metric_obj = zombie_metric.ZombieMetric(shell=fake_shell)
        metric_obj_res = metric_obj.gather_metric()
        exp_num = 1
        exp_pid = ['12345']

        self.assertEquals(metric_obj_res[metric_obj.NUM_OTHER_ZOMBIES],
                          exp_num)
        self.assertEquals(metric_obj_res[metric_obj.OTHER_ZOMBIES], exp_pid)


if __name__ == '__main__':
    unittest.main()
