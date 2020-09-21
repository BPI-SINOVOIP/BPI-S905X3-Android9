# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Help functions used by different throttlers."""

import os
import re

import utils_lib


# A list of file names that should not be throttled, that is, not modified by
# deletion, trimming or compression.
NON_THROTTLEABLE_FILE_NAMES = set([
        '.autoserv_execute',
        '.parse.lock',
        '.parse.log',
        '.parser_execute',
        'control',
        'control.srv',
        'host_keyvals',
        'job_report.html',
        'keyval',
        'profiling',
        'result_summary.html',
        'sponge_invocation.xml',
        'status',
        'status.log',

        # ACTS related files:
        'test_run_details.txt',
        'test_run_error.txt',
        'test_run_info.txt',
        'test_run_summary.json',
        ])

# A list of file name patterns that should not be throttled, that is, not
# modified by deletion, deduping, trimming or compression.
NON_THROTTLEABLE_FILE_PATTERNS = [
        '.*/BUILD_INFO-.*',   # ACTS test result files.
        '.*/AndroidDevice.*', # ACTS test result files.
        ]

# Regex of result files sorted based on priority. Files can be throttled first
# have higher priority.
RESULT_THROTTLE_PRIORITY = [
        '(.*/)?sysinfo/var/log/.*',
        '(.*/)?sysinfo/var/log_diff/.*',
        '(.*/)?sysinfo/.*',
        # The last one matches any file.
        '.*',
        ]

# Regex of file names for Autotest debug logs. These files should be preserved
# without throttling if possible.
AUTOTEST_LOG_PATTERN ='.*\.(DEBUG|ERROR|INFO|WARNING)$'

def _list_files(files, all_files=None):
    """Get all files in the given directories.

    @param files: A list of ResultInfo objects for files in a directory.
    @param all_files: A list of ResultInfo objects collected so far.
    @return: A list of all collected ResultInfo objects.
    """
    if all_files is None:
        all_files = []
    for info in files:
        if info.is_dir:
            _list_files(info.files, all_files)
        else:
            all_files.append(info)
    return all_files


def sort_result_files(summary):
    """Sort result infos based on priority.

    @param summary: A ResultInfo object containing result summary.
    @return: A tuple of (sorted_files, grouped_files)
            sorted_files: A list of ResultInfo, sorted based on file size and
                priority based on RESULT_THROTTLE_PRIORITY.
            grouped_files: A dictionary of ResultInfo grouped by each item in
                RESULT_THROTTLE_PRIORITY.
    """
    all_files = _list_files(summary.files)

    # Scan all file paths and group them based on the throttle priority.
    grouped_files = {pattern: [] for pattern in RESULT_THROTTLE_PRIORITY}
    for info in all_files:
        for pattern in RESULT_THROTTLE_PRIORITY:
            if re.match(pattern, info.path):
                grouped_files[pattern].append(info)
                break

    sorted_files = []
    for pattern in RESULT_THROTTLE_PRIORITY:
        # Sort the files in each group by file size, largest first.
        infos = grouped_files[pattern]
        infos.sort(key=lambda info: -info.trimmed_size)
        sorted_files.extend(infos)

    return sorted_files, grouped_files


def get_throttleable_files(file_infos, extra_patterns=[]):
    """Filter the files can be throttled.

    @param file_infos: A list of ResultInfo objects.
    @param extra_patterns: Extra patterns of file path that should not be
            throttled.
    @yield: ResultInfo objects that can be throttled.
    """
    for info in file_infos:
        # Skip the files being deleted in earlier throttling.
        if info.trimmed_size == 0:
            continue
        if info.name in NON_THROTTLEABLE_FILE_NAMES:
            continue
        pattern_matched = False
        for pattern in extra_patterns + NON_THROTTLEABLE_FILE_PATTERNS:
            if re.match(pattern, info.path):
                pattern_matched = True
                break

        if not pattern_matched:
            yield info


def check_throttle_limit(summary, max_result_size_KB):
    """Check if throttling is enough already.

    @param summary: A ResultInfo object containing result summary.
    @param max_result_size_KB: Maximum test result size in KB.
    @return: True if the result directory has been trimmed to be smaller than
            max_result_size_KB.
    """
    if (summary.trimmed_size <= max_result_size_KB * 1024):
        utils_lib.LOG('Maximum result size is reached (current result'
                      'size is %s (limit is %s).' %
                      (utils_lib.get_size_string(summary.trimmed_size),
                       utils_lib.get_size_string(max_result_size_KB * 1024)))
        return True
    else:
        return False


def try_delete_file_on_disk(path):
    """Try to delete the give file on disk.

    @param path: Path to the file.
    @returns: True if the file is deleted, False otherwise.
    """
    try:
        utils_lib.LOG('Deleting file %s.' % path)
        os.remove(path)
        return True
    except OSError as e:
        utils_lib.LOG('Failed to delete file %s, Error: %s' % (path, e))