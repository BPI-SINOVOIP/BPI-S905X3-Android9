#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""unittest for result_info.py
"""

import copy
import os
import shutil
import tempfile
import unittest

import common
from autotest_lib.client.bin.result_tools import result_info
from autotest_lib.client.bin.result_tools import unittest_lib
from autotest_lib.client.bin.result_tools import utils_lib


_SIZE = 100
_EXPECTED_SUMMARY = {
    '': {utils_lib.ORIGINAL_SIZE_BYTES: 13 * _SIZE,
         utils_lib.TRIMMED_SIZE_BYTES: 4 * _SIZE,
         utils_lib.DIRS: [
                 {'file1': {utils_lib.ORIGINAL_SIZE_BYTES: _SIZE}},
                 {'folder1': {utils_lib.ORIGINAL_SIZE_BYTES: 11 * _SIZE,
                              utils_lib.TRIMMED_SIZE_BYTES: 2 * _SIZE,
                              utils_lib.DIRS: [
                                {'file2': {
                                    utils_lib.ORIGINAL_SIZE_BYTES: 10 * _SIZE,
                                    utils_lib.TRIMMED_SIZE_BYTES: _SIZE}},
                                {'file3': {
                                  utils_lib.ORIGINAL_SIZE_BYTES: _SIZE}}]}},
                 {'folder2': {utils_lib.ORIGINAL_SIZE_BYTES: _SIZE,
                              utils_lib.DIRS:
                                [{'file2': {
                                    utils_lib.ORIGINAL_SIZE_BYTES: _SIZE}
                                  }],
                             }
                  }
            ]
        }
    }

_EXPECTED_SINGLE_FILE_SUMMARY = {
    '': {utils_lib.ORIGINAL_SIZE_BYTES: unittest_lib.SIZE,
         utils_lib.DIRS: [
                 {'file1': {utils_lib.ORIGINAL_SIZE_BYTES: unittest_lib.SIZE}},
                 ]
         }
    }

class ResultInfoUnittest(unittest.TestCase):
    """Test class for ResultInfo.
    """

    def setUp(self):
        """Setup directory for test."""
        self.test_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Cleanup the test directory."""
        shutil.rmtree(self.test_dir, ignore_errors=True)

    def testLoadJson(self):
        """Test method load_summary_json_file and related update methods."""
        summary_file = os.path.join(self.test_dir, 'summary.json')
        result_info.save_summary(_EXPECTED_SUMMARY, summary_file)
        summary = result_info.load_summary_json_file(summary_file)

        self.assertEqual(_EXPECTED_SUMMARY, summary,
                         'Failed to match the loaded json file.')
        # Save the json of the new summary, confirm it matches the old one.
        summary_file_new = os.path.join(self.test_dir, 'summary_new.json')
        result_info.save_summary(summary, summary_file_new)
        summary_new = result_info.load_summary_json_file(summary_file_new)
        self.assertEqual(
                _EXPECTED_SUMMARY, summary_new,
                'Failed to match the loaded json file after it is saved again.')

        # Validate the details of result_info.
        self.assertEqual(summary.path, self.test_dir)
        self.assertEqual(summary.get_file('file1').path,
                         os.path.join(self.test_dir, 'file1'))
        self.assertEqual(summary.get_file('folder1').get_file('file2').path,
                         os.path.join(self.test_dir, 'folder1', 'file2'))

        # Validate the details of a deep copy of result_info.
        new_summary = copy.deepcopy(summary)
        # Modify old summary, to make sure the clone is not changed.
        summary._path = None
        summary.get_file('file1')._path = None
        summary.get_file('file1').original_size = 0
        summary.get_file('folder1').get_file('file2')._path = None

        self.assertEqual(new_summary.path, self.test_dir)
        self.assertEqual(new_summary.get_file('file1').path,
                         os.path.join(self.test_dir, 'file1'))
        self.assertEqual(new_summary.get_file('file1').original_size, _SIZE)
        self.assertEqual(id(new_summary.get_file('file1')._parent_result_info),
                         id(new_summary))
        self.assertNotEqual(id(new_summary), id(summary))
        self.assertEqual(new_summary.get_file('folder1').get_file('file2').path,
                         os.path.join(self.test_dir, 'folder1', 'file2'))

    def testInit(self):
        """Test __init__ method fails when both name and original_info are set.
        """
        self.assertRaises(result_info.ResultInfoError, result_info.ResultInfo,
                          'parent_dir', 'file_name', None,
                          {'name': 'file_name'})

    def testUpdateSize(self):
        """Test sizes updated in all parent nodes after leaf node is updated.
        """
        summary_file = os.path.join(self.test_dir, 'summary.json')
        result_info.save_summary(_EXPECTED_SUMMARY, summary_file)
        summary = result_info.load_summary_json_file(summary_file)
        file2 = summary.get_file('folder1').get_file('file2')
        file2.trimmed_size = 2 * _SIZE

        # Check all parent result info have size updated.
        self.assertEqual(file2.trimmed_size, 2 * _SIZE)
        self.assertEqual(summary.get_file('folder1').trimmed_size, 3 * _SIZE)
        self.assertEqual(summary.trimmed_size, 5 * _SIZE)

        file2.original_size = 11 * _SIZE
        self.assertEqual(summary.get_file('folder1').original_size, 12 * _SIZE)
        self.assertEqual(summary.original_size, 14 * _SIZE)

        file2.collected_size = 20 * _SIZE
        self.assertEqual(summary.get_file('folder1').collected_size, 21 * _SIZE)
        self.assertEqual(summary.collected_size, 23 * _SIZE)

    def TestBuildFromPath_SingleFile(self):
        """Test method build_from_path for a single file."""
        file1 = os.path.join(self.test_dir, 'file1')
        unittest_lib.create_file(file1)
        summary = result_info.ResultInfo.build_from_path(file1)
        self.assertEqual(_EXPECTED_SINGLE_FILE_SUMMARY, summary)


# this is so the test can be run in standalone mode
if __name__ == '__main__':
    """Main"""
    unittest.main()
