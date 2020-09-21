#!/usr/bin/env python2
#
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Given a specially-formatted JSON object, generates results report(s).

The JSON object should look like:
{"data": BenchmarkData, "platforms": BenchmarkPlatforms}

BenchmarkPlatforms is a [str], each of which names a platform the benchmark
  was run on (e.g. peppy, shamu, ...). Note that the order of this list is
  related with the order of items in BenchmarkData.

BenchmarkData is a {str: [PlatformData]}. The str is the name of the benchmark,
and a PlatformData is a set of data for a given platform. There must be one
PlatformData for each benchmark, for each element in BenchmarkPlatforms.

A PlatformData is a [{str: float}], where each str names a metric we recorded,
and the float is the value for that metric. Each element is considered to be
the metrics collected from an independent run of this benchmark. NOTE: Each
PlatformData is expected to have a "retval" key, with the return value of
the benchmark. If the benchmark is successful, said return value should be 0.
Otherwise, this will break some of our JSON functionality.

Putting it all together, a JSON object will end up looking like:
  { "platforms": ["peppy", "peppy-new-crosstool"],
    "data": {
      "bench_draw_line": [
        [{"time (ms)": 1.321, "memory (mb)": 128.1, "retval": 0},
         {"time (ms)": 1.920, "memory (mb)": 128.4, "retval": 0}],
        [{"time (ms)": 1.221, "memory (mb)": 124.3, "retval": 0},
         {"time (ms)": 1.423, "memory (mb)": 123.9, "retval": 0}]
      ]
    }
  }

Which says that we ran a benchmark on platforms named peppy, and
  peppy-new-crosstool.
We ran one benchmark, named bench_draw_line.
It was run twice on each platform.
Peppy's runs took 1.321ms and 1.920ms, while peppy-new-crosstool's took 1.221ms
  and 1.423ms. None of the runs failed to complete.
