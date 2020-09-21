#!/usr/bin/env python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Parse the output of 'huddly-updater --info --log_to=stdout'.
"""

from __future__ import print_function

TOKEN_FW_CHUNK_HEADER = 'Firmware package:'
TOKEN_PERIPHERAL_CHUNK_HEADER = 'Camera Peripheral:'
TOKEN_BOOT = 'bootloader:'
TOKEN_APP = 'app:'
TOKEN_REV = 'hw_rev:'


def parse_fw_vers(chunk):
    """Parse huddly-updater command output.

    The parser logic heavily depends on the output format.

    @param chunk: The huddly-updater output. See CHUNK_FILENAME for example.

    @returns a dictionary containing the version strings
            for the firmware package and for the peripheral.
    """
    dic = {}
    target = ''
    for line in chunk.split('\n'):
        if TOKEN_FW_CHUNK_HEADER in line:
            target = 'package'
            dic[target] = {}
            continue
        elif TOKEN_PERIPHERAL_CHUNK_HEADER in line:
            target = 'peripheral'
            dic[target] = {}
            continue

        if not target:
            continue

        fields = line.split(':')
        if fields.__len__() < 2:
            continue

        val = fields[1].strip()

        if TOKEN_BOOT in line:
            dic[target]['boot'] = val
        elif TOKEN_APP in line:
            dic[target]['app'] = val
        elif TOKEN_REV in line:
            dic[target]['hw_rev'] = val
        else:
            continue

    return dic
