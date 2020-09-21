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

from host_controller.command_processor import base_command_processor


class CommandInfo(base_command_processor.BaseCommandProcessor):
    '''Command processor for info command.

    Attributes:
        arg_parser: ConsoleArgumentParser object, argument parser.
        console: cmd.Cmd console object.
        command: string, command name which this processor will handle.
        command_detail: string, detailed explanation for the command.
    '''

    command = 'info'
    command_detail = 'Show status.'

    def Run(self, arg_line):
        '''Shows the console's session status information.

        Args:
            arg_line: string, line of command arguments
        '''
        print('device image: %s' % self.console.device_image_info)
        print('test suite: %s' % self.console.test_suite_info)
        print('test result: %s' % self.console.test_results)
        print('fetch info: %s' % self.console.fetch_info)
