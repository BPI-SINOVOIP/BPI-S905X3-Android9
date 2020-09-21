#! /usr/bin/python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""The gdb dejagnu test wrapper."""
import optparse
import os
from os import path
import re
import shutil
import stat
import sys
import tempfile
import time

from cros_utils import command_executer
from cros_utils import logger
from cros_utils import misc

from run_dejagnu import TryAcquireMachine

_VALID_TEST_RESULTS = ['FAIL', 'UNRESOLVED', 'XPASS', 'ERROR', 'UNSUPPORTED',
                       'PASS']


def ProcessArguments(argv):
  """Processing/validating script arguments."""
  parser = optparse.OptionParser(description=(
      'Launches gdb dejagnu test in chroot for chromeos toolchain, compares '
      'the test result with a repository baseline and prints out the result.'),
                                 usage='run_dejagnu options')
  parser.add_option('-c',
                    '--chromeos_root',
                    dest='chromeos_root',
                    help='Required. Specify chromeos root')
  parser.add_option('-m',
                    '--mount',
                    dest='mount',
                    help=('Specify gdb source to mount instead of "auto". '
                          'Under "auto" mode, which is the default - gdb is '
                          'checked out and built automatically at default '
                          'directories. Under "mount" mode '
                          '- the gdb_source is set to "$chromeos_'
                          'root/chroot/usr/local/toolchain_root/gdb", which is '
                          'the mount point for this option value.'))
  parser.add_option('-b',
                    '--board',
                    dest='board',
                    help=('Required. Specify board.'))
  parser.add_option('-r',
                    '--remote',
                    dest='remote',
                    help=('Required. Specify addresses/names of the board, '
                          'seperate each address/name using comma(\',\').'))
  parser.add_option('--cleanup',
                    dest='cleanup',
                    default=None,
                    help=('Optional. Values to this option could be '
                          '\'chroot\' (delete chroot) and '
                          '\'chromeos\' (delete the whole chromeos tree).'))

  options, args = parser.parse_args(argv)

  if not options.chromeos_root:
    raise SyntaxError('Missing argument for --chromeos_root.')
  if not options.remote:
    raise SyntaxError('Missing argument for --remote.')
  if not options.board:
    raise SyntaxError('Missing argument for --board.')
  if options.cleanup == 'mount' and not options.mount:
    raise SyntaxError('--cleanup=\'mount\' not valid unless --mount is given.')
  if options.cleanup and not (options.cleanup == 'mount' or
                              options.cleanup == 'chroot' or
                              options.cleanup == 'chromeos'):
    raise SyntaxError('Invalid option value for --cleanup')

  return options


