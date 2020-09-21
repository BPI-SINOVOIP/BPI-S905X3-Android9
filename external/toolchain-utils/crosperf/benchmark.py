# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Define a type that wraps a Benchmark instance."""
from __future__ import print_function

import math
from scipy import stats

# See crbug.com/673558 for how these are estimated.
_estimated_stddev = {
    'octane': 0.015,
    'kraken': 0.019,
    'speedometer': 0.007,
    'dromaeo.domcoreattr': 0.023,
    'dromaeo.domcoremodify': 0.011,
    'smoothness.tough_webgl_cases': 0.025,
    'graphics_WebGLAquarium': 0.008,
    'page_cycler_v2.typical_25': 0.021,
}


# Get #samples needed to guarantee a given confidence interval, assuming the
# samples follow normal distribution.
def _samples(b):
  # TODO: Make this an option
  # CI = (0.9, 0.02), i.e., 90% chance that |sample mean - true mean| < 2%.
  p = 0.9
  e = 0.02
  if b not in _estimated_stddev:
    return 1
  d = _estimated_stddev[b]
  # Get at least 2 samples so as to calculate standard deviation, which is
  # needed in T-test for p-value.
  n = int(math.ceil((stats.norm.isf((1 - p) / 2) * d / e)**2))
  return n if n > 1 else 2


class Benchmark(object):
  """Class representing a benchmark to be run.

  Contains details of the benchmark suite, arguments to pass to the suite,
  iterations to run the benchmark suite and so on. Note that the benchmark name
  can be different to the test suite name. For example, you may want to have
  two different benchmarks which run the same test_name with different
  arguments.
  """

  def __init__(self,
               name,
               test_name,
               test_args,
               iterations,
               rm_chroot_tmp,
               perf_args,
               suite='',
               show_all_results=False,
               retries=0,
               run_local=False):
    self.name = name
    #For telemetry, this is the benchmark name.
    self.test_name = test_name
    #For telemetry, this is the data.
    self.test_args = test_args
    self.iterations = iterations if iterations > 0 else _samples(name)
    self.perf_args = perf_args
    self.rm_chroot_tmp = rm_chroot_tmp
    self.iteration_adjusted = False
    self.suite = suite
    self.show_all_results = show_all_results
    self.retries = retries
    if self.suite == 'telemetry':
      self.show_all_results = True
    if run_local and self.suite != 'telemetry_Crosperf':
      raise RuntimeError('run_local is only supported by telemetry_Crosperf.')
    self.run_local = run_local
