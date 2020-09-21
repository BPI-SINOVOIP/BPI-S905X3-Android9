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

import copy
import logging
import itertools
import operator
import os

from vts.runners.host import const
from vts.utils.python.common import cmd_utils
from vts.utils.python.os import path_utils

from vts.testcases.kernel.ltp.shell_environment import shell_environment
from vts.testcases.kernel.ltp import ltp_enums
from vts.testcases.kernel.ltp import ltp_configs
from vts.testcases.kernel.ltp import requirements


class EnvironmentRequirementChecker(object):
    """LTP testcase environment checker.

    This class contains a dictionary for some known environment
    requirements for a set of test cases and several environment
    check functions to be mapped with. All check functions' results
    are cached in a dictionary for multiple use.

    Attributes:
        _REQUIREMENT_DEFINITIONS: dictionary {string, obj}, a map between
            requirement name and the actual definition class object
        _result_cache: dictionary {requirement_check_method_name:
            (bool, string)}, a map between check method name and cached result
            tuples (boolean, note)
        _executable_available: dict {string, bool}, a map between executable
                                path and its existance on target
        _shell_env: ShellEnvironment object, which checks and sets
            shell environments given a shell mirror
        shell: shell mirror object, can be used to execute shell
            commands on target side through runner
        ltp_bin_host_path: string, host path of ltp binary
    """

    def __init__(self, shell):
        self.shell = shell
        self._result_cache = {}
        self._executable_available = {}
        self._shell_env = shell_environment.ShellEnvironment(self.shell)
        self._REQUIREMENT_DEFINITIONS = requirements.GetRequrementDefinitions()

    @property
    def shell(self):
        """Get the runner's shell mirror object to execute commands"""
        return self._shell

    @shell.setter
    def shell(self, shell):
        """Set the runner's shell mirror object to execute commands"""
        self._shell = shell

    def Cleanup(self):
        """Run all cleanup jobs at the end of tests"""
        self._shell_env.Cleanup()

    def GetRequirements(self, test_case):
        """Get a list of requirements for a fiven test case

        Args:
            test_case: TestCase object, the test case to query
        """
        result = copy.copy(ltp_configs.REQUIREMENT_FOR_ALL)

        result.extend(rule
                      for rule, tests in
                      ltp_configs.REQUIREMENTS_TO_TESTCASE.iteritems()
                      if test_case.fullname in tests)

        result.extend(rule
                      for rule, tests in
                      ltp_configs.REQUIREMENT_TO_TESTSUITE.iteritems()
                      if test_case.testsuite in tests)

        return list(set(result))

    def Check(self, test_case):
        """Check whether a given test case's requirement has been satisfied.
        Skip the test if not.

        If check failed, this method returns False and the reason is set
        to test_case.note.

        Args:
            test_case: TestCase object, a given test case to check

        Returns:
            True if check pass; False otherwise
        """
        if (test_case.requirement_state ==
                ltp_enums.RequirementState.UNSATISFIED or
                not self.TestBinaryExists(test_case)):
            return False

        for requirement in self.GetRequirements(test_case):
            if requirement not in self._result_cache:
                definitions = self._REQUIREMENT_DEFINITIONS[requirement]
                self._result_cache[
                    requirement] = self._shell_env.ExecuteDefinitions(
                        definitions)

            result, note = self._result_cache[requirement]
            logging.info("Result for %s's requirement %s is %s", test_case,
                         requirement, result)
            if result is False:
                test_case.requirement_state = ltp_enums.RequirementState.UNSATISFIED
                test_case.note = note
                return False

        test_case.requirement_state = ltp_enums.RequirementState.SATISFIED
        return True

    def CheckAllTestCaseExecutables(self, test_cases):
        """Run a batch job to check executable exists and set permissions.

        The result will be stored in self._executable_available for use in
        TestBinaryExists method.

        Args:
            test_case: list of TestCase objects.
        """
        executables_generators = (
            test_case.GetRequiredExecutablePaths(self.ltp_bin_host_path)
            for test_case in test_cases)
        executables = list(
            set(itertools.chain.from_iterable(executables_generators)))

        # Set all executables executable permission using chmod.
        logging.info("Setting permissions on device")
        permission_command = "chmod 775 %s" % path_utils.JoinTargetPath(
            ltp_configs.LTPBINPATH, '*')
        permission_result = self.shell.Execute(permission_command)
        if permission_result[const.EXIT_CODE][0]:
            logging.error("Permission command '%s' failed.",
                          permission_command)

        # Check existence of all executables used in test definition.
        # Some executables needed by test cases but not listed in test
        # definition will not be checked here
        logging.info("Checking binary existence on host")

        executable_exists_results = map(os.path.exists, executables)

        self._executable_available = dict(
            zip(executables, executable_exists_results))

        not_exists = [
            exe for exe, exists in self._executable_available.iteritems()
            if not exists
        ]
        if not_exists:
            logging.info("The following binaries does not exist: %s",
                         not_exists)

        logging.info("Finished checking binary existence on host.")

        # Check whether all the internal binaries in path needed exist
        bin_path_exist_commands = [
            "which %s" % bin for bin in ltp_configs.INTERNAL_BINS
        ]
        bin_path_results = map(
            operator.not_,
            self.shell.Execute(bin_path_exist_commands)[const.EXIT_CODE])

        bin_path_results = map(
            operator.not_,
            self.shell.Execute(bin_path_exist_commands)[const.EXIT_CODE])

        self._executable_available.update(
            dict(zip(ltp_configs.INTERNAL_BINS, bin_path_results)))

    def TestBinaryExists(self, test_case):
        """Check whether the given test case's binary exists.

        Args:
            test_case: TestCase, the object representing the test case

        Return:
            True if exists, False otherwise
        """
        if test_case.requirement_state == ltp_enums.RequirementState.UNSATISFIED:
            logging.warn("[Checker] Attempting to run test case that has "
                         "already been checked and requirement not satisfied."
                         "%s" % test_case)
            return False

        executables = test_case.GetRequiredExecutablePaths(
            self.ltp_bin_host_path)
        results = [
            self._executable_available[executable]
            for executable in executables
        ]

        if not all(results):
            test_case.requirement_state = ltp_enums.RequirementState.UNSATISFIED
            test_case.note = "Some executables not exist: {}".format(
                zip(executables, results))
            logging.error("[Checker] Binary existance check failed for {}. "
                          "Reason: {}".format(test_case, test_case.note))
            return False
        else:
            return True
