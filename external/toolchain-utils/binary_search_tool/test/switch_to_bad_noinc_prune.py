#!/usr/bin/env python2
"""Switch part of the objects file in working set to (possible) bad ones.

The "portion" is defined by the file (which is passed as the only argument to
this script) content. Every line in the file is an object index, which will be
set to good (mark as 0).

This switch script is made for the noincremental-prune test. This makes sure
that, after pruning starts (>1 bad item is found), that the number of args sent
to the switch scripts is equals to the actual number of items (i.e. checking
that noincremental always holds).

Warning: This switch script assumes the --file_args option
"""

from __future__ import print_function

import shutil
import sys

import common


def Main(argv):
  """Switch part of the objects file in working set to (possible) bad ones."""
  working_set = common.ReadWorkingSet()
  objects_file = common.ReadObjectsFile()
  object_index = common.ReadObjectIndex(argv[1])

  for oi in object_index:
    working_set[oi] = objects_file[oi]

  shutil.copy(argv[1], './noinc_prune_bad')

  common.WriteWorkingSet(working_set)

  return 0


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
