#!/usr/bin/env python2
#
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# pylint: disable=cros-logging-import

"""Script to help generate json format report from raw data."""
from __future__ import print_function

import argparse
import config
import json
import logging
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

    parser.add_argument(
        '-i', '--input', help='Specify the input result file name.')

    parser.add_argument(
        '-o', '--output', help='Specify the output JSON format result file')

    parser.add_argument(
        '-p',
        '--platform',
        help='Indicate the platform(experiment or device) name '
        'to be shown in JSON')

    parser.add_argument(
        '--iterations',
        type=int,
        help='How many iterations does the result include.')
    return parser.parse_args(argv)

# Collect data and generate JSON {} tuple from benchmark result
def collect_data(infile, bench, it):
    result_dict = {}
    with open(infile + str(it)) as fin:
        if bench not in config.bench_parser_dict:
            logging.error('Please input the correct benchmark name.')
            raise ValueError('Wrong benchmark name: %s' % bench)
        parse = config.bench_parser_dict[bench]
        result_dict = parse(bench, fin)
    return result_dict

# If there is no original output file, create a new one and init it.
def create_outfile(outfile, bench):
    with open(outfile, 'w') as fout:
        obj_null = {'data': {bench.lower(): []}, 'platforms': []}
        json.dump(obj_null, fout)

# Seek the original output file and try to add new result into it.
def get_outfile(outfile, bench):
    try:
        return open(outfile)
    except IOError:
        create_outfile(outfile, bench)
        return open(outfile)

def main(argv):
    arguments = _parse_arguments_internal(argv)

    bench = arguments.bench
    infile = arguments.input
    outfile = arguments.output
    platform = arguments.platform
    iteration = arguments.iterations

    result = []
    for i in xrange(iteration):
        result += collect_data(infile, bench, i)

    with get_outfile(outfile, bench) as fout:
        obj = json.load(fout)
    obj['platforms'].append(platform)
    obj['data'][bench.lower()].append(result)
    with open(outfile, 'w') as fout:
        json.dump(obj, fout)


if __name__ == '__main__':
    main(sys.argv[1:])
