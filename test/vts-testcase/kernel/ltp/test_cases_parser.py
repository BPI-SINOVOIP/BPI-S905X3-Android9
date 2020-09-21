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

import os
import logging
import itertools

from vts.runners.host import const
from vts.testcases.kernel.ltp import ltp_configs
from vts.testcases.kernel.ltp import ltp_enums
from vts.testcases.kernel.ltp import test_case
from vts.testcases.kernel.ltp.configs import stable_tests
from vts.testcases.kernel.ltp.configs import disabled_tests
from vts.utils.python.common import filter_utils


class TestCasesParser(object):
    """Load a ltp vts testcase definition file and parse it into a generator.

    Attributes:
        _data_path: string, the vts data path on host side
        _filter_func: function, a filter method that will emit exception if a test is filtered
        _ltp_tests_filter: list of string, filter for tests that are stable and disabled
    """

    def __init__(self, data_path, filter_func):
        self._data_path = data_path
        self._filter_func = filter_func
        self._ltp_tests_filter = filter_utils.Filter(
            stable_tests.STABLE_TESTS,
            disabled_tests.DISABLED_TESTS,
            enable_regex=True)
        self._ltp_tests_filter.ExpandBitness()

    def ValidateDefinition(self, line):
        """Validate a tab delimited test case definition.

        Will check whether the given line of definition has three parts
        separated by tabs.
        It will also trim leading and ending white spaces for each part
        in returned tuple (if valid).

        Returns:
            A tuple in format (test suite, test name, test command) if
            definition is valid. None otherwise.
        """
        items = [
            item.strip()
            for item in line.split(ltp_enums.Delimiters.TESTCASE_DEFINITION)
        ]
        if not len(items) == 3 or not items:
            return None
        else:
            return items

    def Load(self,
             ltp_dir,
             n_bit,
             test_filter,
             run_staging=False,
             is_low_mem=False):
        """Read the definition file and yields a TestCase generator.

        Args:
            ltp_dir: string, directory that contains ltp binaries and scripts
            n_bit: int, bitness
            test_filter: Filter object, test name filter from base_test
            run_staging: bool, whether to use staging configuration
            is_low_mem: bool, whether to use low memory device configuration
        """
        scenario_groups = (ltp_configs.TEST_SUITES_LOW_MEM
                           if is_low_mem else ltp_configs.TEST_SUITES)
        logging.info('LTP scenario groups: %s', scenario_groups)

        run_scritp = self.GenerateLtpRunScript(scenario_groups)

        for line in run_scritp:
            items = self.ValidateDefinition(line)
            if not items:
                continue

            testsuite, testname, command = items
            if is_low_mem and testsuite.endswith(
                    ltp_configs.LOW_MEMORY_SCENARIO_GROUP_SUFFIX):
                testsuite = testsuite[:-len(
                    ltp_configs.LOW_MEMORY_SCENARIO_GROUP_SUFFIX)]

            # Tests failed to build will have prefix "DISABLED_"
            if testname.startswith("DISABLED_"):
                logging.info("[Parser] Skipping test case {}-{}. Reason: "
                             "not built".format(testsuite, testname))
                continue

            # Some test cases contain semicolons in their commands,
            # and we replace them with &&
            command = command.replace(';', '&&')

            # Some test cases have hardcoded "/tmp" in the command
            # we replace that with ltp_configs.TMPDIR
            command = command.replace('/tmp', ltp_configs.TMPDIR)

            testcase = test_case.TestCase(
                testsuite=testsuite, testname=testname, command=command)

            test_display_name = "{}_{}bit".format(str(testcase), n_bit)

            # Check runner's base_test filtering method
            try:
                self._filter_func(test_display_name)
            except:
                logging.info("[Parser] Skipping test case %s. Reason: "
                             "filtered" % testcase.fullname)
                testcase.is_filtered = True
                testcase.note = "filtered"

            # For skipping tests that are not designed or ready for Android,
            # check for bit specific test in disabled list as well as non-bit specific
            if ((self._ltp_tests_filter.IsInExcludeFilter(str(testcase)) or
                 self._ltp_tests_filter.IsInExcludeFilter(test_display_name)) and
                    not test_filter.IsInIncludeFilter(test_display_name)):
                logging.info("[Parser] Skipping test case %s. Reason: "
                             "disabled" % testcase.fullname)
                continue

            # For separating staging tests from stable tests
            if not self._ltp_tests_filter.IsInIncludeFilter(test_display_name):
                if not run_staging and not test_filter.IsInIncludeFilter(
                        test_display_name):
                    # Skip staging tests in stable run
                    continue
                else:
                    testcase.is_staging = True
                    testcase.note = "staging"
            else:
                if run_staging:
                    # Skip stable tests in staging run
                    continue

            logging.info("[Parser] Adding test case %s." % testcase.fullname)
            yield testcase

    def ReadCommentedTxt(self, filepath):
        '''Read a lines of a file that are not commented by #.

        Args:
            filepath: string, path of file to read

        Returns:
            A set of string representing non-commented lines in given file
        '''
        if not filepath:
            logging.error('Invalid file path')
            return None

        with open(filepath, 'r') as f:
            lines_gen = (line.strip() for line in f)
            return set(
                line for line in lines_gen
                if line and not line.startswith('#'))

    def GenerateLtpTestCases(self, testsuite, disabled_tests_list):
        '''Generate test cases for each ltp test suite.

        Args:
            testsuite: string, test suite name

        Returns:
            A list of string
        '''
        testsuite_script = os.path.join(self._data_path,
                                        ltp_configs.LTP_RUNTEST_DIR, testsuite)

        result = []
        for line in open(testsuite_script, 'r'):
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            testname = line.split()[0]
            testname_prefix = ('DISABLED_'
                               if testname in disabled_tests_list else '')
            testname_modified = testname_prefix + testname

            result.append("\t".join(
                [testsuite, testname_modified, line[len(testname):].strip()]))
        return result

    def GenerateLtpRunScript(self, scenario_groups):
        '''Given a scenario group generate test case script.

        Args:
            scenario_groups: list of string, name of test scenario groups to use

        Returns:
            A list of string
        '''
        disabled_tests_path = os.path.join(
            self._data_path, ltp_configs.LTP_DISABLED_BUILD_TESTS_CONFIG_PATH)
        disabled_tests_list = self.ReadCommentedTxt(disabled_tests_path)

        result = []
        for testsuite in scenario_groups:
            result.extend(
                self.GenerateLtpTestCases(testsuite, disabled_tests_list))

        #TODO(yuexima): remove duplicate

        return result
