#!/usr/bin/python
#
# Copyright 2011 Google Inc. All Rights Reserved.
"""Script to profile a page cycler, and get it back to the host."""

import copy
import optparse
import os
import pickle
import re
import sys
import tempfile
import time

import build_chrome_browser
import cros_login
import lock_machine
import run_tests
from cros_utils import command_executer
from cros_utils import logger
from cros_utils import misc


class CyclerProfiler:
  REMOTE_TMP_DIR = '/tmp'

  def __init__(self, chromeos_root, board, cycler, profile_dir, remote):
    self._chromeos_root = chromeos_root
    self._cycler = cycler
    self._profile_dir = profile_dir
    self._remote = remote
    self._board = board
    self._ce = command_executer.GetCommandExecuter()
    self._l = logger.GetLogger()

    self._gcov_prefix = os.path.join(self.REMOTE_TMP_DIR, self._GetProfileDir())

  def _GetProfileDir(self):
    return misc.GetCtargetFromBoard(self._board, self._chromeos_root)

  def _CopyTestData(self):
    page_cycler_dir = os.path.join(self._chromeos_root, 'distfiles', 'target',
                                   'chrome-src-internal', 'src', 'data',
                                   'page_cycler')
    if not os.path.isdir(page_cycler_dir):
      raise RuntimeError('Page cycler dir %s not found!' % page_cycler_dir)
    self._ce.CopyFiles(page_cycler_dir,
                       os.path.join(self.REMOTE_TMP_DIR, 'page_cycler'),
                       dest_machine=self._remote,
                       chromeos_root=self._chromeos_root,
                       recursive=True,
                       dest_cros=True)

  def _PrepareTestData(self):
    # chmod files so everyone can read them.
    command = ('cd %s && find page_cycler -type f | xargs chmod a+r' %
               self.REMOTE_TMP_DIR)
    self._ce.CrosRunCommand(command,
                            chromeos_root=self._chromeos_root,
                            machine=self._remote)
    command = ('cd %s && find page_cycler -type d | xargs chmod a+rx' %
               self.REMOTE_TMP_DIR)
    self._ce.CrosRunCommand(command,
                            chromeos_root=self._chromeos_root,
                            machine=self._remote)

  def _CopyProfileToHost(self):
    dest_dir = os.path.join(self._profile_dir,
                            os.path.basename(self._gcov_prefix))
    # First remove the dir if it exists already
    if os.path.exists(dest_dir):
      command = 'rm -rf %s' % dest_dir
      self._ce.RunCommand(command)

    # Strip out the initial prefix for the Chrome directory before doing the
    # copy.
    chrome_dir_prefix = misc.GetChromeSrcDir()

    command = 'mkdir -p %s' % dest_dir
    self._ce.RunCommand(command)
    self._ce.CopyFiles(self._gcov_prefix,
                       dest_dir,
                       src_machine=self._remote,
                       chromeos_root=self._chromeos_root,
                       recursive=True,
                       src_cros=True)

  def _RemoveRemoteProfileDir(self):
    command = 'rm -rf %s' % self._gcov_prefix
    self._ce.CrosRunCommand(command,
                            chromeos_root=self._chromeos_root,
                            machine=self._remote)

  def _LaunchCycler(self, cycler):
    command = (
        'DISPLAY=:0 '
        'XAUTHORITY=/home/chronos/.Xauthority '
        'GCOV_PREFIX=%s '
        'GCOV_PREFIX_STRIP=3 '
        '/opt/google/chrome/chrome '
        '--no-sandbox '
        '--renderer-clean-exit '
        '--user-data-dir=$(mktemp -d) '
        "--url \"file:///%s/page_cycler/%s/start.html?iterations=10&auto=1\" "
        '--enable-file-cookies '
        '--no-first-run '
        '--js-flags=expose_gc &' % (self._gcov_prefix, self.REMOTE_TMP_DIR,
                                    cycler))

    self._ce.CrosRunCommand(command,
                            chromeos_root=self._chromeos_root,
                            machine=self._remote,
                            command_timeout=60)

  def _PkillChrome(self, signal='9'):
    command = 'pkill -%s chrome' % signal
    self._ce.CrosRunCommand(command,
                            chromeos_root=self._chromeos_root,
                            machine=self._remote)

  def DoProfile(self):
    # Copy the page cycler data to the remote
    self._CopyTestData()
    self._PrepareTestData()
    self._RemoveRemoteProfileDir()

    for cycler in self._cycler.split(','):
      self._ProfileOneCycler(cycler)

    # Copy the profile back
    self._CopyProfileToHost()

  def _ProfileOneCycler(self, cycler):
    # With aura, all that's needed is a stop/start ui.
    self._PkillChrome()
    cros_login.RestartUI(self._remote, self._chromeos_root, login=False)
    # Run the cycler
    self._LaunchCycler(cycler)
    self._PkillChrome(signal='INT')
    # Let libgcov dump the profile.
    # TODO(asharif): There is a race condition here. Fix it later.
    time.sleep(30)


def Main(argv):
  """The main function."""
  # Common initializations
  ###  command_executer.InitCommandExecuter(True)
  command_executer.InitCommandExecuter()
  l = logger.GetLogger()
  ce = command_executer.GetCommandExecuter()
  parser = optparse.OptionParser()
  parser.add_option('--cycler',
                    dest='cycler',
                    default='alexa_us',
                    help=('Comma-separated cyclers to profile. '
                          'Example: alexa_us,moz,moz2'
                          'Use all to profile all cyclers.'))
  parser.add_option('--chromeos_root',
                    dest='chromeos_root',
                    default='../../',
                    help='Output profile directory.')
  parser.add_option('--board',
                    dest='board',
                    default='x86-zgb',
                    help='The target board.')
  parser.add_option('--remote',
                    dest='remote',
                    help=('The remote chromeos machine that'
                          ' has the profile image.'))
  parser.add_option('--profile_dir',
                    dest='profile_dir',
                    default='profile_dir',
                    help='Store profiles in this directory.')

  options, _ = parser.parse_args(argv)

  all_cyclers = ['alexa_us', 'bloat', 'dhtml', 'dom', 'intl1', 'intl2',
                 'morejs', 'morejsnp', 'moz', 'moz2']

  if options.cycler == 'all':
    options.cycler = ','.join(all_cyclers)

  try:
    cp = CyclerProfiler(options.chromeos_root, options.board, options.cycler,
                        options.profile_dir, options.remote)
    cp.DoProfile()
    retval = 0
  except Exception as e:
    retval = 1
    print e
  finally:
    print 'Exiting...'
  return retval


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
