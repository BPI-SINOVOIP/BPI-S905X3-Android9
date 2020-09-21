#!/usr/bin/env python2
#
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# pylint: disable=cros-logging-import

"""Script to re-format json result to one with branch_name and build_id"""
from __future__ import print_function

import argparse
import config
import json
import logging
import os
import subprocess
import sys

# Turn the logging level to INFO before importing other autotest
# code, to avoid having failed import logging messages confuse the
# test_droid user.
logging.basicConfig(level=logging.INFO)


def _parse_arguments_internal(argv):
    parser = argparse.ArgumentParser(description='Convert result to JSON'
                                     'format')
    parser.add_argument(
        '-b', '--bench', help='Generate JSON format file for which benchmark.')
    return parser.parse_args(argv)

def fix_json(bench):
    # Set environment variable for crosperf
    os.environ['PYTHONPATH'] = os.path.dirname(config.toolchain_utils)

    logging.info('Generating Crosperf Report...')
    json_path = os.path.join(config.bench_suite_dir, bench + '_refined')
    crosperf_cmd = [
        os.path.join(config.toolchain_utils, 'generate_report.py'), '--json',
        '-i=' + os.path.join(config.bench_suite_dir, bench + '.json'),
        '-o=' + json_path, '-f'
    ]

    # Run crosperf generate_report.py
    logging.info('Command: %s', crosperf_cmd)
    subprocess.call(crosperf_cmd)

    json_path += '.json'
    with open(json_path) as fout:
        objs = json.load(fout)
    for obj in objs:
        obj['branch_name'] = 'aosp/master'
        obj['build_id'] = 0
    with open(json_path, 'w') as fout:
        json.dump(objs, fout)

    logging.info('JSON file fixed successfully!')

def main(argv):
    arguments = _parse_arguments_internal(argv)

    bench = arguments.bench

    fix_json(bench)

if __name__ == '__main__':
    main(sys.argv[1:])
