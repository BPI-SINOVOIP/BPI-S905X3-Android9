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

from host_controller import console_argument_parser


class BaseCommandProcessor(object):
    '''Base class for command processors.

    Attributes:
        arg_parser: ConsoleArgumentParser object, argument parser.
        console: cmd.Cmd console object.
        command: string, command name which this processor will handle.
        command_detail: string, detailed explanation for the command.
    '''

    command = 'base'
    command_detail = 'Command processor template'

    def _SetUp(self, console):
        '''Internal SetUp function that will call subclass' Setup function.

        Args:
            console: Console object.
        '''
        self.console = console
        self.arg_parser = console_argument_parser.ConsoleArgumentParser(
            self.command, self.command_detail)
        self.SetUp()

    def SetUp(self):
        '''SetUp method for subclass to override.'''
        pass

    def _Run(self, arg_line):
        '''Internal function that will call subclass' Run function.

        Args:
            arg_line: string, line of command arguments
        '''
        ret = self.Run(arg_line)

        if ret is not None:
            if ret == True:  # exit command executed.
                return True
            elif ret == False:
                return False
            else:
                logging.warning("{} coommand returned {}".format(
                    self.command, ret))

    def Run(self, arg_line):
        '''Run method to perform action when invoked from console.

        Args:
            arg_line: string, line of command
        '''
        pass

    def _Help(self):
        '''Internal function that will call subclass' Help function.'''
        self.Help()

    def Help(self):
        '''Help method to print help informations.'''
        if hasattr(self, 'arg_parser') and hasattr(self.console, '_out_file'):
            self.arg_parser.print_help(self.console._out_file)

    def _TearDown(self):
        '''Internal function that will call subclass' TearDown function.'''
        self.TearDown()

    def TearDown(self):
        '''TearDown tasks to be called when console is shutting down.'''
        pass
