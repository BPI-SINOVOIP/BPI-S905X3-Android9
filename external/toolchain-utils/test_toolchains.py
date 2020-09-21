#!/usr/bin/env python2

# Script to test different toolchains against ChromeOS benchmarks.
"""Toolchain team nightly performance test script (local builds)."""

from __future__ import print_function

import argparse
import datetime
import os
import sys
import build_chromeos
import setup_chromeos
from cros_utils import command_executer
from cros_utils import misc
from cros_utils import logger

CROSTC_ROOT = '/usr/local/google/crostc'
MAIL_PROGRAM = '~/var/bin/mail-sheriff'
PENDING_ARCHIVES_DIR = os.path.join(CROSTC_ROOT, 'pending_archives')
NIGHTLY_TESTS_DIR = os.path.join(CROSTC_ROOT, 'nightly_test_reports')


class GCCConfig(object):
  """GCC configuration class."""

  def __init__(self, githash):
    self.githash = githash


class ToolchainConfig(object):
  """Toolchain configuration class."""

  def __init__(self, gcc_config=None):
    self.gcc_config = gcc_config


class ChromeOSCheckout(object):
  """Main class for checking out, building and testing ChromeOS."""

  def __init__(self, board, chromeos_root):
    self._board = board
    self._chromeos_root = chromeos_root
    self._ce = command_executer.GetCommandExecuter()
    self._l = logger.GetLogger()
    self._build_num = None

  def _DeleteChroot(self):
    command = 'cd %s; cros_sdk --delete' % self._chromeos_root
    return self._ce.RunCommand(command)

  def _DeleteCcahe(self):
    # crosbug.com/34956
    command = 'sudo rm -rf %s' % os.path.join(self._chromeos_root, '.cache')
    return self._ce.RunCommand(command)

  def _GetBuildNumber(self):
    """Get the build number of the ChromeOS image from the chroot.

    This function assumes a ChromeOS image has been built in the chroot.
    It translates the 'latest' symlink in the
    <chroot>/src/build/images/<board> directory, to find the actual
    ChromeOS build number for the image that was built.  For example, if
    src/build/image/lumpy/latest ->  R37-5982.0.2014_06_23_0454-a1, then
    This function would parse it out and assign 'R37-5982' to self._build_num.
    This is used to determine the official, vanilla build to use for
    comparison tests.
    """
    # Get the path to 'latest'
    sym_path = os.path.join(
        misc.GetImageDir(self._chromeos_root, self._board), 'latest')
    # Translate the symlink to its 'real' path.
    real_path = os.path.realpath(sym_path)
    # Break up the path and get the last piece
    # (e.g. 'R37-5982.0.2014_06_23_0454-a1"
    path_pieces = real_path.split('/')
    last_piece = path_pieces[-1]
    # Break this piece into the image number + other pieces, and get the
    # image number [ 'R37-5982', '0', '2014_06_23_0454-a1']
    image_parts = last_piece.split('.')
    self._build_num = image_parts[0]

  def _BuildLabelName(self, config):
    pieces = config.split('/')
    compiler_version = pieces[-1]
    label = compiler_version + '_tot_afdo'
    return label

  def _BuildAndImage(self, label=''):
    if (not label or
        not misc.DoesLabelExist(self._chromeos_root, self._board, label)):
      build_chromeos_args = [
          build_chromeos.__file__, '--chromeos_root=%s' % self._chromeos_root,
          '--board=%s' % self._board, '--rebuild'
      ]
      if self._public:
        build_chromeos_args.append('--env=USE=-chrome_internal')

      ret = build_chromeos.Main(build_chromeos_args)
      if ret != 0:
        raise RuntimeError("Couldn't build ChromeOS!")

      if not self._build_num:
        self._GetBuildNumber()
      # Check to see if we need to create the symbolic link for the vanilla
      # image, and do so if appropriate.
      if not misc.DoesLabelExist(self._chromeos_root, self._board, 'vanilla'):
        build_name = '%s-release/%s.0.0' % (self._board, self._build_num)
        full_vanilla_path = os.path.join(os.getcwd(), self._chromeos_root,
                                         'chroot/tmp', build_name)
        misc.LabelLatestImage(self._chromeos_root, self._board, label,
                              full_vanilla_path)
      else:
        misc.LabelLatestImage(self._chromeos_root, self._board, label)
    return label

  def _SetupBoard(self, env_dict, usepkg_flag, clobber_flag):
    env_string = misc.GetEnvStringFromDict(env_dict)
    command = ('%s %s' % (env_string, misc.GetSetupBoardCommand(
        self._board, usepkg=usepkg_flag, force=clobber_flag)))
    ret = self._ce.ChrootRunCommand(self._chromeos_root, command)
    error_str = "Could not setup board: '%s'" % command
    assert ret == 0, error_str

  def _UnInstallToolchain(self):
    command = ('sudo CLEAN_DELAY=0 emerge -C cross-%s/gcc' %
               misc.GetCtargetFromBoard(self._board, self._chromeos_root))
    ret = self._ce.ChrootRunCommand(self._chromeos_root, command)
    if ret != 0:
      raise RuntimeError("Couldn't uninstall the toolchain!")

  def _CheckoutChromeOS(self):
    # TODO(asharif): Setup a fixed ChromeOS version (quarterly snapshot).
    if not os.path.exists(self._chromeos_root):
      setup_chromeos_args = ['--dir=%s' % self._chromeos_root]
      if self._public:
        setup_chromeos_args.append('--public')
      ret = setup_chromeos.Main(setup_chromeos_args)
      if ret != 0:
        raise RuntimeError("Couldn't run setup_chromeos!")

  def _BuildToolchain(self, config):
    # Call setup_board for basic, vanilla setup.
    self._SetupBoard({}, usepkg_flag=True, clobber_flag=False)
    # Now uninstall the vanilla compiler and setup/build our custom
    # compiler.
    self._UnInstallToolchain()
    envdict = {
        'USE': 'git_gcc',
        'GCC_GITHASH': config.gcc_config.githash,
        'EMERGE_DEFAULT_OPTS': '--exclude=gcc'
    }
    self._SetupBoard(envdict, usepkg_flag=False, clobber_flag=False)


