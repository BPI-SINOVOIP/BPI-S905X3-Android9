# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import math
import os
import shutil
import tempfile
import unittest

import common
from autotest_lib.client.bin.result_tools import result_info
from autotest_lib.client.bin.result_tools import unittest_lib
from autotest_lib.client.bin.result_tools import utils_lib
from autotest_lib.client.bin.result_tools import zip_file_throttler


ORIGINAL_SIZE_BYTE = 1000
# Set max result size to 0 to force all files being compressed if possible.
MAX_RESULT_SIZE_KB = 0
FILE_SIZE_THRESHOLD_BYTE = 512

SUMMARY_AFTER_TRIMMING = {
    '': {utils_lib.DIRS: [
            {'BUILD_INFO-x':
                    {utils_lib.ORIGINAL_SIZE_BYTES: ORIGINAL_SIZE_BYTE}},
            {'file1.xml.tgz':
                {utils_lib.ORIGINAL_SIZE_BYTES: ORIGINAL_SIZE_BYTE,
                 utils_lib.TRIMMED_SIZE_BYTES: 148}},
            {'file2.jpg': {utils_lib.ORIGINAL_SIZE_BYTES: ORIGINAL_SIZE_BYTE}},
            {'file3.log.tgz':
                {utils_lib.ORIGINAL_SIZE_BYTES: ORIGINAL_SIZE_BYTE,
                 utils_lib.TRIMMED_SIZE_BYTES: 148}},
            {'folder1': {
                utils_lib.DIRS: [
                    {'file4.tgz': {
                            utils_lib.ORIGINAL_SIZE_BYTES: ORIGINAL_SIZE_BYTE,
                            utils_lib.TRIMMED_SIZE_BYTES: 140}}],
                utils_lib.ORIGINAL_SIZE_BYTES: ORIGINAL_SIZE_BYTE,
                utils_lib.TRIMMED_SIZE_BYTES: 140}}],
         utils_lib.ORIGINAL_SIZE_BYTES: 5 * ORIGINAL_SIZE_BYTE,
         utils_lib.TRIMMED_SIZE_BYTES: 2 * ORIGINAL_SIZE_BYTE + 435}
    }

class ZipFileThrottleTest(unittest.TestCase):
    """Test class for zip_file_throttler.throttle method."""

    def setUp(self):
        """Setup directory for test."""
        self.test_dir = tempfile.mkdtemp()
        self.files_not_zip = []
        self.files_to_zip = []

        build_info = os.path.join(self.test_dir, 'BUILD_INFO-x')
        unittest_lib.create_file(build_info, ORIGINAL_SIZE_BYTE)
        self.files_not_zip.append(build_info)

        file1 = os.path.join(self.test_dir, 'file1.xml')
        unittest_lib.create_file(file1, ORIGINAL_SIZE_BYTE)
        self.files_to_zip.append(file1)

        file2 = os.path.join(self.test_dir, 'file2.jpg')
        unittest_lib.create_file(file2, ORIGINAL_SIZE_BYTE)
        self.files_not_zip.append(file2)

        file3 = os.path.join(self.test_dir, 'file3.log')
        unittest_lib.create_file(file3, ORIGINAL_SIZE_BYTE)
        self.files_to_zip.append(file3)

        folder1 = os.path.join(self.test_dir, 'folder1')
        os.mkdir(folder1)
        file4 = os.path.join(folder1, 'file4')
        unittest_lib.create_file(file4, ORIGINAL_SIZE_BYTE)
        self.files_to_zip.append(file4)

    def tearDown(self):
        """Cleanup the test directory."""
        shutil.rmtree(self.test_dir, ignore_errors=True)

    def compareSummary(self, expected, actual):
        """Compare two summaries with tolerance on trimmed sizes.

        @param expected: The expected directory summary.
        @param actual: The actual directory summary after trimming.
        """
        self.assertEqual(expected.original_size, actual.original_size)
        diff = math.fabs(expected.trimmed_size - actual.trimmed_size)
        # Compression may generate different sizes of tgz file, but the
        # difference shouldn't be more than 100 bytes.
        self.assertTrue(
                diff < 100,
                'Compression failed to be verified. Expected size: %d, actual '
                'size: %d' % (expected.trimmed_size, actual.trimmed_size))

        # Match files inside the directories.
        if expected.is_dir:
            expected_info_map = {info.name: info for info in expected.files}
            actual_info_map = {info.name: info for info in actual.files}
            self.assertEqual(set(expected_info_map.keys()),
                             set(actual_info_map.keys()))
            for name in expected_info_map:
                self.compareSummary(
                        expected_info_map[name], actual_info_map[name])

    def testTrim(self):
        """Test throttle method."""
        summary = result_info.ResultInfo.build_from_path(self.test_dir)
        zip_file_throttler.throttle(
                summary,
                max_result_size_KB=MAX_RESULT_SIZE_KB,
                file_size_threshold_byte=FILE_SIZE_THRESHOLD_BYTE)

        expected_summary = result_info.ResultInfo(
                parent_dir=self.test_dir, original_info=SUMMARY_AFTER_TRIMMING)
        self.compareSummary(expected_summary, summary)

        # Verify files that should not be compressed are not changed.
        for f in self.files_not_zip:
            self.assertEqual(ORIGINAL_SIZE_BYTE, os.stat(f).st_size,
                             'File %s should not be compressed!' % f)

        # Verify files that should be zipped are updated.
        for f in self.files_to_zip:
            stat = os.stat(f + '.tgz')
            self.assertTrue(FILE_SIZE_THRESHOLD_BYTE >= stat.st_size,
                            'File %s is not compressed!' % f)


# this is so the test can be run in standalone mode
if __name__ == '__main__':
    """Main"""
    unittest.main()
