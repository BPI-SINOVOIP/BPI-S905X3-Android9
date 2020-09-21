# Copyright 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Utilities for toolchain build."""

from __future__ import print_function

__author__ = 'asharif@google.com (Ahmad Sharif)'

from contextlib import contextmanager
import os
import re
import shutil
import sys
import traceback

import command_executer
import logger

CHROMEOS_SCRIPTS_DIR = '~/trunk/src/scripts'
TOOLCHAIN_UTILS_PATH = '~/trunk/src/platform/dev/toolchain_utils.sh'


def GetChromeOSVersionFromLSBVersion(lsb_version):
  """Get Chromeos version from Lsb version."""
  ce = command_executer.GetCommandExecuter()
  command = ('git ls-remote '
             'https://chromium.googlesource.com/chromiumos/manifest.git')
  ret, out, _ = ce.RunCommandWOutput(command, print_to_console=False)
  assert ret == 0, 'Command %s failed' % command
  lower = []
  for line in out.splitlines():
    mo = re.search(r'refs/heads/release-R(\d+)-(\d+)\.B', line)
    if mo:
      revision = int(mo.group(1))
      build = int(mo.group(2))
      lsb_build = int(lsb_version.split('.')[0])
      if lsb_build > build:
        lower.append(revision)
  lower = sorted(lower)
  if lower:
    return 'R%d-%s' % (lower[-1] + 1, lsb_version)
  else:
    return 'Unknown'


def ApplySubs(string, *substitutions):
  for pattern, replacement in substitutions:
    string = re.sub(pattern, replacement, string)
  return string


def UnitToNumber(unit_num, base=1000):
  """Convert a number with unit to float."""
  unit_dict = {'kilo': base, 'mega': base**2, 'giga': base**3}
  unit_num = unit_num.lower()
  mo = re.search(r'(\d*)(.+)?', unit_num)
  number = mo.group(1)
  unit = mo.group(2)
  if not unit:
    return float(number)
  for k, v in unit_dict.items():
    if k.startswith(unit):
      return float(number) * v
  raise RuntimeError('Unit: %s not found in byte: %s!' % (unit, unit_num))


def GetFilenameFromString(string):
  return ApplySubs(
      string,
      (r'/', '__'),
      (r'\s', '_'),
      (r'[\\$="?^]', ''),)


def GetRoot(scr_name):
  """Break up pathname into (dir+name)."""
  abs_path = os.path.abspath(scr_name)
  return (os.path.dirname(abs_path), os.path.basename(abs_path))


def GetChromeOSKeyFile(chromeos_root):
  return os.path.join(chromeos_root, 'src', 'scripts', 'mod_for_test_scripts',
                      'ssh_keys', 'testing_rsa')


def GetChrootPath(chromeos_root):
  return os.path.join(chromeos_root, 'chroot')


def GetInsideChrootPath(chromeos_root, file_path):
  if not file_path.startswith(GetChrootPath(chromeos_root)):
    raise RuntimeError("File: %s doesn't seem to be in the chroot: %s" %
                       (file_path, chromeos_root))
  return file_path[len(GetChrootPath(chromeos_root)):]


def GetOutsideChrootPath(chromeos_root, file_path):
  return os.path.join(GetChrootPath(chromeos_root), file_path.lstrip('/'))


def FormatQuotedCommand(command):
  return ApplySubs(command, ('"', r'\"'))


def FormatCommands(commands):
  return ApplySubs(
      str(commands), ('&&', '&&\n'), (';', ';\n'), (r'\n+\s*', '\n'))


def GetImageDir(chromeos_root, board):
  return os.path.join(chromeos_root, 'src', 'build', 'images', board)


def LabelLatestImage(chromeos_root, board, label, vanilla_path=None):
  image_dir = GetImageDir(chromeos_root, board)
  latest_image_dir = os.path.join(image_dir, 'latest')
  latest_image_dir = os.path.realpath(latest_image_dir)
  latest_image_dir = os.path.basename(latest_image_dir)
  retval = 0
  with WorkingDirectory(image_dir):
    command = 'ln -sf -T %s %s' % (latest_image_dir, label)
    ce = command_executer.GetCommandExecuter()
    retval = ce.RunCommand(command)
    if retval:
      return retval
    if vanilla_path:
      command = 'ln -sf -T %s %s' % (vanilla_path, 'vanilla')
      retval2 = ce.RunCommand(command)
      return retval2
  return retval


def DoesLabelExist(chromeos_root, board, label):
  image_label = os.path.join(GetImageDir(chromeos_root, board), label)
  return os.path.exists(image_label)


