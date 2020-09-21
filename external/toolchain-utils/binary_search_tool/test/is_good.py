#!/usr/bin/env python2
"""Check to see if the working set produces a good executable."""

from __future__ import print_function

import os
import sys

import common


def Main():
  if not os.path.exists('./is_setup'):
    return 1
  working_set = common.ReadWorkingSet()
  for w in working_set:
    if w == 1:
      return 1  ## False, linking failure
  return 0


if __name__ == '__main__':
  retval = Main()
  sys.exit(retval)
