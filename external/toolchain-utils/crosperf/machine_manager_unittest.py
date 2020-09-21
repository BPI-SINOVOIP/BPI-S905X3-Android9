#!/usr/bin/env python2

# Copyright 2012 Google Inc. All Rights Reserved.
"""Unittest for machine_manager."""

from __future__ import print_function

import os.path
import time
import hashlib

import mock
import unittest

import label
import machine_manager
import image_checksummer
import test_flag

from benchmark import Benchmark
from benchmark_run import MockBenchmarkRun
from cros_utils import command_executer
from cros_utils import logger

# pylint: disable=protected-access


class MyMachineManager(machine_manager.MachineManager):
  """Machine manager for test."""

  def __init__(self, chromeos_root):
    super(MyMachineManager, self).__init__(chromeos_root, 0, 'average', '')

  def _TryToLockMachine(self, cros_machine):
    self._machines.append(cros_machine)
    cros_machine.checksum = ''

  def AddMachine(self, machine_name):
    with self._lock:
      for m in self._all_machines:
        assert m.name != machine_name, 'Tried to double-add %s' % machine_name
      cm = machine_manager.MockCrosMachine(machine_name, self.chromeos_root,
                                           'average')
      assert cm.machine_checksum, (
          'Could not find checksum for machine %s' % machine_name)
      self._all_machines.append(cm)


CHROMEOS_ROOT = '/tmp/chromeos-root'
MACHINE_NAMES = ['lumpy1', 'lumpy2', 'lumpy3', 'daisy1', 'daisy2']
LABEL_LUMPY = label.MockLabel(
    'lumpy', 'lumpy_chromeos_image', 'autotest_dir', CHROMEOS_ROOT, 'lumpy',
    ['lumpy1', 'lumpy2', 'lumpy3', 'lumpy4'], '', '', False, 'average,'
    'gcc', None)
LABEL_MIX = label.MockLabel('mix', 'chromeos_image', 'autotest_dir',
                            CHROMEOS_ROOT, 'mix',
                            ['daisy1', 'daisy2', 'lumpy3',
                             'lumpy4'], '', '', False, 'average', 'gcc', None)


