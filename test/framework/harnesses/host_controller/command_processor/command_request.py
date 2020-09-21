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
from host_controller.tfc import request


class CommandRequest(base_command_processor.BaseCommandProcessor):
    """Command processor for request command.

    Attributes:
        arg_parser: ConsoleArgumentParser object, argument parser.
        console: cmd.Cmd console object.
        command: string, command name which this processor will handle.
        command_detail: string, detailed explanation for the command.
    """

    command = "request"
    command_detail = "Send TFC a request to execute a command."

    # @Override
    def SetUp(self):
        """Initializes the parser for request command."""
        self.arg_parser.add_argument(
            "--cluster",
            required=True,
            help="The cluster to which the request is submitted.")
        self.arg_parser.add_argument(
            "--run-target",
            required=True,
            help="The target device to run the command.")
        self.arg_parser.add_argument(
            "--user",
            required=True,
            help="The name of the user submitting the request.")
        self.arg_parser.add_argument(
            "command",
            metavar="COMMAND",
            nargs="+",
            help='The command to be executed. If the command contains '
            'arguments starting with "-", place the command after '
            '"--" at end of line.')

    # @Override
    def Run(self, arg_line):
        """Sends TFC a request to execute a command."""
        args = self.arg_parser.ParseLine(arg_line)
        req = request.Request(
            cluster=args.cluster,
            command_line=" ".join(args.command),
            run_target=args.run_target,
            user=args.user)
        self.console._tfc_client.NewRequest(req)
