#!/usr/bin/env python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""This simulates a real job by producing a lot of output."""

from __future__ import print_function

__author__ = 'asharif@google.com (Ahmad Sharif)'

import time


def Main():
  """The main function."""

  for j in range(10):
    for i in range(10000):
      print(str(j) + 'The quick brown fox jumped over the lazy dog.' + str(i))
    time.sleep(60)

  return 0


if __name__ == '__main__':
  Main()
