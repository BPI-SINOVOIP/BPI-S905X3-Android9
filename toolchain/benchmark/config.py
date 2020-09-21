# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Configuration file for the benchmark suite."""
from __future__ import print_function

import ConfigParser
import os

from parse_result import parse_Panorama
from parse_result import parse_Dex2oat
from parse_result import parse_Hwui
from parse_result import parse_Skia
from parse_result import parse_Synthmark
from parse_result import parse_Binder

from set_flags import add_flags_Panorama
from set_flags import add_flags_Dex2oat
from set_flags import add_flags_Hwui
from set_flags import add_flags_Skia
from set_flags import add_flags_Synthmark
from set_flags import add_flags_Binder

home = os.environ['HOME']

# Load user configurations for default envrionments
env_config = ConfigParser.ConfigParser(allow_no_value=True)
env_config.read('env_setting')

def get_suite_env(name, path=False):
    variable = env_config.get('Suite_Environment', name)
    if variable:
        if path and not os.path.isdir(variable):
            raise ValueError('The path of %s does not exist.' % name)
        return variable
    else:
        raise ValueError('Please specify %s in env_setting' % name)

# Android source code type: internal or aosp
android_type = get_suite_env('android_type')

# Android home directory specified as android_home,
android_home = get_suite_env('android_home', True)

# The benchmark results will be saved in bench_suite_dir.
# Please create a directory to store the results, default directory is
# android_home/benchtoolchain
bench_suite_dir = get_suite_env('bench_suite_dir', True)

# Crosperf directory is used to generate crosperf report.
toolchain_utils = get_suite_env('toolchain_utils', True)

# Please change both product and architecture at same time
# Product can be chosen from the lunch list of android building.
product_combo = get_suite_env('product_combo')

# Arch can be found from out/target/product
product = get_suite_env('product')

# Benchmarks list is in following variables, you can change it adding new
# benchmarks.
bench_dict = {
    'Panorama': 'packages/apps/LegacyCamera/benchmark/',
    'Dex2oat': 'art/compiler/',
    'Hwui': 'frameworks/base/libs/hwui/',
    'Skia': 'external/skia/',
    'Synthmark': 'synthmark/',
    'Binder': 'frameworks/native/libs/binder/',
}

bench_parser_dict = {
    'Panorama': parse_Panorama,
    'Dex2oat': parse_Dex2oat,
    'Hwui': parse_Hwui,
    'Skia': parse_Skia,
    'Synthmark': parse_Synthmark,
    'Binder': parse_Binder,
}

bench_flags_dict = {
    'Panorama': add_flags_Panorama,
    'Dex2oat': add_flags_Dex2oat,
    'Hwui': add_flags_Hwui,
    'Skia': add_flags_Skia,
    'Synthmark': add_flags_Synthmark,
    'Binder': add_flags_Binder,
}

bench_list = bench_dict.keys()

# Directories used in the benchmark suite
autotest_dir = 'external/autotest/'
out_dir = os.path.join(android_home, 'out')
