# Copyright (c) 2013~2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""SuiteRunner defines the interface from crosperf to test script."""

from __future__ import print_function

import os
import time
import shlex

from cros_utils import command_executer
import test_flag

TEST_THAT_PATH = '/usr/bin/test_that'
AUTOTEST_DIR = '~/trunk/src/third_party/autotest/files'
CHROME_MOUNT_DIR = '/tmp/chrome_root'


def GetProfilerArgs(profiler_args):
  # Remove "--" from in front of profiler args.
  args_list = shlex.split(profiler_args)
  new_list = []
  for arg in args_list:
    if arg[0:2] == '--':
      arg = arg[2:]
    new_list.append(arg)
  args_list = new_list

  # Remove "perf_options=" from middle of profiler args.
  new_list = []
  for arg in args_list:
    idx = arg.find('perf_options=')
    if idx != -1:
      prefix = arg[0:idx]
      suffix = arg[idx + len('perf_options=') + 1:-1]
      new_arg = prefix + "'" + suffix + "'"
      new_list.append(new_arg)
    else:
      new_list.append(arg)
  args_list = new_list

  return ' '.join(args_list)


class SuiteRunner(object):
  """This defines the interface from crosperf to test script."""

  def __init__(self,
               logger_to_use=None,
               log_level='verbose',
               cmd_exec=None,
               cmd_term=None):
    self.logger = logger_to_use
    self.log_level = log_level
    self._ce = cmd_exec or command_executer.GetCommandExecuter(
        self.logger, log_level=self.log_level)
    self._ct = cmd_term or command_executer.CommandTerminator()

  def Run(self, machine, label, benchmark, test_args, profiler_args):
    for i in range(0, benchmark.retries + 1):
      self.PinGovernorExecutionFrequencies(machine, label.chromeos_root)
      if benchmark.suite == 'telemetry':
        self.DecreaseWaitTime(machine, label.chromeos_root)
        ret_tup = self.Telemetry_Run(machine, label, benchmark, profiler_args)
      elif benchmark.suite == 'telemetry_Crosperf':
        self.DecreaseWaitTime(machine, label.chromeos_root)
        ret_tup = self.Telemetry_Crosperf_Run(machine, label, benchmark,
                                              test_args, profiler_args)
      else:
        ret_tup = self.Test_That_Run(machine, label, benchmark, test_args,
                                     profiler_args)
      if ret_tup[0] != 0:
        self.logger.LogOutput('benchmark %s failed. Retries left: %s' %
                              (benchmark.name, benchmark.retries - i))
      elif i > 0:
        self.logger.LogOutput('benchmark %s succeded after %s retries' %
                              (benchmark.name, i))
        break
      else:
        self.logger.LogOutput(
            'benchmark %s succeded on first try' % benchmark.name)
        break
    return ret_tup

  def PinGovernorExecutionFrequencies(self, machine_name, chromeos_root):
    """Set min and max frequencies to max static frequency."""
    # pyformat: disable
    set_cpu_freq = (
        'set -e && '
        # Disable Turbo in Intel pstate driver
        'if [[ -e /sys/devices/system/cpu/intel_pstate/no_turbo ]]; then '
        'echo -n 1 > /sys/devices/system/cpu/intel_pstate/no_turbo; fi; '
        # Set governor to performance for each cpu
        'for f in /sys/devices/system/cpu/cpu*/cpufreq; do '
        'cd $f; '
        'echo performance > scaling_governor; '
        # Uncomment rest of lines to enable setting frequency by crosperf
        #'val=0; '
        #'if [[ -e scaling_available_frequencies ]]; then '
        # pylint: disable=line-too-long
        #'  val=`cat scaling_available_frequencies | tr " " "\\n" | sort -n -b -r`; '
        #'else '
        #'  val=`cat scaling_max_freq | tr " " "\\n" | sort -n -b -r`; fi ; '
        #'set -- $val; '
        #'highest=$1; '
        #'if [[ $# -gt 1 ]]; then '
        #'  case $highest in *1000) highest=$2;; esac; '
        #'fi ;'
        #'echo $highest > scaling_max_freq; '
        #'echo $highest > scaling_min_freq; '
        'done'
    )
    # pyformat: enable
    if self.log_level == 'average':
      self.logger.LogOutput(
          'Pinning governor execution frequencies for %s' % machine_name)
    ret = self._ce.CrosRunCommand(
        set_cpu_freq, machine=machine_name, chromeos_root=chromeos_root)
    self.logger.LogFatalIf(
        ret, 'Could not pin frequencies on machine: %s' % machine_name)

  def DecreaseWaitTime(self, machine_name, chromeos_root):
    """Change the ten seconds wait time for pagecycler to two seconds."""
    FILE = '/usr/local/telemetry/src/tools/perf/page_sets/page_cycler_story.py'
    ret = self._ce.CrosRunCommand(
        'ls ' + FILE, machine=machine_name, chromeos_root=chromeos_root)
    self.logger.LogFatalIf(ret, 'Could not find {} on machine: {}'.format(
        FILE, machine_name))

    if not ret:
      sed_command = 'sed -i "s/_TTI_WAIT_TIME = 10/_TTI_WAIT_TIME = 2/g" '
      ret = self._ce.CrosRunCommand(
          sed_command + FILE, machine=machine_name, chromeos_root=chromeos_root)
      self.logger.LogFatalIf(ret, 'Could not modify {} on machine: {}'.format(
          FILE, machine_name))

  def RebootMachine(self, machine_name, chromeos_root):
    command = 'reboot && exit'
    self._ce.CrosRunCommand(
        command, machine=machine_name, chromeos_root=chromeos_root)
    time.sleep(60)
    # Whenever we reboot the machine, we need to restore the governor settings.
    self.PinGovernorExecutionFrequencies(machine_name, chromeos_root)

  def Test_That_Run(self, machine, label, benchmark, test_args, profiler_args):
    """Run the test_that test.."""
    options = ''
    if label.board:
      options += ' --board=%s' % label.board
    if test_args:
      options += ' %s' % test_args
    if profiler_args:
      self.logger.LogFatal('test_that does not support profiler.')
    command = 'rm -rf /usr/local/autotest/results/*'
    self._ce.CrosRunCommand(
        command, machine=machine, chromeos_root=label.chromeos_root)

    # We do this because some tests leave the machine in weird states.
    # Rebooting between iterations has proven to help with this.
    self.RebootMachine(machine, label.chromeos_root)

    autotest_dir = AUTOTEST_DIR
    if label.autotest_path != '':
      autotest_dir = label.autotest_path

    autotest_dir_arg = '--autotest_dir %s' % autotest_dir
    # For non-telemetry tests, specify an autotest directory only if the
    # specified directory is different from default (crosbug.com/679001).
    if autotest_dir == AUTOTEST_DIR:
      autotest_dir_arg = ''

    command = (('%s %s --fast '
                '%s %s %s') % (TEST_THAT_PATH, autotest_dir_arg, options,
                               machine, benchmark.test_name))
    if self.log_level != 'verbose':
      self.logger.LogOutput('Running test.')
      self.logger.LogOutput('CMD: %s' % command)
    # Use --no-ns-pid so that cros_sdk does not create a different
    # process namespace and we can kill process created easily by
    # their process group.
    return self._ce.ChrootRunCommandWOutput(
        label.chromeos_root,
        command,
        command_terminator=self._ct,
        cros_sdk_options='--no-ns-pid')

  def RemoveTelemetryTempFile(self, machine, chromeos_root):
    filename = 'telemetry@%s' % machine
    fullname = os.path.join(chromeos_root, 'chroot', 'tmp', filename)
    if os.path.exists(fullname):
      os.remove(fullname)

  def Telemetry_Crosperf_Run(self, machine, label, benchmark, test_args,
                             profiler_args):
    if not os.path.isdir(label.chrome_src):
      self.logger.LogFatal('Cannot find chrome src dir to'
                           ' run telemetry: %s' % label.chrome_src)

    # Check for and remove temporary file that may have been left by
    # previous telemetry runs (and which might prevent this run from
    # working).
    self.RemoveTelemetryTempFile(machine, label.chromeos_root)

    # For telemetry runs, we can use the autotest copy from the source
    # location. No need to have one under /build/<board>.
    autotest_dir_arg = '--autotest_dir %s' % AUTOTEST_DIR
    if label.autotest_path != '':
      autotest_dir_arg = '--autotest_dir %s' % label.autotest_path

    profiler_args = GetProfilerArgs(profiler_args)
    fast_arg = ''
    if not profiler_args:
      # --fast works unless we are doing profiling (autotest limitation).
      # --fast avoids unnecessary copies of syslogs.
      fast_arg = '--fast'
    args_string = ''
    if test_args:
      # Strip double quotes off args (so we can wrap them in single
      # quotes, to pass through to Telemetry).
      if test_args[0] == '"' and test_args[-1] == '"':
        test_args = test_args[1:-1]
      args_string = "test_args='%s'" % test_args

    cmd = ('{} {} {} --board={} --args="{} run_local={} test={} '
           '{}" {} telemetry_Crosperf'.format(
               TEST_THAT_PATH, autotest_dir_arg, fast_arg, label.board,
               args_string, benchmark.run_local, benchmark.test_name,
               profiler_args, machine))

    # Use --no-ns-pid so that cros_sdk does not create a different
    # process namespace and we can kill process created easily by their
    # process group.
    chrome_root_options = ('--no-ns-pid '
                           '--chrome_root={} --chrome_root_mount={} '
                           "FEATURES=\"-usersandbox\" "
                           'CHROME_ROOT={}'.format(label.chrome_src,
                                                   CHROME_MOUNT_DIR,
                                                   CHROME_MOUNT_DIR))
    if self.log_level != 'verbose':
      self.logger.LogOutput('Running test.')
      self.logger.LogOutput('CMD: %s' % cmd)
    return self._ce.ChrootRunCommandWOutput(
        label.chromeos_root,
        cmd,
        command_terminator=self._ct,
        cros_sdk_options=chrome_root_options)

  def Telemetry_Run(self, machine, label, benchmark, profiler_args):
    telemetry_run_path = ''
    if not os.path.isdir(label.chrome_src):
      self.logger.LogFatal('Cannot find chrome src dir to' ' run telemetry.')
    else:
      telemetry_run_path = os.path.join(label.chrome_src, 'src/tools/perf')
      if not os.path.exists(telemetry_run_path):
        self.logger.LogFatal('Cannot find %s directory.' % telemetry_run_path)

    if profiler_args:
      self.logger.LogFatal('Telemetry does not support the perf profiler.')

    # Check for and remove temporary file that may have been left by
    # previous telemetry runs (and which might prevent this run from
    # working).
    if not test_flag.GetTestMode():
      self.RemoveTelemetryTempFile(machine, label.chromeos_root)

    rsa_key = os.path.join(
        label.chromeos_root,
        'src/scripts/mod_for_test_scripts/ssh_keys/testing_rsa')

    cmd = ('cd {0} && '
           './run_measurement '
           '--browser=cros-chrome '
           '--output-format=csv '
           '--remote={1} '
           '--identity {2} '
           '{3} {4}'.format(telemetry_run_path, machine, rsa_key,
                            benchmark.test_name, benchmark.test_args))
    if self.log_level != 'verbose':
      self.logger.LogOutput('Running test.')
      self.logger.LogOutput('CMD: %s' % cmd)
    return self._ce.RunCommandWOutput(cmd, print_to_console=False)

  def CommandTerminator(self):
    return self._ct

  def Terminate(self):
    self._ct.Terminate()


class MockSuiteRunner(object):
  """Mock suite runner for test."""

  def __init__(self):
    self._true = True

  def Run(self, *_args):
    if self._true:
      return [0, '', '']
    else:
      return [0, '', '']
