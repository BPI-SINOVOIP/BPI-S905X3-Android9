#!/usr/bin/python
#
# Copyright 2010 Google Inc. All Rights Reserved.

import os.path
import pickle
import sys
import xmlrpclib

from automation.common import job
from automation.common import job_group
from automation.common import machine


def Main():
  server = xmlrpclib.Server('http://localhost:8000')

  command = os.path.join(
      os.path.dirname(sys.argv[0]), '../../produce_output.py')

  pwd_job = job.Job('pwd_job', command)
  pwd_job.DependsOnMachine(machine.MachineSpecification(os='linux'))

  group = job_group.JobGroup('pwd_client', [pwd_job])
  server.ExecuteJobGroup(pickle.dumps(group))


if __name__ == '__main__':
  Main()
