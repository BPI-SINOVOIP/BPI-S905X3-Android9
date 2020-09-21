#!/usr/bin/env python2
"""Check to see if the working set produces a good executable.

This test script is made for the noincremental-prune test. This makes sure
that, after pruning starts (>1 bad item is found), that the number of args sent
to the switch scripts is equals to the actual number of items (i.e. checking
that noincremental always holds).
"""

from __future__ import print_function

import os
import sys

import common


def Main():
  working_set = common.ReadWorkingSet()

  with open('noinc_prune_good', 'r') as good_args:
    num_good_args = len(good_args.readlines())

  with open('noinc_prune_bad', 'r') as bad_args:
    num_bad_args = len(bad_args.readlines())

  num_args = num_good_args + num_bad_args
  if num_args != len(working_set):
    print('Only %d args, expected %d' % (num_args, len(working_set)))
    print('%d good args, %d bad args' % (num_good_args, num_bad_args))
    return 3

  os.remove('noinc_prune_bad')
  os.remove('noinc_prune_good')

  if not os.path.exists('./is_setup'):
    return 1
  for w in working_set:
    if w == 1:
      return 1  ## False, linking failure
  return 0


if __name__ == '__main__':
  retval = Main()
  sys.exit(retval)
