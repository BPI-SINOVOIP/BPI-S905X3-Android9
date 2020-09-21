#!/usr/bin/env python2
#
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script for running nightly compiler tests on ChromeOS.

This script launches a buildbot to build ChromeOS with the latest compiler on
a particular board; then it finds and downloads the trybot image and the
corresponding official image, and runs crosperf performance tests comparing
the two.  It then generates a report, emails it to the c-compiler-chrome, as
well as copying the images into the seven-day reports directory.
"""

# Script to test different toolchains against ChromeOS benchmarks.

from __future__ import print_function

import argparse
import datetime
import os
import re
import sys
import time

from cros_utils import command_executer
from cros_utils import logger

from cros_utils import buildbot_utils

# CL that uses LLVM-Next to build the images (includes chrome).
USE_LLVM_NEXT_PATCH = '513590'

CROSTC_ROOT = '/usr/local/google/crostc'
ROLE_ACCOUNT = 'mobiletc-prebuild'
TOOLCHAIN_DIR = os.path.dirname(os.path.realpath(__file__))
MAIL_PROGRAM = '~/var/bin/mail-sheriff'
PENDING_ARCHIVES_DIR = os.path.join(CROSTC_ROOT, 'pending_archives')
NIGHTLY_TESTS_DIR = os.path.join(CROSTC_ROOT, 'nightly_test_reports')

IMAGE_DIR = '{board}-{image_type}'
IMAGE_VERSION_STR = r'{chrome_version}-{tip}\.{branch}\.{branch_branch}'
IMAGE_FS = IMAGE_DIR + '/' + IMAGE_VERSION_STR
TRYBOT_IMAGE_FS = 'trybot-' + IMAGE_FS + '-{build_id}'
PFQ_IMAGE_FS = IMAGE_FS + '-rc1'
IMAGE_RE_GROUPS = {
    'board': r'(?P<board>\S+)',
    'image_type': r'(?P<image_type>\S+)',
    'chrome_version': r'(?P<chrome_version>R\d+)',
    'tip': r'(?P<tip>\d+)',
    'branch': r'(?P<branch>\d+)',
    'branch_branch': r'(?P<branch_branch>\d+)',
    'build_id': r'(?P<build_id>b\d+)'
}
TRYBOT_IMAGE_RE = TRYBOT_IMAGE_FS.format(**IMAGE_RE_GROUPS)


class ToolchainComparator(object):
  """Class for doing the nightly tests work."""

  def __init__(self,
               board,
               remotes,
               chromeos_root,
               weekday,
               patches,
               noschedv2=False):
    self._board = board
    self._remotes = remotes
    self._chromeos_root = chromeos_root
    self._base_dir = os.getcwd()
    self._ce = command_executer.GetCommandExecuter()
    self._l = logger.GetLogger()
    self._build = '%s-release' % board
    self._patches = patches.split(',') if patches else []
    self._patches_string = '_'.join(str(p) for p in self._patches)
    self._noschedv2 = noschedv2

    if not weekday:
      self._weekday = time.strftime('%a')
    else:
      self._weekday = weekday
    timestamp = datetime.datetime.strftime(datetime.datetime.now(),
                                           '%Y-%m-%d_%H:%M:%S')
    self._reports_dir = os.path.join(
        NIGHTLY_TESTS_DIR,
        '%s.%s' % (timestamp, board),)

  def _GetVanillaImageName(self, trybot_image):
    """Given a trybot artifact name, get latest vanilla image name.

    Args:
      trybot_image: artifact name such as
          'trybot-daisy-release/R40-6394.0.0-b1389'

    Returns:
      Latest official image name, e.g. 'daisy-release/R57-9089.0.0'.
    """
    mo = re.search(TRYBOT_IMAGE_RE, trybot_image)
    assert mo
    dirname = IMAGE_DIR.replace('\\', '').format(**mo.groupdict())
    return buildbot_utils.GetLatestImage(self._chromeos_root, dirname)

  def _GetNonAFDOImageName(self, trybot_image):
    """Given a trybot artifact name, get corresponding non-AFDO image name.

    We get the non-AFDO image from the PFQ builders. This image
    is not generated for all the boards and, the closest PFQ image
    was the one build for the previous ChromeOS version (the chrome
    used in the current version is the one validated in the previous
    version).
    The previous ChromeOS does not always exist either. So, we try
    a couple of versions before.

    Args:
      trybot_image: artifact name such as
          'trybot-daisy-release/R40-6394.0.0-b1389'

    Returns:
      Corresponding chrome PFQ image name, e.g.
      'daisy-chrome-pfq/R40-6393.0.0-rc1'.
    """
    mo = re.search(TRYBOT_IMAGE_RE, trybot_image)
    assert mo
    image_dict = mo.groupdict()
    image_dict['image_type'] = 'chrome-pfq'
    for _ in xrange(2):
      image_dict['tip'] = str(int(image_dict['tip']) - 1)
      nonafdo_image = PFQ_IMAGE_FS.replace('\\', '').format(**image_dict)
      if buildbot_utils.DoesImageExist(self._chromeos_root, nonafdo_image):
        return nonafdo_image
    return ''

  def _FinishSetup(self):
    """Make sure testing_rsa file is properly set up."""
    # Fix protections on ssh key
    command = ('chmod 600 /var/cache/chromeos-cache/distfiles/target'
               '/chrome-src-internal/src/third_party/chromite/ssh_keys'
               '/testing_rsa')
    ret_val = self._ce.ChrootRunCommand(self._chromeos_root, command)
    if ret_val != 0:
      raise RuntimeError('chmod for testing_rsa failed')

  def _TestImages(self, trybot_image, vanilla_image, nonafdo_image):
    """Create crosperf experiment file.

    Given the names of the trybot, vanilla and non-AFDO images, create the
    appropriate crosperf experiment file and launch crosperf on it.
    """
    experiment_file_dir = os.path.join(self._chromeos_root, '..', self._weekday)
    experiment_file_name = '%s_toolchain_experiment.txt' % self._board

    compiler_string = 'llvm'
    if USE_LLVM_NEXT_PATCH in self._patches_string:
      experiment_file_name = '%s_llvm_next_experiment.txt' % self._board
      compiler_string = 'llvm_next'

    experiment_file = os.path.join(experiment_file_dir, experiment_file_name)
    experiment_header = """
    board: %s
    remote: %s
    retries: 1
    """ % (self._board, self._remotes)
    experiment_tests = """
    benchmark: all_toolchain_perf {
      suite: telemetry_Crosperf
      iterations: 0
    }

    benchmark: page_cycler_v2.typical_25 {
      suite: telemetry_Crosperf
      iterations: 0
      run_local: False
      retries: 0
    }
    """

    with open(experiment_file, 'w') as f:
      f.write(experiment_header)
      f.write(experiment_tests)

      # Now add vanilla to test file.
      official_image = """
          vanilla_image {
            chromeos_root: %s
            build: %s
            compiler: llvm
          }
          """ % (self._chromeos_root, vanilla_image)
      f.write(official_image)

      # Now add non-AFDO image to test file.
      if nonafdo_image:
        official_nonafdo_image = """
          nonafdo_image {
            chromeos_root: %s
            build: %s
            compiler: llvm
          }
          """ % (self._chromeos_root, nonafdo_image)
        f.write(official_nonafdo_image)

      label_string = '%s_trybot_image' % compiler_string

      # Reuse autotest files from vanilla image for trybot images
      autotest_files = os.path.join('/tmp', vanilla_image, 'autotest_files')
      experiment_image = """
          %s {
            chromeos_root: %s
            build: %s
            autotest_path: %s
            compiler: %s
          }
          """ % (label_string, self._chromeos_root, trybot_image,
                 autotest_files, compiler_string)
      f.write(experiment_image)

    crosperf = os.path.join(TOOLCHAIN_DIR, 'crosperf', 'crosperf')
    noschedv2_opts = '--noschedv2' if self._noschedv2 else ''
    command = ('{crosperf} --no_email=True --results_dir={r_dir} '
               '--json_report=True {noschedv2_opts} {exp_file}').format(
                   crosperf=crosperf,
                   r_dir=self._reports_dir,
                   noschedv2_opts=noschedv2_opts,
                   exp_file=experiment_file)

    ret = self._ce.RunCommand(command)
    if ret != 0:
      raise RuntimeError('Crosperf execution error!')
    else:
      # Copy json report to pending archives directory.
      command = 'cp %s/*.json %s/.' % (self._reports_dir, PENDING_ARCHIVES_DIR)
      ret = self._ce.RunCommand(command)
    return

  def _SendEmail(self):
    """Find email message generated by crosperf and send it."""
    filename = os.path.join(self._reports_dir, 'msg_body.html')
    if (os.path.exists(filename) and
        os.path.exists(os.path.expanduser(MAIL_PROGRAM))):
      email_title = 'buildbot llvm test results'
      if USE_LLVM_NEXT_PATCH in self._patches_string:
        email_title = 'buildbot llvm_next test results'
      command = ('cat %s | %s -s "%s, %s" -team -html' %
                 (filename, MAIL_PROGRAM, email_title, self._board))
      self._ce.RunCommand(command)

  def DoAll(self):
    """Main function inside ToolchainComparator class.

    Launch trybot, get image names, create crosperf experiment file, run
    crosperf, and copy images into seven-day report directories.
    """
    date_str = datetime.date.today()
    description = 'master_%s_%s_%s' % (self._patches_string, self._build,
                                       date_str)
    build_id, trybot_image = buildbot_utils.GetTrybotImage(
        self._chromeos_root,
        self._build,
        self._patches,
        description,
        other_flags=['--notests'],
        build_toolchain=True)

    print('trybot_url: \
       https://uberchromegw.corp.google.com/i/chromiumos.tryserver/builders/release/builds/%s'
          % build_id)
    if len(trybot_image) == 0:
      self._l.LogError('Unable to find trybot_image for %s!' % description)
      return 1

    vanilla_image = self._GetVanillaImageName(trybot_image)
    nonafdo_image = self._GetNonAFDOImageName(trybot_image)

    print('trybot_image: %s' % trybot_image)
    print('vanilla_image: %s' % vanilla_image)
    print('nonafdo_image: %s' % nonafdo_image)

    if os.getlogin() == ROLE_ACCOUNT:
      self._FinishSetup()

    self._TestImages(trybot_image, vanilla_image, nonafdo_image)
    self._SendEmail()
    return 0


def Main(argv):
  """The main function."""

  # Common initializations
  command_executer.InitCommandExecuter()
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--remote', dest='remote', help='Remote machines to run tests on.')
  parser.add_argument(
      '--board', dest='board', default='x86-zgb', help='The target board.')
  parser.add_argument(
      '--chromeos_root',
      dest='chromeos_root',
      help='The chromeos root from which to run tests.')
  parser.add_argument(
      '--weekday',
      default='',
      dest='weekday',
      help='The day of the week for which to run tests.')
  parser.add_argument(
      '--patch',
      dest='patches',
      help='The patches to use for the testing, '
      "seprate the patch numbers with ',' "
      'for more than one patches.')
  parser.add_argument(
      '--noschedv2',
      dest='noschedv2',
      action='store_true',
      default=False,
      help='Pass --noschedv2 to crosperf.')

  options = parser.parse_args(argv[1:])
  if not options.board:
    print('Please give a board.')
    return 1
  if not options.remote:
    print('Please give at least one remote machine.')
    return 1
  if not options.chromeos_root:
    print('Please specify the ChromeOS root directory.')
    return 1

  fc = ToolchainComparator(options.board, options.remote, options.chromeos_root,
                           options.weekday, options.patches, options.noschedv2)
  return fc.DoAll()


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
