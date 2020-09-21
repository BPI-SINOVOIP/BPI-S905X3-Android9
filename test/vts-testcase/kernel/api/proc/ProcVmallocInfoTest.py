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


class ProcVmallocInfoTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/vmallocinfo provides info on vmalloc'd ranges.'''

    start = 'lines'

    def t_CALLER(self, t):
        '[^ ^\t^\n^-^=^+^/]+\+0x[a-f0-9]+/0x[a-f0-9]+'
        t.value = t.value.split('+')
        return t

    t_PAGES = literal_token('pages')
    t_IOREMAP = literal_token('ioremap')
    t_MODULE = literal_token('\[[^\n^\0]*\]')
    t_VMALLOC = literal_token('vmalloc')
    t_VMAP = literal_token('vmap')
    t_USER = literal_token('user')
    t_VPAGES = literal_token('vpages')
    t_VM_AREA = literal_token('vm_area')
    t_UNPURGED = literal_token('unpurged')
    t_VM_MAP_RAM = literal_token('vm_map_ram')

    t_ignore = ' '

    def t_PHYS(self, t):
        r'phys=(0x)?[a-f0-9]+'
        t.value = [t.value[:4], int(t.value[5:], 16)]
        return t

    p_lines = repeat_rule('line')

    def p_line(self, p):
        'line : addr_range NUMBER caller module pages phys ioremap vmalloc vmap user vpages vm_vm_area NEWLINE'
        p[0] = p[1:]

    def p_addr_range(self, p):
        'addr_range : HEX_LITERAL DASH HEX_LITERAL'
        p[0] = [p[1], p[3]]

    def p_module(self, p):
        '''module : MODULE
                  | empty'''
        p[0] = p[1]

    def p_pages(self, p):
        '''pages : PAGES EQUALS NUMBER
                 | empty'''
        p[0] = [] if len(p) == 2 else [p[1], p[3]]

    def p_phys(self, p):
        '''phys : PHYS
                | empty'''
        p[0] = p[1]

    def p_ioremap(self, p):
        '''ioremap : IOREMAP
                   | empty'''
        p[0] = p[1]

    def p_vmalloc(self, p):
        '''vmalloc : VMALLOC
                   | empty'''
        p[0] = p[1]

    def p_vmap(self, p):
        '''vmap : VMAP
                | empty'''
        p[0] = p[1]

    def p_user(self, p):
        '''user : USER
                | empty'''
        p[0] = p[1]

    def p_vpages(self, p):
        '''vpages : VPAGES
                  | empty'''
        p[0] = p[1]

    def p_vm_vm_area(self, p):
        '''vm_vm_area : UNPURGED VM_AREA
                      | VM_MAP_RAM
                      | empty'''
        if len(p) == 2:
            p[0] = []
        else:
            p[0] = p[1:]

    def p_caller(self, p):
        '''caller : CALLER
                  | HEX_LITERAL
                  | empty'''
        p[0] = p[1]

    def get_path(self):
        return "/proc/vmallocinfo"
