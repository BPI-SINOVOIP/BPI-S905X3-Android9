# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A list of advertisements data for testing purpose."""


ADVERTISEMENT1 = {
    'Path': '/org/bluez/test/advertisement1',
    'Type': 'peripheral',
    'ManufacturerData': {'0xff01': [0x1a, 0x1b, 0x1c, 0x1d, 0x1e]},
    'ServiceUUIDs': ['180D', '180F'],
    'SolicitUUIDs': [],
    'ServiceData': {'9991': [0x11, 0x12, 0x13, 0x14, 0x15]},
    'IncludeTxPower': True}


ADVERTISEMENT2 = {
    'Path': '/org/bluez/test/advertisement2',
    'Type': 'peripheral',
    'ManufacturerData': {'0xff02': [0x2a, 0x2b, 0x2c, 0x2d, 0x2e]},
    'ServiceUUIDs': ['1821'],
    'SolicitUUIDs': [],
    'ServiceData': {'9992': [0x21, 0x22, 0x23, 0x24, 0x25]},
    'IncludeTxPower': True}


ADVERTISEMENT3 = {
    'Path': '/org/bluez/test/advertisement3',
    'Type': 'peripheral',
    'ManufacturerData': {'0xff03': [0x3a, 0x3b, 0x3c, 0x3d, 0x3e]},
    'ServiceUUIDs': ['1819', '180E'],
    'SolicitUUIDs': [],
    'ServiceData': {'9993': [0x31, 0x32, 0x33, 0x34, 0x35]},
    'IncludeTxPower': True}


ADVERTISEMENT4 = {
    'Path': '/org/bluez/test/advertisement4',
    'Type': 'peripheral',
    'ManufacturerData': {'0xff04': [0x4a, 0x4b, 0x4c, 0x4d, 0x4e]},
    'ServiceUUIDs': ['1808', '1810'],
    'SolicitUUIDs': [],
    'ServiceData': {'9994': [0x41, 0x42, 0x43, 0x44, 0x45]},
    'IncludeTxPower': True}


ADVERTISEMENT5 = {
    'Path': '/org/bluez/test/advertisement5',
    'Type': 'peripheral',
    'ManufacturerData': {'0xff05': [0x5a, 0x5b, 0x5c, 0x5d, 0x5e]},
    'ServiceUUIDs': ['1818', '181B'],
    'SolicitUUIDs': [],
    'ServiceData': {'9995': [0x51, 0x52, 0x53, 0x54, 0x55]},
    'IncludeTxPower': True}


ADVERTISEMENT6 = {
    'Path': '/org/bluez/test/advertisement6',
    'Type': 'peripheral',
    'ManufacturerData': {'0xff06': [0x6a, 0x6b, 0x6c, 0x6d, 0x6e]},
    'ServiceUUIDs': ['1820'],
    'SolicitUUIDs': [],
    'ServiceData': {'9996': [0x61, 0x62, 0x63, 0x64, 0x65]},
    'IncludeTxPower': True}


ADVERTISEMENTS = [ADVERTISEMENT1, ADVERTISEMENT2, ADVERTISEMENT3,
                  ADVERTISEMENT4, ADVERTISEMENT5, ADVERTISEMENT6]
