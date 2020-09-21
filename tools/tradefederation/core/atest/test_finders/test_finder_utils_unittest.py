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

"""Unittests for test_finder_utils."""

import os
import unittest
import mock

# pylint: disable=import-error
import atest_error
import constants
import module_info
import unittest_constants as uc
import unittest_utils
from test_finders import test_finder_utils

CLASS_DIR = 'foo/bar/jank/src/android/jank/cts/ui'
FIND_TWO = uc.ROOT + 'other/dir/test.java\n' + uc.FIND_ONE
VTS_XML = 'VtsAndroidTest.xml'
VTS_BITNESS_XML = 'VtsBitnessAndroidTest.xml'
VTS_PUSH_DIR = 'vts_push_files'
VTS_XML_TARGETS = {'VtsTestName',
                   'DATA/nativetest/vts_treble_vintf_test/vts_treble_vintf_test',
                   'DATA/nativetest64/vts_treble_vintf_test/vts_treble_vintf_test',
                   'DATA/lib/libhidl-gen-hash.so',
                   'DATA/lib64/libhidl-gen-hash.so',
                   'hal-hidl-hash/frameworks/hardware/interfaces/current.txt',
                   'hal-hidl-hash/hardware/interfaces/current.txt',
                   'hal-hidl-hash/system/hardware/interfaces/current.txt',
                   'hal-hidl-hash/system/libhidl/transport/current.txt',
                   'target_with_delim',
                   'out/dir/target',
                   'push_file1_target1',
                   'push_file1_target2',
                   'push_file2_target1',
                   'push_file2_target2'}
XML_TARGETS = {'CtsJankDeviceTestCases', 'perf-setup.sh', 'cts-tradefed',
               'GtsEmptyTestApp'}
PATH_TO_MODULE_INFO_WITH_AUTOGEN = {
    'foo/bar/jank' : [{'auto_test_config' : True}]}
PATH_TO_MODULE_INFO_WITH_SUB_AUTOGEN = {
    'foo/bar/jank/src/android' : [{'auto_test_config' : True}]}
PATH_TO_MODULE_INFO_WITH_MULTI_AUTOGEN = {
    'foo/bar/jank' : [{'auto_test_config' : True},
                      {'auto_test_config' : True}]}

