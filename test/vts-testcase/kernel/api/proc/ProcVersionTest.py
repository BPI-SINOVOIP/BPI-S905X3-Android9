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
from vts.testcases.kernel.api.proc import KernelProcFileTestBase


class ProcVersionTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/version displays the kernel version and build information.'''

    def parse_contents(self, contents):
        return self.parse_line("{} version {} ({}@{}) ({}) {}\n", contents)

    def result_correct(self, parse_result):
        if parse_result[0] != 'Linux' or len(parse_result[1].split('.')) != 3:
            logging.error("Not a valid linux version!")
            return False
        return True

    def get_path(self):
        return "/proc/version"
