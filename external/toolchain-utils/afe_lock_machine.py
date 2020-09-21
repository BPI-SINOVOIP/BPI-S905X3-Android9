#!/usr/bin/env python2
#
# Copyright 2015 Google INc.  All Rights Reserved.
"""This module controls locking and unlocking of test machines."""

from __future__ import print_function

import argparse
import getpass
import os
import sys
import traceback

from cros_utils import logger
from cros_utils import machines


class AFELockException(Exception):
  """Base class for exceptions in this module."""


class MachineNotPingable(AFELockException):
  """Raised when machine does not respond to ping."""


class MissingHostInfo(AFELockException):
  """Raised when cannot find info about machine on machine servers."""


class UpdateNonLocalMachine(AFELockException):
  """Raised when user requests to add/remove a ChromeOS HW Lab machine.."""


class DuplicateAdd(AFELockException):
  """Raised when user requests to add a machine that's already on the server."""


class UpdateServerError(AFELockException):
  """Raised when attempt to add/remove a machine from local server fails."""


class LockingError(AFELockException):
  """Raised when server fails to lock/unlock machine as requested."""


class DontOwnLock(AFELockException):
  """Raised when user attmepts to unlock machine locked by someone else."""
  # This should not be raised if the user specified '--force'


class NoAFEServer(AFELockException):
  """Raised when cannot find/access the autotest server."""


class AFEAccessError(AFELockException):
  """Raised when cannot get information about lab machine from lab server."""


