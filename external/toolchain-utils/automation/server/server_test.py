#!/usr/bin/python
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Machine manager unittest.

MachineManagerTest tests MachineManager.
"""

__author__ = 'asharif@google.com (Ahmad Sharif)'

import server
import unittest


class ServerTest(unittest.TestCase):

  def setUp(self):
    pass

  def testGetAllJobs(self):
    s = server.Server()
    print s.GetAllJobs()


if __name__ == '__main__':
  unittest.main()
