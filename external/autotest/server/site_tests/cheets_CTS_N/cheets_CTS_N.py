# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# repohooks/pre-upload.py currently does not run pylint. But for developers who
# want to check their code manually we disable several harmless pylint warnings
# which just distract from more serious remaining issues.
#
# The instance variable _android_cts is not defined in __init__().
# pylint: disable=attribute-defined-outside-init
#
# Many short variable names don't follow the naming convention.
# pylint: disable=invalid-name

import logging
import os

from autotest_lib.server import utils
from autotest_lib.server.cros import tradefed_test

# Maximum default time allowed for each individual CTS module.
_CTS_TIMEOUT_SECONDS = 3600

# Public download locations for android cts bundles.
_DL_CTS = 'https://dl.google.com/dl/android/cts/'
_CTS_URI = {
    'arm': _DL_CTS + 'android-cts-7.1_r14-linux_x86-arm.zip',
    'x86': _DL_CTS + 'android-cts-7.1_r14-linux_x86-x86.zip',
    'media': _DL_CTS + 'android-cts-media-1.4.zip',
}


class cheets_CTS_N(tradefed_test.TradefedTest):
    """Sets up tradefed to run CTS tests."""
    version = 1

    # TODO(bmgordon): Remove kahlee once the bulk of failing tests are fixed.
    _BOARD_RETRY = {'betty': 0, 'kahlee': 0}
    _CHANNEL_RETRY = {'dev': 5, 'beta': 5, 'stable': 5}

    def _get_default_bundle_url(self, bundle):
        return _CTS_URI[bundle]

    def _get_tradefed_base_dir(self):
        return 'android-cts'

    def _tradefed_run_command(self,
                              module=None,
                              plan=None,
                              session_id=None,
                              test_class=None,
                              test_method=None):
        """Builds the CTS tradefed 'run' command line.

        There are five different usages:

        1. Test a module: assign the module name via |module|.
        2. Test a plan: assign the plan name via |plan|.
        3. Continue a session: assign the session ID via |session_id|.
        4. Run all test cases of a class: assign the class name via
           |test_class|.
        5. Run a specific test case: assign the class name and method name in
           |test_class| and |test_method|.

        @param module: the name of test module to be run.
        @param plan: name of the plan to be run.
        @param session_id: tradefed session id to continue.
        @param test_class: the name of the class of which all test cases will
                           be run.
        @param test_name: the name of the method of |test_class| to be run.
                          Must be used with |test_class|.
        @return: list of command tokens for the 'run' command.
        """
        if module is not None:
            # Run a particular module (used to be called package in M).
            cmd = ['run', 'commandAndExit', 'cts', '--module', module]
            if test_class is not None:
                if test_method is not None:
                    cmd += ['-t', test_class + '#' + test_method]
                else:
                    cmd += ['-t', test_class]
        elif plan is not None and session_id is not None:
            # In 7.1 R2 we can only retry session_id with the original plan.
            cmd = ['run', 'commandAndExit', 'cts', '--plan', plan,
                   '--retry', '%d' % session_id]
        elif plan is not None:
            # Subplan for any customized CTS test plan in form of xml.
            cmd = ['run', 'commandAndExit', 'cts', '--subplan', plan]
        else:
            logging.warning('Running all tests. This can take several days.')
            cmd = ['run', 'commandAndExit', 'cts']
        # We handle media download ourselves in the lab, as lazy as possible.
        cmd.append('--precondition-arg')
        cmd.append('skip-media-download')
        # If we are running outside of the lab we can collect more data.
        if not utils.is_in_container():
            logging.info('Running outside of lab, adding extra debug options.')
            cmd.append('--log-level-display=DEBUG')
            cmd.append('--screenshot-on-failure')
            # TODO(ihf): Add log collection once b/28333587 fixed.
            #cmd.append('--collect-deqp-logs')
        # TODO(ihf): Add tradefed_test.adb_keepalive() and remove
        # --disable-reboot. This might be more efficient.
        # At early stage, cts-tradefed tries to reboot the device by
        # "adb reboot" command. In a real Android device case, when the
        # rebooting is completed, adb connection is re-established
        # automatically, and cts-tradefed expects that behavior.
        # However, in ARC, it doesn't work, so the whole test process
        # is just stuck. Here, disable the feature.
        cmd.append('--disable-reboot')
        # Create a logcat file for each individual failure.
        cmd.append('--logcat-on-failure')
        return cmd

    def _get_timeout_factor(self):
        """Returns the factor to be multiplied to the timeout parameter.
        The factor is determined by the number of ABIs to run."""
        if self._timeoutfactor is None:
            abilist = self._run('adb', args=('shell', 'getprop',
                'ro.product.cpu.abilist')).stdout.split(',')
            prefix = {'x86': 'x86', 'arm': 'armeabi-'}.get(self._abi)
            self._timeoutfactor = (1 if prefix is None else
                sum(1 for abi in abilist if abi.startswith(prefix)))
        return self._timeoutfactor

    def _run_tradefed(self, commands):
        """Kick off CTS.

        @param commands: the command(s) to pass to CTS.
        @param datetime_id: For 'continue' datetime of previous run is known.
        @return: The result object from utils.run.
        """
        cts_tradefed = os.path.join(self._repository, 'tools', 'cts-tradefed')
        with tradefed_test.adb_keepalive(self._get_adb_target(),
                                         self._install_paths):
            for command in commands:
                logging.info('RUN: ./cts-tradefed %s', ' '.join(command))
                output = self._run(
                    cts_tradefed,
                    args=tuple(command),
                    timeout=self._timeout * self._get_timeout_factor(),
                    verbose=True,
                    ignore_status=False,
                    # Make sure to tee tradefed stdout/stderr to autotest logs
                    # continuously during the test run.
                    stdout_tee=utils.TEE_TO_LOGS,
                    stderr_tee=utils.TEE_TO_LOGS)
            logging.info('END: ./cts-tradefed %s\n', ' '.join(command))
        return output

    def _should_skip_test(self):
        """Some tests are expected to fail and are skipped."""
        # newbie and novato are x86 VMs without binary translation. Skip the ARM
        # tests.
        no_ARM_ABI_test_boards = ('newbie', 'novato', 'novato-arc64')
        if self._get_board_name(self._host) in no_ARM_ABI_test_boards:
            if self._abi == 'arm':
                return True
        return False

    def generate_test_command(self, target_module, target_plan, target_class,
                              target_method, tradefed_args, session_id=0):
        """Generates the CTS command and name to use based on test arguments.

        @param target_module: the name of test module to run.
        @param target_plan: the name of the test plan to run.
        @param target_class: the name of the class to be tested.
        @param target_method: the name of the method to be tested.
        @param tradefed_args: a list of args to pass to tradefed.
        @param session_id: tradefed session_id.
        """
        if target_module is not None:
            if target_class is not None:
                test_name = 'testcase.%s' % target_class
                if target_method is not None:
                    test_name += '.' + target_method
                test_command = self._tradefed_run_command(
                    module=target_module,
                    test_class=target_class,
                    test_method=target_method,
                    session_id=session_id)
            else:
                test_name = 'module.%s' % target_module
                test_command = self._tradefed_run_command(
                    module=target_module, session_id=session_id)
        elif target_plan is not None:
            test_name = 'plan.%s' % target_plan
            test_command = self._tradefed_run_command(plan=target_plan)
        elif tradefed_args is not None:
            test_name = 'run tradefed %s' % ' '.join(tradefed_args)
            test_command = tradefed_args
        else:
            test_command = self._tradefed_run_command()
            test_name = 'all_CTS'

        logging.info('CTS command: %s', test_command)
        return test_command, test_name

    def run_once(self,
                 target_module=None,
                 target_plan=None,
                 target_class=None,
                 target_method=None,
                 needs_push_media=False,
                 tradefed_args=None,
                 precondition_commands=[],
                 login_precondition_commands=[],
                 timeout=_CTS_TIMEOUT_SECONDS):
        """Runs the specified CTS once, but with several retries.

        There are four usages:
        1. Test the whole module named |target_module|.
        2. Test with a plan named |target_plan|.
        3. Run all the test cases of class named |target_class|.
        4. Run a specific test method named |target_method| of class
           |target_class|.
        5. Run an arbitrary tradefed command.

        @param target_module: the name of test module to run.
        @param target_plan: the name of the test plan to run.
        @param target_class: the name of the class to be tested.
        @param target_method: the name of the method to be tested.
        @param needs_push_media: need to push test media streams.
        @param timeout: time after which tradefed can be interrupted.
        @param precondition_commands: a list of scripts to be run on the
        dut before the test is run, the scripts must already be installed.
        @param login_precondition_commands: a list of scripts to be run on the
        dut before the log-in for the test is performed.
        @param tradefed_args: a list of args to pass to tradefed.
        """

        # On dev and beta channels timeouts are sharp, lenient on stable.
        self._timeout = timeout
        if self._get_release_channel(self._host) == 'stable':
            self._timeout += 3600
        # Retries depend on channel.
        self._timeoutfactor = None

        test_command, test_name = self.generate_test_command(target_module,
                                                             target_plan,
                                                             target_class,
                                                             target_method,
                                                             tradefed_args)

        self._run_tradefed_with_retries(target_module, test_command, test_name,
                                        target_plan, needs_push_media, _CTS_URI,
                                        login_precondition_commands,
                                        precondition_commands)
