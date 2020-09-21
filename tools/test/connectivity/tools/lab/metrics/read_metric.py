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
import os

from metrics.metric import Metric


class ReadMetric(Metric):
    """Find read speed of /dev/sda using hdparm

    Attributes:
      NUM_RUNS: number of times hdparm is run
    """
    NUM_RUNS = 3
    COMMAND = 'for i in {1..%s}; do hdparm -Tt /dev/sda; done'
    # Fields for response dictionary
    CACHED_READ_RATE = 'cached_read_rate'
    BUFFERED_READ_RATE = 'buffered_read_rate'

    def is_privileged(self):
        """Checks if this module is being ran as the necessary root user.

        Returns:
            T if being run as root, F if not.
        """

        return os.getuid() == 0

    def gather_metric(self):
        """Finds read speed of /dev/sda

        Takes approx 50 seconds to return, runs hdparm 3 times at
        18 seconds each time to get an average. Should be performed on an
        inactive system, with no other active processes.

        Returns:
            A dict with the following fields:
              cached_read_rate: cached reads in MB/sec
              buffered_read_rate: buffered disk reads in MB/sec
        """
        # Run shell command
        # Example stdout:

        # /dev/sda:
        # Timing cached reads:   18192 MB in  2.00 seconds = 9117.49 MB/sec
        # Timing buffered disk reads: 414 MB in  3.07 seconds = 134.80 MB/sec

        # /dev/sda:
        # Timing cached reads:   18100 MB in  2.00 seconds = 9071.00 MB/sec
        # Timing buffered disk reads: 380 MB in  3.01 seconds = 126.35 MB/sec

        # /dev/sda:
        # Timing cached reads:   18092 MB in  2.00 seconds = 9067.15 MB/sec
        # Timing buffered disk reads: 416 MB in  3.01 seconds = 138.39 MB/sec

        if not self.is_privileged():
            return {self.CACHED_READ_RATE: None, self.BUFFERED_READ_RATE: None}

        result = self._shell.run(self.COMMAND % self.NUM_RUNS).stdout

        cached_reads = 0.0
        buffered_reads = 0.0
        # Calculate averages
        for ln in result.splitlines():
            if ln.startswith(' Timing cached'):
                cached_reads += float(ln.split()[-2])
            elif ln.startswith(' Timing buffered'):
                buffered_reads += float(ln.split()[-2])
        # Create response dictionary
        response = {
            self.CACHED_READ_RATE: cached_reads / self.NUM_RUNS,
            self.BUFFERED_READ_RATE: buffered_reads / self.NUM_RUNS
        }
        return response
