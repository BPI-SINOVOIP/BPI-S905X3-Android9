#!/usr/bin/env python
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
import argparse
import glob
import logging
import os
import shutil
import subprocess
import textwrap


THIS_DIR = os.path.realpath(os.path.dirname(__file__))


def logger():
    return logging.getLogger(__name__)


def check_call(cmd):
    logger().debug('Running `%s`', ' '.join(cmd))
    subprocess.check_call(cmd)


def remove(path):
    logger().debug('remove `%s`', path)
    os.remove(path)


def fetch_artifact(branch, build, pattern):
    fetch_artifact_path = '/google/data/ro/projects/android/fetch_artifact'
    cmd = [fetch_artifact_path, '--branch', branch, '--target=linux',
           '--bid', build, pattern]
    check_call(cmd)


def api_str(api_level):
    return 'android-{}'.format(api_level)


def start_branch(build):
    branch_name = 'update-' + (build or 'latest')
    logger().info('Creating branch %s', branch_name)
    check_call(['repo', 'start', branch_name, '.'])


def remove_old_release(install_dir):
    if os.path.exists(os.path.join(install_dir, '.git')):
        logger().info('Removing old install directory "%s"', install_dir)
        check_call(['git', 'rm', '-rf', install_dir])

    # Need to check again because git won't remove directories if they have
    # non-git files in them.
    if os.path.exists(install_dir):
        shutil.rmtree(install_dir)


def install_new_release(branch, build, install_dir):
    os.makedirs(install_dir)

    artifact_pattern = 'android-ndk-*.tar.bz2'
    logger().info('Fetching %s from %s (artifacts matching %s)', build, branch,
                  artifact_pattern)
    fetch_artifact(branch, build, artifact_pattern)
    artifacts = glob.glob('android-ndk-*.tar.bz2')
    try:
        assert len(artifacts) == 1
        artifact = artifacts[0]

        logger().info('Extracting release')
        cmd = ['tar', 'xf', artifact, '-C', install_dir, '--wildcards',
               '--strip-components=1', '*/platforms', '*/sources',
               '*/source.properties']
        check_call(cmd)
    finally:
        for artifact in artifacts:
            os.unlink(artifact)


def remove_unneeded_files(install_dir):
    for path, _dirs, files in os.walk(os.path.join(install_dir, 'platforms')):
        for file_name in files:
            if file_name.endswith('.so'):
                file_path = os.path.join(path, file_name)
                remove(file_path)

    for path, _dirs, files in os.walk(os.path.join(install_dir, 'sources')):
        for file_name in files:
            if file_name == 'Android.bp':
                file_path = os.path.join(path, file_name)
                remove(file_path)

    remove_stls = ['gabi++', 'gnu-libstdc++', 'stlport']
    for stl in remove_stls:
        shutil.rmtree(os.path.join(install_dir, 'sources/cxx-stl', stl))


def make_symlinks(install_dir):
    old_dir = os.getcwd()
    os.chdir(os.path.join(THIS_DIR, install_dir, 'platforms'))

    first_api = 9
    first_lp64_api = 21

    for api in xrange(first_api, first_lp64_api):
        if not os.path.exists(api_str(api)):
            continue

        for arch in ('arch-arm64', 'arch-mips64', 'arch-x86_64'):
            src = os.path.join('..', api_str(first_lp64_api), arch)
            dst = os.path.join(api_str(api), arch)
            if os.path.islink(dst):
                os.unlink(dst)
            os.symlink(src, dst)

    os.chdir(old_dir)


def commit(branch, build, install_dir):
    logger().info('Making commit')
    check_call(['git', 'add', install_dir])
    message = textwrap.dedent("""\
        Update NDK prebuilts to build {build}.

        Taken from branch {branch}.""").format(branch=branch, build=build)
    check_call(['git', 'commit', '-m', message])


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-b', '--branch', default='master-ndk',
        help='Branch to pull build from.')
    parser.add_argument(
        'major_release', help='Major release being installed, e.g. "r11".')
    parser.add_argument('--build', required=True, help='Build number to pull.')
    parser.add_argument(
        '--use-current-branch', action='store_true',
        help='Perform the update in the current branch. Do not repo start.')
    parser.add_argument(
        '-v', '--verbose', action='count', default=0,
        help='Increase output verbosity.')
    return parser.parse_args()


def main():
    os.chdir(THIS_DIR)

    args = get_args()
    verbose_map = (logging.WARNING, logging.INFO, logging.DEBUG)
    verbosity = args.verbose
    if verbosity > 2:
        verbosity = 2
    logging.basicConfig(level=verbose_map[verbosity])

    install_dir = os.path.realpath(args.major_release)

    if not args.use_current_branch:
        start_branch(args.build)
    remove_old_release(install_dir)
    install_new_release(args.branch, args.build, install_dir)
    remove_unneeded_files(install_dir)
    make_symlinks(install_dir)
    commit(args.branch, args.build, install_dir)


if __name__ == '__main__':
    main()
