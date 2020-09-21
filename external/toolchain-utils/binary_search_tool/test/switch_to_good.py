#!/usr/bin/env python2
"""Change portions of the object files to good.

The "portion" is defined by the file (which is passed as the only argument to
this script) content. Every line in the file is an object index, which will be
set to good (mark as 0).
"""

from __future__ import print_function

import sys

import common


def Main(argv):
  working_set = common.ReadWorkingSet()
  object_index = common.ReadObjectIndex(argv[1])

  for oi in object_index:
    working_set[int(oi)] = 0

  common.WriteWorkingSet(working_set)

  return 0


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
