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

from utils import job
from utils import shell


class Metric(object):
    """Interface class for metric gathering.

    Attributes:
        _shell: a shell.ShellCommand object
    """

    def __init__(self, shell=shell.ShellCommand(job)):
        self._shell = shell

    def gather_metric(self):
        """Gathers all values that this metric watches.

        Mandatory for every class that extends Metric. Should always return.

        Returns:
          A dict mapping keys (class level constant strings representing
          fields of a metric) to their statistics.

        Raises:
          NotImplementedError: A metric did not implement this function.
        """
        raise NotImplementedError()
