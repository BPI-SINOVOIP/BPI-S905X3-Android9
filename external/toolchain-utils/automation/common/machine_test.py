#!/usr/bin/python
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Machine manager unittest.

MachineManagerTest tests MachineManager.
"""

__author__ = 'asharif@google.com (Ahmad Sharif)'

import machine
import unittest


class MachineTest(unittest.TestCase):

  def setUp(self):
    pass

  def testPrintMachine(self):
    mach = machine.Machine('ahmad.mtv', 'core2duo', 4, 'linux', 'asharif')
    self.assertTrue('ahmad.mtv' in str(mach))


if __name__ == '__main__':
  unittest.main()