#pylint: disable=protected-access
class TestFinderUtilsUnittests(unittest.TestCase):
    """Unit tests for test_finder_utils.py"""

    def test_split_methods(self):
        """Test _split_methods method."""
        # Class
        unittest_utils.assert_strict_equal(
            self,
            test_finder_utils.split_methods('Class.Name'),
            ('Class.Name', set()))
        unittest_utils.assert_strict_equal(
            self,
            test_finder_utils.split_methods('Class.Name#Method'),
            ('Class.Name', {'Method'}))
        unittest_utils.assert_strict_equal(
            self,
            test_finder_utils.split_methods('Class.Name#Method,Method2'),
            ('Class.Name', {'Method', 'Method2'}))
        unittest_utils.assert_strict_equal(
            self,
            test_finder_utils.split_methods('Class.Name#Method,Method2'),
            ('Class.Name', {'Method', 'Method2'}))
        unittest_utils.assert_strict_equal(
            self,
            test_finder_utils.split_methods('Class.Name#Method,Method2'),
            ('Class.Name', {'Method', 'Method2'}))
        self.assertRaises(
            atest_error.TooManyMethodsError, test_finder_utils.split_methods,
            'class.name#Method,class.name.2#method')
        # Path
        unittest_utils.assert_strict_equal(
            self,
            test_finder_utils.split_methods('foo/bar/class.java'),
            ('foo/bar/class.java', set()))
        unittest_utils.assert_strict_equal(
            self,
            test_finder_utils.split_methods('foo/bar/class.java#Method'),
            ('foo/bar/class.java', {'Method'}))

    @mock.patch('__builtin__.raw_input', return_value='1')
    def test_extract_test_path(self, _):
        """Test extract_test_dir method."""
        path = os.path.join(uc.ROOT, CLASS_DIR, uc.CLASS_NAME + '.java')
        unittest_utils.assert_strict_equal(
            self, test_finder_utils.extract_test_path(uc.FIND_ONE), path)
        path = os.path.join(uc.ROOT, CLASS_DIR, uc.CLASS_NAME + '.java')
        unittest_utils.assert_strict_equal(
            self, test_finder_utils.extract_test_path(FIND_TWO), path)

    @mock.patch('os.path.isdir')
    def test_is_equal_or_sub_dir(self, mock_isdir):
        """Test is_equal_or_sub_dir method."""
        self.assertTrue(test_finder_utils.is_equal_or_sub_dir('/a/b/c', '/'))
        self.assertTrue(test_finder_utils.is_equal_or_sub_dir('/a/b/c', '/a'))
        self.assertTrue(test_finder_utils.is_equal_or_sub_dir('/a/b/c',
                                                              '/a/b/c'))
        self.assertFalse(test_finder_utils.is_equal_or_sub_dir('/a/b',
                                                               '/a/b/c'))
        self.assertFalse(test_finder_utils.is_equal_or_sub_dir('/a', '/f'))
        mock_isdir.return_value = False
        self.assertFalse(test_finder_utils.is_equal_or_sub_dir('/a/b', '/a'))

    @mock.patch('os.path.isdir', return_value=True)
    @mock.patch('os.path.isfile',
                side_effect=unittest_utils.isfile_side_effect)
    def test_find_parent_module_dir(self, _isfile, _isdir):
        """Test _find_parent_module_dir method."""
        abs_class_dir = '/%s' % CLASS_DIR
        mock_module_info = mock.Mock(spec=module_info.ModuleInfo)
        mock_module_info.path_to_module_info = {}
        unittest_utils.assert_strict_equal(
            self,
            test_finder_utils.find_parent_module_dir(uc.ROOT,
                                                     abs_class_dir,
                                                     mock_module_info),
            uc.MODULE_DIR)

    @mock.patch('os.path.isdir', return_value=True)
    @mock.patch('os.path.isfile', return_value=False)
    def test_find_parent_module_dir_with_autogen_config(self, _isfile, _isdir):
        """Test _find_parent_module_dir method."""
        abs_class_dir = '/%s' % CLASS_DIR
        mock_module_info = mock.Mock(spec=module_info.ModuleInfo)
        mock_module_info.path_to_module_info = PATH_TO_MODULE_INFO_WITH_AUTOGEN
        unittest_utils.assert_strict_equal(
            self,
            test_finder_utils.find_parent_module_dir(uc.ROOT,
                                                     abs_class_dir,
                                                     mock_module_info),
            uc.MODULE_DIR)

    @mock.patch('os.path.isdir', return_value=True)
    @mock.patch('os.path.isfile', side_effect=[False] * 5 + [True])
    def test_find_parent_module_dir_with_autogen_subconfig(self, _isfile, _isdir):
        """Test _find_parent_module_dir method.

        This case is testing when the auto generated config is in a
        sub-directory of a larger test that contains a test config in a parent
        directory.
        """
        abs_class_dir = '/%s' % CLASS_DIR
        mock_module_info = mock.Mock(spec=module_info.ModuleInfo)
        mock_module_info.path_to_module_info = (
            PATH_TO_MODULE_INFO_WITH_SUB_AUTOGEN)
        unittest_utils.assert_strict_equal(
            self,
            test_finder_utils.find_parent_module_dir(uc.ROOT,
                                                     abs_class_dir,
                                                     mock_module_info),
            uc.MODULE_DIR)

    @mock.patch('os.path.isdir', return_value=True)
    @mock.patch('os.path.isfile', return_value=False)
    def test_find_parent_module_dir_with_multi_autogens(self, _isfile, _isdir):
        """Test _find_parent_module_dir method.

        This case ignores folders with multiple autogenerated configs defined.
        """
        abs_class_dir = '/%s' % CLASS_DIR
        mock_module_info = mock.Mock(spec=module_info.ModuleInfo)
        mock_module_info.path_to_module_info = (
            PATH_TO_MODULE_INFO_WITH_MULTI_AUTOGEN)
        self.assertRaises(
            atest_error.TestWithNoModuleError,
            test_finder_utils.find_parent_module_dir,
            uc.ROOT,
            abs_class_dir,
            mock_module_info)

    @mock.patch('os.path.isdir', return_value=True)
    @mock.patch('os.path.isfile', return_value=False)
    @mock.patch.object(test_finder_utils, 'is_robolectric_module',
                       return_value=True)
    def test_find_parent_module_dir_robo(self, _is_robo, _isfile, _isdir):
        """Test _find_parent_module_dir method.

        Make sure we behave as expected when we encounter a robo module path.
        """
        abs_class_dir = '/%s' % CLASS_DIR
        mock_module_info = mock.Mock(spec=module_info.ModuleInfo)
        rel_class_dir_path = os.path.relpath(abs_class_dir, uc.ROOT)
        mock_module_info.path_to_module_info = {rel_class_dir_path: [{}]}
        unittest_utils.assert_strict_equal(
            self,
            test_finder_utils.find_parent_module_dir(uc.ROOT,
                                                     abs_class_dir,
                                                     mock_module_info),
            rel_class_dir_path)

    def test_get_targets_from_xml(self):
        """Test get_targets_from_xml method."""
        # Mocking Etree is near impossible, so use a real file, but mocking
        # ModuleInfo is still fine. Just have it return False when it finds a
        # module that states it's not a module.
        mock_module_info = mock.Mock(spec=module_info.ModuleInfo)
        mock_module_info.is_module.side_effect = lambda module: (
            not module == 'is_not_module')
        xml_file = os.path.join(uc.TEST_DATA_DIR, constants.MODULE_CONFIG)
        unittest_utils.assert_strict_equal(
            self,
            test_finder_utils.get_targets_from_xml(xml_file, mock_module_info),
            XML_TARGETS)

    @mock.patch.object(test_finder_utils, '_VTS_PUSH_DIR',
                       os.path.join(uc.TEST_DATA_DIR, VTS_PUSH_DIR))
    def test_get_targets_from_vts_xml(self):
        """Test get_targets_from_xml method."""
        # Mocking Etree is near impossible, so use a real file, but mock out
        # ModuleInfo,
        mock_module_info = mock.Mock(spec=module_info.ModuleInfo)
        mock_module_info.is_module.return_value = True
        xml_file = os.path.join(uc.TEST_DATA_DIR, VTS_XML)
        unittest_utils.assert_strict_equal(
            self,
            test_finder_utils.get_targets_from_vts_xml(xml_file, '',
                                                       mock_module_info),
            VTS_XML_TARGETS)

    @mock.patch('subprocess.check_output')
    def test_get_ignored_dirs(self, _mock_check_output):
        """Test _get_ignored_dirs method."""
        build_top = '/a/b'
        _mock_check_output.return_value = ('/a/b/c/.find-ignore\n'
                                           '/a/b/out/.out-dir\n'
                                           '/a/b/d/.out-dir\n\n')
        # Case 1: $OUT_DIR = ''. No customized out dir.
        os_environ_mock = {constants.ANDROID_BUILD_TOP: build_top,
                           constants.ANDROID_OUT_DIR: ''}
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            correct_ignore_dirs = ['/a/b/c', '/a/b/out', '/a/b/d']
            ignore_dirs = test_finder_utils._get_ignored_dirs()
            self.assertEqual(ignore_dirs, correct_ignore_dirs)
        # Case 2: $OUT_DIR = 'out2'
        test_finder_utils._get_ignored_dirs.cached_ignore_dirs = []
        os_environ_mock = {constants.ANDROID_BUILD_TOP: build_top,
                           constants.ANDROID_OUT_DIR: 'out2'}
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            correct_ignore_dirs = ['/a/b/c', '/a/b/out', '/a/b/d', '/a/b/out2']
            ignore_dirs = test_finder_utils._get_ignored_dirs()
            self.assertEqual(ignore_dirs, correct_ignore_dirs)
        # Case 3: The $OUT_DIR is abs dir but not under $ANDROID_BUILD_TOP
        test_finder_utils._get_ignored_dirs.cached_ignore_dirs = []
        os_environ_mock = {constants.ANDROID_BUILD_TOP: build_top,
                           constants.ANDROID_OUT_DIR: '/x/y/e/g'}
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            correct_ignore_dirs = ['/a/b/c', '/a/b/out', '/a/b/d']
            ignore_dirs = test_finder_utils._get_ignored_dirs()
            self.assertEqual(ignore_dirs, correct_ignore_dirs)
        # Case 4: The $OUT_DIR is abs dir and under $ANDROID_BUILD_TOP
        test_finder_utils._get_ignored_dirs.cached_ignore_dirs = []
        os_environ_mock = {constants.ANDROID_BUILD_TOP: build_top,
                           constants.ANDROID_OUT_DIR: '/a/b/e/g'}
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            correct_ignore_dirs = ['/a/b/c', '/a/b/out', '/a/b/d', '/a/b/e/g']
            ignore_dirs = test_finder_utils._get_ignored_dirs()
            self.assertEqual(ignore_dirs, correct_ignore_dirs)
        # Case 5: There is a file of '.out-dir' under $OUT_DIR.
        test_finder_utils._get_ignored_dirs.cached_ignore_dirs = []
        os_environ_mock = {constants.ANDROID_BUILD_TOP: build_top,
                           constants.ANDROID_OUT_DIR: 'out'}
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            correct_ignore_dirs = ['/a/b/c', '/a/b/out', '/a/b/d']
            ignore_dirs = test_finder_utils._get_ignored_dirs()
            self.assertEqual(ignore_dirs, correct_ignore_dirs)
        # Case 6: Testing cache. All of the changes are useless.
        _mock_check_output.return_value = ('/a/b/X/.find-ignore\n'
                                           '/a/b/YY/.out-dir\n'
                                           '/a/b/d/.out-dir\n\n')
        os_environ_mock = {constants.ANDROID_BUILD_TOP: build_top,
                           constants.ANDROID_OUT_DIR: 'new'}
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            cached_answer = ['/a/b/c', '/a/b/out', '/a/b/d']
            none_cached_answer = ['/a/b/X', '/a/b/YY', '/a/b/d', 'a/b/new']
            ignore_dirs = test_finder_utils._get_ignored_dirs()
            self.assertEqual(ignore_dirs, cached_answer)
            self.assertNotEqual(ignore_dirs, none_cached_answer)

    def test_is_2nd_arch_module(self):
        """Test is_2nd_arch_module correctly detects the module."""
        is_2nd_arch_module = {'for_2nd_arch': [True]}
        is_not_2nd_arch_module = {'for_2nd_arch': [False]}
        is_not_2nd_arch_module_again = {'for_2nd_arch': []}

        self.assertTrue(test_finder_utils.is_2nd_arch_module(is_2nd_arch_module))
        self.assertFalse(test_finder_utils.is_2nd_arch_module(is_not_2nd_arch_module))
        self.assertFalse(test_finder_utils.is_2nd_arch_module(is_not_2nd_arch_module_again))
        self.assertFalse(test_finder_utils.is_2nd_arch_module({}))

if __name__ == '__main__':
    unittest.main()
