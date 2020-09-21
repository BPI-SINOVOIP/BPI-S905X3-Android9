# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""This contains some mock instances for testing."""

from __future__ import print_function

from benchmark import Benchmark
from label import MockLabel

perf_args = 'record -a -e cycles'
label1 = MockLabel(
    'test1',
    'image1',
    'autotest_dir',
    '/tmp/test_benchmark_run',
    'x86-alex',
    'chromeos-alex1',
    image_args='',
    cache_dir='',
    cache_only=False,
    log_level='average',
    compiler='gcc')

label2 = MockLabel(
    'test2',
    'image2',
    'autotest_dir',
    '/tmp/test_benchmark_run_2',
    'x86-alex',
    'chromeos-alex2',
    image_args='',
    cache_dir='',
    cache_only=False,
    log_level='average',
    compiler='gcc')

benchmark1 = Benchmark('benchmark1', 'autotest_name_1', 'autotest_args', 2, '',
                       perf_args, '', '')

benchmark2 = Benchmark('benchmark2', 'autotest_name_2', 'autotest_args', 2, '',
                       perf_args, '', '')

keyval = {}
keyval[0] = {
    '': 'PASS',
    'milliseconds_1': '1',
    'milliseconds_2': '8',
    'milliseconds_3': '9.2',
    'test{1}': '2',
    'test{2}': '4',
    'ms_1': '2.1',
    'total': '5',
    'bool': 'True'
}

keyval[1] = {
    '': 'PASS',
    'milliseconds_1': '3',
    'milliseconds_2': '5',
    'ms_1': '2.2',
    'total': '6',
    'test{1}': '3',
    'test{2}': '4',
    'bool': 'FALSE'
}

keyval[2] = {
    '': 'PASS',
    'milliseconds_4': '30',
    'milliseconds_5': '50',
    'ms_1': '2.23',
    'total': '6',
    'test{1}': '5',
    'test{2}': '4',
    'bool': 'FALSE'
}

keyval[3] = {
    '': 'PASS',
    'milliseconds_1': '3',
    'milliseconds_6': '7',
    'ms_1': '2.3',
    'total': '7',
    'test{1}': '2',
    'test{2}': '6',
    'bool': 'FALSE'
}

keyval[4] = {
    '': 'PASS',
    'milliseconds_1': '3',
    'milliseconds_8': '6',
    'ms_1': '2.3',
    'total': '7',
    'test{1}': '2',
    'test{2}': '6',
    'bool': 'TRUE'
}

keyval[5] = {
    '': 'PASS',
    'milliseconds_1': '3',
    'milliseconds_8': '6',
    'ms_1': '2.2',
    'total': '7',
    'test{1}': '2',
    'test{2}': '2',
    'bool': 'TRUE'
}

keyval[6] = {
    '': 'PASS',
    'milliseconds_1': '3',
    'milliseconds_8': '6',
    'ms_1': '2',
    'total': '7',
    'test{1}': '2',
    'test{2}': '4',
    'bool': 'TRUE'
}

keyval[7] = {
    '': 'PASS',
    'milliseconds_1': '3',
    'milliseconds_8': '6',
    'ms_1': '1',
    'total': '7',
    'test{1}': '1',
    'test{2}': '6',
    'bool': 'TRUE'
}

keyval[8] = {
    '': 'PASS',
    'milliseconds_1': '3',
    'milliseconds_8': '6',
    'ms_1': '3.3',
    'total': '7',
    'test{1}': '2',
    'test{2}': '8',
    'bool': 'TRUE'
}
