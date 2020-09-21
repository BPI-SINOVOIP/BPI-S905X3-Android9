# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import re

import throttler_lib
import utils_lib


# File extensions that can not be shrunk., as partial content will corrupt the
# file.
UNSHRINKABLE_EXTENSIONS = set([
        '.bin',
        '.data',
        '.dmp',
        '.gz',
        '.htm',
        '.html',
        '.img',
        '.jpg',
        '.json',
        '.png',
        '.tar',
        '.tgz',
        '.xml',
        '.xz',
        '.zip',
        ])

# Regex for files that should not be shrunk.
UNSHRINKABLE_FILE_PATTERNS = [
        ]

TRIMMED_FILE_HEADER = '!!! This file is trimmed !!!\n'
ORIGINAL_SIZE_TEMPLATE = 'Original size: %d bytes\n\n'
# Regex pattern to retrieve the original size of the file.
ORIGINAL_SIZE_REGEX = 'Original size: (\d+) bytes'
TRIMMED_FILE_INJECT_TEMPLATE = """

========================================================================
  < %d > characters are trimmed here.
========================================================================

"""

# Percent of file content to keep at the beginning and end of the file, default
# to 20%.
HEAD_SIZE_PERCENT = 0.20

# Default size in byte to trim the file down to.
DEFAULT_FILE_SIZE_LIMIT_BYTE = 100 * 1024

def _trim_file(file_info, file_size_limit_byte):
    """Remove the file content in the middle to reduce the file size.

    @param file_info: A ResultInfo object containing summary for the file to be
            shrunk.
    @param file_size_limit_byte: Maximum file size in bytes after trimming.
    """
    utils_lib.LOG('Trimming file %s to reduce size from %d bytes to %d bytes' %
                  (file_info.path, file_info.original_size,
                   file_size_limit_byte))
    new_path = os.path.join(os.path.dirname(file_info.path),
                            file_info.name + '_trimmed')
    original_size_bytes = file_info.original_size
    with open(new_path, 'w') as new_file, open(file_info.path) as old_file:
        # Read the beginning part of the old file, if it's already started with
        # TRIMMED_FILE_HEADER, no need to add the header again.
        header =  old_file.read(len(TRIMMED_FILE_HEADER))
        if header != TRIMMED_FILE_HEADER:
            new_file.write(TRIMMED_FILE_HEADER)
            new_file.write(ORIGINAL_SIZE_TEMPLATE % file_info.original_size)
        else:
            line = old_file.readline()
            match = re.match(ORIGINAL_SIZE_REGEX, line)
            if match:
                original_size_bytes = int(match.group(1))
        header_size_bytes = new_file.tell()
        # Move old file reader to the beginning of the file.
        old_file.seek(0, os.SEEK_SET)

        new_file.write(old_file.read(
                int((file_size_limit_byte - header_size_bytes) *
                    HEAD_SIZE_PERCENT)))
        # Position to seek from the end of the file.
        seek_pos = -(file_size_limit_byte - new_file.tell() -
                     len(TRIMMED_FILE_INJECT_TEMPLATE))
        bytes_to_skip = original_size_bytes + seek_pos - old_file.tell()
        # Adjust seek position based on string TRIMMED_FILE_INJECT_TEMPLATE
        seek_pos += len(str(bytes_to_skip)) - 2
        bytes_to_skip = original_size_bytes + seek_pos - old_file.tell()
        new_file.write(TRIMMED_FILE_INJECT_TEMPLATE % bytes_to_skip)
        old_file.seek(seek_pos, os.SEEK_END)
        new_file.write(old_file.read())
    stat = os.stat(file_info.path)
    if not throttler_lib.try_delete_file_on_disk(file_info.path):
        # Clean up the intermediate file.
        throttler_lib.try_delete_file_on_disk(new_path)
        utils_lib.LOG('Failed to shrink %s' % file_info.path)
        return

    os.rename(new_path, file_info.path)
    # Modify the new file's timestamp to the old one.
    os.utime(file_info.path, (stat.st_atime, stat.st_mtime))
    # Update the trimmed_size.
    file_info.trimmed_size = file_info.size


def _get_shrinkable_files(file_infos, file_size_limit_byte):
    """Filter the files that can be throttled.

    @param file_infos: A list of ResultInfo objects.
    @param file_size_limit_byte: Minimum file size in bytes to be throttled.
    @yield: ResultInfo objects that can be shrunk.
    """
    for info in file_infos:
        ext = os.path.splitext(info.name)[1].lower()
        if ext in UNSHRINKABLE_EXTENSIONS:
            continue

        match_found = False
        for pattern in UNSHRINKABLE_FILE_PATTERNS:
            if re.match(pattern, info.name):
                match_found = True
                break
        if match_found:
            continue

        if info.trimmed_size <= file_size_limit_byte:
            continue

        yield info


def throttle(summary, max_result_size_KB,
             file_size_limit_byte=DEFAULT_FILE_SIZE_LIMIT_BYTE,
             skip_autotest_log=False):
    """Throttle the files in summary by trimming file content.

    Stop throttling until all files are processed or the result file size is
    already reduced to be under the given max_result_size_KB.

    @param summary: A ResultInfo object containing result summary.
    @param max_result_size_KB: Maximum test result size in KB.
    @param file_size_limit_byte: Limit each file's size in the summary to be
            under the given threshold, until all files are processed or the
            result size is under the given max_result_size_KB.
    @param skip_autotest_log: True to skip shrink Autotest logs, default is
            False.
    """
    file_infos, _ = throttler_lib.sort_result_files(summary)
    extra_patterns = ([throttler_lib.AUTOTEST_LOG_PATTERN] if skip_autotest_log
                      else [])
    file_infos = throttler_lib.get_throttleable_files(
            file_infos, extra_patterns)
    file_infos = _get_shrinkable_files(file_infos, file_size_limit_byte)
    for info in file_infos:
        _trim_file(info, file_size_limit_byte)

        if throttler_lib.check_throttle_limit(summary, max_result_size_KB):
            return
