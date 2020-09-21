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

from health import constant_health_analyzer_wrapper as cha
import unittest


class HealthyIfEqualsTest(unittest.TestCase):
    def test_is_healthy_correct_string_inputs_return_true(self):
        sample_metric = {'verify': {'serial1': 'device', 'serial2': 'device'}}
        analyzer = cha.HealthyIfValsEqual(key='verify', constant='device')
        self.assertTrue(analyzer.is_healthy(sample_metric))

    def test_is_healthy_one_incorrect_inputs(self):
        sample_metric = {
            'verify': {
                'serial1': 'disconnected',
                'serial2': 'device'
            }
        }
        analyzer = cha.HealthyIfValsEqual(key='verify', constant='device')
        self.assertFalse(analyzer.is_healthy(sample_metric))

    def test_is_healthy_all_incorrect_inputs(self):
        sample_metric = {
            'verify': {
                'serial1': 'disconnected',
                'serial2': 'unauthorized'
            }
        }
        analyzer = cha.HealthyIfValsEqual(key='verify', constant='device')
        self.assertFalse(analyzer.is_healthy(sample_metric))


if __name__ == '__main__':
    unittest.main()