class MachineManagerTest(unittest.TestCase):
  """Test for machine manager class."""

  msgs = []
  image_log = []
  log_fatal_msgs = []
  fake_logger_count = 0
  fake_logger_msgs = []

  mock_cmd_exec = mock.Mock(spec=command_executer.CommandExecuter)

  mock_logger = mock.Mock(spec=logger.Logger)

  mock_lumpy1 = mock.Mock(spec=machine_manager.CrosMachine)
  mock_lumpy2 = mock.Mock(spec=machine_manager.CrosMachine)
  mock_lumpy3 = mock.Mock(spec=machine_manager.CrosMachine)
  mock_lumpy4 = mock.Mock(spec=machine_manager.CrosMachine)
  mock_daisy1 = mock.Mock(spec=machine_manager.CrosMachine)
  mock_daisy2 = mock.Mock(spec=machine_manager.CrosMachine)

  @mock.patch.object(os.path, 'isdir')

  # pylint: disable=arguments-differ
  def setUp(self, mock_isdir):

    mock_isdir.return_value = True
    self.mm = machine_manager.MachineManager(
        '/usr/local/chromeos', 0, 'average', None, self.mock_cmd_exec,
        self.mock_logger)

    self.mock_lumpy1.name = 'lumpy1'
    self.mock_lumpy2.name = 'lumpy2'
    self.mock_lumpy3.name = 'lumpy3'
    self.mock_lumpy4.name = 'lumpy4'
    self.mock_daisy1.name = 'daisy1'
    self.mock_daisy2.name = 'daisy2'
    self.mock_lumpy1.machine_checksum = 'lumpy123'
    self.mock_lumpy2.machine_checksum = 'lumpy123'
    self.mock_lumpy3.machine_checksum = 'lumpy123'
    self.mock_lumpy4.machine_checksum = 'lumpy123'
    self.mock_daisy1.machine_checksum = 'daisy12'
    self.mock_daisy2.machine_checksum = 'daisy12'
    self.mock_lumpy1.checksum_string = 'lumpy_checksum_str'
    self.mock_lumpy2.checksum_string = 'lumpy_checksum_str'
    self.mock_lumpy3.checksum_string = 'lumpy_checksum_str'
    self.mock_lumpy4.checksum_string = 'lumpy_checksum_str'
    self.mock_daisy1.checksum_string = 'daisy_checksum_str'
    self.mock_daisy2.checksum_string = 'daisy_checksum_str'
    self.mock_lumpy1.cpuinfo = 'lumpy_cpu_info'
    self.mock_lumpy2.cpuinfo = 'lumpy_cpu_info'
    self.mock_lumpy3.cpuinfo = 'lumpy_cpu_info'
    self.mock_lumpy4.cpuinfo = 'lumpy_cpu_info'
    self.mock_daisy1.cpuinfo = 'daisy_cpu_info'
    self.mock_daisy2.cpuinfo = 'daisy_cpu_info'
    self.mm._all_machines.append(self.mock_daisy1)
    self.mm._all_machines.append(self.mock_daisy2)
    self.mm._all_machines.append(self.mock_lumpy1)
    self.mm._all_machines.append(self.mock_lumpy2)
    self.mm._all_machines.append(self.mock_lumpy3)

  def testGetMachines(self):
    manager = MyMachineManager(CHROMEOS_ROOT)
    for m in MACHINE_NAMES:
      manager.AddMachine(m)
    names = [m.name for m in manager.GetMachines(LABEL_LUMPY)]
    self.assertEqual(names, ['lumpy1', 'lumpy2', 'lumpy3'])

  def testGetAvailableMachines(self):
    manager = MyMachineManager(CHROMEOS_ROOT)
    for m in MACHINE_NAMES:
      manager.AddMachine(m)
    for m in manager._all_machines:
      if int(m.name[-1]) % 2:
        manager._TryToLockMachine(m)
    names = [m.name for m in manager.GetAvailableMachines(LABEL_LUMPY)]
    self.assertEqual(names, ['lumpy1', 'lumpy3'])

  @mock.patch.object(time, 'sleep')
  @mock.patch.object(command_executer.CommandExecuter, 'RunCommand')
  @mock.patch.object(command_executer.CommandExecuter, 'CrosRunCommand')
  @mock.patch.object(image_checksummer.ImageChecksummer, 'Checksum')
  def test_image_machine(self, mock_checksummer, mock_run_croscmd, mock_run_cmd,
                         mock_sleep):

    def FakeMD5Checksum(_input_str):
      return 'machine_fake_md5_checksum'

    self.fake_logger_count = 0
    self.fake_logger_msgs = []

    def FakeLogOutput(msg):
      self.fake_logger_count += 1
      self.fake_logger_msgs.append(msg)

    def ResetValues():
      self.fake_logger_count = 0
      self.fake_logger_msgs = []
      mock_run_cmd.reset_mock()
      mock_run_croscmd.reset_mock()
      mock_checksummer.reset_mock()
      mock_sleep.reset_mock()
      machine.checksum = 'fake_md5_checksum'
      self.mm.checksum = None
      self.mm.num_reimages = 0

    self.mock_cmd_exec.CrosRunCommand = mock_run_croscmd
    self.mock_cmd_exec.RunCommand = mock_run_cmd

    self.mm.logger.LogOutput = FakeLogOutput
    machine = self.mock_lumpy1
    machine._GetMD5Checksum = FakeMD5Checksum
    machine.checksum = 'fake_md5_checksum'
    mock_checksummer.return_value = 'fake_md5_checksum'
    self.mock_cmd_exec.log_level = 'verbose'

    test_flag.SetTestMode(True)
    # Test 1: label.image_type == "local"
    LABEL_LUMPY.image_type = 'local'
    self.mm.ImageMachine(machine, LABEL_LUMPY)
    self.assertEqual(mock_run_cmd.call_count, 0)
    self.assertEqual(mock_run_croscmd.call_count, 0)

    #Test 2: label.image_type == "trybot"
    ResetValues()
    LABEL_LUMPY.image_type = 'trybot'
    mock_run_cmd.return_value = 0
    self.mm.ImageMachine(machine, LABEL_LUMPY)
    self.assertEqual(mock_run_croscmd.call_count, 0)
    self.assertEqual(mock_checksummer.call_count, 0)

    # Test 3: label.image_type is neither local nor trybot; retval from
    # RunCommand is 1, i.e. image_chromeos fails...
    ResetValues()
    LABEL_LUMPY.image_type = 'other'
    mock_run_cmd.return_value = 1
    try:
      self.mm.ImageMachine(machine, LABEL_LUMPY)
    except RuntimeError:
      self.assertEqual(mock_checksummer.call_count, 0)
      self.assertEqual(mock_run_cmd.call_count, 2)
      self.assertEqual(mock_run_croscmd.call_count, 1)
      self.assertEqual(mock_sleep.call_count, 1)
      image_call_args_str = mock_run_cmd.call_args[0][0]
      image_call_args = image_call_args_str.split(' ')
      self.assertEqual(image_call_args[0], 'python')
      self.assertEqual(image_call_args[1].split('/')[-1], 'image_chromeos.pyc')
      image_call_args = image_call_args[2:]
      self.assertEqual(image_call_args, [
          '--chromeos_root=/tmp/chromeos-root', '--image=lumpy_chromeos_image',
          '--image_args=', '--remote=lumpy1', '--logging_level=average',
          '--board=lumpy'
      ])
      self.assertEqual(mock_run_croscmd.call_args[0][0], 'reboot && exit')

    # Test 4: Everything works properly. Trybot image type.
    ResetValues()
    LABEL_LUMPY.image_type = 'trybot'
    mock_run_cmd.return_value = 0
    self.mm.ImageMachine(machine, LABEL_LUMPY)
    self.assertEqual(mock_checksummer.call_count, 0)
    self.assertEqual(mock_run_croscmd.call_count, 0)
    self.assertEqual(mock_sleep.call_count, 0)

  def test_compute_common_checksum(self):

    self.mm.machine_checksum = {}
    self.mm.ComputeCommonCheckSum(LABEL_LUMPY)
    self.assertEqual(self.mm.machine_checksum['lumpy'], 'lumpy123')
    self.assertEqual(len(self.mm.machine_checksum), 1)

    self.mm.machine_checksum = {}
    self.assertRaises(machine_manager.BadChecksum,
                      self.mm.ComputeCommonCheckSum, LABEL_MIX)

  def test_compute_common_checksum_string(self):
    self.mm.machine_checksum_string = {}
    self.mm.ComputeCommonCheckSumString(LABEL_LUMPY)
    self.assertEqual(len(self.mm.machine_checksum_string), 1)
    self.assertEqual(self.mm.machine_checksum_string['lumpy'],
                     'lumpy_checksum_str')

    self.mm.machine_checksum_string = {}
    self.mm.ComputeCommonCheckSumString(LABEL_MIX)
    self.assertEqual(len(self.mm.machine_checksum_string), 1)
    self.assertEqual(self.mm.machine_checksum_string['mix'],
                     'daisy_checksum_str')

  @mock.patch.object(command_executer.CommandExecuter, 'CrosRunCommandWOutput')
  def test_try_to_lock_machine(self, mock_cros_runcmd):
    self.assertRaises(self.mm._TryToLockMachine, None)

    mock_cros_runcmd.return_value = [0, 'false_lock_checksum', '']
    self.mock_cmd_exec.CrosRunCommandWOutput = mock_cros_runcmd
    self.mm._machines = []
    self.mm._TryToLockMachine(self.mock_lumpy1)
    self.assertEqual(len(self.mm._machines), 1)
    self.assertEqual(self.mm._machines[0], self.mock_lumpy1)
    self.assertEqual(self.mock_lumpy1.checksum, 'false_lock_checksum')
    self.assertEqual(mock_cros_runcmd.call_count, 1)
    cmd_str = mock_cros_runcmd.call_args[0][0]
    self.assertEqual(cmd_str, 'cat /usr/local/osimage_checksum_file')
    args_dict = mock_cros_runcmd.call_args[1]
    self.assertEqual(len(args_dict), 2)
    self.assertEqual(args_dict['machine'], self.mock_lumpy1.name)
    self.assertEqual(args_dict['chromeos_root'], '/usr/local/chromeos')

  @mock.patch.object(machine_manager, 'CrosMachine')
  def test_add_machine(self, mock_machine):

    mock_machine.machine_checksum = 'daisy123'
    self.assertEqual(len(self.mm._all_machines), 5)
    self.mm.AddMachine('daisy3')
    self.assertEqual(len(self.mm._all_machines), 6)

    self.assertRaises(Exception, self.mm.AddMachine, 'lumpy1')

  def test_remove_machine(self):
    self.mm._machines = self.mm._all_machines
    self.assertTrue(self.mock_lumpy2 in self.mm._machines)
    self.mm.RemoveMachine(self.mock_lumpy2.name)
    self.assertFalse(self.mock_lumpy2 in self.mm._machines)

  def test_force_same_image_to_all_machines(self):
    self.image_log = []

    def FakeImageMachine(machine, label_arg):
      image = label_arg.chromeos_image
      self.image_log.append('Pushed %s onto %s' % (image, machine.name))

    def FakeSetUpChecksumInfo():
      pass

    self.mm.ImageMachine = FakeImageMachine
    self.mock_lumpy1.SetUpChecksumInfo = FakeSetUpChecksumInfo
    self.mock_lumpy2.SetUpChecksumInfo = FakeSetUpChecksumInfo
    self.mock_lumpy3.SetUpChecksumInfo = FakeSetUpChecksumInfo

    self.mm.ForceSameImageToAllMachines(LABEL_LUMPY)
    self.assertEqual(len(self.image_log), 3)
    self.assertEqual(self.image_log[0],
                     'Pushed lumpy_chromeos_image onto lumpy1')
    self.assertEqual(self.image_log[1],
                     'Pushed lumpy_chromeos_image onto lumpy2')
    self.assertEqual(self.image_log[2],
                     'Pushed lumpy_chromeos_image onto lumpy3')

  @mock.patch.object(image_checksummer.ImageChecksummer, 'Checksum')
  @mock.patch.object(hashlib, 'md5')
  def test_acquire_machine(self, mock_md5, mock_checksum):

    self.msgs = []
    self.log_fatal_msgs = []

    def FakeLock(machine):
      self.msgs.append('Tried to lock %s' % machine.name)

    def FakeLogFatal(msg):
      self.log_fatal_msgs.append(msg)

    self.mm._TryToLockMachine = FakeLock
    self.mm.logger.LogFatal = FakeLogFatal

    mock_md5.return_value = '123456'
    mock_checksum.return_value = 'fake_md5_checksum'

    self.mm._machines = self.mm._all_machines
    self.mock_lumpy1.locked = True
    self.mock_lumpy2.locked = True
    self.mock_lumpy3.locked = False
    self.mock_lumpy3.checksum = 'fake_md5_checksum'
    self.mock_daisy1.locked = True
    self.mock_daisy2.locked = False
    self.mock_daisy2.checksum = 'fake_md5_checksum'

    self.mock_lumpy1.released_time = time.time()
    self.mock_lumpy2.released_time = time.time()
    self.mock_lumpy3.released_time = time.time()
    self.mock_daisy1.released_time = time.time()
    self.mock_daisy2.released_time = time.time()

    # Test 1. Basic test. Acquire lumpy3.
    self.mm.AcquireMachine(LABEL_LUMPY)
    m = self.mock_lumpy1
    self.assertEqual(m, self.mock_lumpy1)
    self.assertTrue(self.mock_lumpy1.locked)
    self.assertEqual(mock_md5.call_count, 0)
    self.assertEqual(self.msgs, [
        'Tried to lock lumpy1', 'Tried to lock lumpy2', 'Tried to lock lumpy3'
    ])

    # Test the second return statment (machine is unlocked, has no checksum)
    save_locked = self.mock_lumpy1.locked
    self.mock_lumpy1.locked = False
    self.mock_lumpy1.checksum = None
    m = self.mm.AcquireMachine(LABEL_LUMPY)
    self.assertEqual(m, self.mock_lumpy1)
    self.assertTrue(self.mock_lumpy1.locked)

    # Test the third return statement:
    #   - machine is unlocked
    #   - checksums don't match
    #   - current time minus release time is > 20.
    self.mock_lumpy1.locked = False
    self.mock_lumpy1.checksum = '123'
    self.mock_lumpy1.released_time = time.time() - 8
    m = self.mm.AcquireMachine(LABEL_LUMPY)
    self.assertEqual(m, self.mock_lumpy1)
    self.assertTrue(self.mock_lumpy1.locked)

    # Test all machines are already locked.
    m = self.mm.AcquireMachine(LABEL_LUMPY)
    self.assertIsNone(m)

    # Restore values of mock_lumpy1, so other tests succeed.
    self.mock_lumpy1.locked = save_locked
    self.mock_lumpy1.checksum = '123'

  def test_get_available_machines(self):
    self.mm._machines = self.mm._all_machines

    machine_list = self.mm.GetAvailableMachines()
    self.assertEqual(machine_list, self.mm._all_machines)

    machine_list = self.mm.GetAvailableMachines(LABEL_MIX)
    self.assertEqual(machine_list,
                     [self.mock_daisy1, self.mock_daisy2, self.mock_lumpy3])

    machine_list = self.mm.GetAvailableMachines(LABEL_LUMPY)
    self.assertEqual(machine_list,
                     [self.mock_lumpy1, self.mock_lumpy2, self.mock_lumpy3])

  def test_get_machines(self):
    machine_list = self.mm.GetMachines()
    self.assertEqual(machine_list, self.mm._all_machines)

    machine_list = self.mm.GetMachines(LABEL_MIX)
    self.assertEqual(machine_list,
                     [self.mock_daisy1, self.mock_daisy2, self.mock_lumpy3])

    machine_list = self.mm.GetMachines(LABEL_LUMPY)
    self.assertEqual(machine_list,
                     [self.mock_lumpy1, self.mock_lumpy2, self.mock_lumpy3])

  def test_release_machines(self):

    self.mm._machines = [self.mock_lumpy1, self.mock_daisy2]

    self.mock_lumpy1.locked = True
    self.mock_daisy2.locked = True

    self.assertTrue(self.mock_lumpy1.locked)
    self.mm.ReleaseMachine(self.mock_lumpy1)
    self.assertFalse(self.mock_lumpy1.locked)
    self.assertEqual(self.mock_lumpy1.status, 'Available')

    self.assertTrue(self.mock_daisy2.locked)
    self.mm.ReleaseMachine(self.mock_daisy2)
    self.assertFalse(self.mock_daisy2.locked)
    self.assertEqual(self.mock_daisy2.status, 'Available')

    # Test double-relase...
    self.assertRaises(AssertionError, self.mm.ReleaseMachine, self.mock_lumpy1)

  def test_cleanup(self):
    self.mock_logger.reset_mock()
    self.mm.Cleanup()
    self.assertEqual(self.mock_logger.call_count, 0)

  OUTPUT_STR = ('Machine Status:\nMachine                        Thread     '
                'Lock Status                    Checksum'
                '                        \nlumpy1                         test '
                'run   True PENDING                   123'
                '                             \nlumpy2                         '
                'test run   False PENDING                   123'
                '                             \nlumpy3                         '
                'test run   False PENDING                   123'
                '                             \ndaisy1                         '
                'test run   False PENDING                   678'
                '                             \ndaisy2                         '
                'test run   True PENDING                   678'
                '                             ')

  def test_as_string(self):

    mock_logger = mock.Mock(spec=logger.Logger)

    bench = Benchmark(
        'page_cycler_v2.netsim.top_10',  # name
        'page_cycler_v2.netsim.top_10',  # test_name
        '',  # test_args
        1,  # iteratins
        False,  # rm_chroot_tmp
        '',  # perf_args
        suite='telemetry_Crosperf')  # suite

    test_run = MockBenchmarkRun('test run', bench, LABEL_LUMPY, 1, [], self.mm,
                                mock_logger, 'verbose', '')

    self.mm._machines = [
        self.mock_lumpy1, self.mock_lumpy2, self.mock_lumpy3, self.mock_daisy1,
        self.mock_daisy2
    ]

    self.mock_lumpy1.test_run = test_run
    self.mock_lumpy2.test_run = test_run
    self.mock_lumpy3.test_run = test_run
    self.mock_daisy1.test_run = test_run
    self.mock_daisy2.test_run = test_run

    self.mock_lumpy1.locked = True
    self.mock_lumpy2.locked = False
    self.mock_lumpy3.locked = False
    self.mock_daisy1.locked = False
    self.mock_daisy2.locked = True

    self.mock_lumpy1.checksum = '123'
    self.mock_lumpy2.checksum = '123'
    self.mock_lumpy3.checksum = '123'
    self.mock_daisy1.checksum = '678'
    self.mock_daisy2.checksum = '678'

    output = self.mm.AsString()
    self.assertEqual(output, self.OUTPUT_STR)

  def test_get_all_cpu_info(self):
    info = self.mm.GetAllCPUInfo([LABEL_LUMPY, LABEL_MIX])
    self.assertEqual(info,
                     'lumpy\n-------------------\nlumpy_cpu_info\n\n\nmix\n-'
                     '------------------\ndaisy_cpu_info\n\n\n')


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

