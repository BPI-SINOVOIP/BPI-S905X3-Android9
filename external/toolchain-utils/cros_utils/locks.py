# Copyright 2015 The Chromium OS Authors. All rights reserved.
"""Utilities for locking machines."""

from __future__ import print_function

import time

import afe_lock_machine

import logger


def AcquireLock(machines, chromeos_root, timeout=1200):
  """Acquire lock for machine(s) with timeout, using AFE server for locking."""
  start_time = time.time()
  locked = True
  sleep_time = min(10, timeout / 10.0)
  while True:
    try:
      afe_lock_machine.AFELockManager(machines, False, chromeos_root,
                                      None).UpdateMachines(True)
      break
    except Exception as e:
      if time.time() - start_time > timeout:
        locked = False
        logger.GetLogger().LogWarning(
            'Could not acquire lock on {0} within {1} seconds: {2}'.format(
                repr(machines), timeout, str(e)))
        break
      time.sleep(sleep_time)
  return locked


def ReleaseLock(machines, chromeos_root):
  """Release locked machine(s), using AFE server for locking."""
  unlocked = True
  try:
    afe_lock_machine.AFELockManager(machines, False, chromeos_root,
                                    None).UpdateMachines(False)
  except Exception as e:
    unlocked = False
    logger.GetLogger().LogWarning('Could not unlock %s. %s' %
                                  (repr(machines), str(e)))
  return unlocked
