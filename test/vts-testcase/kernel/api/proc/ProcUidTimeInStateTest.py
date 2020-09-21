#
# Copyright (C) 2018 The Android Open Source Project
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


class ProcUidTimeInStateTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/uid_time_in_state provides the time each UID's processes spend
    executing at each available frequency.

    This is an Android specific file.
    '''

    start = 'uid_time_table'

    t_UID = literal_token(r'uid')

    p_uid_times = repeat_rule('uid_time')
    p_numbers = repeat_rule('NUMBER')

    t_ignore = ' '

    def p_uid_time_table(self, p):
        'uid_time_table : freqs uid_times'
        p[0] = p[1:]

    def p_freqs(self, p):
        'freqs : UID COLON NUMBERs NEWLINE'
        p[0] = p[3]

    def p_uid_time(self, p):
        'uid_time : NUMBER COLON NUMBERs NEWLINE'
        p[0] = [p[1], p[3]]

    def get_path(self):
        return "/proc/uid_time_in_state"

    def file_optional(self):
        # This file is optional until implemented in Android common kernel
        return True

    def result_correct(self, result):
        freqs, times = result
        return all(len(time[1]) == len(freqs) for time in times)
