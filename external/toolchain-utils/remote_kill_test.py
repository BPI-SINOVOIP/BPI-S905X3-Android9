#!/usr/bin/env python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Script to wrap test_that script.

Run this script and kill it. Then run ps -ef to see if sleep
is still running,.
"""

from __future__ import print_function

__author__ = 'asharif@google.com (Ahmad Sharif)'

import argparse
import os
import sys

from cros_utils import command_executer


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

  _ = parser.parse_args(argv)
  ce = command_executer.GetCommandExecuter()
  ce.RunCommand('ls; sleep 10000', machine=os.uname()[1])
  return 0


if __name__ == '__main__':
  Main(sys.argv[1:])
