#!/usr/bin/env python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Timeout test for command_executer."""

from __future__ import print_function

__author__ = 'asharif@google.com (Ahmad Sharif)'

import argparse
import sys

from cros_utils import command_executer


def Usage(parser, message):
  print('ERROR: %s' % message)
  parser.print_help()
  sys.exit(0)


def Main(argv):
  parser = argparse.ArgumentParser()
  _ = parser.parse_args(argv)

  command = 'sleep 1000'
  ce = command_executer.GetCommandExecuter()
  ce.RunCommand(command, command_timeout=1)
  return 0


if __name__ == '__main__':
  Main(sys.argv[1:])
