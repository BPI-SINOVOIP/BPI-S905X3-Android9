# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os

from autotest_lib.client.common_lib import error
from autotest_lib.server import test
from autotest_lib.server import utils


class tast_Runner(test.test):
    """Autotest server test that runs a Tast test suite.

    Tast is an integration-testing framework analagous to the test-running
    portion of Autotest. See
    https://chromium.googlesource.com/chromiumos/platform/tast/ for more
    information.

    This class runs the "tast" command locally to execute a Tast test suite on a
    remote DUT.
    """
    version = 1

    # Maximum time to wait for the tast command to complete, in seconds.
    _EXEC_TIMEOUT_SEC = 600

    # JSON file written by the tast command containing test results.
    _RESULTS_FILENAME = 'results.json'

    # Maximum number of failing tests to include in error message.
    _MAX_TEST_NAMES_IN_ERROR = 3

    # Default paths where Tast files are installed.
    _TAST_PATH = '/usr/bin/tast'
    _REMOTE_BUNDLE_DIR = '/usr/libexec/tast/bundles/remote'
    _REMOTE_DATA_DIR = '/usr/share/tast/data/remote'
    _REMOTE_TEST_RUNNER_PATH = '/usr/bin/remote_test_runner'

    # When Tast is deployed from CIPD packages in the lab, it's installed under
    # this prefix rather than under the root directory.
    _CIPD_INSTALL_ROOT = '/opt/infra-tools'

    def initialize(self, host, test_exprs):
        """
        @param host: remote.RemoteHost instance representing DUT.
        @param test_exprs: Array of strings describing tests to run.

        @raises error.TestFail if the Tast installation couldn't be found.
        """
        self._host = host
        self._test_exprs = test_exprs

        # The data dir can be missing if no remote tests registered data files,
        # but all other files must exist.
        self._tast_path = self._get_path(self._TAST_PATH)
        self._remote_bundle_dir = self._get_path(self._REMOTE_BUNDLE_DIR)
        self._remote_data_dir = self._get_path(self._REMOTE_DATA_DIR,
                                               allow_missing=True)
        self._remote_test_runner_path = self._get_path(
                self._REMOTE_TEST_RUNNER_PATH)

    def run_once(self):
        """Runs the test suite once."""
        self._log_version()
        self._run_tast()
        self._parse_results()

    def _get_path(self, path, allow_missing=False):
        """Returns the path to an installed Tast-related file or directory.

        @param path Absolute path in root filesystem, e.g. "/usr/bin/tast".
        @param allow_missing True if it's okay for the path to be missing.

        @return: Absolute path within install root, e.g.
            "/opt/infra-tools/usr/bin/tast", or an empty string if the path
            wasn't found and allow_missing is True.

        @raises error.TestFail if the path couldn't be found and allow_missing
            is False.
        """
        if os.path.exists(path):
            return path

        cipd_path = os.path.join(self._CIPD_INSTALL_ROOT,
                                 os.path.relpath(path, '/'))
        if os.path.exists(cipd_path):
            return cipd_path

        if allow_missing:
            return ''
        raise error.TestFail('Neither %s nor %s exists' % (path, cipd_path))

    def _log_version(self):
        """Runs the tast command locally to log its version."""
        try:
            utils.run([self._tast_path, '-version'],
                      timeout=self._EXEC_TIMEOUT_SEC,
                      stdout_tee=utils.TEE_TO_LOGS,
                      stderr_tee=utils.TEE_TO_LOGS,
                      stderr_is_expected=True,
                      stdout_level=logging.INFO,
                      stderr_level=logging.ERROR)
        except error.CmdError as e:
            logging.error('Failed to log tast version: %s' % str(e))

    def _run_tast(self):
        """Runs the tast command locally to perform testing against the DUT.

        @raises error.TestFail if the tast command fails or times out (but not
            if individual tests fail).
        """
        cmd = [
            self._tast_path,
            '-verbose',
            '-logtime=false',
            'run',
            '-build=false',
            '-resultsdir=' + self.resultsdir,
            '-remotebundledir=' + self._remote_bundle_dir,
            '-remotedatadir=' + self._remote_data_dir,
            '-remoterunner=' + self._remote_test_runner_path,
            self._host.hostname,
        ]
        cmd.extend(self._test_exprs)

        logging.info('Running ' +
                     ' '.join([utils.sh_quote_word(a) for a in cmd]))
        try:
            utils.run(cmd,
                      ignore_status=False,
                      timeout=self._EXEC_TIMEOUT_SEC,
                      stdout_tee=utils.TEE_TO_LOGS,
                      stderr_tee=utils.TEE_TO_LOGS,
                      stderr_is_expected=True,
                      stdout_level=logging.INFO,
                      stderr_level=logging.ERROR)
        except error.CmdError as e:
            raise error.TestFail('Failed to run tast: %s' % str(e))
        except error.CmdTimeoutError as e:
            raise error.TestFail('Got timeout while running tast: %s' % str(e))

    def _parse_results(self):
        """Parses results written by the tast command.

        @raises error.TestFail if one or more tests failed.
        """
        path = os.path.join(self.resultsdir, self._RESULTS_FILENAME)
        failed = []
        with open(path, 'r') as f:
            for test in json.load(f):
                if test['errors']:
                    name = test['name']
                    for err in test['errors']:
                        logging.warning('%s: %s', name, err['reason'])
                    # TODO(derat): Report failures in flaky tests in some other
                    # way.
                    if 'flaky' not in test.get('attr', []):
                        failed.append(name)

        if failed:
            msg = '%d failed: ' % len(failed)
            msg += ' '.join(sorted(failed)[:self._MAX_TEST_NAMES_IN_ERROR])
            if len(failed) > self._MAX_TEST_NAMES_IN_ERROR:
                msg += ' ...'
            raise error.TestFail(msg)
