#!/usr/bin/env python3.4
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


class OtaTool(object):
    """A Wrapper for an OTA Update command or tool.

    Each OtaTool acts as a facade to the underlying command or tool used to
    update the device.
    """

    def __init__(self, command):
        """Creates an OTA Update tool with the given properties.

        Args:
            command: A string that is used as the command line tool
        """
        self.command = command

    def update(self, ota_runner):
        """Begins the OTA Update. Returns after the update has installed.

        Args:
            ota_runner: The OTA Runner that handles the device information.
        """
        raise NotImplementedError()

    def cleanup(self, ota_runner):
        """A cleanup method for the OTA Tool to run after the update completes.

        Args:
            ota_runner: The OTA Runner that handles the device information.
        """
        pass
