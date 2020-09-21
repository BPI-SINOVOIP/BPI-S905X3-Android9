#!/usr/bin/env python
#
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

"""Unittests for cli_translator."""

import unittest
import os
import re
import mock

import atest_error
import cli_translator as cli_t
import constants
import test_finder_handler
import test_mapping
import unittest_constants as uc
import unittest_utils
from test_finders import test_finder_base

# TEST_MAPPING related consts
TEST_MAPPING_DIR = os.path.join(uc.TEST_DATA_DIR, 'test_mapping', 'folder1')

SEARCH_DIR_RE = re.compile(r'^find ([^ ]*).*$')


#pylint: disable=unused-argument
def gettestinfos_side_effect(test_names, test_mapping_test_details=None):
    """Mock return values for _get_test_info."""
    test_infos = set()
    for test_name in test_names:
        if test_name == uc.MODULE_NAME:
            test_infos.add(uc.MODULE_INFO)
        if test_name == uc.CLASS_NAME:
            test_infos.add(uc.CLASS_INFO)
    return test_infos


#pylint: disable=protected-access
#pylint: disable=no-self-use
class CLITranslatorUnittests(unittest.TestCase):
    """Unit tests for cli_t.py"""

    def setUp(self):
        """Run before execution of every test"""
        self.ctr = cli_t.CLITranslator()

    @mock.patch.object(test_finder_handler, 'get_find_methods_for_test')
    def test_get_test_infos(self, mock_getfindmethods):
        """Test _get_test_infos method."""
        ctr = cli_t.CLITranslator()
        find_method_return_module_info = lambda x, y: uc.MODULE_INFO
        # pylint: disable=invalid-name
        find_method_return_module_class_info = (lambda x, test: uc.MODULE_INFO
                                                if test == uc.MODULE_NAME
                                                else uc.CLASS_INFO)
        find_method_return_nothing = lambda x, y: None
        one_test = [uc.MODULE_NAME]
        mult_test = [uc.MODULE_NAME, uc.CLASS_NAME]

        # Let's make sure we return what we expect.
        expected_test_infos = {uc.MODULE_INFO}
        mock_getfindmethods.return_value = [
            test_finder_base.Finder(None, find_method_return_module_info)]
        unittest_utils.assert_strict_equal(
            self, ctr._get_test_infos(one_test), expected_test_infos)

        # Check we receive multiple test infos.
        expected_test_infos = {uc.MODULE_INFO, uc.CLASS_INFO}
        mock_getfindmethods.return_value = [
            test_finder_base.Finder(None, find_method_return_module_class_info)]
        unittest_utils.assert_strict_equal(
            self, ctr._get_test_infos(mult_test), expected_test_infos)

        # Let's make sure we raise an error when we have no tests found.
        mock_getfindmethods.return_value = [
            test_finder_base.Finder(None, find_method_return_nothing)]
        self.assertRaises(atest_error.NoTestFoundError, ctr._get_test_infos,
                          one_test)

        # Check the method works for test mapping.
        test_detail1 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST)
        test_detail2 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST_WITH_OPTION)
        expected_test_infos = {uc.MODULE_INFO, uc.CLASS_INFO}
        mock_getfindmethods.return_value = [
            test_finder_base.Finder(None, find_method_return_module_class_info)]
        test_infos = ctr._get_test_infos(
            mult_test, [test_detail1, test_detail2])
        unittest_utils.assert_strict_equal(
            self, test_infos, expected_test_infos)
        for test_info in test_infos:
            if test_info == uc.MODULE_INFO:
                self.assertEqual(
                    test_detail1.options,
                    test_info.data[constants.TI_MODULE_ARG])
            else:
                self.assertEqual(
                    test_detail2.options,
                    test_info.data[constants.TI_MODULE_ARG])

    @mock.patch.object(cli_t.CLITranslator, '_find_tests_by_test_mapping')
    @mock.patch.object(cli_t.CLITranslator, '_get_test_infos',
                       side_effect=gettestinfos_side_effect)
    #pylint: disable=unused-argument
    def test_translate(self, _info, mock_testmapping):
        """Test translate method."""
        # Check that we can find a class.
        targets, test_infos = self.ctr.translate([uc.CLASS_NAME])
        unittest_utils.assert_strict_equal(
            self, targets, uc.CLASS_BUILD_TARGETS)
        unittest_utils.assert_strict_equal(self, test_infos, {uc.CLASS_INFO})

        # Check that we get all the build targets we expect.
        targets, test_infos = self.ctr.translate([uc.MODULE_NAME,
                                                  uc.CLASS_NAME])
        unittest_utils.assert_strict_equal(
            self, targets, uc.MODULE_CLASS_COMBINED_BUILD_TARGETS)
        unittest_utils.assert_strict_equal(self, test_infos, {uc.MODULE_INFO,
                                                              uc.CLASS_INFO})

        # Check that test mappings feeds into get_test_info properly.
        test_detail1 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST)
        test_detail2 = test_mapping.TestDetail(uc.TEST_MAPPING_TEST_WITH_OPTION)
        mock_testmapping.return_value = ([test_detail1, test_detail2], None)
        targets, test_infos = self.ctr.translate([])
        unittest_utils.assert_strict_equal(
            self, targets, uc.MODULE_CLASS_COMBINED_BUILD_TARGETS)
        unittest_utils.assert_strict_equal(self, test_infos, {uc.MODULE_INFO,
                                                              uc.CLASS_INFO})

    def test_find_tests_by_test_mapping(self):
        """Test _find_tests_by_test_mapping method."""
        tests, all_tests = self.ctr._find_tests_by_test_mapping(
            path=TEST_MAPPING_DIR, file_name='test_mapping_sample')
        expected = set(['test2', 'test1'])
        expected_all_tests = {'presubmit': expected,
                              'postsubmit': set(['test3'])}
        self.assertEqual(expected, tests)
        self.assertEqual(expected_all_tests, all_tests)

        tests, all_tests = self.ctr._find_tests_by_test_mapping(
            path=TEST_MAPPING_DIR, test_group=constants.TEST_GROUP_POSTSUBMIT,
            file_name='test_mapping_sample')
        expected = set(['test1', 'test2', 'test3'])
        self.assertEqual(expected, tests)
        self.assertEqual(expected_all_tests, all_tests)

if __name__ == '__main__':
    unittest.main()
