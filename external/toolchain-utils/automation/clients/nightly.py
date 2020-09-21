#!/usr/bin/python
#
# Copyright 2010 Google Inc. All Rights Reserved.

import optparse
import pickle
import sys
import xmlrpclib

from automation.clients.helper import chromeos
from automation.common import job_group


def Main(argv):
  parser = optparse.OptionParser()
  parser.add_option('-c',
                    '--chromeos_version',
                    dest='chromeos_version',
                    default='quarterly',
                    help='ChromeOS version to use.')
  parser.add_option('-t',
                    '--toolchain',
                    dest='toolchain',
                    default='latest-toolchain',
                    help='Toolchain to use {latest-toolchain,gcc_46}.')
  parser.add_option('-b',
                    '--board',
                    dest='board',
                    default='x86-generic',
                    help='Board to use for the nightly job.')
  options = parser.parse_args(argv)[0]

  toolchain = options.toolchain
  board = options.board
  chromeos_version = options.chromeos_version

  # Build toolchain
  jobs_factory = chromeos.JobsFactory(chromeos_version=chromeos_version,
                                      board=board,
                                      toolchain=toolchain)
  benchmark_job = jobs_factory.BuildAndBenchmark()

  group_label = 'nightly_client_%s' % board
  group = job_group.JobGroup(group_label, [benchmark_job], True, False)

  server = xmlrpclib.Server('http://localhost:8000')
  server.ExecuteJobGroup(pickle.dumps(group))


if __name__ == '__main__':
  Main(sys.argv)
