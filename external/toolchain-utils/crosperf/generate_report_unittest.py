#!/usr/bin/env python2
#
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Test for generate_report.py."""

from __future__ import division
from __future__ import print_function

from StringIO import StringIO

import copy
import json
import mock
import test_flag
import unittest

import generate_report
import results_report


class _ContextualStringIO(StringIO):
  """StringIO that can be used in `with` statements."""

  def __init__(self, *args):
    StringIO.__init__(self, *args)

  def __enter__(self):
    return self

  def __exit__(self, _type, _value, _traceback):
    pass


class GenerateReportTests(unittest.TestCase):
  """Tests for generate_report.py."""

  def testCountBenchmarks(self):
    runs = {
        'foo': [[{}, {}, {}], [{}, {}, {}, {}]],
        'bar': [],
        'baz': [[], [{}], [{}, {}, {}]]
    }
    results = generate_report.CountBenchmarks(runs)
    expected_results = [('foo', 4), ('bar', 0), ('baz', 3)]
    self.assertItemsEqual(expected_results, results)

  def testCutResultsInPlace(self):
    bench_data = {
        'foo': [[{
            'a': 1,
            'b': 2,
            'c': 3
        }, {
            'a': 3,
            'b': 2.5,
            'c': 1
        }]],
        'bar': [[{
            'd': 11,
            'e': 12,
            'f': 13
        }]],
        'baz': [[{
            'g': 12,
            'h': 13
        }]],
        'qux': [[{
            'i': 11
        }]],
    }
    original_bench_data = copy.deepcopy(bench_data)

    max_keys = 2
    results = generate_report.CutResultsInPlace(
        bench_data, max_keys=max_keys, complain_on_update=False)
    # Cuts should be in-place.
    self.assertIs(results, bench_data)
    self.assertItemsEqual(original_bench_data.keys(), bench_data.keys())
    for bench_name, original_runs in original_bench_data.iteritems():
      bench_runs = bench_data[bench_name]
      self.assertEquals(len(original_runs), len(bench_runs))
      # Order of these sub-lists shouldn't have changed.
      for original_list, new_list in zip(original_runs, bench_runs):
        self.assertEqual(len(original_list), len(new_list))
        for original_keyvals, sub_keyvals in zip(original_list, new_list):
          # sub_keyvals must be a subset of original_keyvals
          self.assertDictContainsSubset(sub_keyvals, original_keyvals)

  def testCutResultsInPlaceLeavesRetval(self):
    bench_data = {
        'foo': [[{
            'retval': 0,
            'a': 1
        }]],
        'bar': [[{
            'retval': 1
        }]],
        'baz': [[{
            'RETVAL': 1
        }]],
    }
    results = generate_report.CutResultsInPlace(
        bench_data, max_keys=0, complain_on_update=False)
    # Just reach into results assuming we know it otherwise outputs things
    # sanely. If it doesn't, testCutResultsInPlace should give an indication as
    # to what, exactly, is broken.
    self.assertEqual(results['foo'][0][0].items(), [('retval', 0)])
    self.assertEqual(results['bar'][0][0].items(), [('retval', 1)])
    self.assertEqual(results['baz'][0][0].items(), [])

  def _RunMainWithInput(self, args, input_obj):
    assert '-i' not in args
    args += ['-i', '-']
    input_buf = _ContextualStringIO(json.dumps(input_obj))
    with mock.patch('generate_report.PickInputFile', return_value=input_buf) \
        as patched_pick:
      result = generate_report.Main(args)
      patched_pick.assert_called_once_with('-')
      return result

  @mock.patch('generate_report.RunActions')
  def testMain(self, mock_run_actions):
    # Email is left out because it's a bit more difficult to test, and it'll be
    # mildly obvious if it's failing.
    args = ['--json', '--html', '--text']
    return_code = self._RunMainWithInput(args, {'platforms': [], 'data': {}})
    self.assertEqual(0, return_code)
    self.assertEqual(mock_run_actions.call_count, 1)
    ctors = [ctor for ctor, _ in mock_run_actions.call_args[0][0]]
    self.assertItemsEqual(ctors, [
        results_report.JSONResultsReport,
        results_report.TextResultsReport,
        results_report.HTMLResultsReport,
    ])

  @mock.patch('generate_report.RunActions')
  def testMainSelectsHTMLIfNoReportsGiven(self, mock_run_actions):
    args = []
    return_code = self._RunMainWithInput(args, {'platforms': [], 'data': {}})
    self.assertEqual(0, return_code)
    self.assertEqual(mock_run_actions.call_count, 1)
    ctors = [ctor for ctor, _ in mock_run_actions.call_args[0][0]]
    self.assertItemsEqual(ctors, [results_report.HTMLResultsReport])

  # We only mock print_exc so we don't have exception info printed to stdout.
  @mock.patch('generate_report.WriteFile', side_effect=ValueError('Oh noo'))
  @mock.patch('traceback.print_exc')
  def testRunActionsRunsAllActionsRegardlessOfExceptions(
      self, mock_print_exc, mock_write_file):
    actions = [(None, 'json'), (None, 'html'), (None, 'text'), (None, 'email')]
    output_prefix = '-'
    ok = generate_report.RunActions(
        actions, {}, output_prefix, overwrite=False, verbose=False)
    self.assertFalse(ok)
    self.assertEqual(mock_write_file.call_count, len(actions))
    self.assertEqual(mock_print_exc.call_count, len(actions))

  @mock.patch('generate_report.WriteFile')
  def testRunActionsReturnsTrueIfAllActionsSucceed(self, mock_write_file):
    actions = [(None, 'json'), (None, 'html'), (None, 'text')]
    output_prefix = '-'
    ok = generate_report.RunActions(
        actions, {}, output_prefix, overwrite=False, verbose=False)
    self.assertEqual(mock_write_file.call_count, len(actions))
    self.assertTrue(ok)


if __name__ == '__main__':
  test_flag.SetTestMode(True)
  unittest.main()
