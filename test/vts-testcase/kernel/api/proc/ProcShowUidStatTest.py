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


class ProcShowUidStatTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/uid_cputime/show_uid_stat provides the time a UID's processes spend
    in user and kernel space.

    This is an Android specific file.
    '''

    start = 'lines'
    p_lines = KernelProcFileTestBase.repeat_rule('line')

    def p_line(self, p):
        '''line : NUMBER COLON SPACE NUMBER SPACE NUMBER SPACE NUMBER NEWLINE
                | NUMBER COLON SPACE NUMBER SPACE NUMBER NEWLINE'''
        if len(p) == 10:
            p[0] = [p[1], p[4], p[6], p[8]]
        else:
            p[0] = [p[1], p[4], p[6]]

    def get_path(self):
        return "/proc/uid_cputime/show_uid_stat"
