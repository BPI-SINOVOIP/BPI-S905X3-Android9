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

from host_controller.command_processor import base_command_processor


class CommandRetry(base_command_processor.BaseCommandProcessor):

    command = "retry"
    command_detail = "Retry last run test plan for certain times."

    # @Override
    def SetUp(self):
        """Initializes the parser for retry command."""
        self.arg_parser.add_argument(
            "--count",
            type=int,
            default=30,
            help="Retry count. Default retry count is 30.")

    # @Override
    def Run(self, arg_line):
        """Retry last run plan for certain times."""
        args = self.arg_parser.ParseLine(arg_line)
        retry_count = args.count

        if "vts" not in self.console.test_suite_info:
            print("test_suite_info doesn't have 'vts': %s" %
                  self.console.test_suite_info)
            return False

        tools_path = os.path.dirname(self.console.test_suite_info["vts"])
        vts_root_path = os.path.dirname(tools_path)
        results_path = os.path.join(vts_root_path, "results")

        former_result_count = len([
            dir for dir in os.listdir(results_path)
            if os.path.isdir(os.path.join(results_path, dir))
            and not os.path.islink(os.path.join(results_path, dir))
        ])

        if former_result_count < 1:
            print("No test plan has been run yet, former results count is %d" %
                  former_result_count)
            return False

        for result_index in range(retry_count):
            session_id = former_result_count - 1 + result_index
            retry_test_command = "test --keep-result -- %s --retry %d" % (
                self.console.test_result["suite_plan"], session_id)
            self.console.onecmd(retry_test_command)
