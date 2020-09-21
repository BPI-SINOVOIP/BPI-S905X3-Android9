#!/usr/bin/python
#
# Copyright 2010 Google Inc. All Rights Reserved.

import pickle
import xmlrpclib

from automation.common import job
from automation.common import job_group
from automation.common import machine


def Main():
  server = xmlrpclib.Server('http://localhost:8000')

  command = ['echo These following 3 lines should be the same', 'pwd', '$(pwd)',
             'echo ${PWD}']

  pwd_job = job.Job('pwd_job', ' && '.join(command))
  pwd_job.DependsOnMachine(machine.MachineSpecification(os='linux'))

  group = job_group.JobGroup('pwd_client', [pwd_job])
  server.ExecuteJobGroup(pickle.dumps(group))


if __name__ == '__main__':
  Main()
