# Copyright 2010 Google Inc. All Rights Reserved.

from automation.common import job
from automation.common import machine


def CreateLinuxJob(label, command, lock=False, timeout=4 * 60 * 60):
  to_return = job.Job(label, command, timeout)
  to_return.DependsOnMachine(machine.MachineSpecification(os='linux',
                                                          lock_required=lock))
  return to_return
