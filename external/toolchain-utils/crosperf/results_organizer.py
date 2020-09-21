# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Parse data from benchmark_runs for tabulator."""

from __future__ import print_function

import errno
import json
import os
import re
import sys

from cros_utils import misc

_TELEMETRY_RESULT_DEFAULTS_FILE = 'default-telemetry-results.json'
_DUP_KEY_REGEX = re.compile(r'(\w+)\{(\d+)\}')


def _AdjustIteration(benchmarks, max_dup, bench):
  """Adjust the interation numbers if they have keys like ABCD{i}."""
  for benchmark in benchmarks:
    if benchmark.name != bench or benchmark.iteration_adjusted:
      continue
    benchmark.iteration_adjusted = True
    benchmark.iterations *= (max_dup + 1)


def _GetMaxDup(data):
  """Find the maximum i inside ABCD{i}.

  data should be a [[[Key]]], where Key is a string that may look like
  ABCD{i}.
  """
  max_dup = 0
  for label in data:
    for run in label:
      for key in run:
        match = _DUP_KEY_REGEX.match(key)
        if match:
          max_dup = max(max_dup, int(match.group(2)))
  return max_dup


def _Repeat(func, times):
  """Returns the result of running func() n times."""
  return [func() for _ in xrange(times)]


def _DictWithReturnValues(retval, pass_fail):
  """Create a new dictionary pre-populated with success/fail values."""
  new_dict = {}
  # Note: 0 is a valid retval; test to make sure it's not None.
  if retval is not None:
    new_dict['retval'] = retval
  if pass_fail:
    new_dict[''] = pass_fail
  return new_dict


def _GetNonDupLabel(max_dup, runs):
  """Create new list for the runs of the same label.

  Specifically, this will split out keys like foo{0}, foo{1} from one run into
  their own runs. For example, given a run like:
    {"foo": 1, "bar{0}": 2, "baz": 3, "qux{1}": 4, "pirate{0}": 5}

  You'll get:
    [{"foo": 1, "baz": 3}, {"bar": 2, "pirate": 5}, {"qux": 4}]

  Hands back the lists of transformed runs, all concatenated together.
  """
  new_runs = []
  for run in runs:
    run_retval = run.get('retval', None)
    run_pass_fail = run.get('', None)
    new_run = {}
    # pylint: disable=cell-var-from-loop
    added_runs = _Repeat(
        lambda: _DictWithReturnValues(run_retval, run_pass_fail), max_dup)
    for key, value in run.iteritems():
      match = _DUP_KEY_REGEX.match(key)
      if not match:
        new_run[key] = value
      else:
        new_key, index_str = match.groups()
        added_runs[int(index_str) - 1][new_key] = str(value)
    new_runs.append(new_run)
    new_runs += added_runs
  return new_runs


def _DuplicatePass(result, benchmarks):
  """Properly expands keys like `foo{1}` in `result`."""
  for bench, data in result.iteritems():
    max_dup = _GetMaxDup(data)
    # If there's nothing to expand, there's nothing to do.
    if not max_dup:
      continue
    for i, runs in enumerate(data):
      data[i] = _GetNonDupLabel(max_dup, runs)
    _AdjustIteration(benchmarks, max_dup, bench)


def _ReadSummaryFile(filename):
  """Reads the summary file at filename."""
  dirname, _ = misc.GetRoot(filename)
  fullname = os.path.join(dirname, _TELEMETRY_RESULT_DEFAULTS_FILE)
  try:
    # Slurp the summary file into a dictionary. The keys in the dictionary are
    # the benchmark names. The value for a key is a list containing the names
    # of all the result fields that should be returned in a 'default' report.
    with open(fullname) as in_file:
      return json.load(in_file)
  except IOError as e:
    # ENOENT means "no such file or directory"
    if e.errno == errno.ENOENT:
      return {}
    raise


def _MakeOrganizeResultOutline(benchmark_runs, labels):
  """Creates the "outline" of the OrganizeResults result for a set of runs.

  Report generation returns lists of different sizes, depending on the input
  data. Depending on the order in which we iterate through said input data, we
  may populate the Nth index of a list, then the N-1st, then the N+Mth, ...

  It's cleaner to figure out the "skeleton"/"outline" ahead of time, so we don't
  have to worry about resizing while computing results.
  """
  # Count how many iterations exist for each benchmark run.
  # We can't simply count up, since we may be given an incomplete set of
  # iterations (e.g. [r.iteration for r in benchmark_runs] == [1, 3])
  iteration_count = {}
  for run in benchmark_runs:
    name = run.benchmark.name
    old_iterations = iteration_count.get(name, -1)
    # N.B. run.iteration starts at 1, not 0.
    iteration_count[name] = max(old_iterations, run.iteration)

  # Result structure: {benchmark_name: [[{key: val}]]}
  result = {}
  for run in benchmark_runs:
    name = run.benchmark.name
    num_iterations = iteration_count[name]
    # default param makes cros lint be quiet about defining num_iterations in a
    # loop.
    make_dicts = lambda n=num_iterations: _Repeat(dict, n)
    result[name] = _Repeat(make_dicts, len(labels))
  return result


def OrganizeResults(benchmark_runs, labels, benchmarks=None, json_report=False):
  """Create a dict from benchmark_runs.

  The structure of the output dict is as follows:
  {"benchmark_1":[
    [{"key1":"v1", "key2":"v2"},{"key1":"v1", "key2","v2"}]
    #one label
    []
    #the other label
    ]
   "benchmark_2":
    [
    ]}.
  """
  result = _MakeOrganizeResultOutline(benchmark_runs, labels)
  label_names = [label.name for label in labels]
  label_indices = {name: i for i, name in enumerate(label_names)}
  summary_file = _ReadSummaryFile(sys.argv[0])
  if benchmarks is None:
    benchmarks = []

  for benchmark_run in benchmark_runs:
    if not benchmark_run.result:
      continue
    benchmark = benchmark_run.benchmark
    label_index = label_indices[benchmark_run.label.name]
    cur_label_list = result[benchmark.name][label_index]
    cur_dict = cur_label_list[benchmark_run.iteration - 1]

    show_all_results = json_report or benchmark.show_all_results
    if not show_all_results:
      summary_list = summary_file.get(benchmark.test_name)
      if summary_list:
        summary_list.append('retval')
      else:
        # Did not find test_name in json file; show everything.
        show_all_results = True
    for test_key in benchmark_run.result.keyvals:
      if show_all_results or test_key in summary_list:
        cur_dict[test_key] = benchmark_run.result.keyvals[test_key]
    # Occasionally Telemetry tests will not fail but they will not return a
    # result, either.  Look for those cases, and force them to be a fail.
    # (This can happen if, for example, the test has been disabled.)
    if len(cur_dict) == 1 and cur_dict['retval'] == 0:
      cur_dict['retval'] = 1
      benchmark_run.result.keyvals['retval'] = 1
      # TODO: This output should be sent via logger.
      print(
          "WARNING: Test '%s' appears to have succeeded but returned"
          ' no results.' % benchmark.name,
          file=sys.stderr)
    if json_report and benchmark_run.machine:
      cur_dict['machine'] = benchmark_run.machine.name
      cur_dict['machine_checksum'] = benchmark_run.machine.checksum
      cur_dict['machine_string'] = benchmark_run.machine.checksum_string
  _DuplicatePass(result, benchmarks)
  return result
