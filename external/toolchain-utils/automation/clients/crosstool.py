#!/usr/bin/python
#
# Copyright 2011 Google Inc. All Rights Reserved.

__author__ = 'kbaclawski@google.com (Krystian Baclawski)'

import logging
import optparse
import pickle
import sys
import xmlrpclib

from automation.clients.helper import crosstool
from automation.common import job_group
from automation.common import logger


class CrosstoolNightlyClient(object):
  VALID_TARGETS = ['gcc-4.6.x-ubuntu_lucid-arm',
                   'gcc-4.6.x-ubuntu_lucid-x86_64',
                   'gcc-4.6.x-grtev2-armv7a-vfpv3.d16-hard',
                   'gcc-4.6.x-glibc-2.11.1-grte',
                   'gcc-4.6.x-glibc-2.11.1-powerpc']
  VALID_BOARDS = ['qemu', 'pandaboard', 'unix']

  def __init__(self, target, boards):
    assert target in self.VALID_TARGETS
    assert all(board in self.VALID_BOARDS for board in boards)

    self._target = target
    self._boards = boards

  def Run(self):
    server = xmlrpclib.Server('http://localhost:8000')
    server.ExecuteJobGroup(pickle.dumps(self.CreateJobGroup()))

  def CreateJobGroup(self):
    factory = crosstool.JobsFactory()

    checkout_crosstool_job, checkout_dir, manifests_dir = \
        factory.CheckoutCrosstool(self._target)

    all_jobs = [checkout_crosstool_job]

    # Build crosstool target
    build_release_job, build_tree_dir = factory.BuildRelease(checkout_dir,
                                                             self._target)
    all_jobs.append(build_release_job)

    testruns = []

    # Perform crosstool tests
    for board in self._boards:
      for component in ('gcc', 'binutils'):
        test_job, testrun_dir = factory.RunTests(checkout_dir, build_tree_dir,
                                                 self._target, board, component)
        all_jobs.append(test_job)
        testruns.append(testrun_dir)

    if testruns:
      all_jobs.append(factory.GenerateReport(testruns, manifests_dir,
                                             self._target, self._boards))

    return job_group.JobGroup('Crosstool Nightly Build (%s)' % self._target,
                              all_jobs, True, False)


@logger.HandleUncaughtExceptions
def Main(argv):
  valid_boards_string = ', '.join(CrosstoolNightlyClient.VALID_BOARDS)

  parser = optparse.OptionParser()
  parser.add_option(
      '-b',
      '--board',
      dest='boards',
      action='append',
      choices=CrosstoolNightlyClient.VALID_BOARDS,
      default=[],
      help=('Run DejaGNU tests on selected boards: %s.' % valid_boards_string))
  options, args = parser.parse_args(argv)

  if len(args) == 2:
    target = args[1]
  else:
    logging.error('Exactly one target required as a command line argument!')
    logging.info('List of valid targets:')
    for pair in enumerate(CrosstoolNightlyClient.VALID_TARGETS, start=1):
      logging.info('%d) %s', pair)
    sys.exit(1)

  option_list = [opt.dest for opt in parser.option_list if opt.dest]

  kwargs = dict((option, getattr(options, option)) for option in option_list)

  client = CrosstoolNightlyClient(target, **kwargs)
  client.Run()


if __name__ == '__main__':
  logger.SetUpRootLogger(level=logging.DEBUG, display_flags={'name': False})
  Main(sys.argv)
