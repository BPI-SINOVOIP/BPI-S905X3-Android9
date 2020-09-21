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

import os
import shutil

from host_controller.command_processor import base_command_processor


class CommandCopy(base_command_processor.BaseCommandProcessor):
    """Command processor for copy command.

    Attributes:
        arg_parser: ConsoleArgumentParser object, argument parser.
        console: cmd.Cmd console object.
        command: string, command name which this processor will handle.
        command_detail: string, detailed explanation for the command.
    """

    command = "copy"
    command_detail = "Copy a file."

    # @Override
    def Run(self, arg_line):
        """Copy a file from source to destination path."""
        src, dst = arg_line.split()
        if dst == "{vts_tf_home}":
            dst = os.path.dirname(self.console.test_suite_info["vts"])
        elif "{" in dst:
            print("unknown dst %s" % dst)
            return
        shutil.copy(src, dst)