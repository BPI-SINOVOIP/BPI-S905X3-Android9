# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility to deploy and run result utils on a DUT.

This module is the one imported by other Autotest code and run result
throttling. Other modules in result_tools are designed to be copied to DUT and
executed with command line. That's why other modules (except view.py and
unittests) don't import the common module.
"""

import logging
import os

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils as client_utils

try:
    from chromite.lib import metrics
except ImportError:
    metrics = client_utils.metrics_mock


CONFIG = global_config.global_config
ENABLE_RESULT_THROTTLING = CONFIG.get_config_value(
        'AUTOSERV', 'enable_result_throttling', type=bool, default=False)

_THROTTLE_OPTION_FMT = '-m %s'
_BUILD_DIR_SUMMARY_CMD = '%s/result_tools/utils.py -p %s %s'
_BUILD_DIR_SUMMARY_TIMEOUT = 120
_FIND_DIR_SUMMARY_TIMEOUT = 10
_CLEANUP_DIR_SUMMARY_CMD = '%s/result_tools/utils.py -p %s -d'
_CLEANUP_DIR_SUMMARY_TIMEOUT = 10

# Default autotest directory on host
DEFAULT_AUTOTEST_DIR = '/usr/local/autotest'

# File patterns to be excluded from deploying to the dut.
_EXCLUDES = ['*.pyc', '*unittest.py', 'common.py', '__init__.py', 'runner.py',
             'view.py']

# A set of hostnames that have result tools already deployed.
_deployed_duts = set()

def _deploy_result_tools(host):
    """Send result tools to the dut.

    @param host: Host to run the result tools.
    """
    logging.debug('Deploy result utilities to %s', host.hostname)
    with metrics.SecondsTimer(
            'chromeos/autotest/job/send_result_tools_duration',
            fields={'dut_host_name': host.hostname}) as fields:
        try:
            result_tools_dir = os.path.dirname(__file__)
            host.send_file(result_tools_dir, DEFAULT_AUTOTEST_DIR,
                           excludes = _EXCLUDES)
            fields['success'] = True
        except error.AutotestHostRunError:
            logging.debug('Failed to deploy result tools using `excludes`. Try '
                          'again without `excludes`.')
            host.send_file(result_tools_dir, DEFAULT_AUTOTEST_DIR)
            fields['success'] = False
        _deployed_duts.add(host.hostname)


def run_on_client(host, client_results_dir, cleanup_only=False):
    """Run result utils on the given host.

    @param host: Host to run the result utils.
    @param client_results_dir: Path to the results directory on the client.
    @param cleanup_only: True to delete all existing directory summary files in
            the given directory.
    @return: True: If the command runs on client without error.
             False: If the command failed with error in result throttling.
    """
    success = False
    with metrics.SecondsTimer(
            'chromeos/autotest/job/dir_summary_collection_duration',
            fields={'dut_host_name': host.hostname}) as fields:
        try:
            if host.hostname not in _deployed_duts:
                _deploy_result_tools(host)
            else:
                logging.debug('result tools are already deployed to %s.',
                              host.hostname)

            if cleanup_only:
                logging.debug('Cleaning up directory summary in %s',
                              client_results_dir)
                cmd = (_CLEANUP_DIR_SUMMARY_CMD %
                       (DEFAULT_AUTOTEST_DIR, client_results_dir))
                host.run(cmd, ignore_status=False,
                         timeout=_CLEANUP_DIR_SUMMARY_TIMEOUT)
            else:
                logging.debug('Getting directory summary for %s',
                              client_results_dir)
                throttle_option = ''
                if ENABLE_RESULT_THROTTLING:
                    try:
                        throttle_option = (_THROTTLE_OPTION_FMT %
                                           host.job.max_result_size_KB)
                    except AttributeError:
                        # In case host job is not set, skip throttling.
                        logging.warn('host object does not have job attribute, '
                                     'skipping result throttling.')
                cmd = (_BUILD_DIR_SUMMARY_CMD %
                       (DEFAULT_AUTOTEST_DIR, client_results_dir,
                        throttle_option))
                host.run(cmd, ignore_status=False,
                         timeout=_BUILD_DIR_SUMMARY_TIMEOUT)
                success = True
            fields['success'] = True
        except error.AutoservRunError:
            action = 'cleanup' if cleanup_only else 'create'
            logging.exception(
                    'Non-critical failure: Failed to %s directory summary for '
                    '%s.', action, client_results_dir)
            fields['success'] = False

    return success


def collect_last_summary(host, source_path, dest_path,
                         skip_summary_collection=False):
    """Collect the last directory summary next to the given file path.

    If the given source_path is a directory, return without collecting any
    result summary file, as the summary file should have been collected with the
    directory.

    @param host: The RemoteHost to collect logs from.
    @param source_path: The remote path to collect the directory summary file
            from. If the source_path is a file
    @param dest_path: A path to write the source_path into. The summary file
            will be saved to the same folder.
    @param skip_summary_collection: True to skip summary file collection, only
            to delete the last summary. This is used in case when result
            collection in the dut failed. Default is set to False.
    """
    if not os.path.exists(dest_path):
        logging.debug('Source path %s does not exist, no directory summary '
                      'will be collected', dest_path)
        return

    # Test if source_path is a file.
    try:
        host.run('test -f %s' % source_path, timeout=_FIND_DIR_SUMMARY_TIMEOUT)
        is_source_file = True
    except error.AutoservRunError:
        is_source_file = False
        # No need to collect summary files if the source path is a directory,
        # as the summary files should have been copied over with the directory.
        # However, the last summary should be cleaned up so it won't affect
        # later tests.
        skip_summary_collection = True

    source_dir = os.path.dirname(source_path) if is_source_file else source_path
    dest_dir = dest_path if os.path.isdir(dest_path) else dest_path

    # Get the latest directory summary file.
    try:
        summary_pattern = os.path.join(source_dir, 'dir_summary_*.json')
        summary_file = host.run(
                'ls -t %s | head -1' % summary_pattern,
                timeout=_FIND_DIR_SUMMARY_TIMEOUT).stdout.strip()
    except error.AutoservRunError:
        logging.exception(
                'Non-critical failure: Failed to locate the latest directory '
                'summary for %s', source_dir)
        return

    try:
        if not skip_summary_collection:
            host.get_file(
                    summary_file,
                    os.path.join(dest_dir, os.path.basename(summary_file)),
                    preserve_perm=False)
    finally:
        # Remove the collected summary file so it won't affect later tests.
        try:
            host.run('rm %s' % summary_file,
                     timeout=_FIND_DIR_SUMMARY_TIMEOUT).stdout.strip()
        except error.AutoservRunError:
            logging.exception(
                    'Non-critical failure: Failed to delete the latest '
                    'directory summary: %s', summary_file)
