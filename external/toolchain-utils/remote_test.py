#!/usr/bin/env python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Script to wrap test_that script.

This script can login to the chromeos machine using the test private key.
"""

from __future__ import print_function

__author__ = 'asharif@google.com (Ahmad Sharif)'

import argparse
import os
import sys

from cros_utils import command_executer
from cros_utils import misc


def Usage(parser, message):
  print('ERROR: %s' % message)
  parser.print_help()
  sys.exit(0)


def Main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '-c',
      '--chromeos_root',
      dest='chromeos_root',
      help='ChromeOS root checkout directory')
  parser.add_argument(
      '-r', '--remote', dest='remote', help='Remote chromeos device.')
  options = parser.parse_args(argv)
  if options.chromeos_root is None:
    Usage(parser, 'chromeos_root must be given')

  if options.remote is None:
    Usage(parser, 'remote must be given')

  options.chromeos_root = os.path.expanduser(options.chromeos_root)

  command = 'ls -lt /'
  ce = command_executer.GetCommandExecuter()
  ce.CrosRunCommand(
      command, chromeos_root=options.chromeos_root, machine=options.remote)

  version_dir_path, script_name = misc.GetRoot(sys.argv[0])
  version_dir = misc.GetRoot(version_dir_path)[1]

  # Tests to copy directories and files to the chromeos box.
  ce.CopyFiles(
      version_dir_path,
      '/tmp/' + version_dir,
      dest_machine=options.remote,
      dest_cros=True,
      chromeos_root=options.chromeos_root)
  ce.CopyFiles(
      version_dir_path,
      '/tmp/' + version_dir + '1',
      dest_machine=options.remote,
      dest_cros=True,
      chromeos_root=options.chromeos_root)
  ce.CopyFiles(
      sys.argv[0],
      '/tmp/' + script_name,
      recursive=False,
      dest_machine=options.remote,
      dest_cros=True,
      chromeos_root=options.chromeos_root)
  ce.CopyFiles(
      sys.argv[0],
      '/tmp/' + script_name + '1',
      recursive=False,
      dest_machine=options.remote,
      dest_cros=True,
      chromeos_root=options.chromeos_root)

  # Test to copy directories and files from the chromeos box.
  ce.CopyFiles(
      '/tmp/' + script_name,
      '/tmp/hello',
      recursive=False,
      src_machine=options.remote,
      src_cros=True,
      chromeos_root=options.chromeos_root)
  ce.CopyFiles(
      '/tmp/' + script_name,
      '/tmp/' + script_name,
      recursive=False,
      src_machine=options.remote,
      src_cros=True,
      chromeos_root=options.chromeos_root)
  board = ce.CrosLearnBoard(options.chromeos_root, options.remote)
  print(board)
  return 0


if __name__ == '__main__':
  Main(sys.argv[1:])
