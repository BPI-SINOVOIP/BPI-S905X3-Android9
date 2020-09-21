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

"""Unittests for module_info."""

import os
import unittest
import mock

import constants
import module_info
import unittest_constants as uc

JSON_FILE_PATH = os.path.join(uc.TEST_DATA_DIR, uc.JSON_FILE)
EXPECTED_MOD_TARGET = 'tradefed'
EXPECTED_MOD_TARGET_PATH = ['tf/core']
UNEXPECTED_MOD_TARGET = 'this_should_not_be_in_module-info.json'
MOD_NO_PATH = 'module-no-path'
PATH_TO_MULT_MODULES = 'shared/path/to/be/used'
MULT_MOODULES_WITH_SHARED_PATH = ['module2', 'module1']

#pylint: disable=protected-access
class ModuleInfoUnittests(unittest.TestCase):
    """Unit tests for module_info.py"""

    @mock.patch('json.load', return_value={})
    @mock.patch('__builtin__.open', new_callable=mock.mock_open)
    @mock.patch('os.path.isfile', return_value=True)
    def test_load_mode_info_file_out_dir_handling(self, _isfile, _open, _json):
        """Test _load_module_info_file out dir handling."""
        # Test out default out dir is used.
        build_top = '/path/to/top'
        default_out_dir = os.path.join(build_top, 'out/dir/here')
        os_environ_mock = {constants.ANDROID_OUT: default_out_dir,
                           'ANDROID_PRODUCT_OUT': default_out_dir,
                           constants.ANDROID_BUILD_TOP: build_top}
        default_out_dir_mod_targ = 'out/dir/here/module-info.json'
        # Make sure module_info_target is what we think it is.
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            mod_info = module_info.ModuleInfo()
            self.assertEqual(default_out_dir_mod_targ,
                             mod_info.module_info_target)

        # Test out custom out dir is used (OUT_DIR=dir2).
        custom_out_dir = os.path.join(build_top, 'out2/dir/here')
        os_environ_mock = {constants.ANDROID_OUT: custom_out_dir,
                           'OUT_DIR': 'out2',
                           'ANDROID_PRODUCT_OUT': custom_out_dir,
                           constants.ANDROID_BUILD_TOP: build_top}
        custom_out_dir_mod_targ = 'out2/dir/here/module-info.json'
        # Make sure module_info_target is what we think it is.
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            mod_info = module_info.ModuleInfo()
            self.assertEqual(custom_out_dir_mod_targ,
                             mod_info.module_info_target)

        # Test out custom abs out dir is used (OUT_DIR=/tmp/out/dir2).
        abs_custom_out_dir = '/tmp/out/dir'
        os_environ_mock = {constants.ANDROID_OUT: abs_custom_out_dir,
                           'OUT_DIR': abs_custom_out_dir,
                           'ANDROID_PRODUCT_OUT': abs_custom_out_dir,
                           constants.ANDROID_BUILD_TOP: build_top}
        custom_abs_out_dir_mod_targ = '/tmp/out/dir/module-info.json'
        # Make sure module_info_target is what we think it is.
        with mock.patch.dict('os.environ', os_environ_mock, clear=True):
            mod_info = module_info.ModuleInfo()
            self.assertEqual(custom_abs_out_dir_mod_targ,
                             mod_info.module_info_target)

    @mock.patch.object(module_info.ModuleInfo, '_load_module_info_file',)
    def test_get_path_to_module_info(self, mock_load_module):
        """Test that we correctly create the path to module info dict."""
        mod_one = 'mod1'
        mod_two = 'mod2'
        mod_path_one = '/path/to/mod1'
        mod_path_two = '/path/to/mod2'
        mod_info_dict = {mod_one: {constants.MODULE_PATH: [mod_path_one]},
                         mod_two: {constants.MODULE_PATH: [mod_path_two]}}
        mock_load_module.return_value = ('mod_target', mod_info_dict)
        path_to_mod_info = {mod_path_one: [{constants.MODULE_NAME: mod_one,
                                            constants.MODULE_PATH: [mod_path_one]}],
                            mod_path_two: [{constants.MODULE_NAME: mod_two,
                                            constants.MODULE_PATH: [mod_path_two]}]}
        mod_info = module_info.ModuleInfo()
        self.assertDictEqual(path_to_mod_info,
                             mod_info._get_path_to_module_info(mod_info_dict))

    def test_is_module(self):
        """Test that we get the module when it's properly loaded."""
        # Load up the test json file and check that module is in it
        mod_info = module_info.ModuleInfo(module_file=JSON_FILE_PATH)
        self.assertTrue(mod_info.is_module(EXPECTED_MOD_TARGET))
        self.assertFalse(mod_info.is_module(UNEXPECTED_MOD_TARGET))

    def test_get_path(self):
        """Test that we get the module path when it's properly loaded."""
        # Load up the test json file and check that module is in it
        mod_info = module_info.ModuleInfo(module_file=JSON_FILE_PATH)
        self.assertEqual(mod_info.get_paths(EXPECTED_MOD_TARGET),
                         EXPECTED_MOD_TARGET_PATH)
        self.assertEqual(mod_info.get_paths(MOD_NO_PATH), [])

    def test_get_module_names(self):
        """test that we get the module name properly."""
        mod_info = module_info.ModuleInfo(module_file=JSON_FILE_PATH)
        self.assertEqual(mod_info.get_module_names(EXPECTED_MOD_TARGET_PATH[0]),
                         [EXPECTED_MOD_TARGET])
        self.assertEqual(mod_info.get_module_names(PATH_TO_MULT_MODULES),
                         MULT_MOODULES_WITH_SHARED_PATH)



if __name__ == '__main__':
    unittest.main()