def GetBuildPackagesCommand(board, usepkg=False, debug=False):
  if usepkg:
    usepkg_flag = '--usepkg'
  else:
    usepkg_flag = '--nousepkg'
  if debug:
    withdebug_flag = '--withdebug'
  else:
    withdebug_flag = '--nowithdebug'
  return ('%s/build_packages %s --withdev --withtest --withautotest '
          '--skip_toolchain_update %s --board=%s '
          '--accept_licenses=@CHROMEOS' % (CHROMEOS_SCRIPTS_DIR, usepkg_flag,
                                           withdebug_flag, board))


def GetBuildImageCommand(board, dev=False):
  dev_args = ''
  if dev:
    dev_args = '--noenable_rootfs_verification --disk_layout=2gb-rootfs'
  return ('%s/build_image --board=%s %s test' % (CHROMEOS_SCRIPTS_DIR, board,
                                                 dev_args))


def GetSetupBoardCommand(board,
                         gcc_version=None,
                         binutils_version=None,
                         usepkg=None,
                         force=None):
  """Get setup_board command."""
  options = []

  if gcc_version:
    options.append('--gcc_version=%s' % gcc_version)

  if binutils_version:
    options.append('--binutils_version=%s' % binutils_version)

  if usepkg:
    options.append('--usepkg')
  else:
    options.append('--nousepkg')

  if force:
    options.append('--force')

  options.append('--accept_licenses=@CHROMEOS')

  return ('%s/setup_board --board=%s %s' % (CHROMEOS_SCRIPTS_DIR, board,
                                            ' '.join(options)))


def CanonicalizePath(path):
  path = os.path.expanduser(path)
  path = os.path.realpath(path)
  return path


def GetCtargetFromBoard(board, chromeos_root):
  """Get Ctarget from board."""
  base_board = board.split('_')[0]
  command = ('source %s; get_ctarget_from_board %s' % (TOOLCHAIN_UTILS_PATH,
                                                       base_board))
  ce = command_executer.GetCommandExecuter()
  ret, out, _ = ce.ChrootRunCommandWOutput(chromeos_root, command)
  if ret != 0:
    raise ValueError('Board %s is invalid!' % board)
  # Remove ANSI escape sequences.
  out = StripANSIEscapeSequences(out)
  return out.strip()


def GetArchFromBoard(board, chromeos_root):
  """Get Arch from board."""
  base_board = board.split('_')[0]
  command = ('source %s; get_board_arch %s' % (TOOLCHAIN_UTILS_PATH,
                                               base_board))
  ce = command_executer.GetCommandExecuter()
  ret, out, _ = ce.ChrootRunCommandWOutput(chromeos_root, command)
  if ret != 0:
    raise ValueError('Board %s is invalid!' % board)
  # Remove ANSI escape sequences.
  out = StripANSIEscapeSequences(out)
  return out.strip()


def GetGccLibsDestForBoard(board, chromeos_root):
  """Get gcc libs destination from board."""
  arch = GetArchFromBoard(board, chromeos_root)
  if arch == 'x86':
    return '/build/%s/usr/lib/gcc/' % board
  if arch == 'amd64':
    return '/build/%s/usr/lib64/gcc/' % board
  if arch == 'arm':
    return '/build/%s/usr/lib/gcc/' % board
  if arch == 'arm64':
    return '/build/%s/usr/lib/gcc/' % board
  raise ValueError('Arch %s is invalid!' % arch)


def StripANSIEscapeSequences(string):
  string = re.sub(r'\x1b\[[0-9]*[a-zA-Z]', '', string)
  return string


def GetChromeSrcDir():
  return 'var/cache/distfiles/target/chrome-src/src'


def GetEnvStringFromDict(env_dict):
  return ' '.join(["%s=\"%s\"" % var for var in env_dict.items()])


def MergeEnvStringWithDict(env_string, env_dict, prepend=True):
  """Merge env string with dict."""
  if not env_string.strip():
    return GetEnvStringFromDict(env_dict)
  override_env_list = []
  ce = command_executer.GetCommandExecuter()
  for k, v in env_dict.items():
    v = v.strip("\"'")
    if prepend:
      new_env = "%s=\"%s $%s\"" % (k, v, k)
    else:
      new_env = "%s=\"$%s %s\"" % (k, k, v)
    command = '; '.join([env_string, new_env, 'echo $%s' % k])
    ret, out, _ = ce.RunCommandWOutput(command)
    override_env_list.append('%s=%r' % (k, out.strip()))
  ret = env_string + ' ' + ' '.join(override_env_list)
  return ret.strip()


def GetAllImages(chromeos_root, board):
  ce = command_executer.GetCommandExecuter()
  command = ('find %s/src/build/images/%s -name chromiumos_test_image.bin' %
             (chromeos_root, board))
  ret, out, _ = ce.RunCommandWOutput(command)
  assert ret == 0, 'Could not run command: %s' % command
  return out.splitlines()


