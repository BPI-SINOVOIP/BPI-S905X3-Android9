# Copyright 2010 Google Inc. All Rights Reserved.

__author__ = 'asharif@google.com (Ahmad Sharif)'

from fnmatch import fnmatch


class Machine(object):
  """Stores information related to machine and its state."""

  def __init__(self, hostname, label, cpu, cores, os, username):
    self.hostname = hostname
    self.label = label
    self.cpu = cpu
    self.cores = cores
    self.os = os
    self.username = username

    # MachineManager related attributes.
    self.uses = 0
    self.locked = False

  def Acquire(self, exclusively):
    assert not self.locked

    if exclusively:
      self.locked = True
    self.uses += 1

  def Release(self):
    assert self.uses > 0

    self.uses -= 1

    if not self.uses:
      self.locked = False

  def __repr__(self):
    return '{%s: %s@%s}' % (self.__class__.__name__, self.username,
                            self.hostname)

  def __str__(self):
    return '\n'.join(
        ['Machine Information:', 'Hostname: %s' % self.hostname, 'Label: %s' %
         self.label, 'CPU: %s' % self.cpu, 'Cores: %d' % self.cores, 'OS: %s' %
         self.os, 'Uses: %d' % self.uses, 'Locked: %s' % self.locked])


class MachineSpecification(object):
  """Helper class used to find a machine matching your requirements."""

  def __init__(self, hostname='*', label='*', os='*', lock_required=False):
    self.hostname = hostname
    self.label = label
    self.os = os
    self.lock_required = lock_required
    self.preferred_machines = []

  def __str__(self):
    return '\n'.join(['Machine Specification:', 'Name: %s' % self.name, 'OS: %s'
                      % self.os, 'Lock required: %s' % self.lock_required])

  def IsMatch(self, machine):
    return all([not machine.locked, fnmatch(machine.hostname, self.hostname),
                fnmatch(machine.label, self.label), fnmatch(machine.os,
                                                            self.os)])

  def AddPreferredMachine(self, hostname):
    if hostname not in self.preferred_machines:
      self.preferred_machines.append(hostname)
