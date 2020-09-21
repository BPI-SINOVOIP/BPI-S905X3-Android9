#!/usr/bin/env python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Script to enter the ChromeOS chroot with mounted sources.

This script enters the chroot with mounted sources.
"""

from __future__ import print_function

__author__ = 'asharif@google.com (Ahmad Sharif)'

import argparse
import getpass
import os
import pwd
import sys

from cros_utils import command_executer
from cros_utils import logger
from cros_utils import misc


class MountPoint(object):
  """Mount point class"""

  def __init__(self, external_dir, mount_dir, owner, options=None):
    self.external_dir = os.path.realpath(external_dir)
    self.mount_dir = os.path.realpath(mount_dir)
    self.owner = owner
    self.options = options

  def CreateAndOwnDir(self, dir_name):
    retv = 0
    if not os.path.exists(dir_name):
      command = 'mkdir -p ' + dir_name
      command += ' || sudo mkdir -p ' + dir_name
      retv = command_executer.GetCommandExecuter().RunCommand(command)
    if retv != 0:
      return retv
    pw = pwd.getpwnam(self.owner)
    if os.stat(dir_name).st_uid != pw.pw_uid:
      command = 'sudo chown -f ' + self.owner + ' ' + dir_name
      retv = command_executer.GetCommandExecuter().RunCommand(command)
    return retv

  def DoMount(self):
    ce = command_executer.GetCommandExecuter()
    mount_signature = '%s on %s' % (self.external_dir, self.mount_dir)
    command = 'mount'
    retv, out, _ = ce.RunCommandWOutput(command)
    if mount_signature not in out:
      retv = self.CreateAndOwnDir(self.mount_dir)
      logger.GetLogger().LogFatalIf(retv, 'Cannot create mount_dir!')
      retv = self.CreateAndOwnDir(self.external_dir)
      logger.GetLogger().LogFatalIf(retv, 'Cannot create external_dir!')
      retv = self.MountDir()
      logger.GetLogger().LogFatalIf(retv, 'Cannot mount!')
      return retv
    else:
      return 0

  def UnMount(self):
    ce = command_executer.GetCommandExecuter()
    return ce.RunCommand('sudo umount %s' % self.mount_dir)

  def MountDir(self):
    command = 'sudo mount --bind ' + self.external_dir + ' ' + self.mount_dir
    if self.options == 'ro':
      command += ' && sudo mount --bind -oremount,ro ' + self.mount_dir
    retv = command_executer.GetCommandExecuter().RunCommand(command)
    return retv

  def __str__(self):
    ret = ''
    ret += self.external_dir + '\n'
    ret += self.mount_dir + '\n'
    if self.owner:
      ret += self.owner + '\n'
    if self.options:
      ret += self.options + '\n'
    return ret


def Main(argv, return_output=False):
  """The main function."""

  parser = argparse.ArgumentParser()
  parser.add_argument(
      '-c',
      '--chromeos_root',
      dest='chromeos_root',
      default='../..',
      help='ChromeOS root checkout directory.')
  parser.add_argument(
      '-t',
      '--toolchain_root',
      dest='toolchain_root',
      help='Toolchain root directory.')
  parser.add_argument(
      '-o', '--output', dest='output', help='Toolchain output directory')
  parser.add_argument(
      '--sudo',
      dest='sudo',
      action='store_true',
      default=False,
      help='Run the command with sudo.')
  parser.add_argument(
      '-r',
      '--third_party',
      dest='third_party',
      help='The third_party directory to mount.')
  parser.add_argument(
      '-m',
      '--other_mounts',
      dest='other_mounts',
      help='Other mount points in the form: '
      'dir:mounted_dir:options')
  parser.add_argument(
      '-s',
      '--mount-scripts-only',
      dest='mount_scripts_only',
      action='store_true',
      default=False,
      help='Mount only the scripts dir, and not the sources.')
  parser.add_argument(
      'passthrough_argv',
      nargs='*',
      help='Command to be executed inside the chroot.')

  options = parser.parse_args(argv)

  chromeos_root = options.chromeos_root

  chromeos_root = os.path.expanduser(chromeos_root)
  if options.toolchain_root:
    options.toolchain_root = os.path.expanduser(options.toolchain_root)

  chromeos_root = os.path.abspath(chromeos_root)

  tc_dirs = []
  if options.toolchain_root is None or options.mount_scripts_only:
    m = 'toolchain_root not specified. Will not mount toolchain dirs.'
    logger.GetLogger().LogWarning(m)
  else:
    tc_dirs = [
        options.toolchain_root + '/google_vendor_src_branch/gcc',
        options.toolchain_root + '/google_vendor_src_branch/binutils'
    ]

  for tc_dir in tc_dirs:
    if not os.path.exists(tc_dir):
      logger.GetLogger().LogError('toolchain path ' + tc_dir +
                                  ' does not exist!')
      parser.print_help()
      sys.exit(1)

  if not os.path.exists(chromeos_root):
    logger.GetLogger().LogError('chromeos_root ' + options.chromeos_root +
                                ' does not exist!')
    parser.print_help()
    sys.exit(1)

  if not os.path.exists(chromeos_root + '/src/scripts/build_packages'):
    logger.GetLogger().LogError(options.chromeos_root +
                                '/src/scripts/build_packages'
                                ' not found!')
    parser.print_help()
    sys.exit(1)

  version_dir = os.path.realpath(os.path.expanduser(os.path.dirname(__file__)))

  mounted_tc_root = '/usr/local/toolchain_root'
  full_mounted_tc_root = chromeos_root + '/chroot/' + mounted_tc_root
  full_mounted_tc_root = os.path.abspath(full_mounted_tc_root)

  mount_points = []
  for tc_dir in tc_dirs:
    last_dir = misc.GetRoot(tc_dir)[1]
    mount_point = MountPoint(tc_dir, full_mounted_tc_root + '/' + last_dir,
                             getpass.getuser(), 'ro')
    mount_points.append(mount_point)

  # Add the third_party mount point if it exists
  if options.third_party:
    third_party_dir = options.third_party
    logger.GetLogger().LogFatalIf(not os.path.isdir(third_party_dir),
                                  '--third_party option is not a valid dir.')
  else:
    third_party_dir = os.path.abspath(
        '%s/../../../third_party' % os.path.dirname(__file__))

  if os.path.isdir(third_party_dir):
    mount_point = MountPoint(third_party_dir,
                             ('%s/%s' % (full_mounted_tc_root,
                                         os.path.basename(third_party_dir))),
                             getpass.getuser())
    mount_points.append(mount_point)

  output = options.output
  if output is None and options.toolchain_root:
    # Mount the output directory at /usr/local/toolchain_root/output
    output = options.toolchain_root + '/output'

  if output:
    mount_points.append(
        MountPoint(output, full_mounted_tc_root + '/output', getpass.getuser()))

  # Mount the other mount points
  mount_points += CreateMountPointsFromString(options.other_mounts,
                                              chromeos_root + '/chroot/')

  last_dir = misc.GetRoot(version_dir)[1]

  # Mount the version dir (v14) at /usr/local/toolchain_root/v14
  mount_point = MountPoint(version_dir, full_mounted_tc_root + '/' + last_dir,
                           getpass.getuser())
  mount_points.append(mount_point)

  for mount_point in mount_points:
    retv = mount_point.DoMount()
    if retv != 0:
      return retv

  # Finally, create the symlink to build-gcc.
  command = 'sudo chown ' + getpass.getuser() + ' ' + full_mounted_tc_root
  retv = command_executer.GetCommandExecuter().RunCommand(command)

  try:
    CreateSymlink(last_dir + '/build-gcc', full_mounted_tc_root + '/build-gcc')
    CreateSymlink(last_dir + '/build-binutils',
                  full_mounted_tc_root + '/build-binutils')
  except Exception as e:
    logger.GetLogger().LogError(str(e))

  # Now call cros_sdk --enter with the rest of the arguments.
  command = 'cd %s/src/scripts && cros_sdk --enter' % chromeos_root

  if len(options.passthrough_argv) > 1:
    inner_command = ' '.join(options.passthrough_argv[1:])
    inner_command = inner_command.strip()
    if inner_command.startswith('-- '):
      inner_command = inner_command[3:]
    command_file = 'tc_enter_chroot.cmd'
    command_file_path = chromeos_root + '/src/scripts/' + command_file
    retv = command_executer.GetCommandExecuter().RunCommand(
        'sudo rm -f ' + command_file_path)
    if retv != 0:
      return retv
    f = open(command_file_path, 'w')
    f.write(inner_command)
    f.close()
    logger.GetLogger().LogCmd(inner_command)
    retv = command_executer.GetCommandExecuter().RunCommand(
        'chmod +x ' + command_file_path)
    if retv != 0:
      return retv

    if options.sudo:
      command += ' sudo ./' + command_file
    else:
      command += ' ./' + command_file
    retv = command_executer.GetCommandExecuter().RunCommandGeneric(
        command, return_output)
    return retv
  else:
    os.chdir('%s/src/scripts' % chromeos_root)
    ce = command_executer.GetCommandExecuter()
    _, out, _ = ce.RunCommandWOutput('which cros_sdk')
    cros_sdk_binary = out.split()[0]
    return os.execv(cros_sdk_binary, ['', '--enter'])


def CreateMountPointsFromString(mount_strings, chroot_dir):
  # String has options in the form dir:mount:options
  mount_points = []
  if not mount_strings:
    return mount_points
  mount_list = mount_strings.split()
  for mount_string in mount_list:
    mount_values = mount_string.split(':')
    external_dir = mount_values[0]
    mount_dir = mount_values[1]
    if len(mount_values) > 2:
      options = mount_values[2]
    else:
      options = None
    mount_point = MountPoint(external_dir, chroot_dir + '/' + mount_dir,
                             getpass.getuser(), options)
    mount_points.append(mount_point)
  return mount_points


def CreateSymlink(target, link_name):
  logger.GetLogger().LogFatalIf(
      target.startswith('/'), "Can't create symlink to absolute path!")
  real_from_file = misc.GetRoot(link_name)[0] + '/' + target
  if os.path.realpath(real_from_file) != os.path.realpath(link_name):
    if os.path.exists(link_name):
      command = 'rm -rf ' + link_name
      command_executer.GetCommandExecuter().RunCommand(command)
    os.symlink(target, link_name)


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
