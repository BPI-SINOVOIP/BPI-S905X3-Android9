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

from vts.utils.python.common import cmd_utils


class CustomFlasherError(Exception):
    """Raised when there is an error in operations."""


class CustomFlasherProxy():
    """Proxy class for custom flasher tool.

    Attributes:
        serial: string containing devices serial.
        customflasher_str: string containing path to the custom flasher.

    For syntactic reasons, the '-' in  commands need to be replaced
    with '_'. Can directly execute  commands on an object:
    >> fb = Proxy(<serial>)
    >> fb.devices() # will return the console output of "devices".
    """

    def __init__(self, serial=None):
        """Initializes custom flasher proxy."""
        self.serial = serial
        self.customflasher_str = None

    def SetCustomBinaryPath(self, customflasher_path=""):
        """Sets path to flasher binary.

        Args:
            customflasher_path: string, path to custom flasher binary.
        """
        self.customflasher_str = customflasher_path

    def ExecCustomFlasherCmd(self, name, arg_str):
        """Passes joined shell command line to ExecuteOneShellCommand().

        Args:
            name: stirng, command to pass to custom flasher binary.
            arg_str: string, contains additional argument(s).

        Returns:
            contents of stdout if command line returned without error;
            stderr otherwise.
        """
        out, err, error_code = cmd_utils.ExecuteOneShellCommand(
            ' '.join((self.customflasher_str, name, arg_str)))
        if error_code:
            return err
        return out

    def __getattr__(self, name):
        """Passes invoked function's name to ExecCustomFlasherCmd().

        Args:
            name: stirng, 'name' of invoked method.

        Returns:
            output string from ExecCustomFlasherCmd().
        """

        def customflasher_call(*args):
            """Joins *args into one string and passes the name and joined args
            to ExecCustomFlasherCmd().

            Replaces '_' characters in name, so if the invoked method name
            was "__command", this function could pass "--command"
            to custom flasher as a command.

            Returns:
                output string from ExecCustomFlasherCmd().
            """
            clean_name = name.replace('_', '-')
            arg_str = ' '.join(str(elem) for elem in args)
            return self.ExecCustomFlasherCmd(clean_name, arg_str)

        return customflasher_call