CHECKSUM_STRING = ('processor: 0vendor_id: GenuineIntelcpu family: 6model: '
                   '42model name: Intel(R) Celeron(R) CPU 867 @ '
                   '1.30GHzstepping: 7microcode: 0x25cache size: 2048 '
                   'KBphysical id: 0siblings: 2core id: 0cpu cores: 2apicid: '
                   '0initial apicid: 0fpu: yesfpu_exception: yescpuid level: '
                   '13wp: yesflags: fpu vme de pse tsc msr pae mce cx8 apic sep'
                   ' mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse '
                   'sse2 ss ht tm pbe syscall nx rdtscp lm constant_tsc '
                   'arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc '
                   'aperfmperf pni pclmulqdq dtes64 monitor ds_cpl vmx est tm2 '
                   'ssse3 cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic popcnt '
                   'tsc_deadline_timer xsave lahf_lm arat epb xsaveopt pln pts '
                   'dts tpr_shadow vnmi flexpriority ept vpidclflush size: '
                   '64cache_alignment: 64address sizes: 36 bits physical, 48 '
                   'bits virtualpower management:processor: 1vendor_id: '
                   'GenuineIntelcpu family: 6model: 42model name: Intel(R) '
                   'Celeron(R) CPU 867 @ 1.30GHzstepping: 7microcode: 0x25cache'
                   ' size: 2048 KBphysical id: 0siblings: 2core id: 1cpu cores:'
                   ' 2apicid: 2initial apicid: 2fpu: yesfpu_exception: yescpuid'
                   ' level: 13wp: yesflags: fpu vme de pse tsc msr pae mce cx8 '
                   'apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx '
                   'fxsr sse sse2 ss ht tm pbe syscall nx rdtscp lm '
                   'constant_tsc arch_perfmon pebs bts rep_good nopl xtopology '
                   'nonstop_tsc aperfmperf pni pclmulqdq dtes64 monitor ds_cpl '
                   'vmx est tm2 ssse3 cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic '
                   'popcnt tsc_deadline_timer xsave lahf_lm arat epb xsaveopt '
                   'pln pts dts tpr_shadow vnmi flexpriority ept vpidclflush '
                   'size: 64cache_alignment: 64address sizes: 36 bits physical,'
                   ' 48 bits virtualpower management: 4194304')

