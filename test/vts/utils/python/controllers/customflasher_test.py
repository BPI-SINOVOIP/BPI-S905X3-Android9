#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import unittest

from vts.utils.python.controllers import customflasher


class CustomFlasherTest(unittest.TestCase):
    """Tests for CustomFlasher."""

    def testExecCmd(self):
        """Test for __getattr__().

            Tests if CustomFlasherProxy gets binary path and commands
            to the binary and passes to cmd_util
            as one joined shell command line.
        """
        flashTool = customflasher.CustomFlasherProxy()
        flashTool.SetCustomBinaryPath("myBinaryPath")
        flashTool.__arg("aboutSomething")
        flashTool.assert_called_with("myBinaryPath --arg", "aboutSomething")


if __name__ == "__main__":
    unittest.main()
