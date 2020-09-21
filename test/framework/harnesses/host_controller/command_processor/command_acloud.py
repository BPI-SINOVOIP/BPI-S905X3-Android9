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

import logging

from host_controller.acloud import acloud_client
from host_controller.command_processor import base_command_processor


class CommandAcloud(base_command_processor.BaseCommandProcessor):
    '''Command processor for acloud command.

    Attributes:
        arg_parser: ConsoleArgumentParser object, argument parser.
        console: cmd.Cmd console object.
        command: string, command name which this processor will handle.
        command_detail: string, detailed explanation for the command.
    '''

    command = 'acloud'
    command_detail = 'Create acloud instances.'

    def Run(self, arg_line):
        '''Creates an acloud instance and connects to it via adb.

        Args:
            arg_line: string, line of command arguments
        '''
        args = self.arg_parser.ParseLine(arg_line)

        if args.provider == "ab":
            if args.build_id.lower() == "latest":
                build_id = self.console._build_provider["ab"].GetLatestBuildId(
                    args.branch,
                    args.target)
        else:
            # TODO(yuexima): support more provider types.
            logging.error("Provider %s not supported yet." % args.provider)
            return

        ac = acloud_client.ACloudClient()
        ac.PrepareConfig(args.config_path)
        ac.CreateInstance(args.build_id)
        ac.ConnectInstanceToAdb(ah.GetInstanceIP())

    def SetUp(self):
        """Initializes the parser for acloud command."""
        self.arg_parser.add_argument(
            "--build_id",
            help="Build ID to use.")
        self.arg_parser.add_argument(
            "--provider",
            default="ab",
            choices=("local_fs", "gcs", "pab", "ab"),
            help="Build provider type")
        self.arg_parser.add_argument(
            "--branch",  # not required for local_fs
            help="Branch to grab the artifact from.")
        self.arg_parser.add_argument(
            "--target",  # not required for local_fs
            help="Target product to grab the artifact from.")
        self.arg_parser.add_argument(
            "--config_path",
            required=True,
            help="Acloud config path.")
