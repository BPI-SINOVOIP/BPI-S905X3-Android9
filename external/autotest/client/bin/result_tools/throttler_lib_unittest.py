# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import unittest

import common
from autotest_lib.client.bin.result_tools import result_info
from autotest_lib.client.bin.result_tools import throttler_lib
from autotest_lib.client.bin.result_tools import unittest_lib
from autotest_lib.client.bin.result_tools import utils_lib


# This unittest doesn't care about the size, so the size info can be shared to
# make the code cleaner.
FILE_SIZE_DICT = {utils_lib.ORIGINAL_SIZE_BYTES: unittest_lib.SIZE}

SAMPLE_SUMMARY = {
  '': {utils_lib.ORIGINAL_SIZE_BYTES: 7 * unittest_lib.SIZE,
       utils_lib.DIRS: [
         {'file1': FILE_SIZE_DICT},
         {'file2.tar': {utils_lib.ORIGINAL_SIZE_BYTES: 2 * unittest_lib.SIZE}},
         {'file.deleted': {utils_lib.ORIGINAL_SIZE_BYTES: unittest_lib.SIZE,
                           utils_lib.TRIMMED_SIZE_BYTES: 0}},
         {'keyval': FILE_SIZE_DICT},
         {'sysinfo':
            {utils_lib.ORIGINAL_SIZE_BYTES: 2 * unittest_lib.SIZE,
             utils_lib.DIRS: [
                {'file3': FILE_SIZE_DICT},
                {'var':
                  {utils_lib.ORIGINAL_SIZE_BYTES: unittest_lib.SIZE,
                   utils_lib.DIRS: [
                     {'log': {utils_lib.ORIGINAL_SIZE_BYTES: unittest_lib.SIZE,
                              utils_lib.DIRS: [
                                 {'file4': FILE_SIZE_DICT}
                                 ],
                              }
                      }
                    ],
                   },
                 }
                ],
             }
            },
          ],
       }
  }

EXPECTED_FILES = [
        ['', 'sysinfo', 'var', 'log', 'file4'],
        ['', 'sysinfo', 'file3'],
        ['', 'file2.tar'],
        ['', 'file1'],
        ['', 'keyval'],
        ['', 'file.deleted'],
        ]

EXPECTED_THROTTABLE_FILES = [
        ['', 'sysinfo', 'var', 'log', 'file4'],
        ['', 'sysinfo', 'file3'],
        ['', 'file2.tar'],
        ]

class ThrottlerLibTest(unittest.TestCase):
    """Test class for methods in throttler_lib."""

    def testSortResultFiles(self):
        """Test method sort_result_files"""
        summary = result_info.ResultInfo(parent_dir='',
                                         original_info=SAMPLE_SUMMARY)
        sorted_files, _ = throttler_lib.sort_result_files(summary)
        self.assertEqual(len(EXPECTED_FILES), len(sorted_files))
        for i in range(len(EXPECTED_FILES)):
            self.assertEqual(os.path.join(*EXPECTED_FILES[i]),
                             sorted_files[i].path)

    def testGetThrottableFiles(self):
        """Test method get_throttleable_files"""
        summary = result_info.ResultInfo(parent_dir='',
                                         original_info=SAMPLE_SUMMARY)
        sorted_files, _ = throttler_lib.sort_result_files(summary)
        throttleables = throttler_lib.get_throttleable_files(
                sorted_files, ['.*file1'])

        throttleables = list(throttleables)
        self.assertEqual(len(EXPECTED_THROTTABLE_FILES), len(throttleables))
        for i in range(len(EXPECTED_THROTTABLE_FILES)):
            self.assertEqual(os.path.join(*EXPECTED_THROTTABLE_FILES[i]),
                             throttleables[i].path)


# this is so the test can be run in standalone mode
if __name__ == '__main__':
    """Main"""
    unittest.main()
