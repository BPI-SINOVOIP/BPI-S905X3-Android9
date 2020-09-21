import image_chromeos
import lock_machine
import sys
import threading
import time
from cros_utils import command_executer
from cros_utils import logger


class CrosMachine(object):

  def __init__(self, name):
    self.name = name
    self.image = None
    self.checksum = None
    self.locked = False
    self.released_time = time.time()
    self.autotest_run = None

  def __str__(self):
    l = []
    l.append(self.name)
    l.append(str(self.image))
    l.append(str(self.checksum))
    l.append(str(self.locked))
    l.append(str(self.released_time))
    return ', '.join(l)


class MachineManagerSingleton(object):
  _instance = None
  _lock = threading.RLock()
  _all_machines = []
  _machines = []
  image_lock = threading.Lock()
  num_reimages = 0
  chromeos_root = None
  no_lock = False

  def __new__(cls, *args, **kwargs):
    with cls._lock:
      if not cls._instance:
        cls._instance = super(MachineManagerSingleton, cls).__new__(cls, *args,
                                                                    **kwargs)
      return cls._instance

  def TryToLockMachine(self, cros_machine):
    with self._lock:
      assert cros_machine, "Machine can't be None"
      for m in self._machines:
        assert m.name != cros_machine.name, ('Tried to double-lock %s' %
                                             cros_machine.name)
      if self.no_lock:
        locked = True
      else:
        locked = lock_machine.Machine(cros_machine.name).Lock(True, sys.argv[0])
      if locked:
        ce = command_executer.GetCommandExecuter()
        command = 'cat %s' % image_chromeos.checksum_file
        ret, out, err = ce.CrosRunCommandWOutput(
            command,
            chromeos_root=self.chromeos_root,
            machine=cros_machine.name)
        if ret == 0:
          cros_machine.checksum = out.strip()
        self._machines.append(cros_machine)
      else:
        logger.GetLogger().LogOutput("Warning: Couldn't lock: %s" %
                                     cros_machine.name)

  # This is called from single threaded mode.
  def AddMachine(self, machine_name):
    with self._lock:
      for m in self._all_machines:
        assert m.name != machine_name, 'Tried to double-add %s' % machine_name
      self._all_machines.append(CrosMachine(machine_name))

  def AcquireMachine(self, image_checksum):
    with self._lock:
      # Lazily external lock machines
      if not self._machines:
        for m in self._all_machines:
          self.TryToLockMachine(m)
      assert self._machines, ('Could not lock any machine in %s' %
                              self._all_machines)

      ###      for m in self._machines:
      ###        if (m.locked and time.time() - m.released_time < 10 and
      ###            m.checksum == image_checksum):
      ###          return None
      for m in [machine for machine in self._machines if not machine.locked]:
        if m.checksum == image_checksum:
          m.locked = True
          m.autotest_run = threading.current_thread()
          return m
      for m in [machine for machine in self._machines if not machine.locked]:
        if not m.checksum:
          m.locked = True
          m.autotest_run = threading.current_thread()
          return m
      for m in [machine for machine in self._machines if not machine.locked]:
        if time.time() - m.released_time > 20:
          m.locked = True
          m.autotest_run = threading.current_thread()
          return m
    return None

  def ReleaseMachine(self, machine):
    with self._lock:
      for m in self._machines:
        if machine.name == m.name:
          assert m.locked == True, 'Tried to double-release %s' % m.name
          m.released_time = time.time()
          m.locked = False
          m.status = 'Available'
          break

  def __del__(self):
    with self._lock:
      # Unlock all machines.
      for m in self._machines:
        if not self.no_lock:
          assert lock_machine.Machine(m.name).Unlock(True) == True, (
              "Couldn't unlock machine: %s" % m.name)

  def __str__(self):
    with self._lock:
      l = ['MachineManager Status:']
      for m in self._machines:
        l.append(str(m))
      return '\n'.join(l)

  def AsString(self):
    with self._lock:
      stringify_fmt = '%-30s %-10s %-4s %-25s %-32s'
      header = stringify_fmt % ('Machine', 'Thread', 'Lock', 'Status',
                                'Checksum')
      table = [header]
      for m in self._machines:
        if m.autotest_run:
          autotest_name = m.autotest_run.name
          autotest_status = m.autotest_run.status
        else:
          autotest_name = ''
          autotest_status = ''

        try:
          machine_string = stringify_fmt % (m.name, autotest_name, m.locked,
                                            autotest_status, m.checksum)
        except:
          machine_string = ''
        table.append(machine_string)
      return 'Machine Status:\n%s' % '\n'.join(table)
