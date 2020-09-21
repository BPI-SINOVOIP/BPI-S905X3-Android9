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
#   UnLESS required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import io
import mock
import sys
import unittest

from health_checker import HealthChecker
from health.constant_health_analyzer import HealthyIfGreaterThanConstantNumber
from health.constant_health_analyzer import HealthyIfLessThanConstantNumber


class HealthCheckerTestCase(unittest.TestCase):
    def test_correct_comparers(self):
        fake_config = {
            'RamMetric': {
                'free': {
                    'compare': 'GREATER_THAN',
                    'constant': 100
                }
            }
        }
        checker = HealthChecker(fake_config)
        self.assertEquals(checker._analyzers[0][0], 'RamMetric')
        self.assertIsInstance(checker._analyzers[0][1],
                              HealthyIfGreaterThanConstantNumber)

    def test_multiple_compares_one_metric(self):
        fake_config = {
            'DiskMetric': {
                'avail': {
                    'compare': 'GREATER_THAN',
                    'constant': 200
                },
                'percent_used': {
                    'compare': 'LESS_THAN',
                    'constant': 70
                }
            }
        }
        checker = HealthChecker(fake_config)
        expected_analyzers = [('DiskMetric',
                               HealthyIfGreaterThanConstantNumber(
                                   'avail', 200)),
                              ('DiskMetric', HealthyIfLessThanConstantNumber(
                                  'percent_used', 70))]
        # Have to sort the lists by first element of tuple to ensure equality
        self.assertEqual(
            checker._analyzers.sort(key=lambda x: x[0]),
            expected_analyzers.sort(key=lambda x: x[0]))

    def test_get_unhealthy_return_none(self):
        # all metrics are healthy
        fake_config = {
            'DiskMetric': {
                'avail': {
                    'compare': 'GREATER_THAN',
                    'constant': 50
                },
                'percent_used': {
                    'compare': 'LESS_THAN',
                    'constant': 70
                }
            }
        }
        checker = HealthChecker(fake_config)
        fake_metric_response = {
            'DiskMetric': {
                'total': 100,
                'used': 0,
                'avail': 100,
                'percent_used': 0
            }
        }
        # Should return an empty list, which evalutates to false
        self.assertFalse(checker.get_unhealthy(fake_metric_response))

    def test_get_unhealthy_return_all_unhealthy(self):
        fake_config = {
            'DiskMetric': {
                'avail': {
                    'compare': 'GREATER_THAN',
                    'constant': 50
                },
                'percent_used': {
                    'compare': 'LESS_THAN',
                    'constant': 70
                }
            }
        }
        checker = HealthChecker(fake_config)
        fake_metric_response = {
            'DiskMetric': {
                'total': 100,
                'used': 90,
                'avail': 10,
                'percent_used': 90
            }
        }
        expected_unhealthy = ['DiskMetric']
        self.assertEqual(
            set(checker.get_unhealthy(fake_metric_response)),
            set(expected_unhealthy))

    def test_get_unhealthy_check_vals_metric(self):
        fake_config = {
            "verify": {
                "devices": {
                    "constant": "device",
                    "compare": "EQUALS_DICT"
                }
            }
        }
        checker = HealthChecker(fake_config)
        fake_metric_response = {
            'verify': {
                'devices': {
                    'serialnumber': 'unauthorized'
                }
            }
        }
        expected_unhealthy = ['verify']
        self.assertEqual(
            set(checker.get_unhealthy(fake_metric_response)),
            set(expected_unhealthy))

    def test_get_healthy_check_vals_metric(self):
        fake_config = {
            "verify": {
                "devices": {
                    "constant": "device",
                    "compare": "EQUALS_DICT"
                }
            }
        }
        checker = HealthChecker(fake_config)
        fake_metric_response = {
            'verify': {
                'devices': {
                    'serialnumber': 'device'
                }
            }
        }
        expected_unhealthy = []
        self.assertEqual(
            set(checker.get_unhealthy(fake_metric_response)),
            set(expected_unhealthy))

    def test_catch_key_error_metric_name(self):
        fake_config = {
            "not_verify": {
                "devices": {
                    "constant": "device",
                    "compare": "EQUALS_DICT"
                }
            }
        }
        checker = HealthChecker(fake_config)
        fake_metric_response = {
            'verify': {
                'devices': {
                    'serialnumber': 'device'
                }
            }
        }
        try:
            checker.get_unhealthy(fake_metric_response)
        except KeyError as error:
            self.fail('did not catch %s' % error)

    def test_catch_key_error_metric_field(self):
        fake_config = {
            "verify": {
                "not_devices": {
                    "constant": "device",
                    "compare": "EQUALS_DICT"
                }
            }
        }
        checker = HealthChecker(fake_config)
        fake_metric_response = {
            'verify': {
                'devices': {
                    'serialnumber': 'device'
                }
            }
        }
        try:
            checker.get_unhealthy(fake_metric_response)
        except KeyError as error:
            self.fail('get_unhealthy did not internally handle %s' % error)


if __name__ == '__main__':
    unittest.main()
