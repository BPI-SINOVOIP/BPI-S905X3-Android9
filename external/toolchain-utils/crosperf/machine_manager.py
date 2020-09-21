# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Machine Manager module."""

from __future__ import print_function

import collections
import file_lock_machine
import hashlib
import image_chromeos
import math
import os.path
import re
import sys
import threading
import time

import test_flag
from cros_utils import command_executer
from cros_utils import logger

CHECKSUM_FILE = '/usr/local/osimage_checksum_file'


class BadChecksum(Exception):
  """Raised if all machines for a label don't have the same checksum."""
  pass


class BadChecksumString(Exception):
  """Raised if all machines for a label don't have the same checksum string."""
  pass


class MissingLocksDirectory(Exception):
  """Raised when cannot find/access the machine locks directory."""


class CrosCommandError(Exception):
  """Raised when an error occurs running command on DUT."""


class CrosMachine(object):
  """The machine class."""

  def __init__(self, name, chromeos_root, log_level, cmd_exec=None):
    self.name = name
    self.image = None
    # We relate a dut with a label if we reimage the dut using label or we
    # detect at the very beginning that the dut is running this label.
    self.label = None
    self.checksum = None
    self.locked = False
    self.released_time = time.time()
    self.test_run = None
    self.chromeos_root = chromeos_root
    self.log_level = log_level
    self.cpuinfo = None
    self.machine_id = None
    self.checksum_string = None
    self.meminfo = None
    self.phys_kbytes = None
    self.ce = cmd_exec or command_executer.GetCommandExecuter(
        log_level=self.log_level)
    self.SetUpChecksumInfo()

  def SetUpChecksumInfo(self):
    if not self.IsReachable():
      self.machine_checksum = None
      return
    self._GetMemoryInfo()
    self._GetCPUInfo()
    self._ComputeMachineChecksumString()
    self._GetMachineID()
    self.machine_checksum = self._GetMD5Checksum(self.checksum_string)
    self.machine_id_checksum = self._GetMD5Checksum(self.machine_id)

  def IsReachable(self):
    command = 'ls'
    ret = self.ce.CrosRunCommand(
        command, machine=self.name, chromeos_root=self.chromeos_root)
    if ret:
      return False
    return True

  def _ParseMemoryInfo(self):
    line = self.meminfo.splitlines()[0]
    usable_kbytes = int(line.split()[1])
    # This code is from src/third_party/test/files/client/bin/base_utils.py
    # usable_kbytes is system's usable DRAM in kbytes,
    #   as reported by memtotal() from device /proc/meminfo memtotal
    #   after Linux deducts 1.5% to 9.5% for system table overhead
    # Undo the unknown actual deduction by rounding up
    #   to next small multiple of a big power-of-two
    #   eg  12GB - 5.1% gets rounded back up to 12GB
    mindeduct = 0.005  # 0.5 percent
    maxdeduct = 0.095  # 9.5 percent
    # deduction range 1.5% .. 9.5% supports physical mem sizes
    #    6GB .. 12GB in steps of .5GB
    #   12GB .. 24GB in steps of 1 GB
    #   24GB .. 48GB in steps of 2 GB ...
    # Finer granularity in physical mem sizes would require
    #   tighter spread between min and max possible deductions

    # increase mem size by at least min deduction, without rounding
    min_kbytes = int(usable_kbytes / (1.0 - mindeduct))
    # increase mem size further by 2**n rounding, by 0..roundKb or more
    round_kbytes = int(usable_kbytes / (1.0 - maxdeduct)) - min_kbytes
    # find least binary roundup 2**n that covers worst-cast roundKb
    mod2n = 1 << int(math.ceil(math.log(round_kbytes, 2)))
    # have round_kbytes <= mod2n < round_kbytes*2
    # round min_kbytes up to next multiple of mod2n
    phys_kbytes = min_kbytes + mod2n - 1
    phys_kbytes -= phys_kbytes % mod2n  # clear low bits
    self.phys_kbytes = phys_kbytes

  def _GetMemoryInfo(self):
    #TODO yunlian: when the machine in rebooting, it will not return
    #meminfo, the assert does not catch it either
    command = 'cat /proc/meminfo'
    ret, self.meminfo, _ = self.ce.CrosRunCommandWOutput(
        command, machine=self.name, chromeos_root=self.chromeos_root)
    assert ret == 0, 'Could not get meminfo from machine: %s' % self.name
    if ret == 0:
      self._ParseMemoryInfo()

  def _GetCPUInfo(self):
    command = 'cat /proc/cpuinfo'
    ret, self.cpuinfo, _ = self.ce.CrosRunCommandWOutput(
        command, machine=self.name, chromeos_root=self.chromeos_root)
    assert ret == 0, 'Could not get cpuinfo from machine: %s' % self.name

  def _ComputeMachineChecksumString(self):
    self.checksum_string = ''
    exclude_lines_list = ['MHz', 'BogoMIPS', 'bogomips']
    for line in self.cpuinfo.splitlines():
      if not any(e in line for e in exclude_lines_list):
        self.checksum_string += line
    self.checksum_string += ' ' + str(self.phys_kbytes)

  def _GetMD5Checksum(self, ss):
    if ss:
      return hashlib.md5(ss).hexdigest()
    else:
      return ''

  def _GetMachineID(self):
    command = 'dump_vpd_log --full --stdout'
    _, if_out, _ = self.ce.CrosRunCommandWOutput(
        command, machine=self.name, chromeos_root=self.chromeos_root)
    b = if_out.splitlines()
    a = [l for l in b if 'Product' in l]
    if len(a):
      self.machine_id = a[0]
      return
    command = 'ifconfig'
    _, if_out, _ = self.ce.CrosRunCommandWOutput(
        command, machine=self.name, chromeos_root=self.chromeos_root)
    b = if_out.splitlines()
    a = [l for l in b if 'HWaddr' in l]
    if len(a):
      self.machine_id = '_'.join(a)
      return
    a = [l for l in b if 'ether' in l]
    if len(a):
      self.machine_id = '_'.join(a)
      return
    assert 0, 'Could not get machine_id from machine: %s' % self.name

  def __str__(self):
    l = []
    l.append(self.name)
    l.append(str(self.image))
    l.append(str(self.checksum))
    l.append(str(self.locked))
    l.append(str(self.released_time))
    return ', '.join(l)