class DejagnuExecuter(object):
  """The class wrapper for dejagnu test executer."""

  def __init__(self, base_dir, source_dir, chromeos_root, remote, board,
               cleanup):
    self._l = logger.GetLogger()
    self._chromeos_root = chromeos_root
    self._chromeos_chroot = path.join(chromeos_root, 'chroot')

    self._remote = remote
    self._board = board
    ## Compute target from board
    self._target = misc.GetCtargetFromBoard(board, chromeos_root)
    if not self._target:
      raise RuntimeError('Unsupported board "%s"' % board)
    self._executer = command_executer.GetCommandExecuter()
    self._base_dir = base_dir
    self._tmp_abs = None
    self._cleanup = cleanup
    self._sshflag = ('-o StrictHostKeyChecking=no ' + '-o CheckHostIP=no ' +
                     '-o UserKnownHostsFile=$(mktemp) ')

    if source_dir:
      self._source_dir = source_dir
      self._mount_flag = 'USE="mounted_sources"'
      self.MountSource()
    else:
      self._source_dir = None
      self._mount_flag = ''

  def SetupTestingDir(self):
    self._tmp_abs = tempfile.mkdtemp(
        prefix='dejagnu_',
        dir=path.join(self._chromeos_chroot, 'tmp'))
    self._tmp = self._tmp_abs[len(self._chromeos_chroot):]
    self._tmp_testing_rsa = path.join(self._tmp, 'testing_rsa')
    self._tmp_testing_rsa_abs = path.join(self._tmp_abs, 'testing_rsa')

  def PrepareTestingRsaKeys(self):
    if not path.isfile(self._tmp_testing_rsa_abs):
      shutil.copy(
          path.join(self._chromeos_root,
                    'src/scripts/mod_for_test_scripts/ssh_keys/testing_rsa'),
          self._tmp_testing_rsa_abs)
      os.chmod(self._tmp_testing_rsa_abs, stat.S_IRUSR)

  def PrepareTestFiles(self):
    """Prepare site.exp and board exp files."""
    # Create the boards directory.
    os.mkdir('%s/boards' % self._tmp_abs)

    # Generate the chromeos.exp file.
    with open('%s/boards/gdb.exp.in' % self._base_dir, 'r') as template_file:
      content = template_file.read()

    substitutions = dict({
        '__boardname__': self._board,
        '__board_hostname__': self._remote,
        '__tmp_testing_rsa__': self._tmp_testing_rsa,
        '__tmp_dir__': self._tmp
    })
    for pat, sub in substitutions.items():
      content = content.replace(pat, sub)

    board_file_name = '%s/boards/%s.exp' % (self._tmp_abs, self._board)
    with open(board_file_name, 'w') as board_file:
      board_file.write(content)

    # Generate the site file
    with open('%s/site.exp' % self._tmp_abs, 'w') as site_file:
      site_file.write('set target_list "%s"\n' % self._board)

    with open('%s/boards/gdbserver.sh.in' % self._base_dir, 'r') \
        as template_file:
      content = template_file.read()
    substitutions = dict({
        '__board_hostname__': self._remote,
        '__tmp_testing_rsa__': self._tmp_testing_rsa,
        '__tmp_dir__': self._tmp
    })
    for pat, sub in substitutions.items():
      content = content.replace(pat, sub)

    gdbserver_file_name = '%s/boards/gdbserver.sh' % (self._tmp_abs)
    with open(gdbserver_file_name, 'w') as board_file:
      board_file.write(content)

    st = os.stat(gdbserver_file_name)
    os.chmod(gdbserver_file_name, st.st_mode | stat.S_IXGRP | stat.S_IXUSR)

  def PrepareGdb(self):
    self.PrepareGdbDefault()

  def PrepareGdbDefault(self):
    ret = self._executer.ChrootRunCommandWOutput(
        self._chromeos_root, 'equery w cross-%s/gdb' % self._target)[1]
    ret = path.basename(ret.strip())

    matcher = re.match(r'(.*).ebuild', ret)
    if matcher:
      gdb_reversion = matcher.group(1)
    else:
      raise RuntimeError('Failed to get gdb reversion.')
    gdb_version = gdb_reversion.split('-r')[0]
    gdb_portage_dir = '/var/tmp/portage/cross-%s/%s/work' % (self._target,
                                                             gdb_reversion)
    self._gdb_source_dir = path.join(gdb_portage_dir, gdb_version)

    ret = self._executer.ChrootRunCommand(self._chromeos_root, (
        'sudo %s ebuild $(equery w cross-%s/gdb) clean compile' % (
            self._mount_flag, self._target)))
    if ret:
      raise RuntimeError('ebuild gdb failed.')

  def PrepareGdbserver(self):
    self.PrepareGdbserverDefault()

  def PrepareGdbserverDefault(self):
    cmd = ('./setup_board --board {0}; '
           '{1} emerge-{0} gdb'.format(self._board, self._mount_flag))
    ret = self._executer.ChrootRunCommand(self._chromeos_root,
                                          cmd,
                                          print_to_console=True)
    if ret:
      raise RuntimeError('ebuild gdbserver failed.')

    cmd = ('scp -i {0}  {1} '
           '/build/{2}/usr/bin/gdbserver root@{3}:/usr/local/bin/'.format(
               self._tmp_testing_rsa, self._sshflag, self._board, self._remote))
    ret = self._executer.ChrootRunCommand(self._chromeos_root,
                                          cmd,
                                          print_to_console=True)
    if ret:
      raise RuntimeError('copy gdbserver failed.')

    if self._mount_flag:
      self.MountSource(unmount=False)

  def Cleanup(self):
    if not self._cleanup:
      return

    if self._cleanup == 'chroot' or self._cleanup == 'chromeos':
      self._l.LogOutput('[Cleanup]: Deleting chroot inside \'{0}\''.format(
          self._chromeos_root))
      command = 'cd %s; cros_sdk --delete' % self._chromeos_root
      rv = self._executer.RunCommand(command)
      if rv:
        self._l.LogWarning('Warning - failed to delete chroot.')
      # Delete .cache - crosbug.com/34956
      command = 'sudo rm -fr %s' % os.path.join(self._chromeos_root, '.cache')
      rv = self._executer.RunCommand(command)
      if rv:
        self._l.LogWarning('Warning - failed to delete \'.cache\'.')

    if self._cleanup == 'chromeos':
      self._l.LogOutput('[Cleanup]: Deleting chromeos tree \'{0}\' ...'.format(
          self._chromeos_root))
      command = 'rm -fr {0}'.format(self._chromeos_root)
      rv = self._executer.RunCommand(command)
      if rv:
        self._l.LogWarning('Warning - failed to remove chromeos tree.')

  def MakeCheck(self):
    cmd = ('ssh -i {0} {1}  root@{2} "reboot && exit"'
           .format(self._tmp_testing_rsa, self._sshflag, self._remote))
    self._executer.ChrootRunCommand(self._chromeos_root, cmd)
    time.sleep(40)

    cmd = ('ssh -i {0} {1}  root@{2} '
           '"iptables -A INPUT -p tcp --dport 1234 -j ACCEPT"'.format(
               self._tmp_testing_rsa, self._sshflag, self._remote))
    self._executer.ChrootRunCommand(self._chromeos_root, cmd)

    cmd = ('cd %s ; '
           'DEJAGNU=%s make check' % (path.join(self._gdb_source_dir, 'gdb'),
                                      path.join(self._tmp, 'site.exp')))
    ret = self._executer.ChrootRunCommand(self._chromeos_root, cmd)
    if ret:
      raise RuntimeError('Make check failed.')

  # This method ensures necessary mount points before executing chroot comamnd.
  def MountSource(self, unmount=False):
    script = os.path.join(self._base_dir, 'build_tc.py')
    if unmount:
      mount = '-u'
    else:
      mount = '-m'
    cmd = ('python {0} --chromeos_root={1} '
           '--gdb_dir={2} --board={3} {4}'.format(script, self._chromeos_root,
                                                  self._source_dir, self._board,
                                                  mount))
    rv = self._executer.RunCommand(cmd)
    if rv:
      raise RuntimeError('Mount source failed.')

  def ResultValidate(self):
    self.PrepareResult()
    result = []
    for key, value in self.base_result.items():
      if 'PASS' not in value:
        continue
      if key not in self.test_result:
        continue
      test_result = self.test_result[key]
      if 'PASS' not in test_result:
        result.append(key)
    return result

  def PrepareResult(self):
    test_output = os.path.join(self._gdb_source_dir, 'gdb', 'testsuite',
                               'gdb.sum')
    test_output = misc.GetOutsideChrootPath(self._chromeos_root, test_output)
    base_output = os.path.join(self._base_dir, 'gdb_baseline', self._target)

    self.test_result = self.ParseResult(test_output)
    self.base_result = self.ParseResult(base_output)

  def ParseResult(self, gdb_sum):
    result = {}
    multi_keys = {}
    with open(gdb_sum) as input_sum:
      for line in input_sum:
        line = line.strip()
        r = line.split(':', 1)
        if r[0] in _VALID_TEST_RESULTS:
          key = r[1]
          if r[1] in result:
            if r[1] in multi_keys:
              multi_keys[r[1]] += 1
            else:
              multi_keys[r[1]] = 2
            key = r[1] + '_____{0}_____'.format(multi_keys[r[1]])
          result[key] = r[0]
    return result


def Main(argv):
  opts = ProcessArguments(argv)
  available_machine = TryAcquireMachine(opts.remote)
  executer = DejagnuExecuter(
      misc.GetRoot(argv[0])[0], opts.mount, opts.chromeos_root,
      available_machine._name, opts.board, opts.cleanup)
  # Return value is a 3- or 4-element tuple
  #   element#1 - exit code
  #   element#2 - stdout
  #   element#3 - stderr
  #   element#4 - exception infor
  # Some other scripts need these detailed information.
  ret = (1, '', '')
  try:
    executer.SetupTestingDir()
    executer.PrepareTestingRsaKeys()
    executer.PrepareTestFiles()
    executer.PrepareGdb()
    executer.PrepareGdbserver()
    executer.MakeCheck()
    result = executer.ResultValidate()
    print result
    if result:
      ret = (1, result, '')
    else:
      ret = (0, '', '')

  except Exception as e:
    # At least log the exception on console.
    print e
    # The #4 element encodes the runtime exception.
    ret = (1, '', '', 'Exception happened during execution: \n' + str(e))
  finally:
    executer.Cleanup()
    return ret


if __name__ == '__main__':
  retval = Main(sys.argv)[0]
  sys.exit(retval)
