#!/usr/bin/python
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Tool script for auto dejagnu."""

__author__ = 'shenhan@google.com (Han Shen)'

import getpass
import optparse
import os
from os import path
import re
import shutil
import stat
import sys
import tempfile
import time

import lock_machine
import tc_enter_chroot
from cros_utils import command_executer
from cros_utils import constants
from cros_utils import logger
from cros_utils import misc


def ProcessArguments(argv):
  """Processing/validating script arguments."""
  parser = optparse.OptionParser(description=(
      'Launches gcc dejagnu test in chroot for chromeos toolchain, compares '
      'the test result with a repository baseline and prints out the result.'),
                                 usage='run_dejagnu options')
  parser.add_option('-c',
                    '--chromeos_root',
                    dest='chromeos_root',
                    help='Required. Specify chromeos root')
  parser.add_option('-m',
                    '--mount',
                    dest='mount',
                    help=('Specify gcc source to mount instead of "auto". '
                          'Under "auto" mode, which is the default - gcc is '
                          'checked out and built automatically at default '
                          'directories. Under "mount" mode '
                          '- the gcc_source is set to "$chromeos_'
                          'root/chroot/usr/local/toolchain_root/gcc", which is '
                          'the mount point for this option value, the '
                          'gcc-build-dir then is computed as '
                          '"${gcc_source_dir}-build-${ctarget}". In this mode, '
                          'a complete gcc build must be performed in the '
                          'computed gcc-build-dir beforehand.'))
  parser.add_option('-b',
                    '--board',
                    dest='board',
                    help=('Required. Specify board.'))
  parser.add_option('-r',
                    '--remote',
                    dest='remote',
                    help=('Required. Specify addresses/names of the board, '
                          'seperate each address/name using comma(\',\').'))
  parser.add_option('-f',
                    '--flags',
                    dest='flags',
                    help='Optional. Extra run test flags to pass to dejagnu.')
  parser.add_option('-k',
                    '--keep',
                    dest='keep_intermediate_files',
                    action='store_true',
                    default=False,
                    help=('Optional. Default to false. Do not remove dejagnu '
                          'intermediate files after test run.'))
  parser.add_option('--cleanup',
                    dest='cleanup',
                    default=None,
                    help=('Optional. Values to this option could be '
                          '\'mount\' (unmount gcc source and '
                          'directory directory, '
                          'only valid when --mount is given), '
                          '\'chroot\' (delete chroot) and '
                          '\'chromeos\' (delete the whole chromeos tree).'))
  parser.add_option('-t',
                    '--tools',
                    dest='tools',
                    default='gcc,g++',
                    help=('Optional. Specify which tools to check, using '
                          '","(comma) as separator. A typical value would be '
                          '"g++" so that only g++ tests are performed. '
                          'Defaults to "gcc,g++".'))

  options, args = parser.parse_args(argv)

  if not options.chromeos_root:
    raise SyntaxError('Missing argument for --chromeos_root.')
  if not options.remote:
    raise SyntaxError('Missing argument for --remote.')
  if not options.board:
    raise SyntaxError('Missing argument for --board.')
  if options.cleanup == 'mount' and not options.mount:
    raise SyntaxError('--cleanup=\'mount\' not valid unless --mount is given.')
  if options.cleanup and not (
    options.cleanup == 'mount' or \
      options.cleanup == 'chroot' or options.cleanup == 'chromeos'):
    raise ValueError('Invalid option value for --cleanup')
  if options.cleanup and options.keep_intermediate_files:
    raise SyntaxError('Only one of --keep and --cleanup could be given.')

  return options


class DejagnuExecuter(object):
  """The class wrapper for dejagnu test executer."""

  def __init__(self, base_dir, mount, chromeos_root, remote, board, flags,
               keep_intermediate_files, tools, cleanup):
    self._l = logger.GetLogger()
    self._chromeos_root = chromeos_root
    self._chromeos_chroot = path.join(chromeos_root, 'chroot')
    if mount:
      self._gcc_source_dir_to_mount = mount
      self._gcc_source_dir = path.join(constants.MOUNTED_TOOLCHAIN_ROOT, 'gcc')
    else:
      self._gcc_source_dir = None

    self._remote = remote
    self._board = board
    ## Compute target from board
    self._target = misc.GetCtargetFromBoard(board, chromeos_root)
    if not self._target:
      raise ValueError('Unsupported board "%s"' % board)
    self._executer = command_executer.GetCommandExecuter()
    self._flags = flags or ''
    self._base_dir = base_dir
    self._tmp_abs = None
    self._keep_intermediate_files = keep_intermediate_files
    self._tools = tools.split(',')
    self._cleanup = cleanup

  def SetupTestingDir(self):
    self._tmp_abs = tempfile.mkdtemp(
        prefix='dejagnu_',
        dir=path.join(self._chromeos_chroot, 'tmp'))
    self._tmp = self._tmp_abs[len(self._chromeos_chroot):]
    self._tmp_testing_rsa = path.join(self._tmp, 'testing_rsa')
    self._tmp_testing_rsa_abs = path.join(self._tmp_abs, 'testing_rsa')

  def MakeCheckString(self):
    return ' '.join(['check-{0}'.format(t) for t in self._tools if t])

  def CleanupIntermediateFiles(self):
    if self._tmp_abs and path.isdir(self._tmp_abs):
      if self._keep_intermediate_files:
        self._l.LogOutput(
            'Your intermediate dejagnu files are kept, you can re-run '
            'inside chroot the command:')
        self._l.LogOutput(
          ' DEJAGNU={0} make -C {1} {2} RUNTESTFLAGS="--target_board={3} {4}"' \
            .format(path.join(self._tmp, 'site.exp'), self._gcc_build_dir,
                    self.MakeCheckString(), self._board, self._flags))
      else:
        self._l.LogOutput('[Cleanup] - Removing temp dir - {0}'.format(
            self._tmp_abs))
        shutil.rmtree(self._tmp_abs)

  def Cleanup(self):
    if not self._cleanup:
      return

    # Optionally cleanup mounted diretory, chroot and chromeos tree.
    if self._cleanup == 'mount' or self._cleanup == 'chroot' or \
          self._cleanup == 'chromeos':
      # No exceptions are allowed from this method.
      try:
        self._l.LogOutput('[Cleanup] - Unmounting directories ...')
        self.MountGccSourceAndBuildDir(unmount=True)
      except:
        print 'Warning: failed to unmount gcc source/build directory.'

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
    with open('%s/chromeos.exp.in' % self._base_dir, 'r') as template_file:
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

  def PrepareGcc(self):
    if self._gcc_source_dir:
      self.PrepareGccFromCustomizedPath()
    else:
      self.PrepareGccDefault()
    self._l.LogOutput('Gcc source dir - {0}'.format(self._gcc_source_dir))
    self._l.LogOutput('Gcc build dir - {0}'.format(self._gcc_top_build_dir))

  def PrepareGccFromCustomizedPath(self):
    """Prepare gcc source, build directory from mounted source."""
    # We have these source directories -
    #   _gcc_source_dir
    #     e.g. '/usr/local/toolchain_root/gcc'
    #   _gcc_source_dir_abs
    #     e.g. '/somewhere/chromeos.live/chroot/usr/local/toolchain_root/gcc'
    #   _gcc_source_dir_to_mount
    #     e.g. '/somewhere/gcc'
    self._gcc_source_dir_abs = path.join(self._chromeos_chroot,
                                         self._gcc_source_dir.lstrip('/'))
    if not path.isdir(self._gcc_source_dir_abs) and \
          self._executer.RunCommand(
      'sudo mkdir -p {0}'.format(self._gcc_source_dir_abs)):
      raise RuntimeError("Failed to create \'{0}\' inside chroot.".format(
          self._gcc_source_dir))
    if not (path.isdir(self._gcc_source_dir_to_mount) and
            path.isdir(path.join(self._gcc_source_dir_to_mount, 'gcc'))):
      raise RuntimeError('{0} is not a valid gcc source tree.'.format(
          self._gcc_source_dir_to_mount))

    # We have these build directories -
    #   _gcc_top_build_dir
    #     e.g. '/usr/local/toolchain_root/gcc-build-x86_64-cros-linux-gnu'
    #   _gcc_top_build_dir_abs
    #     e.g. '/somewhere/chromeos.live/chroo/tusr/local/toolchain_root/
    #                 gcc-build-x86_64-cros-linux-gnu'
    #   _gcc_build_dir
    #     e.g. '/usr/local/toolchain_root/gcc-build-x86_64-cros-linux-gnu/gcc'
    #   _gcc_build_dir_to_mount
    #     e.g. '/somewhere/gcc-build-x86_64-cros-linux-gnu'
    self._gcc_top_build_dir = '{0}-build-{1}'.format(
        self._gcc_source_dir.rstrip('/'), self._target)
    self._gcc_build_dir = path.join(self._gcc_top_build_dir, 'gcc')
    self._gcc_build_dir_to_mount = '{0}-build-{1}'.format(
        self._gcc_source_dir_to_mount, self._target)
    self._gcc_top_build_dir_abs = path.join(self._chromeos_chroot,
                                            self._gcc_top_build_dir.lstrip('/'))
    if not path.isdir(self._gcc_top_build_dir_abs) and \
          self._executer.RunCommand(
      'sudo mkdir -p {0}'.format(self._gcc_top_build_dir_abs)):
      raise RuntimeError('Failed to create \'{0}\' inside chroot.'.format(
          self._gcc_top_build_dir))
    if not (path.isdir(self._gcc_build_dir_to_mount) and path.join(
        self._gcc_build_dir_to_mount, 'gcc')):
      raise RuntimeError('{0} is not a valid gcc build tree.'.format(
          self._gcc_build_dir_to_mount))

    # All check passed. Now mount gcc source and build directories.
    self.MountGccSourceAndBuildDir()

  def PrepareGccDefault(self):
    """Auto emerging gcc for building purpose only."""
    ret = self._executer.ChrootRunCommandWOutput(
        self._chromeos_root, 'equery w cross-%s/gcc' % self._target)[1]
    ret = path.basename(ret.strip())
    # ret is expected to be something like 'gcc-4.6.2-r11.ebuild' or
    # 'gcc-9999.ebuild' parse it.
    matcher = re.match('((.*)-r\d+).ebuild', ret)
    if matcher:
      gccrevision, gccversion = matcher.group(1, 2)
    elif ret == 'gcc-9999.ebuild':
      gccrevision = 'gcc-9999'
      gccversion = 'gcc-9999'
    else:
      raise RuntimeError('Failed to get gcc version.')

    gcc_portage_dir = '/var/tmp/portage/cross-%s/%s/work' % (self._target,
                                                             gccrevision)
    self._gcc_source_dir = path.join(gcc_portage_dir, gccversion)
    self._gcc_top_build_dir = (gcc_portage_dir + '/%s-build-%s') % (
        gccversion, self._target)
    self._gcc_build_dir = path.join(self._gcc_top_build_dir, 'gcc')
    gcc_build_dir_abs = path.join(self._chromeos_root, 'chroot',
                                  self._gcc_build_dir.lstrip('/'))
    if not path.isdir(gcc_build_dir_abs):
      ret = self._executer.ChrootRunCommand(self._chromeos_root, (
          'ebuild $(equery w cross-%s/gcc) clean prepare compile' % (
              self._target)))
      if ret:
        raise RuntimeError('ebuild gcc failed.')

  def MakeCheck(self):
    self.MountGccSourceAndBuildDir()
    cmd = ('cd %s ; '
           'DEJAGNU=%s make %s RUNTESTFLAGS="--target_board=%s %s"' %
           (self._gcc_build_dir, path.join(self._tmp, 'site.exp'),
            self.MakeCheckString(), self._board, self._flags))
    self._executer.ChrootRunCommand(self._chromeos_root, cmd)

  def ValidateFailures(self):
    validate_failures_py = path.join(
        self._gcc_source_dir,
        'contrib/testsuite-management/validate_failures.py')
    cmd = 'cd {0} ; {1} --build_dir={0}'.format(self._gcc_top_build_dir,
                                                validate_failures_py)
    self.MountGccSourceAndBuildDir()
    ret = self._executer.ChrootRunCommandWOutput(self._chromeos_root, cmd)
    if ret[0] != 0:
      self._l.LogWarning('*** validate_failures.py exited with non-zero code,'
                         'please run it manually inside chroot - \n'
                         '    ' + cmd)
    return ret

  # This method ensures necessary mount points before executing chroot comamnd.
  def MountGccSourceAndBuildDir(self, unmount=False):
    mount_points = [tc_enter_chroot.MountPoint(self._gcc_source_dir_to_mount,
                                               self._gcc_source_dir_abs,
                                               getpass.getuser(), 'ro'),
                    tc_enter_chroot.MountPoint(self._gcc_build_dir_to_mount,
                                               self._gcc_top_build_dir_abs,
                                               getpass.getuser(), 'rw')]
    for mp in mount_points:
      if unmount:
        if mp.UnMount():
          raise RuntimeError('Failed to unmount {0}'.format(mp.mount_dir))
        else:
          self._l.LogOutput('{0} unmounted successfully.'.format(mp.mount_dir))
      elif mp.DoMount():
        raise RuntimeError(
            'Failed to mount {0} onto {1}'.format(mp.external_dir,
                                                  mp.mount_dir))
      else:
        self._l.LogOutput('{0} mounted successfully.'.format(mp.mount_dir))

# The end of class DejagnuExecuter


def TryAcquireMachine(remotes):
  available_machine = None
  for r in remotes.split(','):
    machine = lock_machine.Machine(r)
    if machine.TryLock(timeout=300, exclusive=True):
      available_machine = machine
      break
    else:
      logger.GetLogger().LogWarning(
          '*** Failed to lock machine \'{0}\'.'.format(r))
  if not available_machine:
    raise RuntimeError("Failed to acquire one machine from \'{0}\'.".format(
        remotes))
  return available_machine


def Main(argv):
  opts = ProcessArguments(argv)
  available_machine = TryAcquireMachine(opts.remote)
  executer = DejagnuExecuter(
      misc.GetRoot(argv[0])[0], opts.mount, opts.chromeos_root,
      available_machine._name, opts.board, opts.flags,
      opts.keep_intermediate_files, opts.tools, opts.cleanup)
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
    executer.PrepareGcc()
    executer.MakeCheck()
    ret = executer.ValidateFailures()
  except Exception as e:
    # At least log the exception on console.
    print e
    # The #4 element encodes the runtime exception.
    ret = (1, '', '', 'Exception happened during execution: \n' + str(e))
  finally:
    available_machine.Unlock(exclusive=True)
    executer.CleanupIntermediateFiles()
    executer.Cleanup()
    return ret


if __name__ == '__main__':
  retval = Main(sys.argv)[0]
  sys.exit(retval)
