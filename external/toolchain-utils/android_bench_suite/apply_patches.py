#!/usr/bin/env python2
#
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script to patch Android repo with diffs that are needed by the suite.

Run this script before running the suite.
"""
from __future__ import print_function

import config
import os
import subprocess

# The patches to be added to the android repo.
# An error may occur if it is already patched, or meets some error.
# FIXME: Needs to be FIXED in the future.
def try_patch_skia():
  skia_dir = os.path.join(config.android_home, config.bench_dict['Skia'])
  # You may want to change the file based on aosp or internal
  if config.android_type == 'internal':
    print('No need to patch skia for internal repo.')
    return
  elif config.android_type == 'aosp':
    skia_patch = os.path.join(
        os.path.dirname(os.path.realpath(__file__)), 'skia_aosp.diff')
  else:
    raise ValueError('Adnroid source type should be either aosp or internal.')
  # FIXME: A quick hack, need to handle errors and check whether has been
  # applied in the future.
  try:
    subprocess.check_call(['git', '-C', skia_dir, 'apply', skia_patch])
    print('Skia patched successfully!')
  except subprocess.CalledProcessError:
    print('Skia patch not applied, error or already patched.')


def try_patch_autotest():
  # Patch autotest, which includes all the testcases on device, setting device,
  # and running the benchmarks
  autotest_dir = os.path.join(config.android_home, config.autotest_dir)
  autotest_patch = os.path.join(
      os.path.dirname(os.path.realpath(__file__)), 'autotest.diff')
  dex2oat_dir = os.path.join(autotest_dir, 'server/site_tests/android_Dex2oat')
  panorama_dir = os.path.join(autotest_dir,
                              'server/site_tests/android_Panorama')
  # FIXME: A quick hack, need to handle errors and check whether has been
  # applied in the future.
  try:
    subprocess.check_call(['git', '-C', autotest_dir, 'apply', autotest_patch])
    subprocess.check_call(['cp', '-rf', 'dex2oat_input', dex2oat_dir])
    subprocess.check_call(['cp', '-rf', 'panorama_input', panorama_dir])
    print('Autotest patched successfully!')
  except subprocess.CalledProcessError:
    print('Autotest patch not applied, error or already patched.')


def try_patch_panorama():
  panorama_dir = os.path.join(config.android_home,
                              config.bench_dict['Panorama'])
  panorama_patch = os.path.join(
      os.path.dirname(os.path.realpath(__file__)), 'panorama.diff')
  # FIXME: A quick hack, need to handle errors and check whether has been
  # applied in the future.
  try:
    subprocess.check_call(['git', '-C', panorama_dir, 'apply', panorama_patch])
    print('Panorama patched successfully!')
  except subprocess.CalledProcessError:
    print('Panorama patch not applied, error or already patched.')


def try_patch_synthmark():
  synthmark_dir = 'devrel/tools/synthmark'
  # FIXME: A quick hack, need to handle errors and check whether has been
  # applied in the future.
  try:
    subprocess.check_call([
        'bash', '-c', 'mkdir devrel && '
        'cd devrel && '
        'repo init -u sso://devrel/manifest && '
        'repo sync tools/synthmark'
    ])
    synthmark_patch = os.path.join(
        os.path.dirname(os.path.realpath(__file__)), 'synthmark.diff')
    subprocess.check_call(['git', '-C', synthmark_dir,
                           'apply', synthmark_patch])

    subprocess.check_call(['mv', '-f', synthmark_dir, config.android_home])
    subprocess.check_call(['rm', '-rf', 'devrel'])
    print('Synthmark patched successfully!')
  except subprocess.CalledProcessError:
    print('Synthmark patch not applied, error or already patched.')


def main():
  try_patch_skia()
  try_patch_autotest()
  try_patch_panorama()
  try_patch_synthmark()


if __name__ == '__main__':
  main()
