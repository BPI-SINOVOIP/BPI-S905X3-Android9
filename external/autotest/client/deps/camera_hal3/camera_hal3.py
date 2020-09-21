#!/usr/bin/python

# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from autotest_lib.client.bin import utils

version = 1

def setup(setup_dir):
    binary = 'arc_camera3_test'
    src_path = os.path.join(os.environ['SYSROOT'], 'usr', 'bin')
    dst_path = os.path.join(os.getcwd(), 'bin')
    os.mkdir(dst_path)
    utils.get_file(os.path.join(src_path, binary),
                   os.path.join(dst_path, binary))


utils.update_version(os.getcwd(), True, version, setup, os.getcwd())
