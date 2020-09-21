# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""A module to generate experiments."""

from __future__ import print_function
import os
import re
import socket

from benchmark import Benchmark
import config
from experiment import Experiment
from label import Label
from label import MockLabel
from results_cache import CacheConditions
import test_flag
import file_lock_machine

# Users may want to run Telemetry tests either individually, or in
# specified sets.  Here we define sets of tests that users may want
# to run together.

telemetry_perfv2_tests = [
    'dromaeo.domcoreattr', 'dromaeo.domcoremodify', 'dromaeo.domcorequery',
    'dromaeo.domcoretraverse', 'kraken', 'octane', 'robohornet_pro', 'sunspider'
]

telemetry_pagecycler_tests = [
    'page_cycler_v2.intl_ar_fa_he',
    'page_cycler_v2.intl_es_fr_pt-BR',
    'page_cycler_v2.intl_hi_ru',
    'page_cycler_v2.intl_ja_zh',
    'page_cycler_v2.intl_ko_th_vi',
    #                              'page_cycler_v2.morejs',
    #                              'page_cycler_v2.moz',
    #                              'page_cycler_v2.netsim.top_10',
    'page_cycler_v2.tough_layout_cases',
    'page_cycler_v2.typical_25'
]

telemetry_toolchain_old_perf_tests = [
    'dromaeo.domcoremodify', 'page_cycler_v2.intl_es_fr_pt-BR',
    'page_cycler_v2.intl_hi_ru', 'page_cycler_v2.intl_ja_zh',
    'page_cycler_v2.intl_ko_th_vi', 'page_cycler_v2.netsim.top_10',
    'page_cycler_v2.typical_25', 'robohornet_pro', 'spaceport',
    'tab_switching.top_10'
]
telemetry_toolchain_perf_tests = [
    'octane',
    'kraken',
    'speedometer',
    'dromaeo.domcoreattr',
    'dromaeo.domcoremodify',
    'smoothness.tough_webgl_cases',
]
graphics_perf_tests = [
    'graphics_GLBench',
    'graphics_GLMark2',
    'graphics_SanAngeles',
    'graphics_WebGLAquarium',
    'graphics_WebGLPerformance',
]
telemetry_crosbolt_perf_tests = [
    'octane',
    'kraken',
    'speedometer',
    'jetstream',
    'startup.cold.blank_page',
    'smoothness.top_25_smooth',
]
crosbolt_perf_tests = [
    'graphics_WebGLAquarium',
    'video_PlaybackPerf.h264',
    'video_PlaybackPerf.vp9',
    'video_WebRtcPerf',
    'BootPerfServerCrosPerf',
    'power_Resume',
    'video_PlaybackPerf.h264',
    'build_RootFilesystemSize',
]

#    'cheets_AntutuTest',
#    'cheets_PerfBootServer',
#    'cheets_CandyCrushTest',
#    'cheets_LinpackTest',
#]


