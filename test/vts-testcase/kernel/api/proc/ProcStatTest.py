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


class ProcStatTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/stat provides kernel and system statistics.'''

    start = 'stat'

    t_CPU = literal_token(r'cpu[0-9]*')
    t_INTR = literal_token(r'intr')
    t_CTXT = literal_token(r'ctxt')
    t_BTIME = literal_token(r'btime')
    t_PROCESSES = literal_token(r'processes')
    t_PROCS_RUNNING = literal_token(r'procs_running')
    t_PROCS_BLOCKED = literal_token(r'procs_blocked')
    t_SOFTIRQ = literal_token(r'softirq')

    p_cpus = repeat_rule('cpu')
    p_numbers = repeat_rule('NUMBER')

    t_ignore = ' '

    def p_stat(self, p):
        'stat : cpus intr ctxt btime processes procs_running procs_blocked softirq'
        p[0] = p[1:]

    def p_cpu(self, p):
        'cpu : CPU NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NEWLINE'
        p[0] = p[1:]

    def p_intr(self, p):
        'intr : INTR NUMBERs NEWLINE'
        p[0] = p[1:]

    def p_ctxt(self, p):
        'ctxt : CTXT NUMBER NEWLINE'
        p[0] = p[1:]

    def p_btime(self, p):
        'btime : BTIME NUMBER NEWLINE'
        p[0] = p[1:]

    def p_processes(self, p):
        'processes : PROCESSES NUMBER NEWLINE'
        p[0] = p[1:]

    def p_procs_running(self, p):
        'procs_running : PROCS_RUNNING NUMBER NEWLINE'
        p[0] = p[1:]

    def p_procs_blocked(self, p):
        'procs_blocked : PROCS_BLOCKED NUMBER NEWLINE'
        p[0] = p[1:]

    def p_softirq(self, p):
        'softirq : SOFTIRQ NUMBERs NEWLINE'
        p[0] = p[1:]

    def get_path(self):
        return "/proc/stat"