def IsFloat(text):
  if text is None:
    return False
  try:
    float(text)
    return True
  except ValueError:
    return False


def RemoveChromeBrowserObjectFiles(chromeos_root, board):
  """Remove any object files from all the posible locations."""
  out_dir = os.path.join(
      GetChrootPath(chromeos_root),
      'var/cache/chromeos-chrome/chrome-src/src/out_%s' % board)
  if os.path.exists(out_dir):
    shutil.rmtree(out_dir)
    logger.GetLogger().LogCmd('rm -rf %s' % out_dir)
  out_dir = os.path.join(
      GetChrootPath(chromeos_root),
      'var/cache/chromeos-chrome/chrome-src-internal/src/out_%s' % board)
  if os.path.exists(out_dir):
    shutil.rmtree(out_dir)
    logger.GetLogger().LogCmd('rm -rf %s' % out_dir)


@contextmanager
def WorkingDirectory(new_dir):
  """Get the working directory."""
  old_dir = os.getcwd()
  if old_dir != new_dir:
    msg = 'cd %s' % new_dir
    logger.GetLogger().LogCmd(msg)
  os.chdir(new_dir)
  yield new_dir
  if old_dir != new_dir:
    msg = 'cd %s' % old_dir
    logger.GetLogger().LogCmd(msg)
  os.chdir(old_dir)


def HasGitStagedChanges(git_dir):
  """Return True if git repository has staged changes."""
  command = 'cd {0} && git diff --quiet --cached --exit-code HEAD'.format(
      git_dir)
  return command_executer.GetCommandExecuter().RunCommand(
      command, print_to_console=False)


def HasGitUnstagedChanges(git_dir):
  """Return True if git repository has un-staged changes."""
  command = 'cd {0} && git diff --quiet --exit-code HEAD'.format(git_dir)
  return command_executer.GetCommandExecuter().RunCommand(
      command, print_to_console=False)


def HasGitUntrackedChanges(git_dir):
  """Return True if git repository has un-tracked changes."""
  command = ('cd {0} && test -z '
             '$(git ls-files --exclude-standard --others)').format(git_dir)
  return command_executer.GetCommandExecuter().RunCommand(
      command, print_to_console=False)


def GitGetCommitHash(git_dir, commit_symbolic_name):
  """Return githash for the symbolic git commit.

  For example, commit_symbolic_name could be
  "cros/gcc.gnu.org/branches/gcc/gcc-4_8-mobile, this function returns the git
  hash for this symbolic name.

  Args:
    git_dir: a git working tree.
    commit_symbolic_name: a symbolic name for a particular git commit.

  Returns:
    The git hash for the symbolic name or None if fails.
  """

  command = ('cd {0} && git log -n 1 --pretty="format:%H" {1}').format(
      git_dir, commit_symbolic_name)
  rv, out, _ = command_executer.GetCommandExecuter().RunCommandWOutput(
      command, print_to_console=False)
  if rv == 0:
    return out.strip()
  return None


def IsGitTreeClean(git_dir):
  """Test if git tree has no local changes.

  Args:
    git_dir: git tree directory.

  Returns:
    True if git dir is clean.
  """
  if HasGitStagedChanges(git_dir):
    logger.GetLogger().LogWarning('Git tree has staged changes.')
    return False
  if HasGitUnstagedChanges(git_dir):
    logger.GetLogger().LogWarning('Git tree has unstaged changes.')
    return False
  if HasGitUntrackedChanges(git_dir):
    logger.GetLogger().LogWarning('Git tree has un-tracked changes.')
    return False
  return True


def GetGitChangesAsList(git_dir, path=None, staged=False):
  """Get changed files as a list.

  Args:
    git_dir: git tree directory.
    path: a relative path that is part of the tree directory, could be null.
    staged: whether to include staged files as well.

  Returns:
    A list containing all the changed files.
  """
  command = 'cd {0} && git diff --name-only'.format(git_dir)
  if staged:
    command += ' --cached'
  if path:
    command += ' -- ' + path
  _, out, _ = command_executer.GetCommandExecuter().RunCommandWOutput(
      command, print_to_console=False)
  rv = []
  for line in out.splitlines():
    rv.append(line)
  return rv


def IsChromeOsTree(chromeos_root):
  return (os.path.isdir(
      os.path.join(chromeos_root, 'src/third_party/chromiumos-overlay')) and
          os.path.isdir(os.path.join(chromeos_root, 'manifest')))


