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
import os


class SystemLoadMetric(Metric):

    # Fields for response dictionary
    LOAD_AVG_1_MIN = 'load_avg_1_min'
    LOAD_AVG_5_MIN = 'load_avg_5_min'
    LOAD_AVG_15_MIN = 'load_avg_15_min'

    def gather_metric(self):
        """Tells average system load

        Returns:
            A dict with the following fields:
              load_avg_1_min, load_avg_5_min,load_avg_15_min:
                float representing system load averages for the
                past 1, 5, and 15 min

        """
        result = os.getloadavg()
        response = {
            self.LOAD_AVG_1_MIN: result[0],
            self.LOAD_AVG_5_MIN: result[1],
            self.LOAD_AVG_15_MIN: result[2]
        }
        return response
