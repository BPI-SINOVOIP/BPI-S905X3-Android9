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


class ProcMisc(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/misc lists miscellaneous drivers registered on the miscellaneous
    major device.
    '''

    t_ignore = ' '

    start = 'drivers'

    p_drivers = repeat_rule('driver')

    def p_line(self, p):
        'driver : NUMBER STRING NEWLINE'
        p[0] = [p[1], p[2]]

    def get_path(self):
        return "/proc/misc"
