#!/usr/bin/env python
#
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

"""Unittests for tf_integration_finder."""

import os
import unittest
import mock

# pylint: disable=import-error
import constants
import unittest_constants as uc
import unittest_utils
from test_finders import test_finder_utils
from test_finders import test_info
from test_finders import tf_integration_finder
from test_runners import atest_tf_test_runner as atf_tr


INT_NAME_CLASS = uc.INT_NAME + ':' + uc.FULL_CLASS_NAME
INT_NAME_METHOD = INT_NAME_CLASS + '#' + uc.METHOD_NAME
GTF_INT_CONFIG = os.path.join(uc.GTF_INT_DIR, uc.GTF_INT_NAME + '.xml')
INT_CLASS_INFO = test_info.TestInfo(
    uc.INT_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    set(),
    data={constants.TI_FILTER: frozenset([uc.CLASS_FILTER]),
          constants.TI_REL_CONFIG: uc.INT_CONFIG})
INT_METHOD_INFO = test_info.TestInfo(
    uc.INT_NAME,
    atf_tr.AtestTradefedTestRunner.NAME,
    set(),
    data={constants.TI_FILTER: frozenset([uc.METHOD_FILTER]),
          constants.TI_REL_CONFIG: uc.INT_CONFIG})


class TFIntegrationFinderUnittests(unittest.TestCase):
    """Unit tests for tf_integration_finder.py"""

    def setUp(self):
        """Set up for testing."""
        self.tf_finder = tf_integration_finder.TFIntegrationFinder()
        self.tf_finder.integration_dirs = [os.path.join(uc.ROOT, uc.INT_DIR),
                                           os.path.join(uc.ROOT, uc.GTF_INT_DIR)]
        self.tf_finder.root_dir = uc.ROOT

    @mock.patch.object(tf_integration_finder.TFIntegrationFinder,
                       '_get_build_targets', return_value=set())
    @mock.patch.object(test_finder_utils, 'get_fully_qualified_class_name',
                       return_value=uc.FULL_CLASS_NAME)
    @mock.patch('subprocess.check_output')
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('os.path.isfile', return_value=True)
    #pylint: disable=unused-argument
    def test_find_test_by_integration_name(self, _isfile, _path, mock_find,
                                           _fcqn, _build):
        """Test find_test_by_integration_name."""
        mock_find.return_value = os.path.join(uc.ROOT, uc.INT_DIR, uc.INT_NAME + '.xml')
        t_info = self.tf_finder.find_test_by_integration_name(uc.INT_NAME)
        unittest_utils.assert_equal_testinfos(self, t_info, uc.INT_INFO)
        t_info = self.tf_finder.find_test_by_integration_name(INT_NAME_CLASS)
        unittest_utils.assert_equal_testinfos(self, t_info, INT_CLASS_INFO)
        t_info = self.tf_finder.find_test_by_integration_name(INT_NAME_METHOD)
        unittest_utils.assert_equal_testinfos(self, t_info, INT_METHOD_INFO)
        not_fully_qual = uc.INT_NAME + ':' + 'someClass'
        t_info = self.tf_finder.find_test_by_integration_name(not_fully_qual)
        unittest_utils.assert_equal_testinfos(self, t_info, INT_CLASS_INFO)
        mock_find.return_value = os.path.join(uc.ROOT, uc.GTF_INT_DIR,
                                              uc.GTF_INT_NAME + '.xml')
        unittest_utils.assert_equal_testinfos(
            self,
            self.tf_finder.find_test_by_integration_name(uc.GTF_INT_NAME),
            uc.GTF_INT_INFO)
        mock_find.return_value = ''
        self.assertIsNone(
            self.tf_finder.find_test_by_integration_name('NotIntName'))

    @mock.patch.object(tf_integration_finder.TFIntegrationFinder,
                       '_get_build_targets', return_value=set())
    @mock.patch('os.path.realpath',
                side_effect=unittest_utils.realpath_side_effect)
    @mock.patch('os.path.isdir', return_value=True)
    @mock.patch('os.path.isfile', return_value=True)
    @mock.patch.object(test_finder_utils, 'find_parent_module_dir')
    @mock.patch('os.path.exists', return_value=True)
    def test_find_int_test_by_path(self, _exists, _find, _isfile, _isdir, _real,
                                   _build):
        """Test find_int_test_by_path."""
        path = os.path.join(uc.INT_DIR, uc.INT_NAME + '.xml')
        unittest_utils.assert_equal_testinfos(
            self, uc.INT_INFO, self.tf_finder.find_int_test_by_path(path))
        path = os.path.join(uc.GTF_INT_DIR, uc.GTF_INT_NAME + '.xml')
        unittest_utils.assert_equal_testinfos(
            self, uc.GTF_INT_INFO, self.tf_finder.find_int_test_by_path(path))

    #pylint: disable=protected-access
    @mock.patch.object(tf_integration_finder.TFIntegrationFinder,
                       '_search_integration_dirs')
    def test_load_xml_file(self, search):
        """Test _load_xml_file and _load_include_tags methods."""
        search.return_value = os.path.join(uc.TEST_DATA_DIR,
                                           'CtsUiDeviceTestCases.xml')
        xml_file = os.path.join(uc.TEST_DATA_DIR, constants.MODULE_CONFIG)
        print 'xml_file: %s' % xml_file
        xml_root = self.tf_finder._load_xml_file(xml_file)
        include_tags = xml_root.findall('.//include')
        self.assertEqual(0, len(include_tags))
        option_tags = xml_root.findall('.//option')
        included = False
        for tag in option_tags:
            if tag.attrib['value'].strip() == 'CtsUiDeviceTestCases.apk':
                included = True
        self.assertTrue(included)


if __name__ == '__main__':
    unittest.main()