class ToolchainComparator(ChromeOSCheckout):
  """Main class for running tests and generating reports."""

  def __init__(self,
               board,
               remotes,
               configs,
               clean,
               public,
               force_mismatch,
               noschedv2=False):
    self._board = board
    self._remotes = remotes
    self._chromeos_root = 'chromeos'
    self._configs = configs
    self._clean = clean
    self._public = public
    self._force_mismatch = force_mismatch
    self._ce = command_executer.GetCommandExecuter()
    self._l = logger.GetLogger()
    timestamp = datetime.datetime.strftime(datetime.datetime.now(),
                                           '%Y-%m-%d_%H:%M:%S')
    self._reports_dir = os.path.join(
        NIGHTLY_TESTS_DIR,
        '%s.%s' % (timestamp, board),)
    self._noschedv2 = noschedv2
    ChromeOSCheckout.__init__(self, board, self._chromeos_root)

  def _FinishSetup(self):
    # Get correct .boto file
    current_dir = os.getcwd()
    src = '/usr/local/google/home/mobiletc-prebuild/.boto'
    dest = os.path.join(current_dir, self._chromeos_root,
                        'src/private-overlays/chromeos-overlay/'
                        'googlestorage_account.boto')
    # Copy the file to the correct place
    copy_cmd = 'cp %s %s' % (src, dest)
    retv = self._ce.RunCommand(copy_cmd)
    if retv != 0:
      raise RuntimeError("Couldn't copy .boto file for google storage.")

    # Fix protections on ssh key
    command = ('chmod 600 /var/cache/chromeos-cache/distfiles/target'
               '/chrome-src-internal/src/third_party/chromite/ssh_keys'
               '/testing_rsa')
    retv = self._ce.ChrootRunCommand(self._chromeos_root, command)
    if retv != 0:
      raise RuntimeError('chmod for testing_rsa failed')

  def _TestLabels(self, labels):
    experiment_file = 'toolchain_experiment.txt'
    image_args = ''
    if self._force_mismatch:
      image_args = '--force-mismatch'
    experiment_header = """
    board: %s
    remote: %s
    retries: 1
    """ % (self._board, self._remotes)
    experiment_tests = """
    benchmark: all_toolchain_perf {
      suite: telemetry_Crosperf
      iterations: 3
    }
    """

    with open(experiment_file, 'w') as f:
      f.write(experiment_header)
      f.write(experiment_tests)
      for label in labels:
        # TODO(asharif): Fix crosperf so it accepts labels with symbols
        crosperf_label = label
        crosperf_label = crosperf_label.replace('-', '_')
        crosperf_label = crosperf_label.replace('+', '_')
        crosperf_label = crosperf_label.replace('.', '')

        # Use the official build instead of building vanilla ourselves.
        if label == 'vanilla':
          build_name = '%s-release/%s.0.0' % (self._board, self._build_num)

          # Now add 'official build' to test file.
          official_image = """
          official_image {
            chromeos_root: %s
            build: %s
          }
          """ % (self._chromeos_root, build_name)
          f.write(official_image)

        else:
          experiment_image = """
          %s {
            chromeos_image: %s
            image_args: %s
          }
          """ % (crosperf_label, os.path.join(
              misc.GetImageDir(self._chromeos_root, self._board), label,
              'chromiumos_test_image.bin'), image_args)
          f.write(experiment_image)

    crosperf = os.path.join(os.path.dirname(__file__), 'crosperf', 'crosperf')
    noschedv2_opts = '--noschedv2' if self._noschedv2 else ''
    command = ('{crosperf} --no_email=True --results_dir={r_dir} '
               '--json_report=True {noschedv2_opts} {exp_file}').format(
                   crosperf=crosperf,
                   r_dir=self._reports_dir,
                   noschedv2_opts=noschedv2_opts,
                   exp_file=experiment_file)

    ret = self._ce.RunCommand(command)
    if ret != 0:
      raise RuntimeError('Crosperf execution error!')
    else:
      # Copy json report to pending archives directory.
      command = 'cp %s/*.json %s/.' % (self._reports_dir, PENDING_ARCHIVES_DIR)
      ret = self._ce.RunCommand(command)
    return

  def _SendEmail(self):
    """Find email msesage generated by crosperf and send it."""
    filename = os.path.join(self._reports_dir, 'msg_body.html')
    if (os.path.exists(filename) and
        os.path.exists(os.path.expanduser(MAIL_PROGRAM))):
      command = ('cat %s | %s -s "Nightly test results, %s" -team -html' %
                 (filename, MAIL_PROGRAM, self._board))
      self._ce.RunCommand(command)

  def DoAll(self):
    self._CheckoutChromeOS()
    labels = []
    labels.append('vanilla')
    for config in self._configs:
      label = self._BuildLabelName(config.gcc_config.githash)
      if not misc.DoesLabelExist(self._chromeos_root, self._board, label):
        self._BuildToolchain(config)
        label = self._BuildAndImage(label)
      labels.append(label)
    self._FinishSetup()
    self._TestLabels(labels)
    self._SendEmail()
    if self._clean:
      ret = self._DeleteChroot()
      if ret != 0:
        return ret
      ret = self._DeleteCcahe()
      if ret != 0:
        return ret
    return 0


