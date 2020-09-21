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

from health import constant_health_analyzer


class ConstantHealthAnalyzerForDict(
        constant_health_analyzer.ConstantHealthAnalyzer):
    """Extends ConstantHealthAnalyzer to check for every key in a dictionary

    Attributes:
        key: a string representing a key to a response dictionary, which is
          also a dictionary
        _constant: constant value to compare every value in metric_results[key] to
        analyzer: a ConstantHealthAnalyzer class
    """

    def is_healthy(self, metric_results):
        """Returns if analyzer().is_healthy() returned true for all values in dict

        Args:
          metric_results: a dictionary containing a dictionary of metric_results
        """
        for key in metric_results[self.key]:
            if not self.analyzer(
                    key, self._constant).is_healthy(metric_results[self.key]):
                return False
        return True


class HealthyIfValsEqual(ConstantHealthAnalyzerForDict):

    analyzer = constant_health_analyzer.HealthyIfEquals
