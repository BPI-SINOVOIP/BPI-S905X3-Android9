#!/usr/bin/python
#
# Copyright 2012 Google Inc. All Rights Reserved.
"""dejagnu_compiler.py: Run dejagnu test."""

__author__ = 'shenhan@google.com  (Han Shen)'

import logging
import optparse
import os
import pickle
import sys
import xmlrpclib

from automation.clients.helper import jobs
from automation.clients.helper import perforce
from automation.common import command as cmd
from automation.common import job_group
from automation.common import logger


class DejagnuCompilerNightlyClient:
  DEPOT2_DIR = '//depot2/'
  P4_CHECKOUT_DIR = 'perforce2/'
  P4_VERSION_DIR = os.path.join(P4_CHECKOUT_DIR, 'gcctools/chromeos/v14')

  def __init__(self, board, remote, p4_snapshot, cleanup):
    self._board = board
    self._remote = remote
    self._p4_snapshot = p4_snapshot
    self._cleanup = cleanup

  def Run(self):
    server = xmlrpclib.Server('http://localhost:8000')
    server.ExecuteJobGroup(pickle.dumps(self.CreateJobGroup()))

  def CheckoutV14Dir(self):
    p4view = perforce.View(self.DEPOT2_DIR, [
        perforce.PathMapping('gcctools/chromeos/v14/...')
    ])
    return self.GetP4Snapshot(p4view)

  def GetP4Snapshot(self, p4view):
    p4client = perforce.CommandsFactory(self.P4_CHECKOUT_DIR, p4view)

    if self._p4_snapshot:
      return p4client.CheckoutFromSnapshot(self._p4_snapshot)
    else:
      return p4client.SetupAndDo(p4client.Sync(), p4client.Remove())

  def CreateJobGroup(self):
    chain = cmd.Chain(self.CheckoutV14Dir(), cmd.Shell(
        'python', os.path.join(self.P4_VERSION_DIR, 'test_gcc_dejagnu.py'),
        '--board=%s' % self._board, '--remote=%s' % self._remote,
        '--cleanup=%s' % self._cleanup))
    label = 'dejagnu'
    job = jobs.CreateLinuxJob(label, chain, timeout=8 * 60 * 60)
    return job_group.JobGroup(label,
                              [job],
                              cleanup_on_failure=True,
                              cleanup_on_completion=True)


@logger.HandleUncaughtExceptions
def Main(argv):
  parser = optparse.OptionParser()
  parser.add_option('-b',
                    '--board',
                    dest='board',
                    help='Run performance tests on these boards')
  parser.add_option('-r',
                    '--remote',
                    dest='remote',
                    help='Run performance tests on these remotes')
  parser.add_option('-p',
                    '--p4_snapshot',
                    dest='p4_snapshot',
                    help=('For development only. '
                          'Use snapshot instead of checking out.'))
  parser.add_option('--cleanup',
                    dest='cleanup',
                    default='mount',
                    help=('Cleanup test directory, values could be one of '
                          '"mount", "chroot" or "chromeos"'))
  options, _ = parser.parse_args(argv)

  if not all([options.board, options.remote]):
    logging.error('Specify a board and remote.')
    return 1

  client = DejagnuCompilerNightlyClient(options.board, options.remote,
                                        options.p4_snapshot, options.cleanup)
  client.Run()
  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv))
