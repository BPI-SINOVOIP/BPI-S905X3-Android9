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

import datetime
import os
import re

from host_controller.build import build_provider_gcs
from host_controller.command_processor import base_command_processor

from vts.utils.python.common import cmd_utils


class CommandUpload(base_command_processor.BaseCommandProcessor):
    """Command processor for upload command.

    Attributes:
        arg_parser: ConsoleArgumentParser object, argument parser.
        console: cmd.Cmd console object.
        command: string, command name which this processor will handle.
        command_detail: string, detailed explanation for the command.
    """

    command = "upload"
    command_detail = "Upload <src> file to <dest> Google Cloud Storage. In <src> and <dest>, variables enclosed in {} are replaced with the values stored in the console."

    def FormatString(self, format_string):
        """Replaces variables with the values in the console's dictionaries.

        Args:
            format_string: The string containing variables enclosed in {}.

        Returns:
            The formatted string.

        Raises:
            KeyError if a variable is not found in the dictionaries or the
            value is empty.
        """

        def ReplaceVariable(match):
            name = match.group(1)
            if name in ("build_id", "branch", "target"):
                value = self.console.fetch_info[name]
            elif name in ("result_full", "result_zip", "suite_plan"):
                value = self.console.test_result[name]
            elif name in ("timestamp"):
                value = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
            else:
                value = None

            if not value:
                raise KeyError(name)

            return value

        return re.sub("{([^}]+)}", ReplaceVariable, format_string)

    # @Override
    def SetUp(self):
        """Initializes the parser for upload command."""
        self.arg_parser.add_argument(
            "--src",
            required=True,
            default="latest-system.img",
            help="Path to a source file to upload. Only single file can be "
            "uploaded per once. Use 'latest- prefix to upload the latest "
            "fetch images. e.g. --src=latest-system.img  If argument "
            "value is not given, the recently fetched system.img will be "
            "uploaded.")
        self.arg_parser.add_argument(
            "--dest",
            required=True,
            help="Google Cloud Storage URL to which the file is uploaded.")

    # @Override
    def Run(self, arg_line):
        """Upload args.src file to args.dest Google Cloud Storage."""
        args = self.arg_parser.ParseLine(arg_line)

        gsutil_path = build_provider_gcs.BuildProviderGCS.GetGsutilPath()
        if not gsutil_path:
            print("Please check gsutil is installed and on your PATH")
            return False

        if args.src.startswith("latest-"):
            src_name = args.src[7:]
            if src_name in self.console.device_image_info:
                src_path = self.console.device_image_info[src_name]
            else:
                print(
                    "Unable to find {} in device_image_info".format(src_name))
                return False
        else:
            try:
                src_paths = self.FormatString(args.src)
            except KeyError as e:
                print("Unknown or uninitialized variable in src: %s" % e)
                return False

        src_path_list = src_paths.split(" ")
        for path in src_path_list:
            src_path = path.strip()
            if not os.path.isfile(src_path):
                print("Cannot find a file: {}".format(src_path))
                return False

        try:
            dest_path = self.FormatString(args.dest)
        except KeyError as e:
            print("Unknown or uninitialized variable in dest: %s" % e)
            return False

        if not dest_path.startswith("gs://"):
            print("{} is not correct GCS url.".format(dest_path))
            return False
        """ TODO(jongmok) : Before upload, login status, authorization,
                            and dest check are required. """
        copy_command = "{} cp {} {}".format(gsutil_path, src_paths, dest_path)
        _, stderr, err_code = cmd_utils.ExecuteOneShellCommand(copy_command)

        if err_code:
            print stderr
