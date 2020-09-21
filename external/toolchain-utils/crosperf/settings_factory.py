# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Setting files for global, benchmark and labels."""

from __future__ import print_function

from field import BooleanField
from field import IntegerField
from field import ListField
from field import TextField
from settings import Settings


class BenchmarkSettings(Settings):
  """Settings used to configure individual benchmarks."""

  def __init__(self, name):
    super(BenchmarkSettings, self).__init__(name, 'benchmark')
    self.AddField(
        TextField(
            'test_name',
            description='The name of the test to run. '
            'Defaults to the name of the benchmark.'))
    self.AddField(
        TextField(
            'test_args', description='Arguments to be passed to the '
            'test.'))
    self.AddField(
        IntegerField(
            'iterations',
            required=False,
            default=0,
            description='Number of iterations to run the test. '
            'If not set, will run each benchmark test the optimum number of '
            'times to get a stable result.'))
    self.AddField(
        TextField(
            'suite', default='', description='The type of the benchmark.'))
    self.AddField(
        IntegerField(
            'retries',
            default=0,
            description='Number of times to retry a '
            'benchmark run.'))
    self.AddField(
        BooleanField(
            'run_local',
            description='Run benchmark harness on the DUT. '
            'Currently only compatible with the suite: '
            'telemetry_Crosperf.',
            required=False,
            default=True))


class LabelSettings(Settings):
  """Settings for each label."""

  def __init__(self, name):
    super(LabelSettings, self).__init__(name, 'label')
    self.AddField(
        TextField(
            'chromeos_image',
            required=False,
            description='The path to the image to run tests '
            'on, for local/custom-built images. See the '
            "'build' option for official or trybot images."))
    self.AddField(
        TextField(
            'autotest_path',
            required=False,
            description='Autotest directory path relative to chroot which '
            'has autotest files for the image to run tests requiring autotest '
            'files.'))
    self.AddField(
        TextField(
            'chromeos_root',
            description='The path to a chromeos checkout which '
            'contains a src/scripts directory. Defaults to '
            'the chromeos checkout which contains the '
            'chromeos_image.'))
    self.AddField(
        ListField(
            'remote',
            description='A comma-separated list of IPs of chromeos'
            'devices to run experiments on.'))
    self.AddField(
        TextField(
            'image_args',
            required=False,
            default='',
            description='Extra arguments to pass to '
            'image_chromeos.py.'))
    self.AddField(
        TextField(
            'cache_dir',
            default='',
            description='The cache dir for this image.'))
    self.AddField(
        TextField(
            'compiler',
            default='gcc',
            description='The compiler used to build the '
            'ChromeOS image (gcc or llvm).'))
    self.AddField(
        TextField(
            'chrome_src',
            description='The path to the source of chrome. '
            'This is used to run telemetry benchmarks. '
            'The default one is the src inside chroot.',
            required=False,
            default=''))
    self.AddField(
        TextField(
            'build',
            description='The xbuddy specification for an '
            'official or trybot image to use for tests. '
            "'/remote' is assumed, and the board is given "
            "elsewhere, so omit the '/remote/<board>/' xbuddy "
            'prefix.',
            required=False,
            default=''))


