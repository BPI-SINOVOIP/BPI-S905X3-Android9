# Copyright 2010 Google Inc. All Rights Reserved.

__author__ = 'asharif@google.com (Ahmad Sharif)'

from operator import attrgetter
import copy
import csv
import threading
import os.path

from automation.common import machine

DEFAULT_MACHINES_FILE = os.path.join(os.path.dirname(__file__), 'test_pool.csv')


class MachineManager(object):
  """Container for list of machines one can run jobs on."""

  @classmethod
  def FromMachineListFile(cls, filename):
    # Read the file and skip header
    csv_file = csv.reader(open(filename, 'rb'), delimiter=',', quotechar='"')
    csv_file.next()

    return cls([machine.Machine(hostname, label, cpu, int(cores), os, user)
                for hostname, label, cpu, cores, os, user in csv_file])

  def __init__(self, machines):
    self._machine_pool = machines
    self._lock = threading.RLock()

  def _GetMachine(self, mach_spec):
    available_pool = [m for m in self._machine_pool if mach_spec.IsMatch(m)]

    if available_pool:
      # find a machine with minimum uses
      uses = attrgetter('uses')

      mach = min(available_pool, key=uses)

      if mach_spec.preferred_machines:
        preferred_pool = [m
                          for m in available_pool
                          if m.hostname in mach_spec.preferred_machines]
        if preferred_pool:
          mach = min(preferred_pool, key=uses)

      mach.Acquire(mach_spec.lock_required)

      return mach

  def GetMachines(self, required_machines):
    """Acquire machines for use by a job."""

    with self._lock:
      acquired_machines = [self._GetMachine(ms) for ms in required_machines]

      if not all(acquired_machines):
        # Roll back acquires
        while acquired_machines:
          mach = acquired_machines.pop()
          if mach:
            mach.Release()

      return acquired_machines

  def GetMachineList(self):
    with self._lock:
      return copy.deepcopy(self._machine_pool)

  def ReturnMachines(self, machines):
    with self._lock:
      for m in machines:
        m.Release()

  def __str__(self):
    return str(self._machine_pool)
