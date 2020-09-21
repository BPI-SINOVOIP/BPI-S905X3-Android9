#!/usr/bin/python

# Copyright (c) 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module to automate the process of deploying to production.

Example usage of this script:
  1. Update both autotest and chromite to the lastest commit that has passed
     the test instance.
     $ ./site_utils/automated_deploy.py
  2. Skip updating a repo, e.g. autotest
     $ ./site_utils/automated_deploy.py --skip_autotest
  3. Update a given repo to a specific commit
     $ ./site_utils/automated_deploy.py --autotest_hash='1234'
"""

import argparse
import os
import re
import sys
import subprocess

import common
from autotest_lib.client.common_lib import revision_control
from autotest_lib.site_utils.lib import infra

AUTOTEST_DIR = common.autotest_dir
GIT_URL = {'autotest':
           'https://chromium.googlesource.com/chromiumos/third_party/autotest',
           'chromite':
           'https://chromium.googlesource.com/chromiumos/chromite'}
PROD_BRANCH = 'prod'
MASTER_AFE = 'cautotest'
NOTIFY_GROUP = 'chromeos-infra-discuss@google.com'

# CIPD packages whose prod refs should be updated.
_CIPD_PACKAGES = (
        'chromiumos/infra/lucifer',
        'chromiumos/infra/tast-cmd',
        'chromiumos/infra/tast-remote-tests-cros',
)


class AutoDeployException(Exception):
    """Raised when any deploy step fails."""


def parse_arguments():
    """Parse command line arguments.

    @returns An argparse.Namespace populated with argument values.
    """
    parser = argparse.ArgumentParser(
            description=('Command to update prod branch for autotest, chromite '
                         'repos. Then deploy new changes to all lab servers.'))
    parser.add_argument('--skip_autotest', action='store_true', default=False,
            help='Skip updating autotest prod branch. Default is False.')
    parser.add_argument('--skip_chromite', action='store_true', default=False,
            help='Skip updating chromite prod branch. Default is False.')
    parser.add_argument('--force_update', action='store_true', default=False,
            help=('Force a deployment without updating both autotest and '
                  'chromite prod branch'))
    parser.add_argument('--autotest_hash', type=str, default=None,
            help='Update autotest prod branch to the given hash. If it is not'
                 ' specified, autotest prod branch will be rebased to '
                 'prod-next branch, which is the latest commit that has '
                 'passed our test instance.')
    parser.add_argument('--chromite_hash', type=str, default=None,
            help='Same as autotest_hash option.')

    results = parser.parse_args(sys.argv[1:])

    # Verify the validity of the options.
    if ((results.skip_autotest and results.autotest_hash) or
        (results.skip_chromite and results.chromite_hash)):
        parser.print_help()
        print 'Cannot specify skip_* and *_hash options at the same time.'
        sys.exit(1)
    if results.force_update:
      results.skip_autotest = True
      results.skip_chromite = True
    return results


def clone_prod_branch(repo):
    """Method to clone the prod branch for a given repo under /tmp/ dir.

    @param repo: Name of the git repo to be cloned.

    @returns path to the cloned repo.
    @raises subprocess.CalledProcessError on a command failure.
    @raised revision_control.GitCloneError when git clone fails.
    """
    repo_dir = '/tmp/%s' % repo
    print 'Cloning %s prod branch under %s' % (repo, repo_dir)
    if os.path.exists(repo_dir):
        infra.local_runner('rm -rf %s' % repo_dir)
    git_repo = revision_control.GitRepo(repo_dir, GIT_URL[repo])
    git_repo.clone(remote_branch=PROD_BRANCH)
    print 'Successfully cloned %s prod branch' % repo
    return repo_dir


def update_prod_branch(repo, repo_dir, hash_to_rebase):
    """Method to update the prod branch of the given repo to the given hash.

    @param repo: Name of the git repo to be updated.
    @param repo_dir: path to the cloned repo.
    @param hash_to_rebase: Hash to rebase the prod branch to. If it is None,
                           prod branch will rebase to prod-next branch.

    @returns the range of the pushed commits as a string. E.g 123...345. If the
        prod branch is already up-to-date, return None.
    @raises subprocess.CalledProcessError on a command failure.
    """
    with infra.chdir(repo_dir):
        print 'Updating %s prod branch.' % repo
        rebase_to = hash_to_rebase if hash_to_rebase else 'origin/prod-next'
        # Check whether prod branch is already up-to-date, which means there is
        # no changes since last push.
        print 'Detecting new changes since last push...'
        diff = infra.local_runner('git log prod..%s --oneline' % rebase_to,
                                  stream_output=True)
        if diff:
            print 'Find new changes, will update prod branch...'
            infra.local_runner('git rebase %s prod' % rebase_to,
                               stream_output=True)
            result = infra.local_runner('git push origin prod',
                                        stream_output=True)
            print 'Successfully pushed %s prod branch!\n' % repo

            # Get the pushed commit range, which is used to get pushed commits
            # using git log E.g. 123..456, then run git log --oneline 123..456.
            grep = re.search('(\w)*\.\.(\w)*', result)

            if not grep:
                raise AutoDeployException(
                    'Fail to get pushed commits for repo %s from git log: %s' %
                    (repo, result))
            return grep.group(0)
        else:
            print 'No new %s changes found since last push.' % repo
            return None


def get_pushed_commits(repo, repo_dir, pushed_commits_range):
    """Method to get the pushed commits.

    @param repo: Name of the updated git repo.
    @param repo_dir: path to the cloned repo.
    @param pushed_commits_range: The range of the pushed commits. E.g 123...345
    @return: the commits that are pushed to prod branch. The format likes this:
             "git log --oneline A...B | grep autotest
              A xxxx
              B xxxx"
    @raises subprocess.CalledProcessError on a command failure.
    """
    print 'Getting pushed CLs for %s repo.' % repo
    if not pushed_commits_range:
        return '\n%s:\nNo new changes since last push.' % repo

    with infra.chdir(repo_dir):
        get_commits_cmd = 'git log --oneline %s' % pushed_commits_range

        pushed_commits = infra.local_runner(
                get_commits_cmd, stream_output=True)
        if repo == 'autotest':
            autotest_commits = ''
            for cl in pushed_commits.splitlines():
                if 'autotest' in cl:
                    autotest_commits += '%s\n' % cl

            pushed_commits = autotest_commits

        print 'Successfully got pushed CLs for %s repo!\n' % repo
        displayed_cmd = get_commits_cmd
        if repo == 'autotest':
          displayed_cmd += ' | grep autotest'
        return '\n%s:\n%s\n%s\n' % (repo, displayed_cmd, pushed_commits)


def kick_off_deploy():
    """Method to kick off deploy script to deploy changes to lab servers.

    @raises subprocess.CalledProcessError on a repo command failure.
    """
    print 'Start deploying changes to all lab servers...'
    with infra.chdir(AUTOTEST_DIR):
        # Then kick off the deploy script.
        deploy_cmd = ('runlocalssh ./site_utils/deploy_server.py -x --afe=%s' %
                      MASTER_AFE)
        infra.local_runner(deploy_cmd, stream_output=True)
        print 'Successfully deployed changes to all lab servers.'


def main(args):
    """Main entry"""
    options = parse_arguments()
    repos = dict()
    if not options.skip_autotest:
        repos.update({'autotest': options.autotest_hash})
    if not options.skip_chromite:
        repos.update({'chromite': options.chromite_hash})

    print 'Moving CIPD prod refs to prod-next'
    for pkg in _CIPD_PACKAGES:
        subprocess.check_call(['cipd', 'set-ref', pkg, '-version', 'prod-next',
                               '-ref', 'prod'])
    try:
        # update_log saves the git log of the updated repo.
        update_log = ''
        for repo, hash_to_rebase in repos.iteritems():
            repo_dir = clone_prod_branch(repo)
            push_commits_range = update_prod_branch(
                repo, repo_dir, hash_to_rebase)
            update_log += get_pushed_commits(repo, repo_dir, push_commits_range)

        kick_off_deploy()
    except revision_control.GitCloneError as e:
        print 'Fail to clone prod branch. Error:\n%s\n' % e
        raise
    except subprocess.CalledProcessError as e:
        print ('Deploy fails when running a subprocess cmd :\n%s\n'
               'Below is the push log:\n%s\n' % (e.output, update_log))
        raise
    except Exception as e:
        print 'Deploy fails with error:\n%s\nPush log:\n%s\n' % (e, update_log)
        raise

    # When deploy succeeds, print the update_log.
    print ('Deploy succeeds!!! Below is the push log of the updated repo:\n%s'
           'Please email this to %s.'% (update_log, NOTIFY_GROUP))


if __name__ == '__main__':
    sys.exit(main(sys.argv))
