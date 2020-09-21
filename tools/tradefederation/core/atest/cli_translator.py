# Copyright 2017, The Android Open Source Project
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

#pylint: disable=too-many-lines
"""
Command Line Translator for atest.
"""

import json
import logging
import os
import sys
import time

import atest_error
import constants
import test_finder_handler
import test_mapping

TEST_MAPPING = 'TEST_MAPPING'


#pylint: disable=no-self-use
class CLITranslator(object):
    """
    CLITranslator class contains public method translate() and some private
    helper methods. The atest tool can call the translate() method with a list
    of strings, each string referencing a test to run. Translate() will
    "translate" this list of test strings into a list of build targets and a
    list of TradeFederation run commands.

    Translation steps for a test string reference:
        1. Narrow down the type of reference the test string could be, i.e.
           whether it could be referencing a Module, Class, Package, etc.
        2. Try to find the test files assuming the test string is one of these
           types of reference.
        3. If test files found, generate Build Targets and the Run Command.
    """

    def __init__(self, module_info=None):
        """CLITranslator constructor

        Args:
            module_info: ModuleInfo class that has cached module-info.json.
        """
        self.mod_info = module_info

    def _get_test_infos(self, tests, test_mapping_test_details=None):
        """Return set of TestInfos based on passed in tests.

        Args:
            tests: List of strings representing test references.
            test_mapping_test_details: List of TestDetail for tests configured
                in TEST_MAPPING files.

        Returns:
            Set of TestInfos based on the passed in tests.
        """
        test_infos = set()
        if not test_mapping_test_details:
            test_mapping_test_details = [None] * len(tests)
        for test, tm_test_detail in zip(tests, test_mapping_test_details):
            test_found = False
            for finder in test_finder_handler.get_find_methods_for_test(
                    self.mod_info, test):
                # For tests in TEST_MAPPING, find method is only related to
                # test name, so the details can be set after test_info object
                # is created.
                test_info = finder.find_method(finder.test_finder_instance,
                                               test)
                if test_info:
                    if tm_test_detail:
                        test_info.data[constants.TI_MODULE_ARG] = (
                            tm_test_detail.options)
                    test_infos.add(test_info)
                    test_found = True
                    break
            if not test_found:
                raise atest_error.NoTestFoundError('No test found for: %s' %
                                                   test)
        return test_infos

    def _find_tests_by_test_mapping(
            self, path='', test_group=constants.TEST_GROUP_PRESUBMIT,
            file_name=TEST_MAPPING):
        """Find tests defined in TEST_MAPPING in the given path.

        Args:
            path: A string of path in source. Default is set to '', i.e., CWD.
            test_group: Group of tests to run. Default is set to `presubmit`.
            file_name: Name of TEST_MAPPING file. Default is set to
                    `TEST_MAPPING`. The argument is added for testing purpose.

        Returns:
            A tuple of (tests, all_tests), where,
            tests is a set of tests (test_mapping.TestDetail) defined in
            TEST_MAPPING file of the given path, and its parent directories,
            with matching test_group.
            all_tests is a dictionary of all tests in TEST_MAPPING files,
            grouped by test group.
        """
        path = os.path.realpath(path)
        if path == constants.ANDROID_BUILD_TOP or path == os.sep:
            return None, None
        tests = set()
        all_tests = {}
        test_mapping_dict = None
        test_mapping_file = os.path.join(path, file_name)
        if os.path.exists(test_mapping_file):
            with open(test_mapping_file) as json_file:
                test_mapping_dict = json.load(json_file)
            for test_group_name, test_list in test_mapping_dict.items():
                grouped_tests = all_tests.setdefault(test_group_name, set())
                grouped_tests.update(
                    [test_mapping.TestDetail(test) for test in test_list])
            for test in test_mapping_dict.get(test_group, []):
                tests.add(test_mapping.TestDetail(test))
        parent_dir_tests, parent_dir_all_tests = (
            self._find_tests_by_test_mapping(
                os.path.dirname(path), test_group, file_name))
        if parent_dir_tests:
            tests.update(parent_dir_tests)
        if parent_dir_all_tests:
            for test_group_name, test_list in parent_dir_all_tests.items():
                grouped_tests = all_tests.setdefault(test_group_name, set())
                grouped_tests.update(test_list)
        if test_group == constants.TEST_GROUP_POSTSUBMIT:
            tests.update(all_tests.get(
                constants.TEST_GROUP_PRESUBMIT, set()))
        return tests, all_tests

    def _gather_build_targets(self, test_infos):
        targets = set()
        for test_info in test_infos:
            targets |= test_info.build_targets
        return targets

    def translate(self, tests):
        """Translate atest command line into build targets and run commands.

        Args:
            tests: A list of strings referencing the tests to run.

        Returns:
            A tuple with set of build_target strings and list of TestInfos.
        """
        # Test details from TEST_MAPPING files
        test_details_list = None
        if not tests:
            # Pull out tests from test mapping
            # TODO(dshi): Support other groups of tests in TEST_MAPPING files,
            # e.g., postsubmit.
            test_details, all_test_details = self._find_tests_by_test_mapping()
            test_details_list = list(test_details)
            if test_details_list:
                tests = [detail.name for detail in test_details_list]
            else:
                logging.warn(
                    'No tests of group %s found in TEST_MAPPING at %s or its '
                    'parent directories.\nYou might be missing atest arguments,'
                    ' try `atest --help` for more information',
                    constants.TEST_GROUP_PRESUBMIT, os.path.realpath(''))
                if all_test_details:
                    tests = ''
                    for test_group, test_list in all_test_details.items():
                        tests += '%s:\n' % test_group
                        for test_detail in sorted(test_list):
                            tests += '\t%s\n' % test_detail
                    logging.warn(
                        'All available tests in TEST_MAPPING files are:\n%s',
                        tests)
                sys.exit(constants.EXIT_CODE_TEST_NOT_FOUND)
        logging.info('Finding tests: %s', tests)
        if test_details_list:
            details = '\n'.join([str(detail) for detail in test_details_list])
            logging.info('Test details:\n%s', details)
        start = time.time()
        test_infos = self._get_test_infos(tests, test_details_list)
        end = time.time()
        logging.debug('Found tests in %ss', end - start)
        for test_info in test_infos:
            logging.debug('%s\n', test_info)
        build_targets = self._gather_build_targets(test_infos)
        return build_targets, test_infos
