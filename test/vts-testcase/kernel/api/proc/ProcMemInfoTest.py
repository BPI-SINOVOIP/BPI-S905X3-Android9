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

from parse import with_pattern
from vts.testcases.kernel.api.proc import KernelProcFileTestBase


@with_pattern(r'[^ ^\t^\n^:^\0]+')
def token_name(text):
    return text

@with_pattern(r'[ ]*[0-9]+')
def token_lu(text):
    return int(text)

@with_pattern(r'(kB)?')
def token_kb(text):
    return text

class ProcMemInfoTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/meminfo reports statistics about memory usage on the system.

    No new fields should be added to the upstream implementation.
    '''

    REQUIRED_FIELDS = {
        "MemTotal",
        "MemFree",
        "MemAvailable",
        "Buffers",
        "Cached",
        "SwapCached",
        "Active",
        "Inactive",
        "Active(anon)",
        "Inactive(anon)",
        "Active(file)",
        "Inactive(file)",
        "Unevictable",
        "Mlocked",
        "SwapTotal",
        "SwapFree",
        "Dirty",
        "Writeback",
        "AnonPages",
        "Mapped",
        "Shmem",
        "Slab",
        "SReclaimable",
        "SUnreclaim",
        "KernelStack",
        "PageTables",
        "NFS_Unstable",
        "Bounce",
        "WritebackTmp",
        "CommitLimit",
        "Committed_AS",
        "VmallocTotal",
        "VmallocUsed",
        "VmallocChunk",
    }

    def parse_contents(self, contents):
        lines = contents.split('\n')
        if lines[-1] != '':
            raise SyntaxError("missing final newline")
        return [self.parse_line("{:name}: {:lu}{:^kb}", line,
            dict(name=token_name, lu=token_lu, kb=token_kb)) for line in lines[:-1]]

    def result_correct(self, parse_result):
        required_fields = self.REQUIRED_FIELDS.copy()
        for line in parse_result:
            if line[0] in required_fields:
                required_fields.remove(line[0])
        if len(required_fields) > 0:
            logging.error("Required fields not present: %s", str(required_fields))
            return False
        return True

    def get_path(self):
        return "/proc/meminfo"
