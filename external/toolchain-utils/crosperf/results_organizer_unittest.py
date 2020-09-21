#!/usr/bin/env python2

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Testing of ResultsOrganizer

   We create some labels, benchmark_runs and then create a ResultsOrganizer,
   after that, we compare the result of ResultOrganizer.
"""

from __future__ import print_function

import unittest

from benchmark_run import BenchmarkRun
from results_cache import Result
from results_organizer import OrganizeResults

import mock_instance

result = {
    'benchmark1': [[{
        '': 'PASS',
        'bool': 'True',
        'milliseconds_1': '1',
        'milliseconds_2': '8',
        'milliseconds_3': '9.2',
        'ms_1': '2.1',
        'total': '5'
    }, {
        '': 'PASS',
        'test': '2'
    }, {
        '': 'PASS',
        'test': '4'
    }, {
        '': 'PASS',
        'bool': 'FALSE',
        'milliseconds_1': '3',
        'milliseconds_2': '5',
        'ms_1': '2.2',
        'total': '6'
    }, {
        '': 'PASS',
        'test': '3'
    }, {
        '': 'PASS',
        'test': '4'
    }], [{
        '': 'PASS',
        'bool': 'FALSE',
        'milliseconds_4': '30',
        'milliseconds_5': '50',
        'ms_1': '2.23',
        'total': '6'
    }, {
        '': 'PASS',
        'test': '5'
    }, {
        '': 'PASS',
        'test': '4'
    }, {
        '': 'PASS',
        'bool': 'FALSE',
        'milliseconds_1': '3',
        'milliseconds_6': '7',
        'ms_1': '2.3',
        'total': '7'
    }, {
        '': 'PASS',
        'test': '2'
    }, {
        '': 'PASS',
        'test': '6'
    }]],
    'benchmark2': [[{
        '': 'PASS',
        'bool': 'TRUE',
        'milliseconds_1': '3',
        'milliseconds_8': '6',
        'ms_1': '2.3',
        'total': '7'
    }, {
        '': 'PASS',
        'test': '2'
    }, {
        '': 'PASS',
        'test': '6'
    }, {
        '': 'PASS',
        'bool': 'TRUE',
        'milliseconds_1': '3',
        'milliseconds_8': '6',
        'ms_1': '2.2',
        'total': '7'
    }, {
        '': 'PASS',
        'test': '2'
    }, {
        '': 'PASS',
        'test': '2'
    }], [{
        '': 'PASS',
        'bool': 'TRUE',
        'milliseconds_1': '3',
        'milliseconds_8': '6',
        'ms_1': '2',
        'total': '7'
    }, {
        '': 'PASS',
        'test': '2'
    }, {
        '': 'PASS',
        'test': '4'
    }, {
        '': 'PASS',
        'bool': 'TRUE',
        'milliseconds_1': '3',
        'milliseconds_8': '6',
        'ms_1': '1',
        'total': '7'
    }, {
        '': 'PASS',
        'test': '1'
    }, {
        '': 'PASS',
        'test': '6'
    }]]
}


class ResultOrganizerTest(unittest.TestCase):
  """Test result organizer."""

  def testResultOrganizer(self):
    labels = [mock_instance.label1, mock_instance.label2]
    benchmarks = [mock_instance.benchmark1, mock_instance.benchmark2]
    benchmark_runs = [None] * 8
    benchmark_runs[0] = BenchmarkRun('b1', benchmarks[0], labels[0], 1, '', '',
                                     '', 'average', '')
    benchmark_runs[1] = BenchmarkRun('b2', benchmarks[0], labels[0], 2, '', '',
                                     '', 'average', '')
    benchmark_runs[2] = BenchmarkRun('b3', benchmarks[0], labels[1], 1, '', '',
                                     '', 'average', '')
    benchmark_runs[3] = BenchmarkRun('b4', benchmarks[0], labels[1], 2, '', '',
                                     '', 'average', '')
    benchmark_runs[4] = BenchmarkRun('b5', benchmarks[1], labels[0], 1, '', '',
                                     '', 'average', '')
    benchmark_runs[5] = BenchmarkRun('b6', benchmarks[1], labels[0], 2, '', '',
                                     '', 'average', '')
    benchmark_runs[6] = BenchmarkRun('b7', benchmarks[1], labels[1], 1, '', '',
                                     '', 'average', '')
    benchmark_runs[7] = BenchmarkRun('b8', benchmarks[1], labels[1], 2, '', '',
                                     '', 'average', '')

    i = 0
    for b in benchmark_runs:
      b.result = Result('', b.label, 'average', 'machine')
      b.result.keyvals = mock_instance.keyval[i]
      i += 1

    organized = OrganizeResults(benchmark_runs, labels, benchmarks)
    self.assertEqual(organized, result)


if __name__ == '__main__':
  unittest.main()
