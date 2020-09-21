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


class DiskMetric(Metric):
    # This command calls df /var/tmp and ignores line 1.
    COMMAND = "df /var/tmp | tail -n +2"
    # Fields for response dictionary
    TOTAL = 'total'
    USED = 'used'
    AVAIL = 'avail'
    PERCENT_USED = 'percent_used'

    def gather_metric(self):
        """Finds disk space metrics for /var/tmp

        Returns:
            A dict with the following fields:
                total: int representing total space in 1k blocks
                used: int representing total used in 1k blocks
                avail: int representing total available in 1k blocks
                percent_used: int representing percentage space used (0-100)
        """
        # Run shell command
        result = self._shell.run(self.COMMAND)

        # Example stdout:
        # /dev/sda1 57542652 18358676  36237928  34% /
        fields = result.stdout.split()
        # Create response dictionary
        response = {
            self.TOTAL: int(fields[1]),
            self.USED: int(fields[2]),
            self.AVAIL: int(fields[3]),
            # Strip the percentage symbol
            self.PERCENT_USED: int(fields[4][:-1])
        }
        return response
