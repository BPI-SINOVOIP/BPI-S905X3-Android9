# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Helper functions to put user defined flags to mk/bp files"""

from __future__ import print_function

import config
import os
import subprocess


# Find the makefile/blueprint based on the benchmark, and make a copy of
# it for restoring later.
def backup_file(bench, file_type):
  mk_file = os.path.join(config.android_home, config.bench_dict[bench],
                         'Android.' + file_type)
  try:
    # Make a copy of the makefile/blueprint so that we can recover it after
    # building the benchmark
    subprocess.check_call([
        'cp', mk_file,
        os.path.join(config.android_home, config.bench_dict[bench],
                     'tmp_makefile')
    ])
  except subprocess.CalledProcessError():
    raise OSError('Cannot backup Android.%s file for %s' % (file_type, bench))


# Insert lines to add LOCAL_CFLAGS/LOCAL_LDFLAGS to the benchmarks
# makefile/blueprint
def replace_flags(bench, android_type, file_type, cflags, ldflags):
  # Use format ["Flag1", "Flag2"] for bp file
  if file_type == 'bp':
    if cflags:
      cflags = '\", \"'.join(cflags.split())
    if ldflags:
      ldflags = '\", \"'.join(ldflags.split())

  if not cflags:
    cflags = ''
  else:
    cflags = '\"' + cflags + '\",'
  if not ldflags:
    ldflags = ''
  else:
    ldflags = '\"' + ldflags + '\",'

  # Two different diffs are used for aosp or internal android repo.
  if android_type == 'aosp':
    bench_diff = bench + '_flags_aosp.diff'
  else:
    bench_diff = bench + '_flags_internal.diff'

  # Replace CFLAGS_FOR_BENCH_SUITE marker with proper cflags
  output = ''
  with open(bench_diff) as f:
    for line in f:
      line = line.replace('CFLAGS_FOR_BENCH_SUITE', cflags)
      line = line.replace('LDFLAGS_FOR_BENCH_SUITE', ldflags)
      output += line

  with open('modified.diff', 'w') as f:
    f.write(output)


def apply_patches(bench):
  bench_dir = os.path.join(config.android_home, config.bench_dict[bench])
  bench_diff = 'modified.diff'
  flags_patch = os.path.join(
      os.path.dirname(os.path.realpath(__file__)), bench_diff)
  try:
    subprocess.check_call(['git', '-C', bench_dir, 'apply', flags_patch])
  except subprocess.CalledProcessError:
    raise OSError('Patch for adding flags for %s does not succeed.' % (bench))


def replace_flags_in_dir(bench, cflags, ldflags):
  bench_mk = os.path.join(config.android_home, config.bench_dict[bench],
                          'Android.mk')

  if not cflags:
    cflags = ''
  if not ldflags:
    ldflags = ''

  output = ''
  with open(bench_mk) as f:
    for line in f:
      line = line.replace('$(CFLAGS_FOR_BENCH_SUITE)', cflags)
      line = line.replace('$(LDFLAGS_FOR_BENCH_SUITE)', ldflags)
      output += line
  with open(bench_mk, 'w') as f:
    f.write(output)


def add_flags_Panorama(cflags, ldflags):
  backup_file('Panorama', 'mk')
  replace_flags_in_dir('Panorama', cflags, ldflags)


def add_flags_Synthmark(cflags, ldflags):
  backup_file('Synthmark', 'mk')
  replace_flags_in_dir('Synthmark', cflags, ldflags)


def add_flags_Skia(cflags, ldflags):
  backup_file('Skia', 'bp')
  replace_flags('Skia', config.android_type, 'bp', cflags, ldflags)
  apply_patches('Skia')


def add_flags_Binder(cflags, ldflags):
  backup_file('Binder', 'bp')
  replace_flags('Binder', config.android_type, 'bp', cflags, ldflags)
  apply_patches('Binder')


def add_flags_Hwui(cflags, ldflags):
  backup_file('Hwui', 'bp')
  replace_flags('Hwui', config.android_type, 'bp', cflags, ldflags)
  apply_patches('Hwui')


def add_flags_Dex2oat(cflags, ldflags):
  backup_file('Dex2oat', 'bp')
  replace_flags('Dex2oat', config.android_type, 'bp', cflags, ldflags)
  apply_patches('Dex2oat')
