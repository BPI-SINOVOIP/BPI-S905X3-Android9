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

from metrics.metric import Metric
import psutil


class CpuMetric(Metric):
    # Fields for response dictionary
    USAGE_PER_CORE = 'usage_per_core'

    def gather_metric(self):
        """Finds CPU usage in percentage per core

        Blocks processes for 0.1 seconds for an accurate CPU usage percentage

        Returns:
            A dict with the following fields:
                usage_per_core: a list of floats corresponding to CPU usage
                per core
        """
        # Create response dictionary
        response = {
            self.USAGE_PER_CORE: psutil.cpu_percent(interval=0.1, percpu=True)
        }
        return response
