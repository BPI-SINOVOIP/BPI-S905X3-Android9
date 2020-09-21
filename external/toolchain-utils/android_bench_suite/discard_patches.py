#!/usr/bin/env python2
#
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script to discard all the patches added to Android for this suite"""

from __future__ import print_function

import config
import os
import subprocess


def discard_git(path):
  try:
    subprocess.check_call(['git', '-C', path, 'reset'])
    subprocess.check_call(['git', '-C', path, 'clean', '-fdx'])
    subprocess.check_call(['git', '-C', path, 'stash'])
    print('Patch in %s removed successfully!' % path)
  except subprocess.CalledProcessError:
    print('Error while removing patch in %s' % path)


def dispatch_skia():
  skia_dir = os.path.join(config.android_home, config.bench_dict['Skia'])
  discard_git(skia_dir)


def dispatch_autotest():
  autotest_dir = os.path.join(config.android_home, config.autotest_dir)
  discard_git(autotest_dir)


def dispatch_panorama():
  panorama_dir = os.path.join(config.android_home,
                              config.bench_dict['Panorama'])
  discard_git(panorama_dir)


def dispatch_synthmark():
  synthmark_dir = 'synthmark'
  try:
    subprocess.check_call(
        ['rm', '-rf',
         os.path.join(config.android_home, synthmark_dir)])
    print('Synthmark patch removed successfully!')
  except subprocess.CalledProcessError:
    print('Synthmark is not removed. Error occurred.')


def main():
  dispatch_skia()
  dispatch_autotest()
  dispatch_panorama()
  dispatch_synthmark()


if __name__ == '__main__':
  main()
