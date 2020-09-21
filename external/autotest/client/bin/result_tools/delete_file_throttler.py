# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This throttler reduces result size by deleting files permanently."""

import os

import throttler_lib
import utils_lib


# Default threshold of file size in KB for a file to be qualified for deletion.
DEFAULT_FILE_SIZE_THRESHOLD_BYTE = 1024 * 1024

# Regex for file path that should not be deleted.
NON_DELETABLE_FILE_PATH_PATTERNS = [
        '.*perf.data$',       # Performance test data.
        ]

def _delete_file(file_info):
    """Delete the given file and update the summary.

    @param file_info: A ResultInfo object containing summary for the file to be
            shrunk.
    """
    utils_lib.LOG('Deleting file %s.' % file_info.path)
    try:
        os.remove(file_info.path)
    except OSError as e:
        utils_lib.LOG('Failed to delete file %s Error: %s' %
                      (file_info.path, e))

    # Update the trimmed_size in ResultInfo.
    file_info.trimmed_size = 0


def throttle(summary, max_result_size_KB,
             file_size_threshold_byte=DEFAULT_FILE_SIZE_THRESHOLD_BYTE,
             exclude_file_patterns=[]):
    """Throttle the files in summary by trimming file content.

    Stop throttling until all files are processed or the result size is already
    reduced to be under the given max_result_size_KB.

    @param summary: A ResultInfo object containing result summary.
    @param max_result_size_KB: Maximum test result size in KB.
    @param file_size_threshold_byte: Threshold of file size in byte for a file
            to be qualified for deletion. All qualified files will be deleted,
            until all files are processed or the result size is under the given
            max_result_size_KB.
    @param exclude_file_patterns: A list of regex pattern for files not to be
            throttled. Default is an empty list.
    """
    file_infos, _ = throttler_lib.sort_result_files(summary)
    file_infos = throttler_lib.get_throttleable_files(
            file_infos,
            exclude_file_patterns + NON_DELETABLE_FILE_PATH_PATTERNS)

    for info in file_infos:
        if info.trimmed_size > file_size_threshold_byte:
            _delete_file(info)
            if throttler_lib.check_throttle_limit(summary, max_result_size_KB):
                return
