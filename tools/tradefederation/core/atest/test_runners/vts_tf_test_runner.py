# Copyright 2018, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
VTS Tradefed test runner class.
"""

import copy
import logging

# pylint: disable=import-error
import atest_tf_test_runner
import atest_utils
import constants


class VtsTradefedTestRunner(atest_tf_test_runner.AtestTradefedTestRunner):
    """TradeFed Test Runner class."""
    NAME = 'VtsTradefedTestRunner'
    EXECUTABLE = 'vts-tradefed'
    _RUN_CMD = ('{exe} run commandAndExit vts-staging-default -m {test} {args}')
    _BUILD_REQ = {'vts-tradefed-standalone'}
    _DEFAULT_ARGS = ['--skip-all-system-status-check',
                     '--skip-preconditions',
                     '--primary-abi-only']

    def __init__(self, results_dir, **kwargs):
        """Init stuff for vts tradefed runner class."""
        super(VtsTradefedTestRunner, self).__init__(results_dir, **kwargs)
        self.run_cmd_dict = {'exe': self.EXECUTABLE,
                             'test': '',
                             'args': ''}

    def get_test_runner_build_reqs(self):
        """Return the build requirements.

        Returns:
            Set of build targets.
        """
        build_req = self._BUILD_REQ
        build_req |= super(VtsTradefedTestRunner,
                           self).get_test_runner_build_reqs()
        return build_req

    def run_tests(self, test_infos, extra_args):
        """Run the list of test_infos.

        Args:
            test_infos: List of TestInfo.
            extra_args: Dict of extra args to add to test run.
        """
        run_cmds = self._generate_run_commands(test_infos, extra_args)
        for run_cmd in run_cmds:
            super(VtsTradefedTestRunner, self).run(run_cmd)

    def _parse_extra_args(self, extra_args):
        """Convert the extra args into something vts-tf can understand.

        We want to transform the top-level args from atest into specific args
        that vts-tradefed supports. The only arg we take as is is EXTRA_ARG
        since that is what the user intentionally wants to pass to the test
        runner.

        Args:
            extra_args: Dict of args

        Returns:
            List of args to append.
        """
        args_to_append = []
        args_not_supported = []
        for arg in extra_args:
            if constants.SERIAL == arg:
                args_to_append.append('--serial')
                args_to_append.append(extra_args[arg])
                continue
            if constants.CUSTOM_ARGS == arg:
                args_to_append.extend(extra_args[arg])
                continue
            args_not_supported.append(arg)
        if args_not_supported:
            logging.info('%s does not support the following args: %s',
                         self.EXECUTABLE, args_not_supported)
        return args_to_append

    # pylint: disable=arguments-differ
    def _generate_run_commands(self, test_infos, extra_args):
        """Generate a list of run commands from TestInfos.

        Args:
            test_infos: List of TestInfo tests to run.
            extra_args: Dict of extra args to add to test run.

        Returns:
            A List of strings that contains the vts-tradefed run command.
        """
        cmds = []
        args = self._DEFAULT_ARGS
        args.extend(self._parse_extra_args(extra_args))
        args.extend(atest_utils.get_result_server_args())
        for test_info in test_infos:
            cmd_dict = copy.deepcopy(self.run_cmd_dict)
            cmd_dict['test'] = test_info.test_name
            cmd_dict['args'] = ' '.join(args)
            cmds.append(self._RUN_CMD.format(**cmd_dict))
        return cmds
