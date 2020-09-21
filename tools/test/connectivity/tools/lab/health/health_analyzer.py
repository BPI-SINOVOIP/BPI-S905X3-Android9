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


class HealthAnalyzer(object):
    """Interface class for determining whether metrics are healthy or unhealthy.

    """

    def is_healthy(self, metric_results):
        """Returns whether a metric is considered healthy

        Function compares the metric mapped to by key it was initialized with

        Args:
          metric_results: a dictionary of metric results with
            key = metric field to compare
        Returns:
          True if metric is healthy, false otherwise
        """
        raise NotImplementedError()