class MachineManager(object):
  """Lock, image and unlock machines locally for benchmark runs.

  This class contains methods and calls to lock, unlock and image
  machines and distribute machines to each benchmark run.  The assumption is
  that all of the machines for the experiment have been globally locked
  (using an AFE server) in the ExperimentRunner, but the machines still need
  to be locally locked/unlocked (allocated to benchmark runs) to prevent
  multiple benchmark runs within the same experiment from trying to use the
  same machine at the same time.
  """

  def __init__(self,
               chromeos_root,
               acquire_timeout,
               log_level,
               locks_dir,
               cmd_exec=None,
               lgr=None):
    self._lock = threading.RLock()
    self._all_machines = []
    self._machines = []
    self.image_lock = threading.Lock()
    self.num_reimages = 0
    self.chromeos_root = None
    self.machine_checksum = {}
    self.machine_checksum_string = {}
    self.acquire_timeout = acquire_timeout
    self.log_level = log_level
    self.locks_dir = locks_dir
    self.ce = cmd_exec or command_executer.GetCommandExecuter(
        log_level=self.log_level)
    self.logger = lgr or logger.GetLogger()

    if self.locks_dir and not os.path.isdir(self.locks_dir):
      raise MissingLocksDirectory(
          'Cannot access locks directory: %s' % self.locks_dir)

    self._initialized_machines = []
    self.chromeos_root = chromeos_root

  def RemoveNonLockedMachines(self, locked_machines):
    for m in self._all_machines:
      if m.name not in locked_machines:
        self._all_machines.remove(m)

    for m in self._machines:
      if m.name not in locked_machines:
        self._machines.remove(m)

  def GetChromeVersion(self, machine):
    """Get the version of Chrome running on the DUT."""

    cmd = '/opt/google/chrome/chrome --version'
    ret, version, _ = self.ce.CrosRunCommandWOutput(
        cmd, machine=machine.name, chromeos_root=self.chromeos_root)
    if ret != 0:
      raise CrosCommandError(
          "Couldn't get Chrome version from %s." % machine.name)

    if ret != 0:
      version = ''
    return version.rstrip()

  def ImageMachine(self, machine, label):
    checksum = label.checksum

    if checksum and (machine.checksum == checksum):
      return
    chromeos_root = label.chromeos_root
    if not chromeos_root:
      chromeos_root = self.chromeos_root
    image_chromeos_args = [
        image_chromeos.__file__, '--no_lock',
        '--chromeos_root=%s' % chromeos_root,
        '--image=%s' % label.chromeos_image,
        '--image_args=%s' % label.image_args, '--remote=%s' % machine.name,
        '--logging_level=%s' % self.log_level
    ]
    if label.board:
      image_chromeos_args.append('--board=%s' % label.board)

    # Currently can't image two machines at once.
    # So have to serialized on this lock.
    save_ce_log_level = self.ce.log_level
    if self.log_level != 'verbose':
      self.ce.log_level = 'average'

    with self.image_lock:
      if self.log_level != 'verbose':
        self.logger.LogOutput('Pushing image onto machine.')
        self.logger.LogOutput('Running image_chromeos.DoImage with %s' %
                              ' '.join(image_chromeos_args))
      retval = 0
      if not test_flag.GetTestMode():
        retval = image_chromeos.DoImage(image_chromeos_args)
      if retval:
        cmd = 'reboot && exit'
        if self.log_level != 'verbose':
          self.logger.LogOutput('reboot & exit.')
        self.ce.CrosRunCommand(
            cmd, machine=machine.name, chromeos_root=self.chromeos_root)
        time.sleep(60)
        if self.log_level != 'verbose':
          self.logger.LogOutput('Pushing image onto machine.')
          self.logger.LogOutput('Running image_chromeos.DoImage with %s' %
                                ' '.join(image_chromeos_args))
        retval = image_chromeos.DoImage(image_chromeos_args)
      if retval:
        raise RuntimeError("Could not image machine: '%s'." % machine.name)
      else:
        self.num_reimages += 1
      machine.checksum = checksum
      machine.image = label.chromeos_image
      machine.label = label

    if not label.chrome_version:
      label.chrome_version = self.GetChromeVersion(machine)

    self.ce.log_level = save_ce_log_level
    return retval

  def ComputeCommonCheckSum(self, label):
    # Since this is used for cache lookups before the machines have been
    # compared/verified, check here to make sure they all have the same
    # checksum (otherwise the cache lookup may not be valid).
    common_checksum = None
    for machine in self.GetMachines(label):
      # Make sure the machine's checksums are calculated.
      if not machine.machine_checksum:
        machine.SetUpChecksumInfo()
      cs = machine.machine_checksum
      # If this is the first machine we've examined, initialize
      # common_checksum.
      if not common_checksum:
        common_checksum = cs
      # Make sure this machine's checksum matches our 'common' checksum.
      if cs != common_checksum:
        raise BadChecksum('Machine checksums do not match!')
    self.machine_checksum[label.name] = common_checksum

  def ComputeCommonCheckSumString(self, label):
    # The assumption is that this function is only called AFTER
    # ComputeCommonCheckSum, so there is no need to verify the machines
    # are the same here.  If this is ever changed, this function should be
    # modified to verify that all the machines for a given label are the
    # same.
    for machine in self.GetMachines(label):
      if machine.checksum_string:
        self.machine_checksum_string[label.name] = machine.checksum_string
        break

  def _TryToLockMachine(self, cros_machine):
    with self._lock:
      assert cros_machine, "Machine can't be None"
      for m in self._machines:
        if m.name == cros_machine.name:
          return
      locked = True
      if self.locks_dir:
        locked = file_lock_machine.Machine(cros_machine.name,
                                           self.locks_dir).Lock(
                                               True, sys.argv[0])
      if locked:
        self._machines.append(cros_machine)
        command = 'cat %s' % CHECKSUM_FILE
        ret, out, _ = self.ce.CrosRunCommandWOutput(
            command,
            chromeos_root=self.chromeos_root,
            machine=cros_machine.name)
        if ret == 0:
          cros_machine.checksum = out.strip()
      elif self.locks_dir:
        self.logger.LogOutput("Couldn't lock: %s" % cros_machine.name)

  # This is called from single threaded mode.
  def AddMachine(self, machine_name):
    with self._lock:
      for m in self._all_machines:
        assert m.name != machine_name, 'Tried to double-add %s' % machine_name

      if self.log_level != 'verbose':
        self.logger.LogOutput('Setting up remote access to %s' % machine_name)
        self.logger.LogOutput(
            'Checking machine characteristics for %s' % machine_name)
      cm = CrosMachine(machine_name, self.chromeos_root, self.log_level)
      if cm.machine_checksum:
        self._all_machines.append(cm)

  def RemoveMachine(self, machine_name):
    with self._lock:
      self._machines = [m for m in self._machines if m.name != machine_name]
      if self.locks_dir:
        res = file_lock_machine.Machine(machine_name,
                                        self.locks_dir).Unlock(True)
        if not res:
          self.logger.LogError("Could not unlock machine: '%s'." % machine_name)

  def ForceSameImageToAllMachines(self, label):
    machines = self.GetMachines(label)
    for m in machines:
      self.ImageMachine(m, label)
      m.SetUpChecksumInfo()

  def AcquireMachine(self, label):
    image_checksum = label.checksum
    machines = self.GetMachines(label)
    check_interval_time = 120
    with self._lock:
      # Lazily external lock machines
      while self.acquire_timeout >= 0:
        for m in machines:
          new_machine = m not in self._all_machines
          self._TryToLockMachine(m)
          if new_machine:
            m.released_time = time.time()
        if self.GetAvailableMachines(label):
          break
        sleep_time = max(1, min(self.acquire_timeout, check_interval_time))
        time.sleep(sleep_time)
        self.acquire_timeout -= sleep_time

      if self.acquire_timeout < 0:
        self.logger.LogFatal(
            'Could not acquire any of the '
            "following machines: '%s'" % ', '.join(machine.name
                                                   for machine in machines))