class GlobalSettings(Settings):
  """Settings that apply per-experiment."""

  def __init__(self, name):
    super(GlobalSettings, self).__init__(name, 'global')
    self.AddField(
        TextField(
            'name',
            description='The name of the experiment. Just an '
            'identifier.'))
    self.AddField(
        TextField(
            'board',
            description='The target board for running '
            'experiments on, e.g. x86-alex.'))
    self.AddField(
        ListField(
            'remote',
            description='A comma-separated list of IPs of '
            'chromeos devices to run experiments on.'))
    self.AddField(
        BooleanField(
            'rerun_if_failed',
            description='Whether to re-run failed test runs '
            'or not.',
            default=False))
    self.AddField(
        BooleanField(
            'rm_chroot_tmp',
            default=False,
            description='Whether to remove the test_that '
            'result in the chroot.'))
    self.AddField(
        ListField(
            'email',
            description='Space-separated list of email '
            'addresses to send email to.'))
    self.AddField(
        BooleanField(
            'rerun',
            description='Whether to ignore the cache and '
            'for tests to be re-run.',
            default=False))
    self.AddField(
        BooleanField(
            'same_specs',
            default=True,
            description='Ensure cached runs are run on the '
            'same kind of devices which are specified as a '
            'remote.'))
    self.AddField(
        BooleanField(
            'same_machine',
            default=False,
            description='Ensure cached runs are run on the '
            'same remote.'))
    self.AddField(
        BooleanField(
            'use_file_locks',
            default=False,
            description='Whether to use the file locks '
            'mechanism (deprecated) instead of the AFE '
            'server lock mechanism.'))
    self.AddField(
        IntegerField(
            'iterations',
            required=False,
            default=0,
            description='Number of iterations to run all tests. '
            'If not set, will run each benchmark test the optimum number of '
            'times to get a stable result.'))
    self.AddField(
        TextField(
            'chromeos_root',
            description='The path to a chromeos checkout which '
            'contains a src/scripts directory. Defaults to '
            'the chromeos checkout which contains the '
            'chromeos_image.'))
    self.AddField(
        TextField(
            'logging_level',
            default='average',
            description='The level of logging desired. '
            "Options are 'quiet', 'average', and 'verbose'."))
    self.AddField(
        IntegerField(
            'acquire_timeout',
            default=0,
            description='Number of seconds to wait for '
            'machine before exit if all the machines in '
            'the experiment file are busy. Default is 0.'))
    self.AddField(
        TextField(
            'perf_args',
            default='',
            description='The optional profile command. It '
            'enables perf commands to record perforamance '
            'related counters. It must start with perf '
            'command record or stat followed by arguments.'))
    self.AddField(
        TextField(
            'cache_dir',
            default='',
            description='The abs path of cache dir. '
            'Default is /home/$(whoami)/cros_scratch.'))
    self.AddField(
        BooleanField(
            'cache_only',
            default=False,
            description='Whether to use only cached '
            'results (do not rerun failed tests).'))
    self.AddField(
        BooleanField(
            'no_email',
            default=False,
            description='Whether to disable the email to '
            'user after crosperf finishes.'))
    self.AddField(
        BooleanField(
            'json_report',
            default=False,
            description='Whether to generate a json version '
            'of the report, for archiving.'))
    self.AddField(
        BooleanField(
            'show_all_results',
            default=False,
            description='When running Telemetry tests, '
            'whether to all the results, instead of just '
            'the default (summary) results.'))
    self.AddField(
        TextField(
            'share_cache',
            default='',
            description='Path to alternate cache whose data '
            'you want to use. It accepts multiple directories '
            'separated by a ",".'))
    self.AddField(
        TextField('results_dir', default='', description='The results dir.'))
    self.AddField(
        TextField(
            'locks_dir',
            default='',
            description='An alternate directory to use for '
            'storing/checking machine locks. Using this field '
            'automatically sets use_file_locks to True.\n'
            'WARNING: If you use your own locks directory, '
            'there is no guarantee that someone else might not '
            'hold a lock on the same machine in a different '
            'locks directory.'))
    self.AddField(
        TextField(
            'chrome_src',
            description='The path to the source of chrome. '
            'This is used to run telemetry benchmarks. '
            'The default one is the src inside chroot.',
            required=False,
            default=''))
    self.AddField(
        IntegerField(
            'retries',
            default=0,
            description='Number of times to retry a '
            'benchmark run.'))


class SettingsFactory(object):
  """Factory class for building different types of Settings objects.

  This factory is currently hardcoded to produce settings for ChromeOS
  experiment files. The idea is that in the future, other types
  of settings could be produced.
  """

  def GetSettings(self, name, settings_type):
    if settings_type == 'label' or not settings_type:
      return LabelSettings(name)
    if settings_type == 'global':
      return GlobalSettings(name)
    if settings_type == 'benchmark':
      return BenchmarkSettings(name)

    raise TypeError("Invalid settings type: '%s'." % settings_type)
