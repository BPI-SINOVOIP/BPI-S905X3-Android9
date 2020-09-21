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


class NumUsersMetric(Metric):

    COMMAND = 'users | wc -w'
    # Fields for response dictionary
    NUM_USERS = 'num_users'

    def gather_metric(self):
        """Returns total (nonunique) users currently logged in to current host

        Returns:
            A dict with the following fields:
              num_users : an int representing the number of users

        """
        # Example stdout:
        # 2
        result = self._shell.run(self.COMMAND).stdout
        response = {self.NUM_USERS: int(result)}
        return response