###      for m in self._machines:
###        if (m.locked and time.time() - m.released_time < 10 and
###            m.checksum == image_checksum):
###          return None
      unlocked_machines = [
          machine for machine in self.GetAvailableMachines(label)
          if not machine.locked
      ]
      for m in unlocked_machines:
        if image_checksum and m.checksum == image_checksum:
          m.locked = True
          m.test_run = threading.current_thread()
          return m
      for m in unlocked_machines:
        if not m.checksum:
          m.locked = True
          m.test_run = threading.current_thread()
          return m
      # This logic ensures that threads waiting on a machine will get a machine
      # with a checksum equal to their image over other threads. This saves time
      # when crosperf initially assigns the machines to threads by minimizing
      # the number of re-images.
      # TODO(asharif): If we centralize the thread-scheduler, we wont need this
      # code and can implement minimal reimaging code more cleanly.
      for m in unlocked_machines:
        if time.time() - m.released_time > 15:
          # The release time gap is too large, so it is probably in the start
          # stage, we need to reset the released_time.
          m.released_time = time.time()
        elif time.time() - m.released_time > 8:
          m.locked = True
          m.test_run = threading.current_thread()
          return m
    return None

  def GetAvailableMachines(self, label=None):
    if not label:
      return self._machines
    return [m for m in self._machines if m.name in label.remote]

  def GetMachines(self, label=None):
    if not label:
      return self._all_machines
    return [m for m in self._all_machines if m.name in label.remote]

  def ReleaseMachine(self, machine):
    with self._lock:
      for m in self._machines:
        if machine.name == m.name:
          assert m.locked, 'Tried to double-release %s' % m.name
          m.released_time = time.time()
          m.locked = False
          m.status = 'Available'
          break

  def Cleanup(self):
    with self._lock:
      # Unlock all machines (via file lock)
      for m in self._machines:
        res = file_lock_machine.Machine(m.name, self.locks_dir).Unlock(True)

        if not res:
          self.logger.LogError("Could not unlock machine: '%s'." % m.name)

  def __str__(self):
    with self._lock:
      l = ['MachineManager Status:'] + [str(m) for m in self._machines]
      return '\n'.join(l)

  def AsString(self):
    with self._lock:
      stringify_fmt = '%-30s %-10s %-4s %-25s %-32s'
      header = stringify_fmt % ('Machine', 'Thread', 'Lock', 'Status',
                                'Checksum')
      table = [header]
      for m in self._machines:
        if m.test_run:
          test_name = m.test_run.name
          test_status = m.test_run.timeline.GetLastEvent()
        else:
          test_name = ''
          test_status = ''

        try:
          machine_string = stringify_fmt % (m.name, test_name, m.locked,
                                            test_status, m.checksum)
        except ValueError:
          machine_string = ''
        table.append(machine_string)
      return 'Machine Status:\n%s' % '\n'.join(table)

  def GetAllCPUInfo(self, labels):
    """Get cpuinfo for labels, merge them if their cpuinfo are the same."""
    dic = collections.defaultdict(list)
    for label in labels:
      for machine in self._all_machines:
        if machine.name in label.remote:
          dic[machine.cpuinfo].append(label.name)
          break
    output_segs = []
    for key, v in dic.iteritems():
      output = ' '.join(v)
      output += '\n-------------------\n'
      output += key
      output += '\n\n\n'
      output_segs.append(output)
    return ''.join(output_segs)

  def GetAllMachines(self):
    return self._all_machines


