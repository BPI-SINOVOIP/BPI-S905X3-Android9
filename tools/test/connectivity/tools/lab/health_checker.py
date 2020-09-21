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

import logging

from health.constant_health_analyzer import HealthyIfGreaterThanConstantNumber
from health.constant_health_analyzer import HealthyIfLessThanConstantNumber
from health.constant_health_analyzer import HealthyIfEquals
from health.constant_health_analyzer import HealthyIfStartsWith
from health.custom_health_analyzer import HealthyIfNotIpAddress
from health.constant_health_analyzer_wrapper import HealthyIfValsEqual


class HealthChecker(object):
    """Takes a dictionary of metrics and returns whether each is healthy
    Attributes:
        _analyzers: a list of metric, analyzer tuples where metric is a string
          representing a metric name ('DiskMetric') and analyzer is a
          constant_health_analyzer object
        config:
        a dict formatted as follows:
        {
            metric_name: {
                field_to_compare: {
                    constant : a constant to compare to
                    compare : a string specifying a way to compare
                }
            }
        }


    """

    COMPARER_CONSTRUCTOR = {
        'GREATER_THAN': lambda k, c: HealthyIfGreaterThanConstantNumber(k, c),
        'LESS_THAN': lambda k, c: HealthyIfLessThanConstantNumber(k, c),
        'EQUALS': lambda k, c: HealthyIfEquals(k, c),
        'IP_ADDR': lambda k, c: HealthyIfNotIpAddress(k),
        'EQUALS_DICT': lambda k, c: HealthyIfValsEqual(k, c),
        'STARTS_WITH': lambda k, c: HealthyIfStartsWith(k, c)
    }

    def __init__(self, config):
        self._config = config
        self._analyzers = []
        # Create comparators from config object
        for metric_name, metric_configs in self._config.items():
            # creates a constant_health_analyzer object for each field
            for field_name in metric_configs:
                compare_type = metric_configs[field_name]['compare']
                constant = metric_configs[field_name]['constant']
                comparer = self.COMPARER_CONSTRUCTOR[compare_type](field_name,
                                                                   constant)
                self._analyzers.append((metric_name, comparer))

    def get_unhealthy(self, response_dict):
        """Calls comparators to check if metrics are healthy

        Attributes:
            response_dict: a dict mapping metric names (as strings) to the
              responses returned from gather_metric()

        Returns:
            a list of keys, where keys are strings representing
            the name of a metric (ex: 'DiskMetric')
        """
        # loop through and check if healthy
        unhealthy_metrics = []
        for (metric, analyzer) in self._analyzers:
            try:
                # if not healthy, add to list so value can be reported
                if not analyzer.is_healthy(response_dict[metric]):
                    unhealthy_metrics.append(metric)
            # don't exit whole program if error in config file, just report
            except KeyError as e:
                logging.warning(
                    'Error in config file, "%s" not a health metric\n' % e)

        return unhealthy_metrics
