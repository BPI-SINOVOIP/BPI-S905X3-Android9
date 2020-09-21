#!/usr/bin/env python2
"""Switch part of the objects file in working set to (possible) bad ones."""

from __future__ import print_function

import sys

import common


def Main(argv):
  """Switch part of the objects file in working set to (possible) bad ones."""
  working_set = common.ReadWorkingSet()
  objects_file = common.ReadObjectsFile()
  object_index = common.ReadObjectIndex(argv[1])

  for oi in object_index:
    working_set[oi] = objects_file[oi]

  common.WriteWorkingSet(working_set)

  return 0


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
