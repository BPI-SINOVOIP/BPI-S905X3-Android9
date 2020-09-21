# Copyright 2011 Google Inc. All Rights Reserved.

__author__ = 'asharif@google.com (Ahmad Sharif)'

import os.path
import re

from automation.clients.helper import jobs
from automation.clients.helper import perforce
from automation.common import command as cmd
from automation.common import machine


class ScriptsFactory(object):

  def __init__(self, chromeos_root, scripts_path):
    self._chromeos_root = chromeos_root
    self._scripts_path = scripts_path

  def SummarizeResults(self, logs_path):
    return cmd.Shell('summarize_results.py', logs_path, path=self._scripts_path)

  def Buildbot(self, config_name):
    buildbot = os.path.join(self._chromeos_root,
                            'chromite/cbuildbot/cbuildbot.py')

    return cmd.Shell(buildbot, '--buildroot=%s' % self._chromeos_root,
                     '--resume', '--noarchive', '--noprebuilts', '--nosync',
                     '--nouprev', '--notests', '--noclean', config_name)

  def RunBenchmarks(self, board, tests):
    image_path = os.path.join(self._chromeos_root, 'src/build/images', board,
                              'latest/chromiumos_image.bin')

    return cmd.Shell('cros_run_benchmarks.py',
                     '--remote=$SECONDARY_MACHINES[0]',
                     '--board=%s' % board,
                     '--tests=%s' % tests,
                     '--full_table',
                     image_path,
                     path='/home/mobiletc-prebuild')

  def SetupChromeOS(self, version='latest', use_minilayout=False):
    setup_chromeos = cmd.Shell('setup_chromeos.py',
                               '--public',
                               '--dir=%s' % self._chromeos_root,
                               '--version=%s' % version,
                               path=self._scripts_path)

    if use_minilayout:
      setup_chromeos.AddOption('--minilayout')
    return setup_chromeos


class CommandsFactory(object):
  DEPOT2_DIR = '//depot2/'
  P4_CHECKOUT_DIR = 'perforce2/'
  P4_VERSION_DIR = os.path.join(P4_CHECKOUT_DIR, 'gcctools/chromeos/v14')

  CHROMEOS_ROOT = 'chromeos'
  CHROMEOS_SCRIPTS_DIR = os.path.join(CHROMEOS_ROOT, 'src/scripts')
  CHROMEOS_BUILDS_DIR = '/home/mobiletc-prebuild/www/chromeos_builds'

  def __init__(self, chromeos_version, board, toolchain, p4_snapshot):
    self.chromeos_version = chromeos_version
    self.board = board
    self.toolchain = toolchain
    self.p4_snapshot = p4_snapshot

    self.scripts = ScriptsFactory(self.CHROMEOS_ROOT, self.P4_VERSION_DIR)

  def AddBuildbotConfig(self, config_name, config_list):
    config_header = 'add_config(%r, [%s])' % (config_name,
                                              ', '.join(config_list))
    config_file = os.path.join(self.CHROMEOS_ROOT,
                               'chromite/cbuildbot/cbuildbot_config.py')
    quoted_config_header = '%r' % config_header
    quoted_config_header = re.sub("'", "\\\"", quoted_config_header)

    return cmd.Pipe(
        cmd.Shell('echo', quoted_config_header),
        cmd.Shell('tee', '--append', config_file))

  def RunBuildbot(self):
    config_dict = {'board': self.board,
                   'build_tests': True,
                   'chrome_tests': True,
                   'unittests': False,
                   'vm_tests': False,
                   'prebuilts': False,
                   'latest_toolchain': True,
                   'useflags': ['chrome_internal'],
                   'usepkg_chroot': True,
                   self.toolchain: True}
    config_name = '%s-toolchain-test' % self.board
    if 'arm' in self.board:
      config_list = ['arm']
    else:
      config_list = []
    config_list.extend(['internal', 'full', 'official', str(config_dict)])

    add_config_shell = self.AddBuildbotConfig(config_name, config_list)
    return cmd.Chain(add_config_shell, self.scripts.Buildbot(config_name))

  def BuildAndBenchmark(self):
    return cmd.Chain(
        self.CheckoutV14Dir(),
        self.SetupChromeOSCheckout(self.chromeos_version, True),
        self.RunBuildbot(),
        self.scripts.RunBenchmarks(self.board, 'BootPerfServer,10:Page,3'))

  def GetP4Snapshot(self, p4view):
    p4client = perforce.CommandsFactory(self.P4_CHECKOUT_DIR, p4view)

    if self.p4_snapshot:
      return p4client.CheckoutFromSnapshot(self.p4_snapshot)
    else:
      return p4client.SetupAndDo(p4client.Sync(), p4client.Remove())

  def CheckoutV14Dir(self):
    p4view = perforce.View(self.DEPOT2_DIR, [
        perforce.PathMapping('gcctools/chromeos/v14/...')
    ])
    return self.GetP4Snapshot(p4view)

  def SetupChromeOSCheckout(self, version, use_minilayout=False):
    version_re = '^\d+\.\d+\.\d+\.[a-zA-Z0-9]+$'

    location = os.path.join(self.CHROMEOS_BUILDS_DIR, version)

    if version in ['weekly', 'quarterly']:
      assert os.path.islink(location), 'Symlink %s does not exist.' % location

      location_expanded = os.path.abspath(os.path.realpath(location))
      version = os.path.basename(location_expanded)

    if version in ['top', 'latest'] or re.match(version_re, version):
      return self.scripts.SetupChromeOS(version, use_minilayout)

    elif version.endswith('bz2') or version.endswith('gz'):
      return cmd.UnTar(location_expanded, self.CHROMEOS_ROOT)

    else:
      signature_file_location = os.path.join(location,
                                             'src/scripts/enter_chroot.sh')
      assert os.path.exists(signature_file_location), (
          'Signature file %s does not exist.' % signature_file_location)

      return cmd.Copy(location, to_dir=self.CHROMEOS_ROOT, recursive=True)


class JobsFactory(object):

  def __init__(self,
               chromeos_version='top',
               board='x86-mario',
               toolchain='trunk',
               p4_snapshot=''):
    self.chromeos_version = chromeos_version
    self.board = board
    self.toolchain = toolchain

    self.commands = CommandsFactory(chromeos_version, board, toolchain,
                                    p4_snapshot)

  def BuildAndBenchmark(self):
    command = self.commands.BuildAndBenchmark()

    label = 'BuildAndBenchmark(%s,%s,%s)' % (self.toolchain, self.board,
                                             self.chromeos_version)

    machine_label = 'chromeos-%s' % self.board

    job = jobs.CreateLinuxJob(label, command)
    job.DependsOnMachine(
        machine.MachineSpecification(label=machine_label,
                                     lock_required=True),
        False)

    return job
