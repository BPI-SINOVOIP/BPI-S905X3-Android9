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
from vts.utils.python.file import target_file_utils


class ProcQtaguidCtrlTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/net/xt_qtaguid/ctrl provides information about tagged sockets.

    File content consists of possibly several lines of socket info followed by a
    single line of events info, followed by a terminating newline.'''

    def parse_contents(self, contents):
        result = []
        lines = contents.split('\n')
        if len(lines) == 0 or lines[-1] != '':
            raise SyntaxError
        for line in lines[:-2]:
            parsed = self.parse_line(
                "sock={:x} tag=0x{:x} (uid={:d}) pid={:d} f_count={:d}", line)
            if any(map(lambda x: x < 0, parsed)):
                raise SyntaxError("Negative numbers not allowed!")
            result.append(parsed)
        parsed = self.parse_line(
            "events: sockets_tagged={:d} sockets_untagged={:d} counter_set_changes={:d} "
            "delete_cmds={:d} iface_events={:d} match_calls={:d} match_calls_prepost={:d} "
            "match_found_sk={:d} match_found_sk_in_ct={:d} match_found_no_sk_in_ct={:d} "
            "match_no_sk={:d} match_no_sk_{:w}={:d}", lines[-2])
        if parsed[-2] not in {"file", "gid"}:
            raise SyntaxError("match_no_sk_{file|gid} incorrect")
        del parsed[-2]
        if any(map(lambda x: x < 0, parsed)):
            raise SyntaxError("Negative numbers not allowed!")
        result.append(parsed)
        return result

    def get_path(self):
        return "/proc/net/xt_qtaguid/ctrl"

    def get_permission_checker(self):
        """Get r/w file permission checker.
        """
        return target_file_utils.IsReadWrite
