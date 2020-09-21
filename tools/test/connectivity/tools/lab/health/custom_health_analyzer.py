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

from health import health_analyzer
from IPy import IP


class HealthyIfNotIpAddress(health_analyzer.HealthAnalyzer):
    def __init__(self, key):
        self.key = key

    def is_healthy(self, metric_result):
        """Returns whether numeric result is an non-IP Address.
        Args:
            metric_result: a dictionary of metric results.

        Returns:
            True if the metric is not an IP Address.
        """

        try:
            IP(metric_result[self.key])
        except (ValueError, TypeError):
            return True
        return False
