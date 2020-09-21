#!/usr/bin/env python2
"""Run full bisection test."""

from __future__ import print_function

import argparse
import os
import sys

from cros_utils import command_executer

TEST_DIR = 'full_bisect_test'
DEFAULT_BISECT_DIR = '/tmp/sysroot_bisect'


def populate_good_files(top_dir, ce, bisect_dir=DEFAULT_BISECT_DIR):
  # 'make clean'
  work_dir = os.path.join(top_dir, TEST_DIR, 'work')
  cmd = 'rm -f %s/*.o' % work_dir
  status = ce.RunCommand(cmd)
  if status != 0:
    print('Error trying to clean out work directory: %s' % cmd)
    return status

  # set up the 'good' source files
  script = os.path.join(top_dir, TEST_DIR, 'make_sources_good.sh')
  status = ce.RunCommand(script)
  if status != 0:
    print('Error setting up "good" source files: %s' % script)
    return status

  export_bisect = 'export BISECT_DIR=%s; ' % bisect_dir
  # build the good source files
  script_path = os.path.join(top_dir, TEST_DIR)
  if os.path.exists('/usr/bin/x86_64-cros-linux-gnu-gcc'):
    build_script = 'chromeos_build.sh'
  else:
    build_script = 'build.sh'
  cmd = ('%s export BISECT_STAGE=POPULATE_GOOD; pushd %s; ./%s; popd' %
         (export_bisect, script_path, build_script))
  status = ce.RunCommand(cmd)
  return status


def populate_bad_files(top_dir, ce, bisect_dir=DEFAULT_BISECT_DIR):
  # 'make clean'
  work_dir = os.path.join(top_dir, TEST_DIR, 'work')
  cmd = 'rm -f %s/*.o' % work_dir
  status = ce.RunCommand(cmd)
  if status != 0:
    print('Error trying to clean out work directory: %s' % cmd)
    return status

  # set up the 'bad' source files
  script = os.path.join(top_dir, TEST_DIR, 'make_sources_bad.sh')
  status = ce.RunCommand(script)
  if status != 0:
    print('Error setting up "bad" source files: %s' % script)
    return status

  export_bisect = 'export BISECT_DIR=%s; ' % bisect_dir
  # build the bad source files
  script_path = os.path.join(top_dir, TEST_DIR)
  if os.path.exists('/usr/bin/x86_64-cros-linux-gnu-gcc'):
    build_script = 'chromeos_build.sh'
  else:
    build_script = 'build.sh'
  cmd = ('%s export BISECT_STAGE=POPULATE_BAD; pushd %s; ./%s ; popd' %
         (export_bisect, script_path, build_script))
  status = ce.RunCommand(cmd)
  return status


def run_main_bisection_test(top_dir, ce):
  test_script = os.path.join(top_dir, TEST_DIR, 'main-bisect-test.sh')
  status = ce.RunCommand(test_script)
  return status


def verify_compiler_and_wrapper():
  # We don't need to do any special setup if running inside a ChromeOS
  # chroot.
  if os.path.exists('/usr/bin/x86_64-cros-linux-gnu-gcc'):
    return

  message = """
*** IMPORTANT --- READ THIS CAREFULLY!! ***

This test uses the command 'gcc' to compile the good/bad versions of the
source program.  BEFORE you can run this script you must make sure that
your compiler wrapper is in the right place, with the right name, so that
a call to 'gcc' will go through your compiler wrapper and "do the right
thing".

Is your compiler wrapper properly set up? [Y/n]
"""

  print(message)
  inp = sys.stdin.readline()
  inp = inp.strip()
  inp = inp.lower()
  return not inp or inp == 'y' or inp == 'yes'


def Main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--dir',
      dest='directory',
      help='Bisection work tree, where good  & bad object '
      'files go.  Default is /tmp/sysroot_bisect')

  options = parser.parse_args(argv)

  # Make sure the compiler wrapper & soft links are properly set up.
  wrapper_is_setup = verify_compiler_and_wrapper()
  if not wrapper_is_setup:
    print('Exiting now.  Please re-run after you have set up the compiler '
          'wrapper.')
    return 0

  # Make sure we're in the correct directory for running this test.
  cwd = os.getcwd()
  if not os.path.exists(os.path.join(cwd, 'full_bisect_test')):
    print('Error:  Wrong directory.  This script must be run from the top level'
          ' of the binary_search_tool tree (under toolchain_utils).')
    return 1

  ce = command_executer.GetCommandExecuter()
  bisect_dir = options.directory
  if not bisect_dir:
    bisect_dir = DEFAULT_BISECT_DIR

  # Make sure BISECT_DIR is clean
  if os.path.exists(bisect_dir):
    cmd = 'rm -Rf %s/*' % bisect_dir
    retv = ce.RunCommand(cmd)
    if retv != 0:
      return retv

  retv = populate_good_files(cwd, ce, bisect_dir)
  if retv != 0:
    return retv

  retv = populate_bad_files(cwd, ce, bisect_dir)
  if retv != 0:
    return retv

  # Set up good/bad work soft links
  cmd = ('rm -f %s/%s/good-objects; ln -s %s/good %s/%s/good-objects' %
         (cwd, TEST_DIR, bisect_dir, cwd, TEST_DIR))

  status = ce.RunCommand(cmd)
  if status != 0:
    print('Error executing: %s; exiting now.' % cmd)
    return status

  cmd = ('rm -f %s/%s/bad-objects; ln -s %s/bad %s/%s/bad-objects' %
         (cwd, TEST_DIR, bisect_dir, cwd, TEST_DIR))

  status = ce.RunCommand(cmd)
  if status != 0:
    print('Error executing: %s; exiting now.' % cmd)
    return status

  retv = run_main_bisection_test(cwd, ce)
  return retv


if __name__ == '__main__':
  retval = Main(sys.argv[1:])
  sys.exit(retval)
