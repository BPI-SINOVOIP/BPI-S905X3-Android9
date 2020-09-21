#!/usr/bin/env python2

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script to use remote try-bot build image with local gcc."""

from __future__ import print_function

import argparse
import glob
import os
import re
import shutil
import socket
import sys
import tempfile
import time

from cros_utils import command_executer
from cros_utils import logger
from cros_utils import manifest_versions
from cros_utils import misc

BRANCH = 'the_actual_branch_used_in_this_script'
TMP_BRANCH = 'tmp_branch'
SLEEP_TIME = 600

# pylint: disable=anomalous-backslash-in-string


def GetPatchNum(output):
  lines = output.splitlines()
  line = [l for l in lines if 'googlesource' in l][0]
  patch_num = re.findall(r'\d+', line)[0]
  if 'chrome-internal' in line:
    patch_num = '*' + patch_num
  return str(patch_num)


def GetPatchString(patch):
  if patch:
    return '+'.join(patch)
  return 'NO_PATCH'


def FindVersionForToolchain(branch, chromeos_root):
  """Find the version number in artifacts link in the tryserver email."""
  # For example: input:  toolchain-3701.42.B
  #              output: R26-3701.42.1
  digits = branch.split('-')[1].split('B')[0]
  manifest_dir = os.path.join(chromeos_root, 'manifest-internal')
  os.chdir(manifest_dir)
  major_version = digits.split('.')[0]
  ce = command_executer.GetCommandExecuter()
  command = 'repo sync . && git branch -a | grep {0}'.format(major_version)
  _, branches, _ = ce.RunCommandWOutput(command, print_to_console=False)
  m = re.search(r'(R\d+)', branches)
  if not m:
    logger.GetLogger().LogFatal('Cannot find version for branch {0}'
                                .format(branch))
  version = m.group(0) + '-' + digits + '1'
  return version


def FindBuildId(description):
  """Find the build id of the build at trybot server."""
  running_time = 0
  while True:
    (result, number) = FindBuildIdFromLog(description)
    if result >= 0:
      return (result, number)
    logger.GetLogger().LogOutput('{0} minutes passed.'
                                 .format(running_time / 60))
    logger.GetLogger().LogOutput('Sleeping {0} seconds.'.format(SLEEP_TIME))
    time.sleep(SLEEP_TIME)
    running_time += SLEEP_TIME


def FindBuildIdFromLog(description):
  """Get the build id from build log."""
  # returns tuple (result, buildid)
  # result == 0, buildid > 0, the build was successful and we have a build id
  # result > 0, buildid > 0,  the whole build failed for some reason but we
  #                           do have a build id.
  # result == -1, buildid == -1, we have not found a finished build for this
  #                              description yet

  file_dir = os.path.dirname(os.path.realpath(__file__))
  commands = ('{0}/cros_utils/buildbot_json.py builds '
              'http://chromegw/p/tryserver.chromiumos/'.format(file_dir))
  ce = command_executer.GetCommandExecuter()
  _, buildinfo, _ = ce.RunCommandWOutput(commands, print_to_console=False)

  my_info = buildinfo.splitlines()
  current_line = 1
  running_job = False
  result = -1

  # result == 0, we have a successful build
  # result > 0, we have a failed build but build id may be valid
  # result == -1, we have not found a finished build for this description
  while current_line < len(my_info):
    my_dict = {}
    while True:
      key = my_info[current_line].split(':')[0].strip()
      value = my_info[current_line].split(':', 1)[1].strip()
      my_dict[key] = value
      current_line += 1
      if 'Build' in key or current_line == len(my_info):
        break
    if ('True' not in my_dict['completed'] and
        str(description) in my_dict['reason']):
      running_job = True
    if ('True' not in my_dict['completed'] or
        str(description) not in my_dict['reason']):
      continue
    result = int(my_dict['result'])
    build_id = int(my_dict['number'])
    if result == 0:
      return (result, build_id)
    else:
      # Found a finished failed build.
      # Keep searching to find a successful one
      pass

  if result > 0 and not running_job:
    return (result, build_id)
  return (-1, -1)


def DownloadImage(target, index, dest, version):
  """Download artifacts from cloud."""
  if not os.path.exists(dest):
    os.makedirs(dest)

  rversion = manifest_versions.RFormatCrosVersion(version)
  print(str(rversion))
  #  ls_cmd = ("gsutil ls gs://chromeos-image-archive/trybot-{0}/{1}-b{2}"
  #            .format(target, rversion, index))
  ls_cmd = ('gsutil ls gs://chromeos-image-archive/trybot-{0}/*-b{2}'.format(
      target, index))

  download_cmd = ('$(which gsutil) cp {0} {1}'.format('{0}', dest))
  ce = command_executer.GetCommandExecuter()

  _, out, _ = ce.RunCommandWOutput(ls_cmd, print_to_console=True)
  lines = out.splitlines()
  download_files = [
      'autotest.tar', 'chromeos-chrome', 'chromiumos_test_image', 'debug.tgz',
      'sysroot_chromeos-base_chromeos-chrome.tar.xz'
  ]
  for line in lines:
    if any([e in line for e in download_files]):
      cmd = download_cmd.format(line)
      if ce.RunCommand(cmd):
        logger.GetLogger().LogFatal('Command {0} failed, existing...'
                                    .format(cmd))


def UnpackImage(dest):
  """Unpack the image, the chroot build dir."""
  chrome_tbz2 = glob.glob(dest + '/*.tbz2')[0]
  commands = ('tar xJf {0}/sysroot_chromeos-base_chromeos-chrome.tar.xz '
              '-C {0} &&'
              'tar xjf {1} -C {0} &&'
              'tar xzf {0}/debug.tgz  -C {0}/usr/lib/ &&'
              'tar xf {0}/autotest.tar -C {0}/usr/local/ &&'
              'tar xJf {0}/chromiumos_test_image.tar.xz -C {0}'.format(
                  dest, chrome_tbz2))
  ce = command_executer.GetCommandExecuter()
  return ce.RunCommand(commands)


def RemoveOldBranch():
  """Remove the branch with name BRANCH."""
  ce = command_executer.GetCommandExecuter()
  command = 'git rev-parse --abbrev-ref HEAD'
  _, out, _ = ce.RunCommandWOutput(command)
  if BRANCH in out:
    command = 'git checkout -B {0}'.format(TMP_BRANCH)
    ce.RunCommand(command)
  command = "git commit -m 'nouse'"
  ce.RunCommand(command)
  command = 'git branch -D {0}'.format(BRANCH)
  ce.RunCommand(command)


def UploadManifest(manifest, chromeos_root, branch='master'):
  """Copy the manifest to $chromeos_root/manifest-internal and upload."""
  chromeos_root = misc.CanonicalizePath(chromeos_root)
  manifest_dir = os.path.join(chromeos_root, 'manifest-internal')
  os.chdir(manifest_dir)
  ce = command_executer.GetCommandExecuter()

  RemoveOldBranch()

  if branch != 'master':
    branch = '{0}'.format(branch)
  command = 'git checkout -b {0} -t cros-internal/{1}'.format(BRANCH, branch)
  ret = ce.RunCommand(command)
  if ret:
    raise RuntimeError('Command {0} failed'.format(command))

  # We remove the default.xml, which is the symbolic link of full.xml.
  # After that, we copy our xml file to default.xml.
  # We did this because the full.xml might be updated during the
  # run of the script.
  os.remove(os.path.join(manifest_dir, 'default.xml'))
  shutil.copyfile(manifest, os.path.join(manifest_dir, 'default.xml'))
  return UploadPatch(manifest)


def GetManifestPatch(manifests, version, chromeos_root, branch='master'):
  """Return a gerrit patch number given a version of manifest file."""
  temp_dir = tempfile.mkdtemp()
  to_file = os.path.join(temp_dir, 'default.xml')
  manifests.GetManifest(version, to_file)
  return UploadManifest(to_file, chromeos_root, branch)


def UploadPatch(source):
  """Up load patch to gerrit, return patch number."""
  commands = ('git add -A . &&'
              "git commit -m 'test' -m 'BUG=None' -m 'TEST=None' "
              "-m 'hostname={0}' -m 'source={1}'".format(
                  socket.gethostname(), source))
  ce = command_executer.GetCommandExecuter()
  ce.RunCommand(commands)

  commands = ('yes | repo upload .   --cbr --no-verify')
  _, _, err = ce.RunCommandWOutput(commands)
  return GetPatchNum(err)


def ReplaceSysroot(chromeos_root, dest_dir, target):
  """Copy unpacked sysroot and image to chromeos_root."""
  ce = command_executer.GetCommandExecuter()
  # get the board name from "board-release". board may contain "-"
  board = target.rsplit('-', 1)[0]
  board_dir = os.path.join(chromeos_root, 'chroot', 'build', board)
  command = 'sudo rm -rf {0}'.format(board_dir)
  ce.RunCommand(command)

  command = 'sudo mv {0} {1}'.format(dest_dir, board_dir)
  ce.RunCommand(command)

  image_dir = os.path.join(chromeos_root, 'src', 'build', 'images', board,
                           'latest')
  command = 'rm -rf {0} && mkdir -p {0}'.format(image_dir)
  ce.RunCommand(command)

  command = 'mv {0}/chromiumos_test_image.bin {1}'.format(board_dir, image_dir)
  return ce.RunCommand(command)


def GccBranchForToolchain(branch):
  if branch == 'toolchain-3428.65.B':
    return 'release-R25-3428.B'
  else:
    return None


def GetGccBranch(branch):
  """Get the remote branch name from branch or version."""
  ce = command_executer.GetCommandExecuter()
  command = 'git branch -a | grep {0}'.format(branch)
  _, out, _ = ce.RunCommandWOutput(command)
  if not out:
    release_num = re.match(r'.*(R\d+)-*', branch)
    if release_num:
      release_num = release_num.group(0)
      command = 'git branch -a | grep {0}'.format(release_num)
      _, out, _ = ce.RunCommandWOutput(command)
      if not out:
        GccBranchForToolchain(branch)
  if not out:
    out = 'remotes/cros/master'
  new_branch = out.splitlines()[0]
  return new_branch


def UploadGccPatch(chromeos_root, gcc_dir, branch):
  """Upload local gcc to gerrit and get the CL number."""
  ce = command_executer.GetCommandExecuter()
  gcc_dir = misc.CanonicalizePath(gcc_dir)
  gcc_path = os.path.join(chromeos_root, 'src/third_party/gcc')
  assert os.path.isdir(gcc_path), ('{0} is not a valid chromeos root'
                                   .format(chromeos_root))
  assert os.path.isdir(gcc_dir), ('{0} is not a valid dir for gcc'
                                  'source'.format(gcc_dir))
  os.chdir(gcc_path)
  RemoveOldBranch()
  if not branch:
    branch = 'master'
  branch = GetGccBranch(branch)
  command = ('git checkout -b {0} -t {1} && ' 'rm -rf *'.format(BRANCH, branch))
  ce.RunCommand(command, print_to_console=False)

  command = ("rsync -az --exclude='*.svn' --exclude='*.git'"
             ' {0}/ .'.format(gcc_dir))
  ce.RunCommand(command)
  return UploadPatch(gcc_dir)


def RunRemote(chromeos_root, branch, patches, is_local, target, chrome_version,
              dest_dir):
  """The actual running commands."""
  ce = command_executer.GetCommandExecuter()

  if is_local:
    local_flag = '--local -r {0}'.format(dest_dir)
  else:
    local_flag = '--remote'
  patch = ''
  for p in patches:
    patch += ' -g {0}'.format(p)
  cbuildbot_path = os.path.join(chromeos_root, 'chromite/cbuildbot')
  os.chdir(cbuildbot_path)
  branch_flag = ''
  if branch != 'master':
    branch_flag = ' -b {0}'.format(branch)
  chrome_version_flag = ''
  if chrome_version:
    chrome_version_flag = ' --chrome_version={0}'.format(chrome_version)
  description = '{0}_{1}_{2}'.format(branch, GetPatchString(patches), target)
  command = ('yes | ./cbuildbot {0} {1} {2} {3} {4} {5}'
             ' --remote-description={6}'
             ' --chrome_rev=tot'.format(patch, branch_flag, chrome_version,
                                        local_flag, chrome_version_flag, target,
                                        description))
  ce.RunCommand(command)

  return description


def Main(argv):
  """The main function."""
  # Common initializations
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '-c',
      '--chromeos_root',
      required=True,
      dest='chromeos_root',
      help='The chromeos_root')
  parser.add_argument(
      '-g', '--gcc_dir', default='', dest='gcc_dir', help='The gcc dir')
  parser.add_argument(
      '-t',
      '--target',
      required=True,
      dest='target',
      help=('The target to be build, the list is at'
            ' $(chromeos_root)/chromite/buildbot/cbuildbot'
            ' --list -all'))
  parser.add_argument('-l', '--local', action='store_true')
  parser.add_argument(
      '-d',
      '--dest_dir',
      dest='dest_dir',
      help=('The dir to build the whole chromeos if'
            ' --local is set'))
  parser.add_argument(
      '--chrome_version',
      dest='chrome_version',
      default='',
      help='The chrome version to use. '
      'Default it will use the latest one.')
  parser.add_argument(
      '--chromeos_version',
      dest='chromeos_version',
      default='',
      help=('The chromeos version to use.'
            '(1) A release version in the format: '
            "'\d+\.\d+\.\d+\.\d+.*'"
            "(2) 'latest_lkgm' for the latest lkgm version"))
  parser.add_argument(
      '-r',
      '--replace_sysroot',
      action='store_true',
      help=('Whether or not to replace the build/$board dir'
            'under the chroot of chromeos_root and copy '
            'the image to src/build/image/$board/latest.'
            ' Default is False'))
  parser.add_argument(
      '-b',
      '--branch',
      dest='branch',
      default='',
      help=('The branch to run trybot, default is None'))
  parser.add_argument(
      '-p',
      '--patch',
      dest='patch',
      default='',
      help=('The patches to be applied, the patches numbers '
            "be seperated by ','"))

  script_dir = os.path.dirname(os.path.realpath(__file__))

  args = parser.parse_args(argv[1:])
  target = args.target
  if args.patch:
    patch = args.patch.split(',')
  else:
    patch = []
  chromeos_root = misc.CanonicalizePath(args.chromeos_root)
  if args.chromeos_version and args.branch:
    raise RuntimeError('You can not set chromeos_version and branch at the '
                       'same time.')

  manifests = None
  if args.branch:
    chromeos_version = ''
    branch = args.branch
  else:
    chromeos_version = args.chromeos_version
    manifests = manifest_versions.ManifestVersions()
    if chromeos_version == 'latest_lkgm':
      chromeos_version = manifests.TimeToVersion(time.mktime(time.gmtime()))
      logger.GetLogger().LogOutput('found version %s for latest LKGM' %
                                   (chromeos_version))
    # TODO: this script currently does not handle the case where the version
    # is not in the "master" branch
    branch = 'master'

  if chromeos_version:
    manifest_patch = GetManifestPatch(manifests, chromeos_version,
                                      chromeos_root)
    patch.append(manifest_patch)
  if args.gcc_dir:
    # TODO: everytime we invoke this script we are getting a different
    # patch for GCC even if GCC has not changed. The description should
    # be based on the MD5 of the GCC patch contents.
    patch.append(UploadGccPatch(chromeos_root, args.gcc_dir, branch))
  description = RunRemote(chromeos_root, branch, patch, args.local, target,
                          args.chrome_version, args.dest_dir)
  if args.local or not args.dest_dir:
    # TODO: We are not checktng the result of cbuild_bot in here!
    return 0

  # return value:
  # 0 => build bot was successful and image was put where requested
  # 1 => Build bot FAILED but image was put where requested
  # 2 => Build bot failed or BUild bot was successful but and image was
  #      not generated or could not be put where expected

  os.chdir(script_dir)
  dest_dir = misc.CanonicalizePath(args.dest_dir)
  (bot_result, build_id) = FindBuildId(description)
  if bot_result > 0 and build_id > 0:
    logger.GetLogger().LogError('Remote trybot failed but image was generated')
    bot_result = 1
  elif bot_result > 0:
    logger.GetLogger().LogError('Remote trybot failed. No image was generated')
    return 2
  if 'toolchain' in branch:
    chromeos_version = FindVersionForToolchain(branch, chromeos_root)
    assert not manifest_versions.IsRFormatCrosVersion(chromeos_version)
  DownloadImage(target, build_id, dest_dir, chromeos_version)
  ret = UnpackImage(dest_dir)
  if ret != 0:
    return 2
  # todo: return a more inteligent return value
  if not args.replace_sysroot:
    return bot_result

  ret = ReplaceSysroot(chromeos_root, args.dest_dir, target)
  if ret != 0:
    return 2

  # got an image and we were successful in placing it where requested
  return bot_result


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
