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


class UptimeMetric(Metric):
    SECONDS_COMMAND = 'cat /proc/uptime | cut -f1 -d\' \''
    READABLE_COMMAND = 'uptime -p | cut -f2- -d\' \''
    # Fields for response dictionary
    TIME_SECONDS = 'time_seconds'
    TIME_READABLE = 'time_readable'

    def get_readable(self):
        # Example stdout:
        # 2 days, 8 hours, 19 minutes
        return self._shell.run(self.READABLE_COMMAND).stdout

    def get_seconds(self):
        # Example stdout:
        # 358350.70
        return self._shell.run(self.SECONDS_COMMAND).stdout

    def gather_metric(self):
        """Tells how long system has been running

        Returns:
            A dict with the following fields:
              time_seconds: uptime in total seconds
              time_readable: time in human readable format

        """

        response = {
            self.TIME_SECONDS: self.get_seconds(),
            self.TIME_READABLE: self.get_readable()
        }
        return response
