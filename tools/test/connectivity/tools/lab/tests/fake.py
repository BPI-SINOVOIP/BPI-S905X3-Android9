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


class FakeResult(object):
    """A fake version of the object returned from ShellCommand.run. """

    def __init__(self, exit_status=0, stdout='', stderr=''):
        self.exit_status = exit_status
        self.stdout = stdout
        self.stderr = stderr


class MockShellCommand(object):
    """A fake ShellCommand object.

    Attributes:
        fake_result: a FakeResult object, or a list of FakeResult objects
        fake_pids: a dictionary that maps string identifier to list of pids
    """

    def __init__(self, fake_result=None, fake_pids=[]):
        self._fake_result = fake_result
        self._fake_pids = fake_pids
        self._counter = 0

    def run(self, command, timeout=3600, ignore_status=False):
        """Returns a FakeResult object.

        Args:
            Same as ShellCommand.run, but none are used in function

        Returns:
            The FakeResult object it was initalized with. If it was initialized
            with a list, returns the next element in list and increments counter

        Raises:
          IndexError: Function was called more times than num elements in list

        """
        if isinstance(self._fake_result, list):
            self._counter += 1
            return self._fake_result[self._counter - 1]
        else:
            return self._fake_result

    def get_command_pids(self, identifier):
        """Returns a generator of fake pids

        Args:
          Same as ShellCommand.get_pids, but none are used in the function
        Returns:
          A generator of the fake pids it was initialized with
        """

        for pid in self._fake_pids[identifier]:
            yield pid