class MockCrosMachine(CrosMachine):
  """Mock cros machine class."""
  # pylint: disable=super-init-not-called

  MEMINFO_STRING = """MemTotal:        3990332 kB
MemFree:         2608396 kB
Buffers:          147168 kB
Cached:           811560 kB
SwapCached:            0 kB
Active:           503480 kB
Inactive:         628572 kB
Active(anon):     174532 kB
Inactive(anon):    88576 kB
Active(file):     328948 kB
Inactive(file):   539996 kB
Unevictable:           0 kB
Mlocked:               0 kB
SwapTotal:       5845212 kB
SwapFree:        5845212 kB
Dirty:              9384 kB
Writeback:             0 kB
AnonPages:        173408 kB
Mapped:           146268 kB
Shmem:             89676 kB
Slab:             188260 kB
SReclaimable:     169208 kB
SUnreclaim:        19052 kB
KernelStack:        2032 kB
PageTables:         7120 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:     7840376 kB
Committed_AS:    1082032 kB
VmallocTotal:   34359738367 kB
VmallocUsed:      364980 kB
VmallocChunk:   34359369407 kB
DirectMap4k:       45824 kB
DirectMap2M:     4096000 kB
"""

  CPUINFO_STRING = """processor: 0
vendor_id: GenuineIntel
cpu family: 6
model: 42
model name: Intel(R) Celeron(R) CPU 867 @ 1.30GHz
stepping: 7
microcode: 0x25
cpu MHz: 1300.000
cache size: 2048 KB
physical id: 0
siblings: 2
core id: 0
cpu cores: 2
apicid: 0
initial apicid: 0
fpu: yes
fpu_exception: yes
cpuid level: 13
wp: yes
flags: fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx rdtscp lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf pni pclmulqdq dtes64 monitor ds_cpl vmx est tm2 ssse3 cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic popcnt tsc_deadline_timer xsave lahf_lm arat epb xsaveopt pln pts dts tpr_shadow vnmi flexpriority ept vpid
bogomips: 2594.17
clflush size: 64
cache_alignment: 64
address sizes: 36 bits physical, 48 bits virtual
power management:

processor: 1
vendor_id: GenuineIntel
cpu family: 6
model: 42
model name: Intel(R) Celeron(R) CPU 867 @ 1.30GHz
stepping: 7
microcode: 0x25
cpu MHz: 1300.000
cache size: 2048 KB
physical id: 0
siblings: 2
core id: 1
cpu cores: 2
apicid: 2
initial apicid: 2
fpu: yes
fpu_exception: yes
cpuid level: 13
wp: yes
flags: fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx rdtscp lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf pni pclmulqdq dtes64 monitor ds_cpl vmx est tm2 ssse3 cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic popcnt tsc_deadline_timer xsave lahf_lm arat epb xsaveopt pln pts dts tpr_shadow vnmi flexpriority ept vpid
bogomips: 2594.17
clflush size: 64
cache_alignment: 64
address sizes: 36 bits physical, 48 bits virtual
power management:
"""

  def __init__(self, name, chromeos_root, log_level):
    self.name = name
    self.image = None
    self.checksum = None
    self.locked = False
    self.released_time = time.time()
    self.test_run = None
    self.chromeos_root = chromeos_root
    self.checksum_string = re.sub(r'\d', '', name)
    #In test, we assume "lumpy1", "lumpy2" are the same machine.
    self.machine_checksum = self._GetMD5Checksum(self.checksum_string)
    self.log_level = log_level
    self.label = None
    self.ce = command_executer.GetCommandExecuter(log_level=self.log_level)
    self._GetCPUInfo()

  def IsReachable(self):
    return True

  def _GetMemoryInfo(self):
    self.meminfo = self.MEMINFO_STRING
    self._ParseMemoryInfo()

  def _GetCPUInfo(self):
    self.cpuinfo = self.CPUINFO_STRING


