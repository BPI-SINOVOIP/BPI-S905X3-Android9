#!/usr/bin/python
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs on autotest servers from a cron job to self update them.

This script is designed to run on all autotest servers to allow them to
automatically self-update based on the manifests used to create their (existing)
repos.
"""

from __future__ import print_function

import ConfigParser
import argparse
import os
import re
import subprocess
import sys
import time

import common

from autotest_lib.client.common_lib import global_config
from autotest_lib.server import utils as server_utils
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers


# How long after restarting a service do we watch it to see if it's stable.
SERVICE_STABILITY_TIMER = 60

# A dict to map update_commands defined in config file to repos or files that
# decide whether need to update these commands. E.g. if no changes under
# frontend repo, no need to update afe.
COMMANDS_TO_REPOS_DICT = {'afe': 'frontend/client/',
                          'tko': 'frontend/client/'}
BUILD_EXTERNALS_COMMAND = 'build_externals'

_RESTART_SERVICES_FILE = os.path.join(os.environ['HOME'],
                                      'push_restart_services')

AFE = frontend_wrappers.RetryingAFE(
        server=server_utils.get_global_afe_hostname(), timeout_min=5,
        delay_sec=10)

class DirtyTreeException(Exception):
    """Raised when the tree has been modified in an unexpected way."""


class UnknownCommandException(Exception):
    """Raised when we try to run a command name with no associated command."""


class UnstableServices(Exception):
    """Raised if a service appears unstable after restart."""


def strip_terminal_codes(text):
    """This function removes all terminal formatting codes from a string.

    @param text: String of text to cleanup.
    @returns String with format codes removed.
    """
    ESC = '\x1b'
    return re.sub(ESC+r'\[[^m]*m', '', text)


def _clean_pyc_files():
    print('Removing .pyc files')
    try:
        subprocess.check_output([
                'find', '.',
                '(',
                # These are ignored to reduce IO load (crbug.com/759780).
                '-path', './site-packages',
                '-o', '-path', './containers',
                '-o', '-path', './logs',
                '-o', '-path', './results',
                ')',
                '-prune',
                '-o', '-name', '*.pyc',
                '-exec', 'rm', '-f', '{}', '+'])
    except Exception as e:
        print('Warning: fail to remove .pyc! %s' % e)


def verify_repo_clean():
    """This function cleans the current repo then verifies that it is valid.

    @raises DirtyTreeException if the repo is still not clean.
    @raises subprocess.CalledProcessError on a repo command failure.
    """
    subprocess.check_output(['git', 'reset', '--hard'])
    # Forcefully blow away any non-gitignored files in the tree.
    subprocess.check_output(['git', 'clean', '-fd'])
    out = subprocess.check_output(['repo', 'status'], stderr=subprocess.STDOUT)
    out = strip_terminal_codes(out).strip()

    if not 'working directory clean' in out:
        raise DirtyTreeException(out)


def _clean_externals():
    """Clean untracked files within ExternalSource and site-packages/

    @raises subprocess.CalledProcessError on a git command failure.
    """
    dirs_to_clean = ['site-packages/', 'ExternalSource/']
    cmd = ['git', 'clean', '-fxd'] + dirs_to_clean
    subprocess.check_output(cmd)


def repo_versions():
    """This function collects the versions of all git repos in the general repo.

    @returns A dictionary mapping project names to git hashes for HEAD.
    @raises subprocess.CalledProcessError on a repo command failure.
    """
    cmd = ['repo', 'forall', '-p', '-c', 'pwd && git log -1 --format=%h']
    output = strip_terminal_codes(subprocess.check_output(cmd))

    # The expected output format is:

    # project chrome_build/
    # /dir/holding/chrome_build
    # 73dee9d
    #
    # project chrome_release/
    # /dir/holding/chrome_release
    # 9f3a5d8

    lines = output.splitlines()

    PROJECT_PREFIX = 'project '

    project_heads = {}
    for n in range(0, len(lines), 4):
        project_line = lines[n]
        project_dir = lines[n+1]
        project_hash = lines[n+2]
        # lines[n+3] is a blank line, but doesn't exist for the final block.

        # Convert 'project chrome_build/' -> 'chrome_build'
        assert project_line.startswith(PROJECT_PREFIX)
        name = project_line[len(PROJECT_PREFIX):].rstrip('/')

        project_heads[name] = (project_dir, project_hash)

    return project_heads


def repo_versions_to_decide_whether_run_cmd_update():
    """Collect versions of repos/files defined in COMMANDS_TO_REPOS_DICT.

    For the update_commands defined in config files, no need to run the command
    every time. Only run it when the repos/files related to the commands have
    been changed.

    @returns A set of tuples: {(cmd, repo_version), ()...}
    """
    results = set()
    for cmd, repo in COMMANDS_TO_REPOS_DICT.iteritems():
        version = subprocess.check_output(
                ['git', 'log', '-1', '--pretty=tformat:%h',
                 '%s/%s' % (common.autotest_dir, repo)])
        results.add((cmd, version.strip()))
    return results


def repo_sync(update_push_servers=False):
    """Perform a repo sync.

    @param update_push_servers: If True, then update test_push servers to ToT.
                                Otherwise, update server to prod branch.
    @raises subprocess.CalledProcessError on a repo command failure.
    """
    subprocess.check_output(['repo', 'sync'])
    if update_push_servers:
        print('Updating push servers, checkout cros/master')
        subprocess.check_output(['git', 'checkout', 'cros/master'],
                                stderr=subprocess.STDOUT)
    else:
        print('Updating server to prod branch')
        subprocess.check_output(['git', 'checkout', 'cros/prod'],
                                stderr=subprocess.STDOUT)
    _clean_pyc_files()


def discover_update_commands():
    """Lookup the commands to run on this server.

    These commonly come from shadow_config.ini, since they vary by server type.

    @returns List of command names in string format.
    """
    try:
        return global_config.global_config.get_config_value(
                'UPDATE', 'commands', type=list)

    except (ConfigParser.NoSectionError, global_config.ConfigError):
        return []


def get_restart_services():
    """Find the services that need restarting on the current server.

    These commonly come from shadow_config.ini, since they vary by server type.

    @returns Iterable of service names in string format.
    """
    with open(_RESTART_SERVICES_FILE) as f:
        for line in f:
            yield line.rstrip()


def update_command(cmd_tag, dryrun=False, use_chromite_master=False):
    """Restart a command.

    The command name is looked up in global_config.ini to find the full command
    to run, then it's executed.

    @param cmd_tag: Which command to restart.
    @param dryrun: If true print the command that would have been run.
    @param use_chromite_master: True if updating chromite to master, rather
                                than prod.

    @raises UnknownCommandException If cmd_tag can't be looked up.
    @raises subprocess.CalledProcessError on a command failure.
    """
    # Lookup the list of commands to consider. They are intended to be
    # in global_config.ini so that they can be shared everywhere.
    cmds = dict(global_config.global_config.config.items(
        'UPDATE_COMMANDS'))

    if cmd_tag not in cmds:
        raise UnknownCommandException(cmd_tag, cmds)

    expanded_command = cmds[cmd_tag].replace('AUTOTEST_REPO',
                                              common.autotest_dir)
    # When updating push servers, pass an arg to build_externals to update
    # chromite to master branch for testing
    if use_chromite_master and cmd_tag == BUILD_EXTERNALS_COMMAND:
        expanded_command += ' --use_chromite_master'

    print('Running: %s: %s' % (cmd_tag, expanded_command))
    if dryrun:
        print('Skip: %s' % expanded_command)
    else:
        try:
            subprocess.check_output(expanded_command, shell=True,
                                    stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError as e:
            print('FAILED:')
            print(e.output)
            raise


def restart_service(service_name, dryrun=False):
    """Restart a service.

    Restarts the standard service with "service <name> restart".

    @param service_name: The name of the service to restart.
    @param dryrun: Don't really run anything, just print out the command.

    @raises subprocess.CalledProcessError on a command failure.
    """
    cmd = ['sudo', 'service', service_name, 'restart']
    print('Restarting: %s' % service_name)
    if dryrun:
        print('Skip: %s' % ' '.join(cmd))
    else:
        subprocess.check_call(cmd, stderr=subprocess.STDOUT)


def service_status(service_name):
    """Return the results "status <name>" for a given service.

    This string is expected to contain the pid, and so to change is the service
    is shutdown or restarted for any reason.

    @param service_name: The name of the service to check on.

    @returns The output of the external command.
             Ex: autofs start/running, process 1931

    @raises subprocess.CalledProcessError on a command failure.
    """
    return subprocess.check_output(['sudo', 'service', service_name, 'status'])


def restart_services(service_names, dryrun=False, skip_service_status=False):
    """Restart services as needed for the current server type.

    Restart the listed set of services, and watch to see if they are stable for
    at least SERVICE_STABILITY_TIMER. It restarts all services quickly,
    waits for that delay, then verifies the status of all of them.

    @param service_names: The list of service to restart and monitor.
    @param dryrun: Don't really restart the service, just print out the command.
    @param skip_service_status: Set to True to skip service status check.
                                Default is False.

    @raises subprocess.CalledProcessError on a command failure.
    @raises UnstableServices if any services are unstable after restart.
    """
    service_statuses = {}

    if dryrun:
        for name in service_names:
            restart_service(name, dryrun=True)
        return

    # Restart each, and record the status (including pid).
    for name in service_names:
        restart_service(name)

    # Skip service status check if --skip-service-status is specified. Used for
    # servers in backup status.
    if skip_service_status:
        print('--skip-service-status is specified, skip checking services.')
        return

    # Wait for a while to let the services settle.
    time.sleep(SERVICE_STABILITY_TIMER)
    service_statuses = {name: service_status(name) for name in service_names}
    time.sleep(SERVICE_STABILITY_TIMER)
    # Look for any services that changed status.
    unstable_services = [n for n in service_names
                         if service_status(n) != service_statuses[n]]

    # Report any services having issues.
    if unstable_services:
        raise UnstableServices(unstable_services)


def run_deploy_actions(cmds_skip=set(), dryrun=False,
                       skip_service_status=False, use_chromite_master=False):
    """Run arbitrary update commands specified in global.ini.

    @param cmds_skip: cmds no need to run since the corresponding repo/file
                      does not change.
    @param dryrun: Don't really restart the service, just print out the command.
    @param skip_service_status: Set to True to skip service status check.
                                Default is False.
    @param use_chromite_master: True if updating chromite to master, rather
                                than prod.

    @raises subprocess.CalledProcessError on a command failure.
    @raises UnstableServices if any services are unstable after restart.
    """
    defined_cmds = set(discover_update_commands())
    cmds = defined_cmds - cmds_skip
    if cmds:
        print('Running update commands:', ', '.join(cmds))
        for cmd in cmds:
            update_command(cmd, dryrun=dryrun,
                           use_chromite_master=use_chromite_master)

    services = list(get_restart_services())
    if services:
        print('Restarting Services:', ', '.join(services))
        restart_services(services, dryrun=dryrun,
                         skip_service_status=skip_service_status)


def report_changes(versions_before, versions_after):
    """Produce a report describing what changed in all repos.

    @param versions_before: Results of repo_versions() from before the update.
    @param versions_after: Results of repo_versions() from after the update.

    @returns string containing a human friendly changes report.
    """
    result = []

    if versions_after:
        for project in sorted(set(versions_before.keys() + versions_after.keys())):
            result.append('%s:' % project)

            _, before_hash = versions_before.get(project, (None, None))
            after_dir, after_hash = versions_after.get(project, (None, None))

            if project not in versions_before:
                result.append('Added.')

            elif project not in versions_after:
                result.append('Removed.')

            elif before_hash == after_hash:
                result.append('No Change.')

            else:
                hashes = '%s..%s' % (before_hash, after_hash)
                cmd = ['git', 'log', hashes, '--oneline']
                out = subprocess.check_output(cmd, cwd=after_dir,
                                              stderr=subprocess.STDOUT)
                result.append(out.strip())

            result.append('')
    else:
        for project in sorted(versions_before.keys()):
            _, before_hash = versions_before[project]
            result.append('%s: %s' % (project, before_hash))
        result.append('')

    return '\n'.join(result)


def parse_arguments(args):
    """Parse command line arguments.

    @param args: The command line arguments to parse. (ususally sys.argsv[1:])

    @returns An argparse.Namespace populated with argument values.
    """
    parser = argparse.ArgumentParser(
            description='Command to update an autotest server.')
    parser.add_argument('--skip-verify', action='store_false',
                        dest='verify', default=True,
                        help='Disable verification of a clean repository.')
    parser.add_argument('--skip-update', action='store_false',
                        dest='update', default=True,
                        help='Skip the repository source code update.')
    parser.add_argument('--skip-actions', action='store_false',
                        dest='actions', default=True,
                        help='Skip the post update actions.')
    parser.add_argument('--skip-report', action='store_false',
                        dest='report', default=True,
                        help='Skip the git version report.')
    parser.add_argument('--actions-only', action='store_true',
                        help='Run the post update actions (restart services).')
    parser.add_argument('--dryrun', action='store_true',
                        help='Don\'t actually run any commands, just log.')
    parser.add_argument('--skip-service-status', action='store_true',
                        help='Skip checking the service status.')
    parser.add_argument('--update_push_servers', action='store_true',
                        help='Indicate to update test_push server. If not '
                             'specify, then update server to production.')
    parser.add_argument('--force-clean-externals', action='store_true',
                        default=False,
                        help='Force a cleanup of all untracked files within '
                             'site-packages/ and ExternalSource/, so that '
                             'build_externals will build from scratch.')
    parser.add_argument('--force_update', action='store_true',
                        help='Force to run the update commands for afe, tko '
                             'and build_externals')

    results = parser.parse_args(args)

    if results.actions_only:
        results.verify = False
        results.update = False
        results.report = False

    # TODO(dgarrett): Make these behaviors support dryrun.
    if results.dryrun:
        results.verify = False
        results.update = False
        results.force_clean_externals = False

    if not results.update_push_servers:
      print('Will skip service check for pushing servers in prod.')
      results.skip_service_status = True
    return results


class ChangeDir(object):

    """Context manager for changing to a directory temporarily."""

    def __init__(self, dir):
        self.new_dir = dir
        self.old_dir = None

    def __enter__(self):
        self.old_dir = os.getcwd()
        os.chdir(self.new_dir)

    def __exit__(self, exc_type, exc_val, exc_tb):
        os.chdir(self.old_dir)


def _sync_chromiumos_repo():
    """Update ~chromeos-test/chromiumos repo."""
    print('Updating ~chromeos-test/chromiumos')
    with ChangeDir(os.path.expanduser('~chromeos-test/chromiumos')):
        ret = subprocess.call(['repo', 'sync'], stderr=subprocess.STDOUT)
        _clean_pyc_files()
    if ret != 0:
        print('Update failed, exited with status: %d' % ret)


def main(args):
    """Main method."""
    os.chdir(common.autotest_dir)
    global_config.global_config.parse_config_file()

    behaviors = parse_arguments(args)

    if behaviors.verify:
        print('Checking tree status:')
        verify_repo_clean()
        print('Tree status: clean')

    if behaviors.force_clean_externals:
       print('Cleaning all external packages and their cache...')
       _clean_externals()
       print('...done.')

    versions_before = repo_versions()
    versions_after = set()
    cmd_versions_before = repo_versions_to_decide_whether_run_cmd_update()
    cmd_versions_after = set()

    if behaviors.update:
        print('Updating Repo.')
        repo_sync(behaviors.update_push_servers)
        versions_after = repo_versions()
        cmd_versions_after = repo_versions_to_decide_whether_run_cmd_update()
        _sync_chromiumos_repo()

    if behaviors.actions:
        # If the corresponding repo/file not change, no need to run the cmd.
        cmds_skip = (set() if behaviors.force_update else
                     {t[0] for t in cmd_versions_before & cmd_versions_after})
        run_deploy_actions(
                cmds_skip, behaviors.dryrun, behaviors.skip_service_status,
                use_chromite_master=behaviors.update_push_servers)

    if behaviors.report:
        print('Changes:')
        print(report_changes(versions_before, versions_after))


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
