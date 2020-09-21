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


class TimeMetric(Metric):

    COMMAND = "date"
    # Fields for response dictionary
    DATE_TIME = 'date_time'

    def gather_metric(self):
        """Returns the system date and time

        Returns:
            A dict with the following fields:
              date_time: String stystem date and time

        """
        # Run shell command
        result = self._shell.run(self.COMMAND).stdout
        # Example stdout:
        # Wed Jul 19 16:53:15 PDT 2017

        response = {
            self.DATE_TIME: result,
        }
        return response
