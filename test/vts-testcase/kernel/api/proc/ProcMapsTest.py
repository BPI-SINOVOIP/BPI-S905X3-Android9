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

from parse import with_pattern
from vts.testcases.kernel.api.proc import KernelProcFileTestBase


@with_pattern(r'[0-9a-f]{8,}')
def token_addr(text):
    return int(text, 16)

@with_pattern(r'[0-9a-f]{2,5}')
def token_mm(text):
    return int(text, 16)

@with_pattern(r'[0-9]+')
def token_lu(text):
    return int(text)

@with_pattern(r'[^\n^\0]*')
def token_path(text):
    return text.strip()

@with_pattern(r'[r-]')
def token_rbit(text):
    return text

@with_pattern(r'[w-]')
def token_wbit(text):
    return text

@with_pattern(r'[x-]')
def token_xbit(text):
    return text

@with_pattern(r'[sp]')
def token_spbit(text):
    return text

class ProcMapsTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/self/maps provides currently mapped memory regions and permissions.'''

    def parse_contents(self, contents):
        result = []
        lines = contents.split('\n')
        if lines[-1] != '':
            raise SyntaxError("missing final newline")
        for line in lines[:-1]:
            parsed = self.parse_line(
                    "{:addr}-{:addr} {:rbit}{:wbit}{:xbit}{:spbit} {:addr} {:mm}:{:mm} {:lu}{:path}",
                line, dict(mm=token_mm, addr=token_addr, lu=token_lu, path=token_path,
                    rbit=token_rbit, wbit=token_wbit, xbit=token_xbit, spbit=token_spbit))
            result.append(parsed)
        return result

    def get_path(self):
        return "/proc/self/maps"
