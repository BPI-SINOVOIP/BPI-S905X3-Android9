# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import tempfile
import unittest

import common
from autotest_lib.client.bin.result_tools import result_info
from autotest_lib.client.bin.result_tools import shrink_file_throttler
from autotest_lib.client.bin.result_tools import unittest_lib
from autotest_lib.client.bin.result_tools import utils_lib


ORIGINAL_SIZE_BYTE = 1000
MAX_RESULT_SIZE_KB = 2
FILE_SIZE_LIMIT_BYTE = 512
LARGE_SIZE_BYTE = 100 * 1024

SUMMARY_AFTER_TRIMMING = {
    '': {utils_lib.DIRS: [
            {'BUILD_INFO-HT7591A00171': {
                    utils_lib.ORIGINAL_SIZE_BYTES: ORIGINAL_SIZE_BYTE}},
            {'file1.xml': {utils_lib.ORIGINAL_SIZE_BYTES: ORIGINAL_SIZE_BYTE}},
            {'file2.jpg': {utils_lib.ORIGINAL_SIZE_BYTES: ORIGINAL_SIZE_BYTE}},
            {'file3.log': {utils_lib.ORIGINAL_SIZE_BYTES: ORIGINAL_SIZE_BYTE,
                           utils_lib.TRIMMED_SIZE_BYTES: FILE_SIZE_LIMIT_BYTE}},
            {'folder1': {
                utils_lib.DIRS: [
                    {'file4': {
                        utils_lib.ORIGINAL_SIZE_BYTES: ORIGINAL_SIZE_BYTE,
                        utils_lib.TRIMMED_SIZE_BYTES: FILE_SIZE_LIMIT_BYTE}}],
                utils_lib.ORIGINAL_SIZE_BYTES: ORIGINAL_SIZE_BYTE,
                utils_lib.TRIMMED_SIZE_BYTES: FILE_SIZE_LIMIT_BYTE}},
            {'test_run_details.txt': {
                    utils_lib.ORIGINAL_SIZE_BYTES: ORIGINAL_SIZE_BYTE}}],
         utils_lib.ORIGINAL_SIZE_BYTES: 6 * ORIGINAL_SIZE_BYTE,
         utils_lib.TRIMMED_SIZE_BYTES: (
                 4 * ORIGINAL_SIZE_BYTE + 2 * FILE_SIZE_LIMIT_BYTE)}
    }

OLD_TIME = 1498800000

class ShrinkFileThrottleTest(unittest.TestCase):
    """Test class for shrink_file_throttler.throttle method."""

    def setUp(self):
        """Setup directory for test."""
        self.test_dir = tempfile.mkdtemp()
        self.files_not_shrink = []
        self.files_to_shrink = []

        build_info = os.path.join(self.test_dir, 'BUILD_INFO-HT7591A00171')
        unittest_lib.create_file(build_info, ORIGINAL_SIZE_BYTE)
        self.files_not_shrink.append(build_info)

        file1 = os.path.join(self.test_dir, 'file1.xml')
        unittest_lib.create_file(file1, ORIGINAL_SIZE_BYTE)
        self.files_not_shrink.append(file1)

        file2 = os.path.join(self.test_dir, 'file2.jpg')
        unittest_lib.create_file(file2, ORIGINAL_SIZE_BYTE)
        self.files_not_shrink.append(file2)

        file3 = os.path.join(self.test_dir, 'file3.log')
        unittest_lib.create_file(file3, ORIGINAL_SIZE_BYTE)
        self.files_to_shrink.append(file3)
        os.utime(file3, (OLD_TIME, OLD_TIME))

        file4 = os.path.join(self.test_dir, 'test_run_details.txt')
        unittest_lib.create_file(file4, ORIGINAL_SIZE_BYTE)
        self.files_not_shrink.append(file4)

        folder1 = os.path.join(self.test_dir, 'folder1')
        os.mkdir(folder1)
        file4 = os.path.join(folder1, 'file4')
        unittest_lib.create_file(file4, ORIGINAL_SIZE_BYTE)
        self.files_to_shrink.append(file4)
        os.utime(file4, (OLD_TIME, OLD_TIME))

    def tearDown(self):
        """Cleanup the test directory."""
        shutil.rmtree(self.test_dir, ignore_errors=True)

    def testTrim(self):
        """Test throttle method."""
        summary = result_info.ResultInfo.build_from_path(self.test_dir)
        shrink_file_throttler.throttle(
                summary,
                max_result_size_KB=MAX_RESULT_SIZE_KB,
                file_size_limit_byte=FILE_SIZE_LIMIT_BYTE)

        self.assertEqual(SUMMARY_AFTER_TRIMMING, summary)

        # Verify files that should not be shrunk are not changed.
        for f in self.files_not_shrink:
            self.assertEqual(ORIGINAL_SIZE_BYTE, os.stat(f).st_size,
                             'File %s should not be shrank!' % f)

        # Verify files that should be shrunk are updated.
        for f in self.files_to_shrink:
            stat = os.stat(f)
            self.assertTrue(FILE_SIZE_LIMIT_BYTE >= stat.st_size,
                            'File %s is not shrank!' % f)
            self.assertEqual(OLD_TIME, stat.st_mtime)


class MultipleShrinkFileTest(unittest.TestCase):
    """Test class for shrink_file_throttler.throttle method for files to be
    shrunk multiple times.
    """

    def setUp(self):
        """Setup directory for test."""
        self.test_dir = tempfile.mkdtemp()

        self.file_to_shrink = os.path.join(self.test_dir, 'file1.txt')
        unittest_lib.create_file(self.file_to_shrink, LARGE_SIZE_BYTE)

    def tearDown(self):
        """Cleanup the test directory."""
        shutil.rmtree(self.test_dir, ignore_errors=True)

    def testTrim(self):
        """Shrink the file twice and check its content."""
        summary = result_info.ResultInfo.build_from_path(
                parent_dir=self.test_dir, name=utils_lib.ROOT_DIR,
                top_dir=self.test_dir, parent_result_info=None)
        shrink_file_throttler.throttle(
                summary, max_result_size_KB=60,
                file_size_limit_byte=50 * 1024)
        summary = result_info.ResultInfo.build_from_path(self.test_dir)
        shrink_file_throttler.throttle(
                summary, max_result_size_KB=30,
                file_size_limit_byte=25 * 1024)

        with open(self.file_to_shrink) as f:
            content = f.read()

        original_size_string = (shrink_file_throttler.ORIGINAL_SIZE_TEMPLATE %
                                LARGE_SIZE_BYTE)
        self.assertTrue(original_size_string in content)

        trimmed_size_string = (
                shrink_file_throttler.TRIMMED_FILE_INJECT_TEMPLATE % 76990)
        self.assertTrue(trimmed_size_string in content)


# this is so the test can be run in standalone mode
if __name__ == '__main__':
    """Main"""
    unittest.main()
