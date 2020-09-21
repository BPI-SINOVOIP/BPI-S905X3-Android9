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


class Reporter(object):
    """Base class for the multiple ways to report the data gathered.

    The method report takes in a dictionary where the key is the class that
    generated the value, and the value is the actual data gathered from
    collecting that metric. For example, an UptimeMetric, would have
    UptimeMetric() as key, and '1-02:22:42' as the value.

    Attributes:
      health_checker: a HealthChecker object
    """

    def __init__(self, health_checker):
        self.health_checker = health_checker

    def report(self, responses):
        raise NotImplementedError('Must implement this method')