def Main(argv):
  """The main function."""
  # Common initializations
  ###  command_executer.InitCommandExecuter(True)
  command_executer.InitCommandExecuter()
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--remote', dest='remote', help='Remote machines to run tests on.')
  parser.add_argument(
      '--board', dest='board', default='x86-alex', help='The target board.')
  parser.add_argument(
      '--githashes',
      dest='githashes',
      default='master',
      help='The gcc githashes to test.')
  parser.add_argument(
      '--clean',
      dest='clean',
      default=False,
      action='store_true',
      help='Clean the chroot after testing.')
  parser.add_argument(
      '--public',
      dest='public',
      default=False,
      action='store_true',
      help='Use the public checkout/build.')
  parser.add_argument(
      '--force-mismatch',
      dest='force_mismatch',
      default='',
      help='Force the image regardless of board mismatch')
  parser.add_argument(
      '--noschedv2',
      dest='noschedv2',
      action='store_true',
      default=False,
      help='Pass --noschedv2 to crosperf.')
  options = parser.parse_args(argv)
  if not options.board:
    print('Please give a board.')
    return 1
  if not options.remote:
    print('Please give at least one remote machine.')
    return 1
  toolchain_configs = []
  for githash in options.githashes.split(','):
    gcc_config = GCCConfig(githash=githash)
    toolchain_config = ToolchainConfig(gcc_config=gcc_config)
    toolchain_configs.append(toolchain_config)
  fc = ToolchainComparator(options.board, options.remote, toolchain_configs,
                           options.clean, options.public,
                           options.force_mismatch, options.noschedv2)
  return fc.DoAll()


if __name__ == '__main__':
  retval = Main(sys.argv[1:])
  sys.exit(retval)