class ExperimentFactory(object):
  """Factory class for building an Experiment, given an ExperimentFile as input.

  This factory is currently hardcoded to produce an experiment for running
  ChromeOS benchmarks, but the idea is that in the future, other types
  of experiments could be produced.
  """

  def AppendBenchmarkSet(self, benchmarks, benchmark_list, test_args,
                         iterations, rm_chroot_tmp, perf_args, suite,
                         show_all_results, retries, run_local):
    """Add all the tests in a set to the benchmarks list."""
    for test_name in benchmark_list:
      telemetry_benchmark = Benchmark(
          test_name, test_name, test_args, iterations, rm_chroot_tmp, perf_args,
          suite, show_all_results, retries, run_local)
      benchmarks.append(telemetry_benchmark)

  def GetExperiment(self, experiment_file, working_directory, log_dir):
    """Construct an experiment from an experiment file."""
    global_settings = experiment_file.GetGlobalSettings()
    experiment_name = global_settings.GetField('name')
    board = global_settings.GetField('board')
    remote = global_settings.GetField('remote')
    # This is used to remove the ",' from the remote if user
    # add them to the remote string.
    new_remote = []
    if remote:
      for i in remote:
        c = re.sub('["\']', '', i)
        new_remote.append(c)
    remote = new_remote
    chromeos_root = global_settings.GetField('chromeos_root')
    rm_chroot_tmp = global_settings.GetField('rm_chroot_tmp')
    perf_args = global_settings.GetField('perf_args')
    acquire_timeout = global_settings.GetField('acquire_timeout')
    cache_dir = global_settings.GetField('cache_dir')
    cache_only = global_settings.GetField('cache_only')
    config.AddConfig('no_email', global_settings.GetField('no_email'))
    share_cache = global_settings.GetField('share_cache')
    results_dir = global_settings.GetField('results_dir')
    use_file_locks = global_settings.GetField('use_file_locks')
    locks_dir = global_settings.GetField('locks_dir')
    # If we pass a blank locks_dir to the Experiment, it will use the AFE server
    # lock mechanism.  So if the user specified use_file_locks, but did not
    # specify a locks dir, set the locks  dir to the default locks dir in
    # file_lock_machine.
    if use_file_locks and not locks_dir:
      locks_dir = file_lock_machine.Machine.LOCKS_DIR
    chrome_src = global_settings.GetField('chrome_src')
    show_all_results = global_settings.GetField('show_all_results')
    log_level = global_settings.GetField('logging_level')
    if log_level not in ('quiet', 'average', 'verbose'):
      log_level = 'verbose'
    # Default cache hit conditions. The image checksum in the cache and the
    # computed checksum of the image must match. Also a cache file must exist.
    cache_conditions = [
        CacheConditions.CACHE_FILE_EXISTS, CacheConditions.CHECKSUMS_MATCH
    ]
    if global_settings.GetField('rerun_if_failed'):
      cache_conditions.append(CacheConditions.RUN_SUCCEEDED)
    if global_settings.GetField('rerun'):
      cache_conditions.append(CacheConditions.FALSE)
    if global_settings.GetField('same_machine'):
      cache_conditions.append(CacheConditions.SAME_MACHINE_MATCH)
    if global_settings.GetField('same_specs'):
      cache_conditions.append(CacheConditions.MACHINES_MATCH)

    # Construct benchmarks.
    # Some fields are common with global settings. The values are
    # inherited and/or merged with the global settings values.
    benchmarks = []
    all_benchmark_settings = experiment_file.GetSettings('benchmark')
    for benchmark_settings in all_benchmark_settings:
      benchmark_name = benchmark_settings.name
      test_name = benchmark_settings.GetField('test_name')
      if not test_name:
        test_name = benchmark_name
      test_args = benchmark_settings.GetField('test_args')
      iterations = benchmark_settings.GetField('iterations')
      suite = benchmark_settings.GetField('suite')
      retries = benchmark_settings.GetField('retries')
      run_local = benchmark_settings.GetField('run_local')

      if suite == 'telemetry_Crosperf':
        if test_name == 'all_perfv2':
          self.AppendBenchmarkSet(benchmarks, telemetry_perfv2_tests, test_args,
                                  iterations, rm_chroot_tmp, perf_args, suite,
                                  show_all_results, retries, run_local)
        elif test_name == 'all_pagecyclers':
          self.AppendBenchmarkSet(benchmarks, telemetry_pagecycler_tests,
                                  test_args, iterations, rm_chroot_tmp,
                                  perf_args, suite, show_all_results, retries,
                                  run_local)
        elif test_name == 'all_toolchain_perf':
          self.AppendBenchmarkSet(benchmarks, telemetry_toolchain_perf_tests,
                                  test_args, iterations, rm_chroot_tmp,
                                  perf_args, suite, show_all_results, retries,
                                  run_local)
          # Add non-telemetry toolchain-perf benchmarks:
          benchmarks.append(
              Benchmark(
                  'graphics_WebGLAquarium',
                  'graphics_WebGLAquarium',
                  '',
                  iterations,
                  rm_chroot_tmp,
                  perf_args,
                  '',
                  show_all_results,
                  retries,
                  run_local=False))
        elif test_name == 'all_toolchain_perf_old':
          self.AppendBenchmarkSet(benchmarks,
                                  telemetry_toolchain_old_perf_tests, test_args,
                                  iterations, rm_chroot_tmp, perf_args, suite,
                                  show_all_results, retries, run_local)
        else:
          benchmark = Benchmark(test_name, test_name, test_args, iterations,
                                rm_chroot_tmp, perf_args, suite,
                                show_all_results, retries, run_local)
          benchmarks.append(benchmark)
      else:
        if test_name == 'all_graphics_perf':
          self.AppendBenchmarkSet(
              benchmarks,
              graphics_perf_tests,
              '',
              iterations,
              rm_chroot_tmp,
              perf_args,
              '',
              show_all_results,
              retries,
              run_local=False)
        elif test_name == 'all_crosbolt_perf':
          self.AppendBenchmarkSet(benchmarks, telemetry_crosbolt_perf_tests,
                                  test_args, iterations, rm_chroot_tmp,
                                  perf_args, 'telemetry_Crosperf',
                                  show_all_results, retries, run_local)
          self.AppendBenchmarkSet(
              benchmarks,
              crosbolt_perf_tests,
              '',
              iterations,
              rm_chroot_tmp,
              perf_args,
              '',
              show_all_results,
              retries,
              run_local=False)
        else:
          # Add the single benchmark.
          benchmark = Benchmark(
              benchmark_name,
              test_name,
              test_args,
              iterations,
              rm_chroot_tmp,
              perf_args,
              suite,
              show_all_results,
              retries,
              run_local=False)
          benchmarks.append(benchmark)

    if not benchmarks:
      raise RuntimeError('No benchmarks specified')

    # Construct labels.
    # Some fields are common with global settings. The values are
    # inherited and/or merged with the global settings values.
    labels = []
    all_label_settings = experiment_file.GetSettings('label')
    all_remote = list(remote)
    for label_settings in all_label_settings:
      label_name = label_settings.name
      image = label_settings.GetField('chromeos_image')
      autotest_path = label_settings.GetField('autotest_path')
      chromeos_root = label_settings.GetField('chromeos_root')
      my_remote = label_settings.GetField('remote')
      compiler = label_settings.GetField('compiler')
      new_remote = []
      if my_remote:
        for i in my_remote:
          c = re.sub('["\']', '', i)
          new_remote.append(c)
      my_remote = new_remote
      if image == '':
        build = label_settings.GetField('build')
        if len(build) == 0:
          raise RuntimeError("Can not have empty 'build' field!")
        image, autotest_path = label_settings.GetXbuddyPath(
            build, autotest_path, board, chromeos_root, log_level)

      cache_dir = label_settings.GetField('cache_dir')
      chrome_src = label_settings.GetField('chrome_src')

      # TODO(yunlian): We should consolidate code in machine_manager.py
      # to derermine whether we are running from within google or not
      if ('corp.google.com' in socket.gethostname() and
          (not my_remote or
           my_remote == remote and global_settings.GetField('board') != board)):
        my_remote = self.GetDefaultRemotes(board)
      if global_settings.GetField('same_machine') and len(my_remote) > 1:
        raise RuntimeError('Only one remote is allowed when same_machine '
                           'is turned on')
      all_remote += my_remote
      image_args = label_settings.GetField('image_args')
      if test_flag.GetTestMode():
        # pylint: disable=too-many-function-args
        label = MockLabel(label_name, image, autotest_path, chromeos_root,
                          board, my_remote, image_args, cache_dir, cache_only,
                          log_level, compiler, chrome_src)
      else:
        label = Label(label_name, image, autotest_path, chromeos_root, board,
                      my_remote, image_args, cache_dir, cache_only, log_level,
                      compiler, chrome_src)
      labels.append(label)

    if not labels:
      raise RuntimeError('No labels specified')

    email = global_settings.GetField('email')
    all_remote += list(set(my_remote))
    all_remote = list(set(all_remote))
    experiment = Experiment(experiment_name, all_remote, working_directory,
                            chromeos_root, cache_conditions, labels, benchmarks,
                            experiment_file.Canonicalize(), email,
                            acquire_timeout, log_dir, log_level, share_cache,
                            results_dir, locks_dir)

    return experiment

  def GetDefaultRemotes(self, board):
    default_remotes_file = os.path.join(
        os.path.dirname(__file__), 'default_remotes')
    try:
      with open(default_remotes_file) as f:
        for line in f:
          key, v = line.split(':')
          if key.strip() == board:
            remotes = v.strip().split()
            if remotes:
              return remotes
            else:
              raise RuntimeError('There is no remote for {0}'.format(board))
    except IOError:
      # TODO: rethrow instead of throwing different exception.
      raise RuntimeError('IOError while reading file {0}'
                         .format(default_remotes_file))
    else:
      raise RuntimeError('There is not remote for {0}'.format(board))
