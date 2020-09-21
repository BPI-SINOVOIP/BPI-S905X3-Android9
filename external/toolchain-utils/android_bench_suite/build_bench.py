#!/usr/bin/env python2
#
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# pylint: disable=cros-logging-import

"""Script to build the benchmark locally with toolchain settings."""
from __future__ import print_function

import argparse
import config
import logging
import os
import subprocess
import sys

# Turn the logging level to INFO before importing other code, to avoid having
# failed import logging messages confuse the user.
logging.basicConfig(level=logging.INFO)


def _parse_arguments_internal(argv):
  parser = argparse.ArgumentParser(description='Build benchmarks with '
                                   'specified toolchain settings')

  parser.add_argument(
      '-b', '--bench', required=True, help='Select the benchmark to be built.')

  parser.add_argument(
      '-c',
      '--compiler_dir',
      metavar='DIR',
      help='Specify the path to the compiler bin '
      'directory.')

  parser.add_argument(
      '-o', '--build_os', help='Specify the host OS to build benchmark.')

  parser.add_argument(
      '-l',
      '--llvm_prebuilts_version',
      help='Specify the version of prebuilt LLVM.')

  parser.add_argument(
      '-f',
      '--cflags',
      help='Specify the optimization cflags for '
      'the toolchain.')

  parser.add_argument(
      '--ldflags', help='Specify linker flags for the toolchain.')

  return parser.parse_args(argv)


# Set flags for compiling benchmarks, by changing the local
# CFLAGS/LDFLAGS in the android makefile of each benchmark
def set_flags(bench, cflags, ldflags):
  if not cflags:
    logging.info('No CFLAGS specified, using default settings.')
    cflags = ''
  else:
    logging.info('Cflags setting to "%s"...', cflags)

  if not ldflags:
    logging.info('No LDFLAGS specifed, using default settings.')
    ldflags = ''
  else:
    logging.info('Ldflags setting to "%s"...', ldflags)

  add_flags = config.bench_flags_dict[bench]
  add_flags(cflags, ldflags)
  logging.info('Flags set successfully!')


def set_build_os(build_os):
  # Set $BUILD_OS variable for android makefile
  if build_os:
    os.environ['BUILD_OS'] = build_os
    logging.info('BUILD_OS set to "%s"...', build_os)
  else:
    logging.info('No BUILD_OS specified, using linux as default...')


def set_llvm_prebuilts_version(llvm_prebuilts_version):
  # Set $LLVM_PREBUILTS_VERSION for android makefile
  if llvm_prebuilts_version:
    os.environ['LLVM_PREBUILTS_VERSION'] = llvm_prebuilts_version
    logging.info('LLVM_PREBUILTS_VERSION set to "%s"...',
                 llvm_prebuilts_version)
  else:
    logging.info('No LLVM_PREBUILTS_VERSION specified, using default one...')


def set_compiler(compiler):
  # If compiler_dir has been specified, copy the binaries to
  # a temporary location, set BUILD_OS and LLVM_PREBUILTS_VERSION
  # variables to the location
  if compiler:
    # Report error if path not exits
    if not os.path.isdir(compiler):
      logging.error('Error while setting compiler: '
                    'Directory %s does not exist!', compiler)
      raise OSError('Directory %s not exist.' % compiler)

    # Specify temporary directory for compiler
    tmp_dir = os.path.join(config.android_home,
                           'prebuilts/clang/host/linux-x86', 'clang-tmp')

    compiler_content = os.path.join(compiler, '.')

    # Copy compiler to new directory
    try:
      subprocess.check_call(['cp', '-rf', compiler_content, tmp_dir])
    except subprocess.CalledProcessError:
      logging.error('Error while copying the compiler to '
                    'temporary directory %s!', tmp_dir)
      raise

    # Set environment variable
    os.environ['LLVM_PREBUILTS_VERSION'] = 'clang-tmp'

    logging.info('Prebuilt Compiler set as %s.', os.path.abspath(compiler))


def set_compiler_env(bench, compiler, build_os, llvm_prebuilts_version, cflags,
                     ldflags):
  logging.info('Setting compiler options for benchmark...')

  # If no specific prebuilt compiler directory, use BUILD_OS and
  # LLVM_PREBUILTS_VERSION to set the compiler version.
  # Otherwise, use the new prebuilt compiler.
  if not compiler:
    set_build_os(build_os)
    set_llvm_prebuilts_version(llvm_prebuilts_version)
  else:
    set_compiler(compiler)

  set_flags(bench, cflags, ldflags)

  return 0


def remove_tmp_dir():
  tmp_dir = os.path.join(config.android_home, 'prebuilts/clang/host/linux-x86',
                         'clang-tmp')

  try:
    subprocess.check_call(['rm', '-r', tmp_dir])
  except subprocess.CalledProcessError:
    logging.error('Error while removing the temporary '
                  'compiler directory %s!', tmp_dir)
    raise


# Recover the makefile/blueprint from our patch after building
def restore_makefile(bench):
  pwd = os.path.join(config.android_home, config.bench_dict[bench])
  mk_file = os.path.join(pwd, 'Android.mk')
  if not os.path.exists(mk_file):
    mk_file = os.path.join(pwd, 'Android.bp')
  subprocess.check_call(['mv', os.path.join(pwd, 'tmp_makefile'), mk_file])


# Run script to build benchmark
def build_bench(bench, source_dir):
  logging.info('Start building benchmark...')

  raw_cmd = ('cd {android_home} '
             '&& source build/envsetup.sh '
             '&& lunch {product_combo} '
             '&& mmma {source_dir} -j48'.format(
                 android_home=config.android_home,
                 product_combo=config.product_combo,
                 source_dir=source_dir))

  log_file = os.path.join(config.bench_suite_dir, 'build_log')
  with open(log_file, 'a') as logfile:
    log_head = 'Log for building benchmark: %s\n' % (bench)
    logfile.write(log_head)
    try:
      subprocess.check_call(
          ['bash', '-c', raw_cmd], stdout=logfile, stderr=logfile)
    except subprocess.CalledProcessError:
      logging.error('Error while running %s, please check '
                    '%s for more info.', raw_cmd, log_file)
      restore_makefile(bench)
      raise

  logging.info('Logs for building benchmark %s are written to %s.', bench,
               log_file)
  logging.info('Benchmark built successfully!')


def main(argv):
  arguments = _parse_arguments_internal(argv)

  bench = arguments.bench
  compiler = arguments.compiler_dir
  build_os = arguments.build_os
  llvm_version = arguments.llvm_prebuilts_version
  cflags = arguments.cflags
  ldflags = arguments.ldflags

  try:
    source_dir = config.bench_dict[bench]
  except KeyError:
    logging.error('Please select one benchmark from the list below:\n\t' +
                  '\n\t'.join(config.bench_list))
    raise

  set_compiler_env(bench, compiler, build_os, llvm_version, cflags, ldflags)

  build_bench(bench, source_dir)

  # If flags has been set, remember to restore the makefile/blueprint to
  # original ones.
  restore_makefile(bench)

  # If a tmp directory is used for compiler path, remove it after building.
  if compiler:
    remove_tmp_dir()


if __name__ == '__main__':
  main(sys.argv[1:])
