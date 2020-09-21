#!/usr/bin/env python2
"""Change portions of the object files to good.

This file is a test switch script. Used only for the test test_tmp_cleanup.
The "portion" is defined by the file (which is passed as the only argument to
this script) content. Every line in the file is an object index, which will be
set to good (mark as 42).
"""

from __future__ import print_function

import sys

import common


def Main(argv):
  working_set = common.ReadWorkingSet()
  object_index = common.ReadObjectIndex(argv[1])

  # Random number so the results can be checked
  for oi in object_index:
    working_set[int(oi)] = 42

  common.WriteWorkingSet(working_set)
  with open('tmp_file', 'w') as f:
    f.write(argv[1])

  return 0


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
