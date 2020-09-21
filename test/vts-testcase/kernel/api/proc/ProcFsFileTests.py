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

import logging
from vts.runners.host import const
from vts.testcases.kernel.api.proc import KernelProcFileTestBase
from vts.testcases.kernel.api.proc.KernelProcFileTestBase import repeat_rule, literal_token


class ProcDiskstatsTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/diskstats displays I/O statistics of block devices.'''

    t_ignore = ' '
    start = 'lines'
    p_lines = repeat_rule('line')

    def p_line(self, p):
        '''line : NUMBER NUMBER STRING NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NEWLINE'''
        p[0] = p[1:]

    def get_path(self):
        return "/proc/diskstats"

class ProcMountsTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/self/mounts lists the mounted filesystems.

    /proc/mounts must symlink to this file.'''

    def parse_contents(self, contents):
        if len(contents) == 0 or contents[-1] != '\n':
            raise SyntaxError('Missing final newline')
        result = []
        for line in contents.split('\n')[:-1]:
            parsed = line.split(' ')
            parsed[3] = parsed[3].split(',')
            result.append(parsed)
        return result

    def result_correct(self, parse_results):
        for line in parse_results:
            if len(line[3]) < 1 or line[3][0] not in {'rw', 'ro'}:
                logging.error("First attribute must be rw or ro")
                return False
            if line[4] != '0' or line[5] != '0':
                logging.error("Last 2 columns must be 0")
                return False
        return True

    def prepare_test(self, shell, dut):
        # Follow the symlink
        results = shell.Execute('readlink /proc/mounts')
        if results[const.EXIT_CODE][0] != 0:
            return False
        return results[const.STDOUT][0] == 'self/mounts\n'

    def get_path(self):
        return "/proc/self/mounts"

class ProcFilesystemsTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/filesystems lists filesystems currently supported by kernel.'''

    def parse_contents(self, contents):
        if len(contents) == 0 or contents[-1] != '\n':
            raise SyntaxError('Missing final newline')

        result = []
        for line in contents.split('\n')[:-1]:
            parsed = line.split('\t')
            num_columns = len(parsed)
            if num_columns != 2:
                raise SyntaxError('Wrong number of columns.')
            result.append(parsed)
        return result

    def get_path(self):
        return "/proc/filesystems"

class ProcSwapsTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/swaps measures swap space utilization.'''

    REQUIRED_COLUMNS = {
        'Filename',
        'Type',
        'Size',
        'Used',
        'Priority'
    }

    def parse_contents(self, contents):
        if len(contents) == 0 or contents[-1] != '\n':
            raise SyntaxError('Missing final newline')
        return map(lambda x: x.split(), contents.split('\n')[:-1])

    def result_correct(self, result):
        return self.REQUIRED_COLUMNS.issubset(result[0])

    def get_path(self):
        return "/proc/swaps"

    def file_optional(self):
        # It is not mandatory to have this file present
        return True