DUMP_VPD_STRING = """
"PBA_SN"="Pba.txt"
"Product_S/N"="HT4L91SC300208"
"serial_number"="HT4L91SC300208Z"
"System_UUID"="12153006-1755-4f66-b410-c43758a71127"
"shipping_country"="US"
"initial_locale"="en-US"
"keyboard_layout"="xkb:us::eng"
"initial_timezone"="America/Los_Angeles"
"MACAddress"=""
"System_UUID"="29dd9c61-7fa1-4c83-b89a-502e7eb08afe"
"ubind_attribute"="0c433ce7585f486730b682bb05626a12ce2d896e9b57665387f8ce2ccfdcc56d2e2f1483"
"gbind_attribute"="7e9a851324088e269319347c6abb8d1572ec31022fa07e28998229afe8acb45c35a89b9d"
"ActivateDate"="2013-38"
"""

IFCONFIG_STRING = """
eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 172.17.129.247  netmask 255.255.254.0  broadcast 172.17.129.255
        inet6 2620:0:1000:3002:143:fed4:3ff6:279d  prefixlen 64  scopeid 0x0<global>
        inet6 2620:0:1000:3002:4459:1399:1f02:9e4c  prefixlen 64  scopeid 0x0<global>
        inet6 2620:0:1000:3002:d9e4:87b:d4ec:9a0e  prefixlen 64  scopeid 0x0<global>
        inet6 2620:0:1000:3002:7d45:23f1:ea8a:9604  prefixlen 64  scopeid 0x0<global>
        inet6 2620:0:1000:3002:250:b6ff:fe63:db65  prefixlen 64  scopeid 0x0<global>
        inet6 fe80::250:b6ff:fe63:db65  prefixlen 64  scopeid 0x20<link>
        ether 00:50:b6:63:db:65  txqueuelen 1000  (Ethernet)
        RX packets 9817166  bytes 10865181708 (10.1 GiB)
        RX errors 194  dropped 0  overruns 0  frame 194
        TX packets 0  bytes 2265811903 (2.1 GiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

eth1: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
        ether e8:03:9a:9c:50:3d  txqueuelen 1000  (Ethernet)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 16436
        inet 127.0.0.1  netmask 255.0.0.0
        inet6 ::1  prefixlen 128  scopeid 0x10<host>
        loop  txqueuelen 0  (Local Loopback)
        RX packets 981004  bytes 1127468524 (1.0 GiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 981004  bytes 1127468524 (1.0 GiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

wlan0: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
        ether 44:6d:57:20:4a:c5  txqueuelen 1000  (Ethernet)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
"""


