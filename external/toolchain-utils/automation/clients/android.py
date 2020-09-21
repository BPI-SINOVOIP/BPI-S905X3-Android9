#!/usr/bin/python
#
# Copyright 2011 Google Inc. All Rights Reserved.
"""Client for Android nightly jobs.

Does the following jobs:
  1. Checkout android toolchain sources
  2. Build Android toolchain
  3. Build Android tree
  4. Checkout/build/run Android benchmarks (TODO)
  5. Generate size/performance dashboard ? (TODO)
"""

__author__ = 'jingyu@google.com (Jing Yu)'

import optparse
import pickle
import sys
import xmlrpclib

from automation.clients.helper import android
from automation.common import job_group
from automation.common import logger


class AndroidToolchainNightlyClient(object):
  VALID_GCC_VERSIONS = ['4.4.3', '4.6', 'google_main', 'fsf_trunk']

  def __init__(self, gcc_version, is_release):
    assert gcc_version in self.VALID_GCC_VERSIONS
    self.gcc_version = gcc_version
    if is_release:
      self.build_type = 'RELEASE'
    else:
      self.build_type = 'DEVELOPMENT'

  def Run(self):
    server = xmlrpclib.Server('http://localhost:8000')
    server.ExecuteJobGroup(pickle.dumps(self.CreateJobGroup()))

  def CreateJobGroup(self):
    factory = android.JobsFactory(self.gcc_version, self.build_type)

    p4_androidtc_job, checkout_dir_dep = factory.CheckoutAndroidToolchain()

    tc_build_job, tc_prefix_dep = factory.BuildAndroidToolchain(
        checkout_dir_dep)

    tree_build_job = factory.BuildAndroidImage(tc_prefix_dep)

    benchmark_job = factory.Benchmark(tc_prefix_dep)

    all_jobs = [p4_androidtc_job, tc_build_job, tree_build_job, benchmark_job]

    return job_group.JobGroup('androidtoolchain_nightly', all_jobs, True, False)


@logger.HandleUncaughtExceptions
def Main(argv):
  valid_gcc_versions_string = ', '.join(
      AndroidToolchainNightlyClient.VALID_GCC_VERSIONS)

  parser = optparse.OptionParser()
  parser.add_option('--with-gcc-version',
                    dest='gcc_version',
                    default='4.6',
                    action='store',
                    choices=AndroidToolchainNightlyClient.VALID_GCC_VERSIONS,
                    help='gcc version: %s.' % valid_gcc_versions_string)
  parser.add_option('-r',
                    '--release',
                    dest='is_release',
                    default=False,
                    action='store_true',
                    help='Build a release toolchain?')
  options, _ = parser.parse_args(argv)

  option_list = [opt.dest for opt in parser.option_list if opt.dest]

  kwargs = dict((option, getattr(options, option)) for option in option_list)

  client = AndroidToolchainNightlyClient(**kwargs)
  client.Run()


if __name__ == '__main__':
  Main(sys.argv)
