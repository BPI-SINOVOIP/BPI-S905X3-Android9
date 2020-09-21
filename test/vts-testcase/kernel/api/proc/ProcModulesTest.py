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

import re
from vts.utils.python.android import api
from vts.testcases.kernel.api.proc import KernelProcFileTestBase


class ProcModulesTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/modules contains information about loaded kernel modules.'''

    def prepare_test(self, shell, dut):
        api_level = dut.getLaunchApiLevel(strict=False)
        self.require_module = False
        return True

    def parse_contents(self, contents):
        module_present = False
        # MODULE_NAME SIZE REFERENCE_COUNT USER1,USER2, STATE BASE_ADDRESS TAINT_FLAG
        # MODULE_NAME is a string
        # SIZE is an integer
        # REFERENCE_COUNT is an integer or -
        # USER1,USER2, is a list of modules using this module with a trailing comma.
        #   If no modules are using this module or if modules cannot be unloaded then
        #   - will appear. If this mdoule cannot be unloaded then [permanent] will be
        #   the last entry.
        # STATE is either Unloading, Loading, or Live
        # BASE_ADDRESS is a memory address
        # TAINT_FLAG is optional and if present, has characters between ( and )
        test_re = re.compile(r"^\w+ \d+ (\d+|-) (((\w+,)+(\[permanent\],)?)|-) (Unloading|Loading|Live) 0x[0-9a-f]+( \(\w+\))?")
        for line in contents.splitlines():
            if not re.match(test_re, line):
                raise SyntaxError("Malformed entry in /proc/modules: %s" % line)
            else:
                module_present = True
        if self.require_module and not module_present:
            raise SyntaxError("There must be at least one entry in /proc/modules")

        return ''

    def result_correct(self, parse_result):
        return True

    def get_path(self):
        return "/proc/modules"