def DeleteChromeOsTree(chromeos_root, dry_run=False):
  """Delete a ChromeOs tree *safely*.

  Args:
    chromeos_root: dir of the tree, could be a relative one (but be careful)
    dry_run: only prints out the command if True

  Returns:
    True if everything is ok.
  """
  if not IsChromeOsTree(chromeos_root):
    logger.GetLogger().LogWarning(
        '"{0}" does not seem to be a valid chromeos tree, do nothing.'.format(
            chromeos_root))
    return False
  cmd0 = 'cd {0} && cros_sdk --delete'.format(chromeos_root)
  if dry_run:
    print(cmd0)
  else:
    if command_executer.GetCommandExecuter().RunCommand(
        cmd0, print_to_console=True) != 0:
      return False

  cmd1 = ('export CHROMEOSDIRNAME="$(dirname $(cd {0} && pwd))" && '
          'export CHROMEOSBASENAME="$(basename $(cd {0} && pwd))" && '
          'cd $CHROMEOSDIRNAME && sudo rm -fr $CHROMEOSBASENAME'
         ).format(chromeos_root)
  if dry_run:
    print(cmd1)
    return True

  return command_executer.GetCommandExecuter().RunCommand(
      cmd1, print_to_console=True) == 0


def ApplyGerritPatches(chromeos_root, gerrit_patch_string,
                       branch='cros/master'):
  """Apply gerrit patches on a chromeos tree.

  Args:
    chromeos_root: chromeos tree path
    gerrit_patch_string: a patch string just like the one gives to cbuildbot,
    'id1 id2 *id3 ... idn'. A prefix of '* means this is an internal patch.
    branch: the tree based on which to apply the patches.

  Returns:
    True if success.
  """

  ### First of all, we need chromite libs
  sys.path.append(os.path.join(chromeos_root, 'chromite'))
  # Imports below are ok after modifying path to add chromite.
  # Pylint cannot detect that and complains.
  # pylint: disable=import-error
  from lib import git
  from lib import gerrit
  manifest = git.ManifestCheckout(chromeos_root)
  patch_list = gerrit_patch_string.split(' ')
  ### This takes time, print log information.
  logger.GetLogger().LogOutput('Retrieving patch information from server ...')
  patch_info_list = gerrit.GetGerritPatchInfo(patch_list)
  for pi in patch_info_list:
    project_checkout = manifest.FindCheckout(pi.project, strict=False)
    if not project_checkout:
      logger.GetLogger().LogError(
          'Failed to find patch project "{project}" in manifest.'.format(
              project=pi.project))
      return False

    pi_str = '{project}:{ref}'.format(project=pi.project, ref=pi.ref)
    try:
      project_git_path = project_checkout.GetPath(absolute=True)
      logger.GetLogger().LogOutput(
          'Applying patch "{0}" in "{1}" ...'.format(pi_str, project_git_path))
      pi.Apply(project_git_path, branch, trivial=False)
    except Exception:
      traceback.print_exc(file=sys.stdout)
      logger.GetLogger().LogError('Failed to apply patch "{0}"'.format(pi_str))
      return False
  return True


def BooleanPrompt(prompt='Do you want to continue?',
                  default=True,
                  true_value='yes',
                  false_value='no',
                  prolog=None):
  """Helper function for processing boolean choice prompts.

  Args:
    prompt: The question to present to the user.
    default: Boolean to return if the user just presses enter.
    true_value: The text to display that represents a True returned.
    false_value: The text to display that represents a False returned.
    prolog: The text to display before prompt.

  Returns:
    True or False.
  """
  true_value, false_value = true_value.lower(), false_value.lower()
  true_text, false_text = true_value, false_value
  if true_value == false_value:
    raise ValueError(
        'true_value and false_value must differ: got %r' % true_value)

  if default:
    true_text = true_text[0].upper() + true_text[1:]
  else:
    false_text = false_text[0].upper() + false_text[1:]

  prompt = ('\n%s (%s/%s)? ' % (prompt, true_text, false_text))

  if prolog:
    prompt = ('\n%s\n%s' % (prolog, prompt))

  while True:
    try:
      response = raw_input(prompt).lower()
    except EOFError:
      # If the user hits CTRL+D, or stdin is disabled, use the default.
      print()
      response = None
    except KeyboardInterrupt:
      # If the user hits CTRL+C, just exit the process.
      print()
      print('CTRL+C detected; exiting')
      sys.exit()

    if not response:
      return default
    if true_value.startswith(response):
      if not false_value.startswith(response):
        return True
      # common prefix between the two...
    elif false_value.startswith(response):
      return False


# pylint: disable=unused-argument
def rgb2short(r, g, b):
  """Converts RGB values to xterm-256 color."""

  redcolor = [255, 124, 160, 196, 9]
  greencolor = [255, 118, 82, 46, 10]

  if g == 0:
    return redcolor[r / 52]
  if r == 0:
    return greencolor[g / 52]
  return 4
