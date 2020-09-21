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

from health import custom_health_analyzer as ha
import unittest


class HealthyIfNotIpTest(unittest.TestCase):
    def test_hostname_not_ip(self):
        sample_metric = {'hostname': 'random_hostname_of_server'}
        analyzer = ha.HealthyIfNotIpAddress(key='hostname')
        self.assertTrue(analyzer.is_healthy(sample_metric))

    def test_hostname_ip4(self):
        sample_metric = {'hostname': '12.12.12.12'}
        analyzer = ha.HealthyIfNotIpAddress(key='hostname')
        self.assertFalse(analyzer.is_healthy(sample_metric))

    def test_hostname_ip6_empty(self):
        sample_metric = {'hostname': '::1'}
        analyzer = ha.HealthyIfNotIpAddress(key='hostname')
        self.assertFalse(analyzer.is_healthy(sample_metric))

    def test_hostname_ip6_full(self):
        sample_metric = {'hostname': '2001:db8:85a3:0:0:8a2e:370:7334'}
        analyzer = ha.HealthyIfNotIpAddress(key='hostname')
        self.assertFalse(analyzer.is_healthy(sample_metric))


if __name__ == '__main__':
    unittest.main()
