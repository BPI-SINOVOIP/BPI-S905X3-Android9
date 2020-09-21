# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import tempfile
import unittest

import common
from autotest_lib.client.bin.result_tools import delete_file_throttler
from autotest_lib.client.bin.result_tools import result_info
from autotest_lib.client.bin.result_tools import unittest_lib
from autotest_lib.client.bin.result_tools import utils_lib


LARGE_SIZE_BYTE = 1000
MEDIUM_SIZE_BYTE = 800
SMALL_SIZE_BYTE = 100
# Maximum result size is set to 3KB so the file with MEDIUM_SIZE_BYTE will be
# kept.
MAX_RESULT_SIZE_KB = 5
# Any file with size above the threshold is qualified to be deleted.
FILE_SIZE_THRESHOLD_BYTE = 512

SUMMARY_AFTER_THROTTLE = {
    '': {utils_lib.DIRS: [
            {'AndroidDeviceXXX': {
                utils_lib.DIRS: [
                    {'file5': {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE_BYTE}},
                    ],
                utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE_BYTE}},
            {'chrome.123.perf.data': {
                utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE_BYTE}},
            {'file1.xml': {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE_BYTE,
                          utils_lib.TRIMMED_SIZE_BYTES: 0}},
            {'file2.jpg': {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE_BYTE,
                          utils_lib.TRIMMED_SIZE_BYTES: 0}},
            {'file3.log': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE_BYTE}},
            {'file_to_keep': {utils_lib.ORIGINAL_SIZE_BYTES: MEDIUM_SIZE_BYTE}},
            {'folder1': {
                utils_lib.DIRS: [
                    {'file4': {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE_BYTE,
                              utils_lib.TRIMMED_SIZE_BYTES: 0}},
                    {'keyval':
                        {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE_BYTE}},
                    ],
                utils_lib.ORIGINAL_SIZE_BYTES: 2 * LARGE_SIZE_BYTE,
                utils_lib.TRIMMED_SIZE_BYTES: LARGE_SIZE_BYTE}},
            {'test_run_details.txt': {
                utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE_BYTE}}],
         utils_lib.ORIGINAL_SIZE_BYTES:
                7 * LARGE_SIZE_BYTE + SMALL_SIZE_BYTE + MEDIUM_SIZE_BYTE,
         utils_lib.TRIMMED_SIZE_BYTES:
                4 * LARGE_SIZE_BYTE + SMALL_SIZE_BYTE + MEDIUM_SIZE_BYTE}
    }

class ThrottleTest(unittest.TestCase):
    """Test class for shrink_file_throttler.throttle method."""

    def setUp(self):
        """Setup directory for test."""
        self.test_dir = tempfile.mkdtemp()
        self.files_not_deleted = []
        self.files_to_delete = []

        file1 = os.path.join(self.test_dir, 'file1.xml')
        unittest_lib.create_file(file1, LARGE_SIZE_BYTE)
        self.files_to_delete.append(file1)

        file2 = os.path.join(self.test_dir, 'file2.jpg')
        unittest_lib.create_file(file2, LARGE_SIZE_BYTE)
        self.files_to_delete.append(file2)

        file_to_keep = os.path.join(self.test_dir, 'file_to_keep')
        unittest_lib.create_file(file_to_keep, MEDIUM_SIZE_BYTE)
        self.files_not_deleted.append(file_to_keep)

        file3 = os.path.join(self.test_dir, 'file3.log')
        unittest_lib.create_file(file3, SMALL_SIZE_BYTE)
        self.files_not_deleted.append(file3)

        folder1 = os.path.join(self.test_dir, 'folder1')
        os.mkdir(folder1)
        file4 = os.path.join(folder1, 'file4')
        unittest_lib.create_file(file4, LARGE_SIZE_BYTE)
        self.files_to_delete.append(file4)

        protected_file = os.path.join(folder1, 'keyval')
        unittest_lib.create_file(protected_file, LARGE_SIZE_BYTE)
        self.files_not_deleted.append(protected_file)

        folder2 = os.path.join(self.test_dir, 'AndroidDeviceXXX')
        os.mkdir(folder2)
        file5 = os.path.join(folder2, 'file5')
        unittest_lib.create_file(file5, LARGE_SIZE_BYTE)
        self.files_not_deleted.append(file5)

        test_run = os.path.join(self.test_dir, 'test_run_details.txt')
        unittest_lib.create_file(test_run, LARGE_SIZE_BYTE)
        self.files_not_deleted.append(test_run)

        perf_data = os.path.join(self.test_dir, 'chrome.123.perf.data')
        unittest_lib.create_file(perf_data, LARGE_SIZE_BYTE)
        self.files_not_deleted.append(perf_data)

    def tearDown(self):
        """Cleanup the test directory."""
        shutil.rmtree(self.test_dir, ignore_errors=True)

    def testTrim(self):
        """Test throttle method."""
        summary = result_info.ResultInfo.build_from_path(self.test_dir)
        delete_file_throttler.throttle(
                summary,
                max_result_size_KB=MAX_RESULT_SIZE_KB,
                file_size_threshold_byte=FILE_SIZE_THRESHOLD_BYTE)

        self.assertEqual(SUMMARY_AFTER_THROTTLE, summary)

        # Verify files that should not be deleted still exists.
        for f in self.files_not_deleted:
            self.assertTrue(os.stat(f).st_size > 0,
                            'File %s should not be deleted!' % f)

        # Verify files that should be deleted no longer exists.
        for f in self.files_to_delete:
            self.assertFalse(os.path.exists(f), 'File %s is not deleted!' % f)


# this is so the test can be run in standalone mode
if __name__ == '__main__':
    """Main"""
    unittest.main()