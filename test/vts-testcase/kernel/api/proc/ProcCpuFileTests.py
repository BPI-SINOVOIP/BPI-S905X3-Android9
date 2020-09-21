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

from vts.testcases.kernel.api.proc import KernelProcFileTestBase
from vts.testcases.kernel.api.proc.KernelProcFileTestBase import repeat_rule, literal_token


class ProcCpuInfoTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/cpuinfo displays a collection of cpu and architecture specific items.

    The file consists of multiple lines of 'identifier : values' where
    'identifier' is a space separated name that can end in some tabs, and
    'values' are space separated items that can also in in a space.
    Extra newlines are allowed between normal lines.
    '''

    EXPECTED_FIELDS = [
        ['processor', ['\t']],
    ]

    start = 'lines'

    # Any character except for ':' and whitespace is allowed
    t_STRING = r'[^:^ ^\t^\n]+'

    # Numbers and literals are tokenized as strings instead
    t_NUMBER = r'x'
    t_HEX_LITERAL = r'x'
    t_FLOAT =  r'x'

    p_lines = repeat_rule('line')
    p_string_spaces = repeat_rule('string_space', zero_ok=True)
    p_space_items = repeat_rule('space_item', zero_ok=True)
    p_TABs = repeat_rule('TAB', zero_ok=True)

    def p_line(self, p):
        '''line : string_spaces STRING TABs COLON space_items SPACE NEWLINE
                | string_spaces STRING TABs COLON space_items NEWLINE
                | NEWLINE'''
        if len(p) == 2:
            p[0] = []
            return
        p[0] = [p[1] + [p[2], p[3]], p[5], p[6]]

    def p_space_item(self, p):
        'space_item : SPACE STRING'
        p[0] = p[2]

    def p_string_space(self, p):
        'string_space : STRING SPACE'
        p[0] = p[1]

    def result_correct(self, parse_result):
        expected_fields = self.EXPECTED_FIELDS[:]
        for line in parse_result:
            if len(line) > 0:
                if line[0] in expected_fields:
                    expected_fields.remove(line[0])
        return len(expected_fields) == 0

    def get_path(self):
        return "/proc/cpuinfo"


class ProcLoadavgTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/loadavg displays CPU and IO load average over time.'''

    def parse_contents(self, contents):
        return self.parse_line("{:f} {:f} {:f} {:d}/{:d} {:d}\n", contents)

    def get_path(self):
        return "/proc/loadavg"
