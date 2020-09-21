# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import tempfile
import time
import unittest

import common
from autotest_lib.client.bin.result_tools import dedupe_file_throttler
from autotest_lib.client.bin.result_tools import result_info
from autotest_lib.client.bin.result_tools import unittest_lib


# Set to 0 to force maximum throttling.
MAX_RESULT_SIZE_KB = 0

class DedupeFileThrottleTest(unittest.TestCase):
    """Test class for dedupe_file_throttler.throttle method."""

    def setUp(self):
        """Setup directory for test."""
        self.test_dir = tempfile.mkdtemp()
        self.files_to_keep = []
        self.files_to_delete = []
        modification_time = int(time.time()) - 1000

        # Files to be deduped in the root directory of result dir.
        for i in range(6):
            file_name = 'file_%d' % i
            f = os.path.join(self.test_dir, file_name)
            unittest_lib.create_file(f, unittest_lib.SIZE)
            os.utime(f, (modification_time, modification_time))
            modification_time += 1
            if (i < dedupe_file_throttler.OLDEST_FILES_TO_KEEP_COUNT or
                i >= 6 - dedupe_file_throttler.NEWEST_FILES_TO_KEEP_COUNT):
                self.files_to_keep.append(f)
            else:
                self.files_to_delete.append(f)

        folder1 = os.path.join(self.test_dir, 'folder1')
        os.mkdir(folder1)

        # Files should not be deduped.
        for i in range(3):
            file_name = 'file_not_dedupe_%d' % i
            f = os.path.join(folder1, file_name)
            unittest_lib.create_file(f, unittest_lib.SIZE)
            self.files_to_keep.append(f)

        # Files to be deduped in the sub directory of result dir.
        for i in range(10):
            file_name = 'file_in_sub_dir%d' % i
            f = os.path.join(folder1, file_name)
            unittest_lib.create_file(f, unittest_lib.SIZE)
            os.utime(f, (modification_time, modification_time))
            modification_time += 1
            if (i < dedupe_file_throttler.OLDEST_FILES_TO_KEEP_COUNT or
                i >= 10 - dedupe_file_throttler.NEWEST_FILES_TO_KEEP_COUNT):
                self.files_to_keep.append(f)
            else:
                self.files_to_delete.append(f)

    def tearDown(self):
        """Cleanup the test directory."""
        shutil.rmtree(self.test_dir, ignore_errors=True)

    def testTrim(self):
        """Test throttle method."""
        summary = result_info.ResultInfo.build_from_path(self.test_dir)
        dedupe_file_throttler.throttle(
                summary, max_result_size_KB=MAX_RESULT_SIZE_KB)

        # Verify summary sizes are updated.
        self.assertEqual(19 * unittest_lib.SIZE, summary.original_size)
        self.assertEqual(9 * unittest_lib.SIZE, summary.trimmed_size)

        # Verify files that should not be deleted still exists.
        for f in self.files_to_keep:
            self.assertTrue(os.stat(f).st_size > 0,
                            'File %s should not be deleted!' % f)

        # Verify files that should be deleted no longer exists.
        for f in self.files_to_delete:
            self.assertFalse(os.path.exists(f), 'File %s is not deleted!' % f)


# this is so the test can be run in standalone mode
if __name__ == '__main__':
    """Main"""
    unittest.main()
