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


class ProcCmdlineTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/cmdline contains arguments passed to the kernel.'''

    def parse_contents(self, contents):
        if len(contents) == 0 or contents[-1] != '\n':
            raise SyntaxError("missing newline")
        return contents[:-1].split(' ')

    def get_path(self):
        return "/proc/cmdline"
