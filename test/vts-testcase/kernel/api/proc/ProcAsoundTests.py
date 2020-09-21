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
from vts.testcases.kernel.api.proc.KernelProcFileTestBase import repeat_rule, literal_token


class ProcAsoundCardsTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/asound/cards shows the list of currently configured ALSA drivers.

    List entries include index, the id string, short and long descriptions.'''

    t_LBRACKET = literal_token(r'\[')
    t_RBRACKET = literal_token(r'\]')

    t_ignore = ' '

    start = 'drivers'

    p_drivers = repeat_rule('driver')

    def p_driver(self, p):
        'driver : NUMBER id COLON STRING DASH description NEWLINE description NEWLINE'
        p[0] = [p[1], p[2], p[4], p[6], p[8]]

    def p_description(self, p):
        '''description : description STRING
                       | STRING'''
        p[0] = [p[1]] if len(p) == 2 else p[1].append(p[2])

    def p_id(self, p):
        'id : LBRACKET STRING RBRACKET'
        p[0] = p[2]

    def get_path(self):
        return "/proc/asound/cards"
