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

from health import constant_health_analyzer as ha
import unittest


class HealthyIfGreaterThanConstantNumberTest(unittest.TestCase):
    def test_is_healthy_correct_inputs_return_true(self):
        sample_metric = {'a_key': 3}
        analyzer = ha.HealthyIfGreaterThanConstantNumber(
            key='a_key', constant=2)
        self.assertTrue(analyzer.is_healthy(sample_metric))

    def test_is_healthy_correct_inputs_return_false(self):
        sample_metric = {'a_key': 3}
        analyzer = ha.HealthyIfGreaterThanConstantNumber(
            key='a_key', constant=4)
        self.assertFalse(analyzer.is_healthy(sample_metric))


class HealthyIfLessThanConstantNumberTest(unittest.TestCase):
    def test_is_healthy_correct_inputs_return_true(self):
        sample_metric = {'a_key': 1}
        analyzer = ha.HealthyIfLessThanConstantNumber(key='a_key', constant=2)
        self.assertTrue(analyzer.is_healthy(sample_metric))

    def test_is_healthy_correct_inputs_return_false(self):
        sample_metric = {'a_key': 3}
        analyzer = ha.HealthyIfLessThanConstantNumber(key='a_key', constant=2)
        self.assertFalse(analyzer.is_healthy(sample_metric))


class HealthyIfEqualsTest(unittest.TestCase):
    def test_is_healthy_correct_string_inputs_return_true(self):
        sample_metric = {'a_key': "hi"}
        analyzer = ha.HealthyIfEquals(key='a_key', constant="hi")
        self.assertTrue(analyzer.is_healthy(sample_metric))

    def test_is_healthy_correct_num_inputs_return_true(self):
        sample_metric = {'a_key': 1}
        analyzer = ha.HealthyIfEquals(key='a_key', constant=1)
        self.assertTrue(analyzer.is_healthy(sample_metric))

    def test_is_healthy_correct_inputs_return_false(self):
        sample_metric = {'a_key': 3}
        analyzer = ha.HealthyIfEquals(key='a_key', constant=2)
        self.assertFalse(analyzer.is_healthy(sample_metric))


class HealthIfStartsWithTest(unittest.TestCase):
    def test_starts_with_true_str(self):
        sample_metric = {'kernel_release': "3.19-generic-random-12"}
        analyzer = ha.HealthyIfStartsWith(
            key='kernel_release', constant="3.19")
        self.assertTrue(analyzer.is_healthy(sample_metric))

    def test_starts_with_false_str(self):
        sample_metric = {'kernel_release': "3.19-generic-random-12"}
        analyzer = ha.HealthyIfStartsWith(
            key='kernel_release', constant="4.04")
        self.assertFalse(analyzer.is_healthy(sample_metric))

    def test_starts_with_true_non_str(self):
        sample_metric = {'kernel_release': "3.19-generic-random-12"}
        analyzer = ha.HealthyIfStartsWith(key='kernel_release', constant=3.19)
        self.assertTrue(analyzer.is_healthy(sample_metric))

    def test_starts_with_false_non_str(self):
        sample_metric = {'kernel_release': "3.19-generic-random-12"}
        analyzer = ha.HealthyIfStartsWith(key='kernel_release', constant=4.04)
        self.assertFalse(analyzer.is_healthy(sample_metric))


if __name__ == '__main__':
    unittest.main()
