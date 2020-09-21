# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Utilities relating to machine-specific functions."""

from __future__ import print_function

from cros_utils import command_executer


def MachineIsPingable(machine, logging_level='average'):
  """Checks to see if a machine is responding to 'ping'.

  Args:
    machine: String containing the name or ip address of the machine to check.
    logging_level: The logging level with which to initialize the
      command_executer (from command_executor.LOG_LEVEL enum list).

  Returns:
    Boolean indicating whether machine is responding to ping or not.
  """
  ce = command_executer.GetCommandExecuter(log_level=logging_level)
  cmd = 'ping -c 1 -w 3 %s' % machine
  status = ce.RunCommand(cmd)
  return status == 0
