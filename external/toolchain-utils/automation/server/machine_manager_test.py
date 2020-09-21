#!/usr/bin/python
#
# Copyright 2010 Google Inc. All Rights Reserved.

__author__ = 'asharif@google.com (Ahmad Sharif)'

import unittest
from automation.common import machine
from automation.server import machine_manager


class MachineManagerTest(unittest.TestCase):

  def setUp(self):
    self.machine_manager = machine_manager.MachineManager()

  def testPrint(self):
    print self.machine_manager

  def testGetLinuxBox(self):
    mach_spec_list = [machine.MachineSpecification(os='linux')]
    machines = self.machine_manager.GetMachines(mach_spec_list)
    self.assertTrue(machines)

  def testGetChromeOSBox(self):
    mach_spec_list = [machine.MachineSpecification(os='chromeos')]
    machines = self.machine_manager.GetMachines(mach_spec_list)
    self.assertTrue(machines)


if __name__ == '__main__':
  unittest.main()