class AFELockManager(object):
  """Class for locking/unlocking machines vie Autotest Front End servers.

  This class contains methods for checking the locked status of machines
  on both the ChromeOS HW Lab AFE server and a local AFE server.  It also
  has methods for adding/removing machines from the local server, and for
  changing the lock status of machines on either server.  For the ChromeOS
  HW Lab, it only allows access to the toolchain team lab machines, as
  defined in toolchain-utils/crosperf/default_remotes.  By default it will
  look for a local server on chrotomation2.svl.corp.google.com, but an
  alternative local AFE server can be supplied, if desired.

  !!!IMPORTANT NOTE!!!  The AFE server can only be called from the main
  thread/process of a program.  If you launch threads and try to call it
  from a thread, you will get an error.  This has to do with restrictions
  in the Python virtual machine (and signal handling) and cannot be changed.
  """

  LOCAL_SERVER = 'chrotomation2.svl.corp.google.com'

  def __init__(self,
               remotes,
               force_option,
               chromeos_root,
               local_server,
               use_local=True,
               log=None):
    """Initializes an AFELockManager object.

    Args:
      remotes: A list of machine names or ip addresses to be managed.  Names
        and ip addresses should be represented as strings.  If the list is
        empty, the lock manager will get all known machines.
      force_option: A Boolean indicating whether or not to force an unlock of
        a machine that was locked by someone else.
      chromeos_root: The ChromeOS chroot to use for the autotest scripts.
      local_server: A string containing the name or ip address of the machine
        that is running an AFE server, which is to be used for managing
        machines that are not in the ChromeOS HW lab.
      local: A Boolean indicating whether or not to use/allow a local AFE
        server to be used (see local_server argument).
      use_local: Use the local server instead of the official one.
      log: If not None, this is the logger object to be used for writing out
        informational output messages.  It is expected to be an instance of
        Logger class from cros_utils/logger.py.
    """
    self.chromeos_root = chromeos_root
    self.user = getpass.getuser()
    self.logger = log or logger.GetLogger()
    autotest_path = os.path.join(chromeos_root,
                                 'src/third_party/autotest/files')

    sys.path.append(chromeos_root)
    sys.path.append(autotest_path)
    sys.path.append(os.path.join(autotest_path, 'server', 'cros'))

    # We have to wait to do these imports until the paths above have
    # been fixed.
    # pylint: disable=import-error
    from client import setup_modules
    setup_modules.setup(
        base_path=autotest_path, root_module_name='autotest_lib')

    from dynamic_suite import frontend_wrappers

    self.afe = frontend_wrappers.RetryingAFE(
        timeout_min=30, delay_sec=10, debug=False, server='cautotest')

    self.local = use_local
    self.machines = list(set(remotes)) or []
    self.toolchain_lab_machines = self.GetAllToolchainLabMachines()
    if self.machines and self.AllLabMachines():
      self.local = False

    if not self.local:
      self.local_afe = None
    else:
      dargs = {}
      dargs['server'] = local_server or AFELockManager.LOCAL_SERVER
      # Make sure local server is pingable.
      error_msg = ('Local autotest server machine %s not responding to ping.' %
                   dargs['server'])
      self.CheckMachine(dargs['server'], error_msg)
      self.local_afe = frontend_wrappers.RetryingAFE(
          timeout_min=30, delay_sec=10, debug=False, **dargs)
    if not self.machines:
      self.machines = self.toolchain_lab_machines + self.GetAllNonlabMachines()
    self.force = force_option

  def AllLabMachines(self):
    """Check to see if all machines being used are HW Lab machines."""
    all_lab = True
    for m in self.machines:
      if m not in self.toolchain_lab_machines:
        all_lab = False
        break
    return all_lab

  def CheckMachine(self, machine, error_msg):
    """Verifies that machine is responding to ping.

    Args:
      machine: String containing the name or ip address of machine to check.
      error_msg: Message to print if ping fails.

    Raises:
      MachineNotPingable:  If machine is not responding to 'ping'
    """
    if not machines.MachineIsPingable(machine, logging_level='none'):
      cros_machine = machine + '.cros'
      if not machines.MachineIsPingable(cros_machine, logging_level='none'):
        raise MachineNotPingable(error_msg)

  def MachineIsKnown(self, machine):
    """Checks to see if either AFE server knows the given machine.

    Args:
      machine: String containing name or ip address of machine to check.

    Returns:
      Boolean indicating if the machine is in the list of known machines for
        either AFE server.
    """
    if machine in self.toolchain_lab_machines:
      return True
    elif self.local_afe and machine in self.GetAllNonlabMachines():
      return True

    return False

  def GetAllToolchainLabMachines(self):
    """Gets a list of all the toolchain machines in the ChromeOS HW lab.

    Returns:
      A list of names of the toolchain machines in the ChromeOS HW lab.
    """
    machines_file = os.path.join(
        os.path.dirname(__file__), 'crosperf', 'default_remotes')
    machine_list = []
    with open(machines_file, 'r') as input_file:
      lines = input_file.readlines()
      for line in lines:
        _, remotes = line.split(':')
        remotes = remotes.strip()
        for r in remotes.split():
          machine_list.append(r.strip())
    return machine_list

  def GetAllNonlabMachines(self):
    """Gets a list of all known machines on the local AFE server.

    Returns:
      A list of the names of the machines on the local AFE server.
    """
    non_lab_machines = []
    if self.local_afe:
      non_lab_machines = self.local_afe.get_hostnames()
    return non_lab_machines

  def PrintStatusHeader(self, is_lab_machine):
    """Prints the status header lines for machines.

    Args:
      is_lab_machine: Boolean indicating whether to print HW Lab header or
        local machine header (different spacing).
    """
    if is_lab_machine:
      print('\nMachine (Board)\t\t\t\t\tStatus')
      print('---------------\t\t\t\t\t------\n')
    else:
      print('\nMachine (Board)\t\tStatus')
      print('---------------\t\t------\n')

  def RemoveLocalMachine(self, m):
    """Removes a machine from the local AFE server.

    Args:
      m: The machine to remove.

    Raises:
      MissingHostInfo:  Can't find machine to be removed.
    """
    if self.local_afe:
      host_info = self.local_afe.get_hosts(hostname=m)
      if host_info:
        host_info = host_info[0]
        host_info.delete()
      else:
        raise MissingHostInfo('Cannot find/delete machine %s.' % m)

  def AddLocalMachine(self, m):
    """Adds a machine to the local AFE server.

    Args:
      m: The machine to be added.
    """
    if self.local_afe:
      error_msg = 'Machine %s is not responding to ping.' % m
      self.CheckMachine(m, error_msg)
      self.local_afe.create_host(m)

  def AddMachinesToLocalServer(self):
    """Adds one or more machines to the local AFE server.

    Verify that the requested machines are legal to add to the local server,
    i.e. that they are not ChromeOS HW lab machines, and they are not already
    on the local server.  Call AddLocalMachine for each valid machine.

    Raises:
      DuplicateAdd: Attempt to add a machine that is already on the server.
      UpdateNonLocalMachine:  Attempt to add a ChromeOS HW lab machine.
      UpdateServerError:  Something went wrong while attempting to add a
        machine.
    """
    for m in self.machines:
      for cros_name in [m, m + '.cros']:
        if cros_name in self.toolchain_lab_machines:
          raise UpdateNonLocalMachine(
              'Machine %s is already in the ChromeOS HW'
              'Lab.  Cannot add it to local server.' % cros_name)
      host_info = self.local_afe.get_hosts(hostname=m)
      if host_info:
        raise DuplicateAdd('Machine %s is already on the local server.' % m)
      try:
        self.AddLocalMachine(m)
        self.logger.LogOutput('Successfully added %s to local server.' % m)
      except Exception as e:
        traceback.print_exc()
        raise UpdateServerError(
            'Error occurred while attempting to add %s. %s' % (m, str(e)))

  def RemoveMachinesFromLocalServer(self):
    """Removes one or more machines from the local AFE server.

    Verify that the requested machines are legal to remove from the local
    server, i.e. that they are not ChromeOS HW lab machines.  Call
    RemoveLocalMachine for each valid machine.

    Raises:
      UpdateServerError:  Something went wrong while attempting to remove a
        machine.
    """
    for m in self.machines:
      for cros_name in [m, m + '.cros']:
        if cros_name in self.toolchain_lab_machines:
          raise UpdateNonLocalMachine(
              'Machine %s is in the ChromeOS HW Lab. '
              'This script cannot remove lab machines.' % cros_name)
      try:
        self.RemoveLocalMachine(m)
        self.logger.LogOutput('Successfully removed %s from local server.' % m)
      except Exception as e:
        traceback.print_exc()
        raise UpdateServerError('Error occurred while attempting to remove %s '
                                '(%s).' % (m, str(e)))

  def ListMachineStates(self, machine_states):
    """Gets and prints the current status for a list of machines.

    Prints out the current status for all of the machines in the current
    AFELockManager's list of machines (set when the object is initialized).

    Args:
      machine_states: A dictionary of the current state of every machine in
        the current AFELockManager's list of machines.  Normally obtained by
        calling AFELockManager::GetMachineStates.
    """
    local_machines = []
    printed_hdr = False
    for m in machine_states:
      cros_name = m + '.cros'
      if (m in self.toolchain_lab_machines or
          cros_name in self.toolchain_lab_machines):
        name = m if m in self.toolchain_lab_machines else cros_name
        if not printed_hdr:
          self.PrintStatusHeader(True)
          printed_hdr = True
        state = machine_states[m]
        if state['locked']:
          print('%s (%s)\tlocked by %s since %s' %
                (name, state['board'], state['locked_by'], state['lock_time']))
        else:
          print('%s (%s)\tunlocked' % (name, state['board']))
      else:
        local_machines.append(m)

    if local_machines:
      self.PrintStatusHeader(False)
      for m in local_machines:
        state = machine_states[m]
        if state['locked']:
          print('%s (%s)\tlocked by %s since %s' %
                (m, state['board'], state['locked_by'], state['lock_time']))
        else:
          print('%s (%s)\tunlocked' % (m, state['board']))

  def UpdateLockInAFE(self, should_lock_machine, machine):
    """Calls an AFE server to lock/unlock a machine.

    Args:
      should_lock_machine: Boolean indicating whether to lock the machine (True)
        or unlock the machine (False).
      machine: The machine to update.

    Raises:
      LockingError:  An error occurred while attempting to update the machine
        state.
    """
    action = 'lock'
    if not should_lock_machine:
      action = 'unlock'
    kwargs = {'locked': should_lock_machine}
    kwargs['lock_reason'] = 'toolchain user request (%s)' % self.user

    cros_name = machine + '.cros'
    if cros_name in self.toolchain_lab_machines:
      machine = cros_name
    if machine in self.toolchain_lab_machines:
      m = machine.split('.')[0]
      afe_server = self.afe
    else:
      m = machine
      afe_server = self.local_afe

    try:
      afe_server.run(
          'modify_hosts',
          host_filter_data={'hostname__in': [m]},
          update_data=kwargs)
    except Exception as e:
      traceback.print_exc()
      raise LockingError('Unable to %s machine %s. %s' % (action, m, str(e)))

  def UpdateMachines(self, lock_machines):
    """Sets the locked state of the machines to the requested value.

    The machines updated are the ones in self.machines (specified when the
    class object was intialized).

    Args:
      lock_machines: Boolean indicating whether to lock the machines (True) or
        unlock the machines (False).

    Returns:
      A list of the machines whose state was successfully updated.
    """
    updated_machines = []
    for m in self.machines:
      self.UpdateLockInAFE(lock_machines, m)
      # Since we returned from self.UpdateLockInAFE we assume the request
      # succeeded.
      if lock_machines:
        self.logger.LogOutput('Locked machine(s) %s.' % m)
      else:
        self.logger.LogOutput('Unlocked machine(s) %s.' % m)
      updated_machines.append(m)

    return updated_machines

  def _InternalRemoveMachine(self, machine):
    """Remove machine from internal list of machines.

    Args:
      machine: Name of machine to be removed from internal list.
    """
    # Check to see if machine is lab machine and if so, make sure it has
    # ".cros" on the end.
    cros_machine = machine
    if machine.find('rack') > 0 and machine.find('row') > 0:
      if machine.find('.cros') == -1:
        cros_machine = cros_machine + '.cros'

    self.machines = [
        m for m in self.machines if m != cros_machine and m != machine
    ]

  def CheckMachineLocks(self, machine_states, cmd):
    """Check that every machine in requested list is in the proper state.

    If the cmd is 'unlock' verify that every machine is locked by requestor.
    If the cmd is 'lock' verify that every machine is currently unlocked.

    Args:
      machine_states: A dictionary of the current state of every machine in
        the current AFELockManager's list of machines.  Normally obtained by
        calling AFELockManager::GetMachineStates.
      cmd: The user-requested action for the machines: 'lock' or 'unlock'.

    Raises:
      DontOwnLock: The lock on a requested machine is owned by someone else.
    """
    for k, state in machine_states.iteritems():
      if cmd == 'unlock':
        if not state['locked']:
          self.logger.LogWarning('Attempt to unlock already unlocked machine '
                                 '(%s).' % k)
          self._InternalRemoveMachine(k)

        if state['locked'] and state['locked_by'] != self.user:
          raise DontOwnLock('Attempt to unlock machine (%s) locked by someone '
                            'else (%s).' % (k, state['locked_by']))
      elif cmd == 'lock':
        if state['locked']:
          self.logger.LogWarning(
              'Attempt to lock already locked machine (%s)' % k)
          self._InternalRemoveMachine(k)

  def HasAFEServer(self, local):
    """Verifies that the AFELockManager has appropriate AFE server.

    Args:
      local: Boolean indicating whether we are checking for the local server
        (True) or for the global server (False).

    Returns:
      A boolean indicating if the AFELockManager has the requested AFE server.
    """
    if local:
      return self.local_afe is not None
    else:
      return self.afe is not None

  def GetMachineStates(self, cmd=''):
    """Gets the current state of all the requested machines.

    Gets the current state of all the requested machines, both from the HW lab
    sever and from the local server.  Stores the data in a dictionary keyed
    by machine name.

    Args:
      cmd: The command for which we are getting the machine states. This is
        important because if one of the requested machines is missing we raise
        an exception, unless the requested command is 'add'.

    Returns:
      A dictionary of machine states for all the machines in the AFELockManager
      object.

    Raises:
      NoAFEServer:  Cannot find the HW Lab or local AFE server.
      AFEAccessError:  An error occurred when querying the server about a
        machine.
    """
    if not self.HasAFEServer(False):
      raise NoAFEServer('Error: Cannot connect to main AFE server.')

    if self.local and not self.HasAFEServer(True):
      raise NoAFEServer('Error: Cannot connect to local AFE server.')

    machine_list = {}
    for m in self.machines:
      host_info = None
      cros_name = m + '.cros'
      if (m in self.toolchain_lab_machines or
          cros_name in self.toolchain_lab_machines):
        mod_host = m.split('.')[0]
        host_info = self.afe.get_hosts(hostname=mod_host)
        if not host_info:
          raise AFEAccessError('Unable to get information about %s from main'
                               ' autotest server.' % m)
      else:
        host_info = self.local_afe.get_hosts(hostname=m)
        if not host_info and cmd != 'add':
          raise AFEAccessError('Unable to get information about %s from '
                               'local autotest server.' % m)
      if host_info:
        host_info = host_info[0]
        name = host_info.hostname
        values = {}
        values['board'] = host_info.platform if host_info.platform else '??'
        values['locked'] = host_info.locked
        if host_info.locked:
          values['locked_by'] = host_info.locked_by
          values['lock_time'] = host_info.lock_time
        else:
          values['locked_by'] = ''
          values['lock_time'] = ''
        machine_list[name] = values
      else:
        machine_list[m] = {}
    return machine_list