"""

from __future__ import division
from __future__ import print_function

import argparse
import functools
import json
import os
import sys
import traceback

from results_report import BenchmarkResults
from results_report import HTMLResultsReport
from results_report import JSONResultsReport
from results_report import TextResultsReport


def CountBenchmarks(benchmark_runs):
  """Counts the number of iterations for each benchmark in benchmark_runs."""

  # Example input for benchmark_runs:
  # {"bench": [[run1, run2, run3], [run1, run2, run3, run4]]}
  def _MaxLen(results):
    return 0 if not results else max(len(r) for r in results)

  return [(name, _MaxLen(results))
          for name, results in benchmark_runs.iteritems()]


def CutResultsInPlace(results, max_keys=50, complain_on_update=True):
  """Limits the given benchmark results to max_keys keys in-place.

  This takes the `data` field from the benchmark input, and mutates each
  benchmark run to contain `max_keys` elements (ignoring special elements, like
  "retval"). At the moment, it just selects the first `max_keys` keyvals,
  alphabetically.

  If complain_on_update is true, this will print a message noting that a
  truncation occurred.

  This returns the `results` object that was passed in, for convenience.

  e.g.
  >>> benchmark_data = {
  ...   "bench_draw_line": [
  ...     [{"time (ms)": 1.321, "memory (mb)": 128.1, "retval": 0},
  ...      {"time (ms)": 1.920, "memory (mb)": 128.4, "retval": 0}],
  ...     [{"time (ms)": 1.221, "memory (mb)": 124.3, "retval": 0},
  ...      {"time (ms)": 1.423, "memory (mb)": 123.9, "retval": 0}]
  ...   ]
  ... }
  >>> CutResultsInPlace(benchmark_data, max_keys=1, complain_on_update=False)
  {
    'bench_draw_line': [
      [{'memory (mb)': 128.1, 'retval': 0},
       {'memory (mb)': 128.4, 'retval': 0}],
      [{'memory (mb)': 124.3, 'retval': 0},
       {'memory (mb)': 123.9, 'retval': 0}]
    ]
  }
  """
  actually_updated = False
  for bench_results in results.itervalues():
    for platform_results in bench_results:
      for i, result in enumerate(platform_results):
        # Keep the keys that come earliest when sorted alphabetically.
        # Forcing alphabetical order is arbitrary, but necessary; otherwise,
        # the keyvals we'd emit would depend on our iteration order through a
        # map.
        removable_keys = sorted(k for k in result if k != 'retval')
        retained_keys = removable_keys[:max_keys]
        platform_results[i] = {k: result[k] for k in retained_keys}
        # retval needs to be passed through all of the time.
        retval = result.get('retval')
        if retval is not None:
          platform_results[i]['retval'] = retval
        actually_updated = actually_updated or \
          len(retained_keys) != len(removable_keys)

  if actually_updated and complain_on_update:
    print(
        'Warning: Some benchmark keyvals have been truncated.', file=sys.stderr)
  return results


def _ConvertToASCII(obj):
  """Convert an object loaded from JSON to ASCII; JSON gives us unicode."""

  # Using something like `object_hook` is insufficient, since it only fires on
  # actual JSON objects. `encoding` fails, too, since the default decoder always
  # uses unicode() to decode strings.
  if isinstance(obj, unicode):
    return str(obj)
  if isinstance(obj, dict):
    return {_ConvertToASCII(k): _ConvertToASCII(v) for k, v in obj.iteritems()}
  if isinstance(obj, list):
    return [_ConvertToASCII(v) for v in obj]
  return obj


def _PositiveInt(s):
  i = int(s)
  if i < 0:
    raise argparse.ArgumentTypeError('%d is not a positive integer.' % (i,))
  return i


def _AccumulateActions(args):
  """Given program arguments, determines what actions we want to run.

  Returns [(ResultsReportCtor, str)], where ResultsReportCtor can construct a
  ResultsReport, and the str is the file extension for the given report.
  """
  results = []
  # The order of these is arbitrary.
  if args.json:
    results.append((JSONResultsReport, 'json'))
  if args.text:
    results.append((TextResultsReport, 'txt'))
  if args.email:
    email_ctor = functools.partial(TextResultsReport, email=True)
    results.append((email_ctor, 'email'))
  # We emit HTML if nothing else was specified.
  if args.html or not results:
    results.append((HTMLResultsReport, 'html'))
  return results


# Note: get_contents is a function, because it may be expensive (generating some
# HTML reports takes O(seconds) on my machine, depending on the size of the
# input data).
def WriteFile(output_prefix, extension, get_contents, overwrite, verbose):
  """Writes `contents` to a file named "${output_prefix}.${extension}".

  get_contents should be a zero-args function that returns a string (of the
  contents to write).
  If output_prefix == '-', this writes to stdout.
  If overwrite is False, this will not overwrite files.
  """
  if output_prefix == '-':
    if verbose:
      print('Writing %s report to stdout' % (extension,), file=sys.stderr)
    sys.stdout.write(get_contents())
    return

  file_name = '%s.%s' % (output_prefix, extension)
  if not overwrite and os.path.exists(file_name):
    raise IOError('Refusing to write %s -- it already exists' % (file_name,))

  with open(file_name, 'w') as out_file:
    if verbose:
      print('Writing %s report to %s' % (extension, file_name), file=sys.stderr)
    out_file.write(get_contents())


def RunActions(actions, benchmark_results, output_prefix, overwrite, verbose):
  """Runs `actions`, returning True if all succeeded."""
  failed = False

  report_ctor = None  # Make the linter happy
  for report_ctor, extension in actions:
    try:
      get_contents = lambda: report_ctor(benchmark_results).GetReport()
      WriteFile(output_prefix, extension, get_contents, overwrite, verbose)
    except Exception:
      # Complain and move along; we may have more actions that might complete
      # successfully.
      failed = True
      traceback.print_exc()
  return not failed


def PickInputFile(input_name):
  """Given program arguments, returns file to read for benchmark input."""
  return sys.stdin if input_name == '-' else open(input_name)


def _NoPerfReport(_label_name, _benchmark_name, _benchmark_iteration):
  return {}


def _ParseArgs(argv):
  parser = argparse.ArgumentParser(description='Turns JSON into results '
                                   'report(s).')
  parser.add_argument(
      '-v',
      '--verbose',
      action='store_true',
      help='Be a tiny bit more verbose.')
  parser.add_argument(
      '-f',
      '--force',
      action='store_true',
      help='Overwrite existing results files.')
  parser.add_argument(
      '-o',
      '--output',
      default='report',
      type=str,
      help='Prefix of the output filename (default: report). '
      '- means stdout.')
  parser.add_argument(
      '-i',
      '--input',
      required=True,
      type=str,
      help='Where to read the JSON from. - means stdin.')
  parser.add_argument(
      '-l',
      '--statistic-limit',
      default=0,
      type=_PositiveInt,
      help='The maximum number of benchmark statistics to '
      'display from a single run. 0 implies unlimited.')
  parser.add_argument(
      '--json', action='store_true', help='Output a JSON report.')
  parser.add_argument(
      '--text', action='store_true', help='Output a text report.')
  parser.add_argument(
      '--email',
      action='store_true',
      help='Output a text report suitable for email.')
  parser.add_argument(
      '--html',
      action='store_true',
      help='Output an HTML report (this is the default if no '
      'other output format is specified).')
  return parser.parse_args(argv)


def Main(argv):
  args = _ParseArgs(argv)
  # JSON likes to load UTF-8; our results reporter *really* doesn't like
  # UTF-8.
  with PickInputFile(args.input) as in_file:
    raw_results = _ConvertToASCII(json.load(in_file))

  platform_names = raw_results['platforms']
  results = raw_results['data']
  if args.statistic_limit:
    results = CutResultsInPlace(results, max_keys=args.statistic_limit)
  benches = CountBenchmarks(results)
  # In crosperf, a label is essentially a platform+configuration. So, a name of
  # a label and a name of a platform are equivalent for our purposes.
  bench_results = BenchmarkResults(
      label_names=platform_names,
      benchmark_names_and_iterations=benches,
      run_keyvals=results,
      read_perf_report=_NoPerfReport)
  actions = _AccumulateActions(args)
  ok = RunActions(actions, bench_results, args.output, args.force, args.verbose)
  return 0 if ok else 1


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
