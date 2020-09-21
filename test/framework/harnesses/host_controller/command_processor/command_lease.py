#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from host_controller.command_processor import base_command_processor
from host_controller.console_argument_parser import ConsoleArgumentError


class CommandLease(base_command_processor.BaseCommandProcessor):
    """Command processor for lease command.

    Attributes:
        arg_parser: ConsoleArgumentParser object, argument parser.
        console: cmd.Cmd console object.
        command: string, command name which this processor will handle.
        command_detail: string, detailed explanation for the command.
    """

    command = "lease"
    command_detail = "Make a host lease command tasks from TFC."

    def _PrintTasks(self, tasks):
        """Shows a list of command tasks.

        Args:
            devices: A list of DeviceInfo objects.
        """
        attr_names = ("request_id", "command_id", "task_id", "device_serials",
                      "command_line")
        self.console._PrintObjects(tasks, attr_names)

    # @Override
    def SetUp(self):
        """Initializes the parser for lease command."""
        self.arg_parser.add_argument(
            "--host", type=int, help="The index of the host.")

    # @Override
    def Run(self, arg_line):
        """Makes a host lease command tasks from TFC."""
        args = self.arg_parser.ParseLine(arg_line)
        if args.host is None:
            if len(self.console._hosts) > 1:
                raise ConsoleArgumentError("More than one hosts.")
            args.host = 0
        tasks = self.console._hosts[args.host].LeaseCommandTasks()
        self._PrintTasks(tasks)