def Main(argv):
  """Parse the options, initialize lock manager and dispatch proper method.

  Args:
    argv: The options with which this script was invoked.

  Returns:
    0 unless an exception is raised.
  """
  parser = argparse.ArgumentParser()

  parser.add_argument(
      '--list',
      dest='cmd',
      action='store_const',
      const='status',
      help='List current status of all known machines.')
  parser.add_argument(
      '--lock',
      dest='cmd',
      action='store_const',
      const='lock',
      help='Lock given machine(s).')
  parser.add_argument(
      '--unlock',
      dest='cmd',
      action='store_const',
      const='unlock',
      help='Unlock given machine(s).')
  parser.add_argument(
      '--status',
      dest='cmd',
      action='store_const',
      const='status',
      help='List current status of given machine(s).')
  parser.add_argument(
      '--add_machine',
      dest='cmd',
      action='store_const',
      const='add',
      help='Add machine to local machine server.')
  parser.add_argument(
      '--remove_machine',
      dest='cmd',
      action='store_const',
      const='remove',
      help='Remove machine from the local machine server.')
  parser.add_argument(
      '--nolocal',
      dest='local',
      action='store_false',
      default=True,
      help='Do not try to use local machine server.')
  parser.add_argument(
      '--remote', dest='remote', help='machines on which to operate')
  parser.add_argument(
      '--chromeos_root',
      dest='chromeos_root',
      required=True,
      help='ChromeOS root to use for autotest scripts.')
  parser.add_argument(
      '--local_server',
      dest='local_server',
      default=None,
      help='Alternate local autotest server to use.')
  parser.add_argument(
      '--force',
      dest='force',
      action='store_true',
      default=False,
      help='Force lock/unlock of machines, even if not'
      ' current lock owner.')

  options = parser.parse_args(argv)

  if not options.remote and options.cmd != 'status':
    parser.error('No machines specified for operation.')

  if not os.path.isdir(options.chromeos_root):
    parser.error('Cannot find chromeos_root: %s.' % options.chromeos_root)

  if not options.cmd:
    parser.error('No operation selected (--list, --status, --lock, --unlock,'
                 ' --add_machine, --remove_machine).')

  machine_list = []
  if options.remote:
    machine_list = options.remote.split()

  lock_manager = AFELockManager(machine_list, options.force,
                                options.chromeos_root, options.local_server,
                                options.local)

  machine_states = lock_manager.GetMachineStates(cmd=options.cmd)
  cmd = options.cmd

  if cmd == 'status':
    lock_manager.ListMachineStates(machine_states)

  elif cmd == 'lock':
    if not lock_manager.force:
      lock_manager.CheckMachineLocks(machine_states, cmd)
      lock_manager.UpdateMachines(True)

  elif cmd == 'unlock':
    if not lock_manager.force:
      lock_manager.CheckMachineLocks(machine_states, cmd)
      lock_manager.UpdateMachines(False)

  elif cmd == 'add':
    lock_manager.AddMachinesToLocalServer()

  elif cmd == 'remove':
    lock_manager.RemoveMachinesFromLocalServer()

  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
