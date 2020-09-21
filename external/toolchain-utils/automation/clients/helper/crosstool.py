# Copyright 2011 Google Inc. All Rights Reserved.

__author__ = 'kbaclawski@google.com (Krystian Baclawski)'

import os.path
import time

from automation.clients.helper import jobs
from automation.clients.helper import perforce
from automation.common import command as cmd
from automation.common import job


class JobsFactory(object):

  def __init__(self):
    self.commands = CommandsFactory()

  def CheckoutCrosstool(self, target):
    command = self.commands.CheckoutCrosstool()
    new_job = jobs.CreateLinuxJob('CheckoutCrosstool(%s)' % target, command)
    checkout_dir_dep = job.FolderDependency(new_job,
                                            CommandsFactory.CHECKOUT_DIR)
    manifests_dir_dep = job.FolderDependency(
        new_job, os.path.join(self.commands.buildit_path, target), 'manifests')
    return new_job, checkout_dir_dep, manifests_dir_dep

  def BuildRelease(self, checkout_dir, target):
    command = self.commands.BuildRelease(target)
    new_job = jobs.CreateLinuxJob('BuildRelease(%s)' % target, command)
    new_job.DependsOnFolder(checkout_dir)
    build_tree_dep = job.FolderDependency(new_job,
                                          self.commands.buildit_work_dir_path)
    return new_job, build_tree_dep

  def RunTests(self, checkout_dir, build_tree_dir, target, board, component):
    command = self.commands.RunTests(target, board, component)
    new_job = jobs.CreateLinuxJob('RunTests(%s, %s, %s)' %
                                  (target, component, board), command)
    new_job.DependsOnFolder(checkout_dir)
    new_job.DependsOnFolder(build_tree_dir)
    testrun_dir_dep = job.FolderDependency(
        new_job, self.commands.dejagnu_output_path, board)
    return new_job, testrun_dir_dep

  def GenerateReport(self, testrun_dirs, manifests_dir, target, boards):
    command = self.commands.GenerateReport(boards)
    new_job = jobs.CreateLinuxJob('GenerateReport(%s)' % target, command)
    new_job.DependsOnFolder(manifests_dir)
    for testrun_dir in testrun_dirs:
      new_job.DependsOnFolder(testrun_dir)
    return new_job


class CommandsFactory(object):
  CHECKOUT_DIR = 'crosstool-checkout-dir'

  def __init__(self):
    self.buildit_path = os.path.join(self.CHECKOUT_DIR, 'gcctools', 'crosstool',
                                     'v15')

    self.buildit_work_dir = 'buildit-tmp'
    self.buildit_work_dir_path = os.path.join('$JOB_TMP', self.buildit_work_dir)
    self.dejagnu_output_path = os.path.join(self.buildit_work_dir_path,
                                            'dejagnu-output')

    paths = {
        'gcctools': [
            'crosstool/v15/...', 'scripts/...'
        ],
        'gcctools/google_vendor_src_branch': [
            'binutils/binutils-2.21/...', 'gdb/gdb-7.2.x/...',
            'zlib/zlib-1.2.3/...'
        ],
        'gcctools/vendor_src': [
            'gcc/google/gcc-4_6/...'
        ]
    }

    p4view = perforce.View('depot2',
                           perforce.PathMapping.ListFromPathDict(paths))

    self.p4client = perforce.CommandsFactory(self.CHECKOUT_DIR, p4view)

  def CheckoutCrosstool(self):
    p4client = self.p4client

    return p4client.SetupAndDo(p4client.Sync(),
                               p4client.SaveCurrentCLNumber('CLNUM'),
                               p4client.Remove())

  def BuildRelease(self, target):
    clnum_path = os.path.join('$JOB_TMP', self.CHECKOUT_DIR, 'CLNUM')

    toolchain_root = os.path.join('/google/data/rw/projects/toolchains', target,
                                  'unstable')
    toolchain_path = os.path.join(toolchain_root, '${CLNUM}')

    build_toolchain = cmd.Wrapper(
        cmd.Chain(
            cmd.MakeDir(toolchain_path),
            cmd.Shell('buildit',
                      '--keep-work-dir',
                      '--build-type=release',
                      '--work-dir=%s' % self.buildit_work_dir_path,
                      '--results-dir=%s' % toolchain_path,
                      '--force-release=%s' % '${CLNUM}',
                      target,
                      path='.')),
        cwd=self.buildit_path,
        umask='0022',
        env={'CLNUM': '$(< %s)' % clnum_path})

    # remove all but 10 most recent directories
    remove_old_toolchains_from_x20 = cmd.Wrapper(
        cmd.Pipe(
            cmd.Shell('ls', '-1', '-r'), cmd.Shell('sed', '-e', '1,10d'),
            cmd.Shell('xargs', 'rm', '-r', '-f')),
        cwd=toolchain_root)

    return cmd.Chain(build_toolchain, remove_old_toolchains_from_x20)

  def RunTests(self, target, board, component='gcc'):
    dejagnu_flags = ['--outdir=%s' % self.dejagnu_output_path,
                     '--target_board=%s' % board]

    # Look for {pandaboard,qemu}.exp files in
    # //depot/google3/experimental/users/kbaclawski/dejagnu/boards

    site_exp_file = os.path.join('/google/src/head/depot/google3',
                                 'experimental/users/kbaclawski',
                                 'dejagnu/site.exp')

    build_dir_path = os.path.join(target, 'rpmbuild/BUILD/crosstool*-0.0',
                                  'build-%s' % component)

    run_dejagnu = cmd.Wrapper(
        cmd.Chain(
            cmd.MakeDir(self.dejagnu_output_path),
            cmd.Shell('make',
                      'check',
                      '-k',
                      '-j $(grep -c processor /proc/cpuinfo)',
                      'RUNTESTFLAGS="%s"' % ' '.join(dejagnu_flags),
                      'DEJAGNU="%s"' % site_exp_file,
                      ignore_error=True)),
        cwd=os.path.join(self.buildit_work_dir_path, build_dir_path),
        env={'REMOTE_TMPDIR': 'job-$JOB_ID'})

    save_results = cmd.Copy(self.dejagnu_output_path,
                            to_dir='$JOB_TMP/results',
                            recursive=True)

    return cmd.Chain(run_dejagnu, save_results)

  def GenerateReport(self, boards):
    sumfiles = [os.path.join('$JOB_TMP', board, '*.sum') for board in boards]

    return cmd.Wrapper(
        cmd.Shell('dejagnu.sh',
                  'report',
                  '-m',
                  '$JOB_TMP/manifests/*.xfail',
                  '-o',
                  '$JOB_TMP/results/report.html',
                  *sumfiles,
                  path='.'),
        cwd='$HOME/automation/clients/report')
