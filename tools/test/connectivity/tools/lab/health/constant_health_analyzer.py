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


class ConstantHealthAnalyzer(health_analyzer.HealthAnalyzer):
    """Extends HealthAnalyzer for all HealthAnalyzers that compare to a constant

    Attributes:
        key: a string representing a key to a response dictionary
        _constant: a constant value to compare metric_results[key] to
    """

    def __init__(self, key, constant):
        self.key = key
        self._constant = constant

    def __eq__(self, other):
        """ Overwrite comparator so tests can check for correct instance

        Returns True if two of same child class instances were intialized
          with the same key and constant
        """
        return self.key == other.key and self._constant == other._constant\
            and self.__class__.__name__ == other.__class__.__name__


class HealthyIfGreaterThanConstantNumber(ConstantHealthAnalyzer):
    def is_healthy(self, metric_results):
        """Returns whether numeric result is greater than numeric constant
        Args:
          metric_results: a dictionary of metric results.

        Returns:
          True if numeric result is greater than numeric constant
        """

        return metric_results[self.key] > self._constant


class HealthyIfLessThanConstantNumber(ConstantHealthAnalyzer):
    def is_healthy(self, metric_results):
        """Returns whether numeric result is less than numeric constant
        Args:
          metric_results: a dictionary of metric results.

        Returns:
          True if numeric result is less than numeric constant
        """

        return metric_results[self.key] < self._constant


class HealthyIfEquals(ConstantHealthAnalyzer):
    def is_healthy(self, metric_results):
        """Returns whether result is equal to constant
        Args:
          metric_results: a dictionary of metric results.

        Returns:
          True if result is equal to constant
        """
        return metric_results[self.key] == self._constant


class HealthyIfStartsWith(ConstantHealthAnalyzer):
    def is_healthy(self, metric_results):
        """Returns whether result starts with a constant

        Args:
          metric_results: a dictionary of metric results.

        Returns:
          True if result starts with a constant
        """
        return metric_results[self.key].startswith(str(self._constant))
