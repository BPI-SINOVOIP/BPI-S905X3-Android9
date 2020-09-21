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
import signal


class TimeLimit:
    """A class that limits the execution time of functions.

    Attributes:
        seconds: Number of seconds to execute task.
        error_message: What error message provides upon raising exception.

    Raises:
        job.TimeoutError: When raised, gets handled in class, and will not
        affect execution.
    """
    DEF_ERR_MSG = 'Time limit has been reached.'

    def __init__(self, seconds=5, error_message=DEF_ERR_MSG):
        self.seconds = seconds
        self.error_message = error_message
        self.timeout = False

    def handle_timeout(self, signum, frame):
        self.timeout = True
        raise TimeLimitError

    def __enter__(self):
        signal.signal(signal.SIGALRM, self.handle_timeout)
        signal.alarm(self.seconds)

    def __exit__(self, type, value, traceback):
        return self.timeout


class TimeLimitError(Exception):
    pass
