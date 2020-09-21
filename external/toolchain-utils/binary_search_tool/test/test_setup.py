#!/usr/bin/env python2
"""Emulate running of test setup script, is_good.py should fail without this."""

from __future__ import print_function

import sys


def Main():
  # create ./is_setup
  with open('./is_setup', 'w'):
    pass

  return 0


if __name__ == '__main__':
  retval = Main()
  sys.exit(retval)
