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


class RamMetric(Metric):

    COMMAND = "free -m"
    # Fields for response dictionary
    TOTAL = 'total'
    USED = 'used'
    FREE = 'free'
    BUFFERS = 'buffers'
    CACHED = 'cached'

    def gather_metric(self):
        """Finds RAM statistics in MB

        Returns:
            A dict with the following fields:
                total: int representing total physical RAM available in MB
                used: int representing total RAM used by system in MB
                free: int representing total RAM free for new process in MB
                buffers: total RAM buffered by different applications in MB
                cached: total RAM for caching of data in MB
        """
        # Run shell command
        result = self._shell.run(self.COMMAND)
        # Example stdout:
        #           total       used       free     shared    buffers     cached
        # Mem:      64350      34633      29717        556       1744      24692
        # -/+ buffers/cache:     8196      56153
        # Swap:     65459          0      65459

        # Get only second line
        output = result.stdout.splitlines()[1]
        # Split by space
        fields = output.split()
        # Create response dictionary
        response = {
            self.TOTAL: int(fields[1]),
            self.USED: int(fields[2]),
            self.FREE: int(fields[3]),
            # Skip shared column, since obsolete
            self.BUFFERS: int(fields[5]),
            self.CACHED: int(fields[6]),
        }
        return (response)