class MockMachineManager(MachineManager):
  """Mock machine manager class."""

  def __init__(self, chromeos_root, acquire_timeout, log_level, locks_dir):
    super(MockMachineManager, self).__init__(chromeos_root, acquire_timeout,
                                             log_level, locks_dir)

  def _TryToLockMachine(self, cros_machine):
    self._machines.append(cros_machine)
    cros_machine.checksum = ''

  def AddMachine(self, machine_name):
    with self._lock:
      for m in self._all_machines:
        assert m.name != machine_name, 'Tried to double-add %s' % machine_name
      cm = MockCrosMachine(machine_name, self.chromeos_root, self.log_level)
      assert cm.machine_checksum, (
          'Could not find checksum for machine %s' % machine_name)
      # In Original MachineManager, the test is 'if cm.machine_checksum:' - if a
      # machine is unreachable, then its machine_checksum is None. Here we
      # cannot do this, because machine_checksum is always faked, so we directly
      # test cm.IsReachable, which is properly mocked.
      if cm.IsReachable():
        self._all_machines.append(cm)

  def GetChromeVersion(self, machine):
    return 'Mock Chrome Version R50'

  def AcquireMachine(self, label):
    for machine in self._all_machines:
      if not machine.locked:
        machine.locked = True
        return machine
    return None

  def ImageMachine(self, machine_name, label):
    if machine_name or label:
      return 0
    return 1

  def ReleaseMachine(self, machine):
    machine.locked = False

  def GetMachines(self, label=None):
    return self._all_machines

  def GetAvailableMachines(self, label=None):
    return self._all_machines

  def ForceSameImageToAllMachines(self, label=None):
    return 0

  def ComputeCommonCheckSum(self, label=None):
    common_checksum = 12345
    for machine in self.GetMachines(label):
      machine.machine_checksum = common_checksum
    self.machine_checksum[label.name] = common_checksum

  def GetAllMachines(self):
    return self._all_machines
