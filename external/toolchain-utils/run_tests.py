#!/usr/bin/env python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Script to wrap run_remote_tests.sh script.

This script calls run_remote_tests.sh with standard tests.
"""

from __future__ import print_function

__author__ = 'asharif@google.com (Ahmad Sharif)'

import sys


def Main():
  """The main function."""
  print('This script is deprecated.  Use crosperf for running tests.')
  return 1


if __name__ == '__main__':
  sys.exit(Main())
