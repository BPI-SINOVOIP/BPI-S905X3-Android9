# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# repohooks/pre-upload.py currently does not run pylint. But for developers who
# want to check their code manually we disable several harmless pylint warnings
# which just distract from more serious remaining issues.
#
# The instance variable _android_gts is not defined in __init__().
# pylint: disable=attribute-defined-outside-init
#
# Many short variable names don't follow the naming convention.
# pylint: disable=invalid-name

import logging
import os

from autotest_lib.server import utils
from autotest_lib.server.cros import tradefed_test

_PARTNER_GTS_LOCATION = 'gs://chromeos-partner-gts/gts-5.1_r3-4604229.zip'


class cheets_GTS(tradefed_test.TradefedTest):
    """Sets up tradefed to run GTS tests."""
    version = 1

    def _get_default_bundle_url(self, bundle):
        return _PARTNER_GTS_LOCATION


    def _get_tradefed_base_dir(self):
        return 'android-gts'


    def _tradefed_run_command(self, target_module=None, plan=None,
                              session_id=None):
        """Builds the GTS command line.

        @param target_module: the module to be run.
        @param plan: the plan to be run.
        @param session_id: tradfed session id to continue.
        """
        args = ['run', 'commandAndExit', 'gts']
        if target_module is not None:
            args += ['--module', target_module]
        elif plan is not None and session_id is not None:
            args += ['--plan', plan, '--retry', '%d' % session_id]
        return args


    def _run_tradefed(self, commands):
        """Kick off GTS.

        @param commands: the command(s) to pass to GTS.
        @return: The result object from utils.run.
        """
        gts_tradefed = os.path.join(self._repository, 'tools', 'gts-tradefed')
        with tradefed_test.adb_keepalive(self._get_adb_target(),
                                         self._install_paths):
            for command in commands:
                logging.info('RUN: ./gts-tradefed %s', ' '.join(command))
                output = self._run(gts_tradefed,
                                   args=command,
                                   verbose=True,
                                   # Tee tradefed stdout/stderr to logs
                                   # continuously during the test run.
                                   stdout_tee=utils.TEE_TO_LOGS,
                                   stderr_tee=utils.TEE_TO_LOGS)
                logging.info('END: ./gts-tradefed %s\n', ' '.join(command))
        return output


    def run_once(self, target_package=None, tradefed_args=None):
        """Runs GTS with either a target module or a custom command line.

        @param target_package: the name of test module to be run.
        @param tradefed_args: used to pass any specific cmd to GTS binary.
        """
        if tradefed_args:
            test_command = tradefed_args
            test_name = ' '.join(tradefed_args)
        else:
            test_command = self._tradefed_run_command(target_package)
            test_name = 'module.%s' % target_package
        self._run_tradefed_with_retries(target_package, test_command, test_name)
