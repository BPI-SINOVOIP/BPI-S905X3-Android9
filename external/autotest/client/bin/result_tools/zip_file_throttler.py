# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This throttler tries to reduce result size by compress files to tgz file.
"""

import re
import os
import tarfile

import throttler_lib
import utils_lib


# File with extensions that can not be zipped or compression won't reduce file
# size further.
UNZIPPABLE_EXTENSIONS = set([
        '.gz',
        '.jpg',
        '.png',
        '.tgz',
        '.xz',
        '.zip',
        ])

# Regex for files that should not be compressed.
UNZIPPABLE_FILE_PATTERNS = [
        'BUILD_INFO-.*' # ACTS test result files.
        ]

# Default threshold of file size in byte for it to be qualified for compression.
# Files smaller than the threshold will not be compressed.
DEFAULT_FILE_SIZE_THRESHOLD_BYTE = 100 * 1024

def _zip_file(file_info):
    """Zip the file to reduce the file size.

    @param file_info: A ResultInfo object containing summary for the file to be
            shrunk.
    """
    utils_lib.LOG('Compressing file %s' % file_info.path)
    parent_result_info = file_info.parent_result_info
    new_name = file_info.name + '.tgz'
    new_path = os.path.join(os.path.dirname(file_info.path), new_name)
    if os.path.exists(new_path):
        utils_lib.LOG('File %s already exists, removing...' % new_path)
        if not throttler_lib.try_delete_file_on_disk(new_path):
            return
        parent_result_info.remove_file(new_name)
    with tarfile.open(new_path, 'w:gz') as tar:
        tar.add(file_info.path, arcname=os.path.basename(file_info.path))
    stat = os.stat(file_info.path)
    if not throttler_lib.try_delete_file_on_disk(file_info.path):
        # Clean up the intermediate file.
        throttler_lib.try_delete_file_on_disk(new_path)
        utils_lib.LOG('Failed to compress %s' % file_info.path)
        return

    # Modify the new file's timestamp to the old one.
    os.utime(new_path, (stat.st_atime, stat.st_mtime))
    # Get the original file size before compression.
    original_size = file_info.original_size
    parent_result_info.remove_file(file_info.name)
    parent_result_info.add_file(new_name)
    new_file_info = parent_result_info.get_file(new_name)
    # Set the original size to be the size before compression.
    new_file_info.original_size = original_size
    # Set the trimmed size to be the physical file size of the compressed file.
    new_file_info.trimmed_size = new_file_info.size


def _get_zippable_files(file_infos, file_size_threshold_byte):
    """Filter the files that can be throttled.

    @param file_infos: A list of ResultInfo objects.
    @param file_size_threshold_byte: Threshold of file size in byte for it to be
            qualified for compression.
    @yield: ResultInfo objects that can be shrunk.
    """
    for info in file_infos:
        ext = os.path.splitext(info.name)[1].lower()
        if ext in UNZIPPABLE_EXTENSIONS:
            continue

        match_found = False
        for pattern in UNZIPPABLE_FILE_PATTERNS:
            if re.match(pattern, info.name):
                match_found = True
                break
        if match_found:
            continue

        if info.trimmed_size <= file_size_threshold_byte:
            continue

        yield info


def throttle(summary, max_result_size_KB,
             file_size_threshold_byte=DEFAULT_FILE_SIZE_THRESHOLD_BYTE,
             skip_autotest_log=False):
    """Throttle the files in summary by compressing file.

    Stop throttling until all files are processed or the result file size is
    already reduced to be under the given max_result_size_KB.

    @param summary: A ResultInfo object containing result summary.
    @param max_result_size_KB: Maximum test result size in KB.
    @param file_size_threshold_byte: Threshold of file size in byte for it to be
            qualified for compression.
    @param skip_autotest_log: True to skip shrink Autotest logs, default is
            False.
    """
    file_infos, _ = throttler_lib.sort_result_files(summary)
    extra_patterns = ([throttler_lib.AUTOTEST_LOG_PATTERN] if skip_autotest_log
                      else [])
    file_infos = throttler_lib.get_throttleable_files(
            file_infos, extra_patterns)
    file_infos = _get_zippable_files(file_infos, file_size_threshold_byte)
    for info in file_infos:
        _zip_file(info)

        if throttler_lib.check_throttle_limit(summary, max_result_size_KB):
            return