class CrosMachineTest(unittest.TestCase):
  """Test for CrosMachine class."""

  mock_cmd_exec = mock.Mock(spec=command_executer.CommandExecuter)

  @mock.patch.object(machine_manager.CrosMachine, 'SetUpChecksumInfo')
  def test_init(self, mock_setup):

    cm = machine_manager.CrosMachine('daisy.cros', '/usr/local/chromeos',
                                     'average', self.mock_cmd_exec)
    self.assertEqual(mock_setup.call_count, 1)
    self.assertEqual(cm.chromeos_root, '/usr/local/chromeos')
    self.assertEqual(cm.log_level, 'average')

  @mock.patch.object(machine_manager.CrosMachine, 'IsReachable')
  @mock.patch.object(machine_manager.CrosMachine, '_GetMemoryInfo')
  @mock.patch.object(machine_manager.CrosMachine, '_GetCPUInfo')
  @mock.patch.object(machine_manager.CrosMachine,
                     '_ComputeMachineChecksumString')
  @mock.patch.object(machine_manager.CrosMachine, '_GetMachineID')
  @mock.patch.object(machine_manager.CrosMachine, '_GetMD5Checksum')
  def test_setup_checksum_info(self, mock_md5sum, mock_machineid,
                               mock_checkstring, mock_cpuinfo, mock_meminfo,
                               mock_isreachable):

    # Test 1. Machine is not reachable; SetUpChecksumInfo is called via
    # __init__.
    mock_isreachable.return_value = False
    mock_md5sum.return_value = 'md5_checksum'
    cm = machine_manager.CrosMachine('daisy.cros', '/usr/local/chromeos',
                                     'average', self.mock_cmd_exec)
    cm.checksum_string = 'This is a checksum string.'
    cm.machine_id = 'machine_id1'
    self.assertEqual(mock_isreachable.call_count, 1)
    self.assertIsNone(cm.machine_checksum)
    self.assertEqual(mock_meminfo.call_count, 0)

    # Test 2. Machine is reachable. Call explicitly.
    mock_isreachable.return_value = True
    cm.checksum_string = 'This is a checksum string.'
    cm.machine_id = 'machine_id1'
    cm.SetUpChecksumInfo()
    self.assertEqual(mock_isreachable.call_count, 2)
    self.assertEqual(mock_meminfo.call_count, 1)
    self.assertEqual(mock_cpuinfo.call_count, 1)
    self.assertEqual(mock_checkstring.call_count, 1)
    self.assertEqual(mock_machineid.call_count, 1)
    self.assertEqual(mock_md5sum.call_count, 2)
    self.assertEqual(cm.machine_checksum, 'md5_checksum')
    self.assertEqual(cm.machine_id_checksum, 'md5_checksum')
    self.assertEqual(mock_md5sum.call_args_list[0][0][0],
                     'This is a checksum string.')
    self.assertEqual(mock_md5sum.call_args_list[1][0][0], 'machine_id1')

  @mock.patch.object(command_executer.CommandExecuter, 'CrosRunCommand')
  @mock.patch.object(machine_manager.CrosMachine, 'SetUpChecksumInfo')
  def test_is_reachable(self, mock_setup, mock_run_cmd):

    cm = machine_manager.CrosMachine('daisy.cros', '/usr/local/chromeos',
                                     'average', self.mock_cmd_exec)
    self.mock_cmd_exec.CrosRunCommand = mock_run_cmd

    # Test 1. CrosRunCommand returns 1 (fail)
    mock_run_cmd.return_value = 1
    result = cm.IsReachable()
    self.assertFalse(result)
    self.assertEqual(mock_setup.call_count, 1)
    self.assertEqual(mock_run_cmd.call_count, 1)

    # Test 2. CrosRunCommand returns 0 (success)
    mock_run_cmd.return_value = 0
    result = cm.IsReachable()
    self.assertTrue(result)
    self.assertEqual(mock_run_cmd.call_count, 2)
    first_args = mock_run_cmd.call_args_list[0]
    second_args = mock_run_cmd.call_args_list[1]
    self.assertEqual(first_args[0], second_args[0])
    self.assertEqual(first_args[1], second_args[1])
    self.assertEqual(len(first_args[0]), 1)
    self.assertEqual(len(first_args[1]), 2)
    self.assertEqual(first_args[0][0], 'ls')
    args_dict = first_args[1]
    self.assertEqual(args_dict['machine'], 'daisy.cros')
    self.assertEqual(args_dict['chromeos_root'], '/usr/local/chromeos')

  @mock.patch.object(machine_manager.CrosMachine, 'SetUpChecksumInfo')
  def test_parse_memory_info(self, _mock_setup):
    cm = machine_manager.CrosMachine('daisy.cros', '/usr/local/chromeos',
                                     'average', self.mock_cmd_exec)
    cm.meminfo = MEMINFO_STRING
    cm._ParseMemoryInfo()
    self.assertEqual(cm.phys_kbytes, 4194304)

  @mock.patch.object(command_executer.CommandExecuter, 'CrosRunCommandWOutput')
  @mock.patch.object(machine_manager.CrosMachine, 'SetUpChecksumInfo')
  def test_get_memory_info(self, _mock_setup, mock_run_cmd):
    cm = machine_manager.CrosMachine('daisy.cros', '/usr/local/chromeos',
                                     'average', self.mock_cmd_exec)
    self.mock_cmd_exec.CrosRunCommandWOutput = mock_run_cmd
    mock_run_cmd.return_value = [0, MEMINFO_STRING, '']
    cm._GetMemoryInfo()
    self.assertEqual(mock_run_cmd.call_count, 1)
    call_args = mock_run_cmd.call_args_list[0]
    self.assertEqual(call_args[0][0], 'cat /proc/meminfo')
    args_dict = call_args[1]
    self.assertEqual(args_dict['machine'], 'daisy.cros')
    self.assertEqual(args_dict['chromeos_root'], '/usr/local/chromeos')
    self.assertEqual(cm.meminfo, MEMINFO_STRING)
    self.assertEqual(cm.phys_kbytes, 4194304)

    mock_run_cmd.return_value = [1, MEMINFO_STRING, '']
    self.assertRaises(cm._GetMemoryInfo)

  @mock.patch.object(command_executer.CommandExecuter, 'CrosRunCommandWOutput')
  @mock.patch.object(machine_manager.CrosMachine, 'SetUpChecksumInfo')
  def test_get_cpu_info(self, _mock_setup, mock_run_cmd):
    cm = machine_manager.CrosMachine('daisy.cros', '/usr/local/chromeos',
                                     'average', self.mock_cmd_exec)
    self.mock_cmd_exec.CrosRunCommandWOutput = mock_run_cmd
    mock_run_cmd.return_value = [0, CPUINFO_STRING, '']
    cm._GetCPUInfo()
    self.assertEqual(mock_run_cmd.call_count, 1)
    call_args = mock_run_cmd.call_args_list[0]
    self.assertEqual(call_args[0][0], 'cat /proc/cpuinfo')
    args_dict = call_args[1]
    self.assertEqual(args_dict['machine'], 'daisy.cros')
    self.assertEqual(args_dict['chromeos_root'], '/usr/local/chromeos')
    self.assertEqual(cm.cpuinfo, CPUINFO_STRING)

  @mock.patch.object(machine_manager.CrosMachine, 'SetUpChecksumInfo')
  def test_compute_machine_checksum_string(self, _mock_setup):
    cm = machine_manager.CrosMachine('daisy.cros', '/usr/local/chromeos',
                                     'average', self.mock_cmd_exec)
    cm.cpuinfo = CPUINFO_STRING
    cm.meminfo = MEMINFO_STRING
    cm._ParseMemoryInfo()
    cm._ComputeMachineChecksumString()
    self.assertEqual(cm.checksum_string, CHECKSUM_STRING)

  @mock.patch.object(machine_manager.CrosMachine, 'SetUpChecksumInfo')
  def test_get_md5_checksum(self, _mock_setup):
    cm = machine_manager.CrosMachine('daisy.cros', '/usr/local/chromeos',
                                     'average', self.mock_cmd_exec)
    temp_str = 'abcde'
    checksum_str = cm._GetMD5Checksum(temp_str)
    self.assertEqual(checksum_str, 'ab56b4d92b40713acc5af89985d4b786')

    temp_str = ''
    checksum_str = cm._GetMD5Checksum(temp_str)
    self.assertEqual(checksum_str, '')

  @mock.patch.object(command_executer.CommandExecuter, 'CrosRunCommandWOutput')
  @mock.patch.object(machine_manager.CrosMachine, 'SetUpChecksumInfo')
  def test_get_machine_id(self, _mock_setup, mock_run_cmd):
    cm = machine_manager.CrosMachine('daisy.cros', '/usr/local/chromeos',
                                     'average', self.mock_cmd_exec)
    self.mock_cmd_exec.CrosRunCommandWOutput = mock_run_cmd
    mock_run_cmd.return_value = [0, DUMP_VPD_STRING, '']

    cm._GetMachineID()
    self.assertEqual(cm.machine_id, '"Product_S/N"="HT4L91SC300208"')

    mock_run_cmd.return_value = [0, IFCONFIG_STRING, '']
    cm._GetMachineID()
    self.assertEqual(
        cm.machine_id,
        '        ether 00:50:b6:63:db:65  txqueuelen 1000  (Ethernet)_        '
        'ether e8:03:9a:9c:50:3d  txqueuelen 1000  (Ethernet)_        ether '
        '44:6d:57:20:4a:c5  txqueuelen 1000  (Ethernet)')

    mock_run_cmd.return_value = [0, 'invalid hardware config', '']
    self.assertRaises(cm._GetMachineID)


if __name__ == '__main__':
  unittest.main()
