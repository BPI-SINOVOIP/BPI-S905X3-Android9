#!/usr/bin/env python
#
# Copyright 2017 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Driver test library."""

import mock
import unittest


class BaseDriverTest(unittest.TestCase):
    """Base class for driver tests."""

    def setUp(self):
        """Set up test."""
        self._patchers = []

    def tearDown(self):
        """Tear down test."""
        for patcher in reversed(self._patchers):
            patcher.stop()

    def Patch(self, *args, **kwargs):
        """A wrapper for mock.patch.object.

        This wrapper starts a patcher and store it in self._patchers,
        so that we can later stop them in tearDown.

        Args:
          *args: Arguments to pass to mock.patch.
          **kwargs: Keyword arguments to pass to mock.patch.

        Returns:
          Mock object
        """
        patcher = mock.patch.object(*args, **kwargs)
        self._patchers.append(patcher)
        return patcher.start()
