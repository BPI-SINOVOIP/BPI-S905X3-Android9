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
import re

# Handles edge case of acronyms then another word, eg. CPUMetric -> CPU_Metric
# or lower camel case, cpuMetric -> cpu_Metric.
first_cap_re = re.compile('(.)([A-Z][a-z]+)')
# Handles CpuMetric -> Cpu_Metric
all_cap_re = re.compile('([a-z0-9])([A-Z])')


class Runner:
    """Calls metrics and passes response to reporters.

    Attributes:
        metric_list: a list of metric objects
        reporter_list: a list of reporter objects
        object and value is dictionary returned by that response
    """

    def __init__(self, metric_list, reporter_list):
        self.metric_list = metric_list
        self.reporter_list = reporter_list

    def run(self):
        """Calls metrics and passes response to reporters."""
        raise NotImplementedError()


class InstantRunner(Runner):
    def convert_to_snake(self, name):
        """Converts a CamelCaseName to snake_case_name

        Args:
            name: The string you want to convert.
        Returns:
            snake_case_format of name.
        """
        temp_str = first_cap_re.sub(r'\1_\2', name)
        return all_cap_re.sub(r'\1_\2', temp_str).lower()

    def run(self):
        """Calls all metrics, passes responses to reporters."""
        responses = {}
        for metric in self.metric_list:
            # [:-7] removes the ending '_metric'.
            key_name = self.convert_to_snake(metric.__class__.__name__)[:-7]
            responses[key_name] = metric.gather_metric()
        for reporter in self.reporter_list:
            reporter.report(responses)
