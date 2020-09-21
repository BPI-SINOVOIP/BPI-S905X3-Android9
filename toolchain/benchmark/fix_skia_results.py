#!/usr/bin/env python2
#
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# pylint: disable=cros-logging-import
"""Transforms skia benchmark results to ones that crosperf can understand."""

from __future__ import print_function

import itertools
import logging
import json
import sys

# Turn the logging level to INFO before importing other autotest
# code, to avoid having failed import logging messages confuse the
# test_droid user.
logging.basicConfig(level=logging.INFO)

# All of the results we care about, by name.
# Each of these *must* end in _ns, _us, _ms, or _s, since all the metrics we
# collect (so far) are related to time, and we alter the results based on the
# suffix of these strings (so we don't have 0.000421ms per sample, for example)
_RESULT_RENAMES = {
    'memset32_100000_640_480_nonrendering': 'memset_time_ms',
    'path_equality_50%_640_480_nonrendering': 'path_equality_ns',
    'sort_qsort_backward_640_480_nonrendering': 'qsort_us'
}


def _GetFamiliarName(name):
    r = _RESULT_RENAMES[name]
    return r if r else name


def _IsResultInteresting(name):
    return name in _RESULT_RENAMES


def _GetTimeMultiplier(label_name):
    """Given a time (in milliseconds), normalize it to what label_name expects.

    "What label_name expects" meaning "we pattern match against the last few
    non-space chars in label_name."

    This expects the time unit to be separated from anything else by '_'.
    """
    ms_mul = 1000 * 1000.
    endings = [('_ns', 1), ('_us', 1000),
               ('_ms', ms_mul), ('_s', ms_mul * 1000)]
    for end, mul in endings:
        if label_name.endswith(end):
            return ms_mul / mul
    raise ValueError('Unknown ending in "%s"; expecting one of %s' %
                     (label_name, [end for end, _ in endings]))


def _GetTimeDenom(ms):
    """Given a list of times (in milliseconds), find a sane time unit for them.

    Returns the unit name, and `ms` normalized to that time unit.

    >>> _GetTimeDenom([1, 2, 3])
    ('ms', [1.0, 2.0, 3.0])
    >>> _GetTimeDenom([.1, .2, .3])
    ('us', [100.0, 200.0, 300.0])
    """

    ms_mul = 1000 * 1000
    units = [('us', 1000), ('ms', ms_mul), ('s', ms_mul * 1000)]
    for name, mul in reversed(units):
        normalized = [float(t) * ms_mul / mul for t in ms]
        average = sum(normalized) / len(normalized)
        if all(n > 0.1 for n in normalized) and average >= 1:
            return name, normalized

    normalized = [float(t) * ms_mul for t in ms]
    return 'ns', normalized


def _TransformBenchmarks(raw_benchmarks):
    # We get {"results": {"bench_name": Results}}
    # where
    #   Results = {"config_name": {"samples": [float], etc.}}
    #
    # We want {"data": {"skia": [[BenchmarkData]]},
    #          "platforms": ["platform1, ..."]}
    # where
    #   BenchmarkData = {"bench_name": bench_samples[N], ..., "retval": 0}
    #
    # Note that retval is awkward -- crosperf's JSON reporter reports the result
    # as a failure if it's not there. Everything else treats it like a
    # statistic...
    benchmarks = raw_benchmarks['results']
    results = []
    for bench_name, bench_result in benchmarks.iteritems():
        try:
            for cfg_name, keyvals in bench_result.iteritems():
                # Some benchmarks won't have timing data (either it won't exist
                # at all, or it'll be empty); skip them.
                samples = keyvals.get('samples')
                if not samples:
                    continue

                bench_name = '%s_%s' % (bench_name, cfg_name)
                if not _IsResultInteresting(bench_name):
                    continue

                friendly_name = _GetFamiliarName(bench_name)
                if len(results) < len(samples):
                    results.extend({
                        'retval': 0
                    } for _ in xrange(len(samples) - len(results)))

                time_mul = _GetTimeMultiplier(friendly_name)
                for sample, app in itertools.izip(samples, results):
                    assert friendly_name not in app
                    app[friendly_name] = sample * time_mul
        except (KeyError, ValueError) as e:
            logging.error('While converting "%s" (key: %s): %s',
                          bench_result, bench_name, e.message)
            raise

    # Realistically, [results] should be multiple results, where each entry in
    # the list is the result for a different label. Because we only deal with
    # one label at the moment, we need to wrap it in its own list.
    return results


if __name__ == '__main__':

    def _GetUserFile(argv):
        if not argv or argv[0] == '-':
            return sys.stdin
        return open(argv[0])

    def _Main():
        with _GetUserFile(sys.argv[1:]) as in_file:
            obj = json.load(in_file)
        output = _TransformBenchmarks(obj)
        json.dump(output, sys.stdout)

    _Main()
