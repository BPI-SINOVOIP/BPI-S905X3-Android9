#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""unittest for utils.py
"""

import json
import os
import shutil
import tempfile
import time
import unittest

import common
from autotest_lib.client.bin.result_tools import result_info
from autotest_lib.client.bin.result_tools import shrink_file_throttler
from autotest_lib.client.bin.result_tools import throttler_lib
from autotest_lib.client.bin.result_tools import utils as result_utils
from autotest_lib.client.bin.result_tools import utils_lib
from autotest_lib.client.bin.result_tools import view as result_view
from autotest_lib.client.bin.result_tools import unittest_lib

SIZE = unittest_lib.SIZE

# Sizes used for testing throttling
LARGE_SIZE = 1 * 1024 * 1024
SMALL_SIZE = 1 * 1024

EXPECTED_SUMMARY = {
        '': {utils_lib.ORIGINAL_SIZE_BYTES: 4 * SIZE,
             utils_lib.DIRS: [
                     {'file1': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}},
                     {'folder1': {utils_lib.ORIGINAL_SIZE_BYTES: 2 * SIZE,
                                 utils_lib.DIRS: [
                                  {'file2': {
                                      utils_lib.ORIGINAL_SIZE_BYTES: SIZE}},
                                  {'file3': {
                                      utils_lib.ORIGINAL_SIZE_BYTES: SIZE}},
                                  {'symlink': {
                                      utils_lib.ORIGINAL_SIZE_BYTES: 0,
                                      utils_lib.DIRS: []}}]}},
                     {'folder2': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE,
                                 utils_lib.DIRS:
                                     [{'file2':
                                        {utils_lib.ORIGINAL_SIZE_BYTES:
                                         SIZE}}],
                                }}]}}

SUMMARY_1 = {
  '': {utils_lib.ORIGINAL_SIZE_BYTES: 6 * SIZE,
       utils_lib.TRIMMED_SIZE_BYTES: 5 * SIZE,
       utils_lib.DIRS: [
         {'file1': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}},
         {'file2': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}},
         {'file4': {utils_lib.ORIGINAL_SIZE_BYTES: 2 * SIZE,
                   utils_lib.TRIMMED_SIZE_BYTES: SIZE}},
         {'folder_not_overwritten':
            {utils_lib.ORIGINAL_SIZE_BYTES: SIZE,
             utils_lib.DIRS: [
               {'file1': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}}
              ]}},
          {'file_to_be_overwritten': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}},
        ]
      }
  }

SUMMARY_2 = {
  '': {utils_lib.ORIGINAL_SIZE_BYTES: 27 * SIZE,
       utils_lib.DIRS: [
         # `file1` exists and has the same size.
         {'file1': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}},
         # Change the size of `file2` to make sure summary merge works.
         {'file2': {utils_lib.ORIGINAL_SIZE_BYTES: 2 * SIZE}},
         # `file3` is new.
         {'file3': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}},
         # `file4` is old but throttled earlier.
         {'file4': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}},
         # Add a new sub-directory.
         {'folder1': {utils_lib.ORIGINAL_SIZE_BYTES: 20 * SIZE,
                     utils_lib.TRIMMED_SIZE_BYTES: SIZE,
                     utils_lib.DIRS: [
                         # Add a file being trimmed.
                         {'file4': {
                           utils_lib.ORIGINAL_SIZE_BYTES: 20 * SIZE,
                           utils_lib.TRIMMED_SIZE_BYTES: SIZE}
                         }]
                     }},
          # Add a file whose name collides with the previous summary.
          {'folder_not_overwritten': {
            utils_lib.ORIGINAL_SIZE_BYTES: 100 * SIZE}},
          # Add a directory whose name collides with the previous summary.
          {'file_to_be_overwritten':
            {utils_lib.ORIGINAL_SIZE_BYTES: SIZE,
             utils_lib.DIRS: [
               {'file1': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}}]
            }},
          # Folder was collected, not missing from the final result folder.
          {'folder_tobe_deleted':
            {utils_lib.ORIGINAL_SIZE_BYTES: SIZE,
             utils_lib.DIRS: [
               {'file_tobe_deleted': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}}]
            }},
        ]
      }
  }

SUMMARY_3 = {
  '': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE,
       utils_lib.DIRS: [
         {'file10': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}},
         ]
       }
  }

SUMMARY_1_SIZE = 224
SUMMARY_2_SIZE = 388
SUMMARY_3_SIZE = 48

# The final result dir has an extra folder and file, also with `file3` removed
# to test the case that client files are removed on the server side.
EXPECTED_MERGED_SUMMARY = {
  '': {utils_lib.ORIGINAL_SIZE_BYTES:
           40 * SIZE + SUMMARY_1_SIZE + SUMMARY_2_SIZE + SUMMARY_3_SIZE,
       utils_lib.TRIMMED_SIZE_BYTES:
           19 * SIZE + SUMMARY_1_SIZE + SUMMARY_2_SIZE + SUMMARY_3_SIZE,
       # Size collected is SIZE bytes more than total size as an old `file2` of
       # SIZE bytes is overwritten by a newer file.
       utils_lib.COLLECTED_SIZE_BYTES:
           22 * SIZE + SUMMARY_1_SIZE + SUMMARY_2_SIZE + SUMMARY_3_SIZE,
       utils_lib.DIRS: [
         {'file1': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}},
         {'file2': {utils_lib.ORIGINAL_SIZE_BYTES: 2 * SIZE,
                    utils_lib.COLLECTED_SIZE_BYTES: 3 * SIZE}},
         {'file4': {utils_lib.ORIGINAL_SIZE_BYTES: 2 * SIZE,
                    utils_lib.TRIMMED_SIZE_BYTES: SIZE}},
         {'folder_not_overwritten':
            {utils_lib.ORIGINAL_SIZE_BYTES: SIZE,
             utils_lib.DIRS: [
               {'file1': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}}]
            }},
         {'file_to_be_overwritten':
           {utils_lib.ORIGINAL_SIZE_BYTES: SIZE,
            utils_lib.COLLECTED_SIZE_BYTES: 2 * SIZE,
            utils_lib.TRIMMED_SIZE_BYTES: SIZE,
            utils_lib.DIRS: [
              {'file1': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}}]
           }},
         {'file3': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}},
         {'folder1': {utils_lib.ORIGINAL_SIZE_BYTES: 20 * SIZE,
                     utils_lib.TRIMMED_SIZE_BYTES: SIZE,
                     utils_lib.DIRS: [
                         {'file4': {utils_lib.ORIGINAL_SIZE_BYTES: 20 * SIZE,
                                   utils_lib.TRIMMED_SIZE_BYTES: SIZE}
                         }]
                     }},
         {'folder_tobe_deleted':
           {utils_lib.ORIGINAL_SIZE_BYTES: SIZE,
            utils_lib.COLLECTED_SIZE_BYTES: SIZE,
            utils_lib.TRIMMED_SIZE_BYTES: 0,
            utils_lib.DIRS: [
              {'file_tobe_deleted': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE,
                                    utils_lib.COLLECTED_SIZE_BYTES: SIZE,
                                    utils_lib.TRIMMED_SIZE_BYTES: 0}}]
           }},
         {'folder3': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE + SUMMARY_3_SIZE,
                     utils_lib.DIRS: [
                       {'folder31': {
                         utils_lib.ORIGINAL_SIZE_BYTES: SIZE + SUMMARY_3_SIZE,
                         utils_lib.DIRS: [
                             {'file10': {utils_lib.ORIGINAL_SIZE_BYTES: SIZE}},
                             {'dir_summary_3.json': {
                               utils_lib.ORIGINAL_SIZE_BYTES: SUMMARY_3_SIZE}},
                            ]}},
                       ]
                     }},
         {'dir_summary_1.json': {
           utils_lib.ORIGINAL_SIZE_BYTES: SUMMARY_1_SIZE}},
         {'dir_summary_2.json': {
           utils_lib.ORIGINAL_SIZE_BYTES: SUMMARY_2_SIZE}},
         {'folder2': {utils_lib.ORIGINAL_SIZE_BYTES: 10 * SIZE,
                     utils_lib.DIRS: [
                         {'server_file': {
                           utils_lib.ORIGINAL_SIZE_BYTES: 10 * SIZE}
                         }]
                     }},
        ]
      }
  }


class GetDirSummaryTest(unittest.TestCase):
    """Test class for ResultInfo.build_from_path method"""

    def setUp(self):
        """Setup directory for test."""
        self.test_dir = tempfile.mkdtemp()
        file1 = os.path.join(self.test_dir, 'file1')
        unittest_lib.create_file(file1)
        folder1 = os.path.join(self.test_dir, 'folder1')
        os.mkdir(folder1)
        file2 = os.path.join(folder1, 'file2')
        unittest_lib.create_file(file2)
        file3 = os.path.join(folder1, 'file3')
        unittest_lib.create_file(file3)

        folder2 = os.path.join(self.test_dir, 'folder2')
        os.mkdir(folder2)
        file4 = os.path.join(folder2, 'file2')
        unittest_lib.create_file(file4)

        symlink = os.path.join(folder1, 'symlink')
        os.symlink(folder2, symlink)

    def tearDown(self):
        """Cleanup the test directory."""
        shutil.rmtree(self.test_dir, ignore_errors=True)

    def test_BuildFromPath(self):
        """Test method ResultInfo.build_from_path."""
        summary = result_info.ResultInfo.build_from_path(self.test_dir)
        self.assertEqual(EXPECTED_SUMMARY, summary)


class MergeSummaryTest(unittest.TestCase):
    """Test class for merge_summaries method"""

    def setUp(self):
        """Setup directory to match the file structure in MERGED_SUMMARY."""
        self.test_dir = tempfile.mkdtemp() + '/'
        file1 = os.path.join(self.test_dir, 'file1')
        unittest_lib.create_file(file1)
        file2 = os.path.join(self.test_dir, 'file2')
        unittest_lib.create_file(file2, 2*SIZE)
        file3 = os.path.join(self.test_dir, 'file3')
        unittest_lib.create_file(file3, SIZE)
        file4 = os.path.join(self.test_dir, 'file4')
        unittest_lib.create_file(file4, SIZE)
        folder1 = os.path.join(self.test_dir, 'folder1')
        os.mkdir(folder1)
        file4 = os.path.join(folder1, 'file4')
        unittest_lib.create_file(file4, SIZE)

        # Used to test summary in subdirectory.
        folder3 = os.path.join(self.test_dir, 'folder3')
        os.mkdir(folder3)
        folder31 = os.path.join(folder3, 'folder31')
        os.mkdir(folder31)
        file10 = os.path.join(folder31, 'file10')
        unittest_lib.create_file(file10, SIZE)

        folder2 = os.path.join(self.test_dir, 'folder2')
        os.mkdir(folder2)
        server_file = os.path.join(folder2, 'server_file')
        unittest_lib.create_file(server_file, 10*SIZE)
        folder_not_overwritten = os.path.join(
                self.test_dir, 'folder_not_overwritten')
        os.mkdir(folder_not_overwritten)
        file1 = os.path.join(folder_not_overwritten, 'file1')
        unittest_lib.create_file(file1)
        file_to_be_overwritten = os.path.join(
                self.test_dir, 'file_to_be_overwritten')
        os.mkdir(file_to_be_overwritten)
        file1 = os.path.join(file_to_be_overwritten, 'file1')
        unittest_lib.create_file(file1)

        # Save summary file to test_dir
        self.summary_1 = os.path.join(self.test_dir, 'dir_summary_1.json')
        with open(self.summary_1, 'w') as f:
            json.dump(SUMMARY_1, f)
        # Wait for 10ms, to make sure summary_2 has a later time stamp.
        time.sleep(0.01)
        self.summary_2 = os.path.join(self.test_dir, 'dir_summary_2.json')
        with open(self.summary_2, 'w') as f:
            json.dump(SUMMARY_2, f)
        time.sleep(0.01)
        self.summary_3 = os.path.join(self.test_dir, 'folder3', 'folder31',
                                      'dir_summary_3.json')
        with open(self.summary_3, 'w') as f:
            json.dump(SUMMARY_3, f)

    def tearDown(self):
        """Cleanup the test directory."""
        shutil.rmtree(self.test_dir, ignore_errors=True)

    def testMergeSummaries(self):
        """Test method merge_summaries."""
        collected_bytes, merged_summary, files = result_utils.merge_summaries(
                self.test_dir)

        self.assertEqual(EXPECTED_MERGED_SUMMARY, merged_summary)
        self.assertEqual(collected_bytes, 12 * SIZE)
        self.assertEqual(len(files), 3)

    def testMergeSummariesFromNoHistory(self):
        """Test method merge_summaries can handle results with no existing
        summary.
        """
        os.remove(self.summary_1)
        os.remove(self.summary_2)
        os.remove(self.summary_3)
        client_collected_bytes, _, _ = result_utils.merge_summaries(
                self.test_dir)
        self.assertEqual(client_collected_bytes, 0)

    def testBuildView(self):
        """Test build method in result_view module."""
        client_collected_bytes, summary, _ = result_utils.merge_summaries(
                self.test_dir)
        html_file = os.path.join(self.test_dir,
                                 result_view.DEFAULT_RESULT_SUMMARY_NAME)
        result_view.build(client_collected_bytes, summary, html_file)
        # Make sure html_file is created with content.
        self.assertGreater(os.stat(html_file).st_size, 1000)


# Not throttled.
EXPECTED_THROTTLED_SUMMARY_NO_THROTTLE = {
  '': {utils_lib.ORIGINAL_SIZE_BYTES: 3 * LARGE_SIZE + 5 * SMALL_SIZE,
       utils_lib.DIRS: [
           {'files_to_dedupe': {
               utils_lib.ORIGINAL_SIZE_BYTES: 5 * SMALL_SIZE,
               utils_lib.DIRS: [
                   {'file_0.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE}},
                   {'file_1.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE}},
                   {'file_2.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE}},
                   {'file_3.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE}},
                   {'file_4.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE}},
                ]
            }},
           {'files_to_delete': {
               utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE,
               utils_lib.DIRS: [
                   {'file.png': {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE}},
                ]
            }},
           {'files_to_shink': {
               utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE,
               utils_lib.DIRS: [
                   {'file.txt': {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE}},
                ]
            }},
           {'files_to_zip': {
               utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE,
               utils_lib.DIRS: [
                   {'file.xml': {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE}},
                ]
            }},
        ]
       }
    }

SHRINK_SIZE = shrink_file_throttler.DEFAULT_FILE_SIZE_LIMIT_BYTE
EXPECTED_THROTTLED_SUMMARY_WITH_SHRINK = {
  '': {utils_lib.ORIGINAL_SIZE_BYTES: 3 * LARGE_SIZE + 5 * SMALL_SIZE,
       utils_lib.TRIMMED_SIZE_BYTES:
            2 * LARGE_SIZE + 5 * SMALL_SIZE + SHRINK_SIZE,
       utils_lib.DIRS: [
           {'files_to_dedupe': {
               utils_lib.ORIGINAL_SIZE_BYTES: 5 * SMALL_SIZE,
               utils_lib.DIRS: [
                   {'file_0.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE}},
                   {'file_1.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE}},
                   {'file_2.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE}},
                   {'file_3.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE}},
                   {'file_4.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE}},
                ]
            }},
           {'files_to_delete': {
               utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE,
               utils_lib.DIRS: [
                   {'file.png': {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE}},
                ]
            }},
           {'files_to_shink': {
               utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE,
               utils_lib.TRIMMED_SIZE_BYTES: SHRINK_SIZE,
               utils_lib.DIRS: [
                   {'file.txt': {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE,
                                 utils_lib.TRIMMED_SIZE_BYTES: SHRINK_SIZE}},
                ]
            }},
           {'files_to_zip': {
               utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE,
               utils_lib.DIRS: [
                   {'file.xml': {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE}},
                ]
            }},
        ]
       }
    }

EXPECTED_THROTTLED_SUMMARY_WITH_DEDUPE = {
  '': {utils_lib.ORIGINAL_SIZE_BYTES: 3 * LARGE_SIZE + 5 * SMALL_SIZE,
       utils_lib.TRIMMED_SIZE_BYTES:
            2 * LARGE_SIZE + 3 * SMALL_SIZE + SHRINK_SIZE,
       utils_lib.DIRS: [
           {'files_to_dedupe': {
               utils_lib.ORIGINAL_SIZE_BYTES: 5 * SMALL_SIZE,
               utils_lib.TRIMMED_SIZE_BYTES: 3 * SMALL_SIZE,
               utils_lib.DIRS: [
                   {'file_0.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE}},
                   {'file_1.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE}},
                   {'file_2.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE,
                                   utils_lib.TRIMMED_SIZE_BYTES: 0}},
                   {'file_3.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE,
                                   utils_lib.TRIMMED_SIZE_BYTES: 0}},
                   {'file_4.dmp': {utils_lib.ORIGINAL_SIZE_BYTES: SMALL_SIZE}},
                ]
            }},
           {'files_to_delete': {
               utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE,
               utils_lib.DIRS: [
                   {'file.png': {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE}},
                ]
            }},
           {'files_to_shink': {
               utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE,
               utils_lib.TRIMMED_SIZE_BYTES: SHRINK_SIZE,
               utils_lib.DIRS: [
                   {'file.txt': {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE,
                                 utils_lib.TRIMMED_SIZE_BYTES: SHRINK_SIZE}},
                ]
            }},
           {'files_to_zip': {
               utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE,
               utils_lib.DIRS: [
                   {'file.xml': {utils_lib.ORIGINAL_SIZE_BYTES: LARGE_SIZE}},
                ]
            }},
        ]
       }
    }


class ThrottleTest(unittest.TestCase):
    """Test class for _throttle_results method"""

    def setUp(self):
        """Setup directory to match the file structure in MERGED_SUMMARY."""
        self.test_dir = tempfile.mkdtemp()

        folder = os.path.join(self.test_dir, 'files_to_shink')
        os.mkdir(folder)
        file1 = os.path.join(folder, 'file.txt')
        unittest_lib.create_file(file1, LARGE_SIZE)

        folder = os.path.join(self.test_dir, 'files_to_zip')
        os.mkdir(folder)
        file1 = os.path.join(folder, 'file.xml')
        unittest_lib.create_file(file1, LARGE_SIZE)

        folder = os.path.join(self.test_dir, 'files_to_delete')
        os.mkdir(folder)
        file1 = os.path.join(folder, 'file.png')
        unittest_lib.create_file(file1, LARGE_SIZE)

        folder = os.path.join(self.test_dir, 'files_to_dedupe')
        os.mkdir(folder)
        for i in range(5):
            time.sleep(0.01)
            file1 = os.path.join(folder, 'file_%d.dmp' % i)
            unittest_lib.create_file(file1, SMALL_SIZE)

    def tearDown(self):
        """Cleanup the test directory."""
        shutil.rmtree(self.test_dir, ignore_errors=True)

    def testThrottleResults(self):
        """Test _throttle_results method."""
        summary = result_info.ResultInfo.build_from_path(self.test_dir)
        result_utils._throttle_results(summary, LARGE_SIZE * 10 / 1024)
        self.assertEqual(EXPECTED_THROTTLED_SUMMARY_NO_THROTTLE, summary)

        result_utils._throttle_results(summary, LARGE_SIZE * 3 / 1024)
        self.assertEqual(EXPECTED_THROTTLED_SUMMARY_WITH_SHRINK, summary)

    def testThrottleResults_Dedupe(self):
        """Test _throttle_results method with dedupe triggered."""
        # Change AUTOTEST_LOG_PATTERN to protect file.xml from being compressed
        # before deduping kicks in.
        old_pattern = throttler_lib.AUTOTEST_LOG_PATTERN
        throttler_lib.AUTOTEST_LOG_PATTERN = '.*/file.xml'
        try:
            summary = result_info.ResultInfo.build_from_path(self.test_dir)
            result_utils._throttle_results(
                    summary, (2*LARGE_SIZE + 3*SMALL_SIZE + SHRINK_SIZE) / 1024)
            self.assertEqual(EXPECTED_THROTTLED_SUMMARY_WITH_DEDUPE, summary)
        finally:
            throttler_lib.AUTOTEST_LOG_PATTERN = old_pattern

    def testThrottleResults_Zip(self):
        """Test _throttle_results method with dedupe triggered."""
        summary = result_info.ResultInfo.build_from_path(self.test_dir)
        result_utils._throttle_results(
                summary, (LARGE_SIZE + 3*SMALL_SIZE + SHRINK_SIZE) / 1024 + 2)
        self.assertEqual(
                3 * LARGE_SIZE + 5 * SMALL_SIZE, summary.original_size)

        entry = summary.get_file('files_to_zip').get_file('file.xml.tgz')
        self.assertEqual(LARGE_SIZE, entry.original_size)
        self.assertTrue(LARGE_SIZE > entry.trimmed_size)

        # The compressed file size should be less than 2 KB.
        self.assertTrue(
                summary.trimmed_size <
                (LARGE_SIZE + 3*SMALL_SIZE + SHRINK_SIZE + 2 * 1024))
        self.assertTrue(
                summary.trimmed_size >
                (LARGE_SIZE + 3*SMALL_SIZE + SHRINK_SIZE))

    def testThrottleResults_Delete(self):
        """Test _throttle_results method with delete triggered."""
        summary = result_info.ResultInfo.build_from_path(self.test_dir)
        result_utils._throttle_results(
                summary, (3*SMALL_SIZE + SHRINK_SIZE) / 1024 + 2)

        # Confirm the original size is preserved.
        self.assertEqual(3 * LARGE_SIZE + 5 * SMALL_SIZE, summary.original_size)

        # Confirm the deduped, zipped and shrunk files are not deleted.
        # The compressed file is at least 512 bytes.
        self.assertTrue(
                3 * SMALL_SIZE + SHRINK_SIZE + 512 < summary.original_size)

        # Confirm the file to be zipped is compressed and not deleted.
        entry = summary.get_file('files_to_zip').get_file('file.xml.tgz')
        self.assertEqual(LARGE_SIZE, entry.original_size)
        self.assertTrue(LARGE_SIZE > entry.trimmed_size)
        self.assertTrue(entry.trimmed_size > 0)

        # Confirm the file to be deleted is removed.
        entry = summary.get_file('files_to_delete').get_file('file.png')
        self.assertEqual(0, entry.trimmed_size)
        self.assertEqual(LARGE_SIZE, entry.original_size)


# this is so the test can be run in standalone mode
if __name__ == '__main__':
    """Main"""
    unittest.main()
