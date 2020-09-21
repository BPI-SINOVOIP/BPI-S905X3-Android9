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

import struct
import logging
from vts.runners.host import asserts
from vts.testcases.kernel.api.proc import KernelProcFileTestBase

from vts.utils.python.file import target_file_utils


class ProcUidCpuPowerTimeInStateTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/uid_cpupower/time_in_state
    '''

    def parse_contents(self, contents):
        size = struct.calcsize('I')
        return [struct.unpack_from('I', contents, i)[0] for i in range(0, len(contents), size)]

    def result_correct(self, result):
        if not result:
            return False

        if (len(result) - 1) % (result[0] + 1) != 0:
            return False

        return True

    def get_path(self):
        return "/proc/uid_cpupower/time_in_state"

    def file_optional(self):
        # This file is optional until implemented in Android common kernel
        return True

class ProcUidCpuPowerConcurrentActiveTimeTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/uid_cpupower/concurrent_active_time
    '''

    def parse_contents(self, contents):
        size = struct.calcsize('I')
        return [struct.unpack_from('I', contents, i)[0] for i in range(0, len(contents), size)]

    def result_correct(self, result):
        if not result:
            return False

        if (len(result) - 1) % (result[0] + 1) != 0:
            return False

        return True

    def get_path(self):
        return "/proc/uid_cpupower/concurrent_active_time"

    def file_optional(self):
        # This file is optional until implemented in Android common kernel
        return True

class ProcUidCpuPowerConcurrentPolicyTimeTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/uid_cpupower/concurrent_policy_time
    '''

    def parse_contents(self, contents):
        size = struct.calcsize('I')
        return [struct.unpack_from('I', contents, i)[0] for i in range(0, len(contents), size)]

    def result_correct(self, result):
        if not result:
            return False

        policy_cnt = result[0]
        result_len = len(result)

        if policy_cnt + 1 > result_len:
            return False

        line_len = sum(result[1:policy_cnt+1]) + 1

        if (result_len - policy_cnt - 1) % line_len != 0:
            return False

        return True

    def get_path(self):
        return "/proc/uid_cpupower/concurrent_policy_time"

    def file_optional(self):
        # This file is optional until implemented in Android common kernel
        return True
