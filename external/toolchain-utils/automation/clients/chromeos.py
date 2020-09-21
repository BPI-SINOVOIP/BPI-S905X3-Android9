#!/usr/bin/python
#
# Copyright 2011 Google Inc. All Rights Reserved.
"""chromeos.py: Build & Test ChromeOS using custom compilers."""

__author__ = 'asharif@google.com (Ahmad Sharif)'

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


class ChromeOSNightlyClient(object):
  DEPOT2_DIR = '//depot2/'
  P4_CHECKOUT_DIR = 'perforce2/'
  P4_VERSION_DIR = os.path.join(P4_CHECKOUT_DIR, 'gcctools/chromeos/v14')

  def __init__(self, board, remote, gcc_githash, p4_snapshot=''):
    self._board = board
    self._remote = remote
    self._gcc_githash = gcc_githash
    self._p4_snapshot = p4_snapshot

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
    chain = cmd.Chain(
        self.CheckoutV14Dir(),
        cmd.Shell('python',
                  os.path.join(self.P4_VERSION_DIR, 'test_toolchains.py'),
                  '--force-mismatch',
                  '--clean',
                  '--public',  # crbug.com/145822
                  '--board=%s' % self._board,
                  '--remote=%s' % self._remote,
                  '--githashes=%s' % self._gcc_githash))
    label = 'testlabel'
    job = jobs.CreateLinuxJob(label, chain, timeout=24 * 60 * 60)

    return job_group.JobGroup(label, [job], True, False)


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
  parser.add_option('-g',
                    '--gcc_githash',
                    dest='gcc_githash',
                    help='Use this gcc_githash.')
  parser.add_option('-p',
                    '--p4_snapshot',
                    dest='p4_snapshot',
                    default='',
                    help='Use this p4_snapshot.')
  options, _ = parser.parse_args(argv)

  if not all([options.board, options.remote, options.gcc_githash]):
    logging.error('Specify a board, remote and gcc_githash')
    return 1

  client = ChromeOSNightlyClient(options.board,
                                 options.remote,
                                 options.gcc_githash,
                                 p4_snapshot=options.p4_snapshot)
  client.Run()
  return 0


if __name__ == '__main__':
  logger.SetUpRootLogger(level=logging.DEBUG, display_flags={'name': False})
  sys.exit(Main(sys.argv))
