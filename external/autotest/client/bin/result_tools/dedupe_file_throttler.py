# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This throttler tries to remove the remove repeated files sharing the same
prefix, for example, screenshots or dumps in the same folder. The dedupe logic
does not compare the file content, instead, it sorts the files with the same
prefix and remove files in the middle part.
"""

import os
import re

import result_info_lib
import throttler_lib
import utils_lib


# Number of files to keep for the oldest files.
OLDEST_FILES_TO_KEEP_COUNT = 2
# Number of files to keep for the newest files.
NEWEST_FILES_TO_KEEP_COUNT = 1

# Files with path mathing following patterns should not be deduped.
NO_DEDUPE_FILE_PATTERNS = [
        'debug/.*',
        '.*perf.data$',       # Performance test data.
        '.*/debug/.*',
        '.*dir_summary_\d+.json',
        ]

# regex pattern to get the prefix of a file.
PREFIX_PATTERN = '([a-zA-Z_-]*).*'

def _group_by(file_infos, keys):
    """Group the file infos by the given keys.

    @param file_infos: A list of ResultInfo objects.
    @param keys: A list of names of the attribute to group the file infos by.
    @return: A dictionary of grouped_key: [ResultInfo].
    """
    grouped_infos = {}
    for info in file_infos:
        key_values = []
        for key in keys:
            key_values.append(getattr(info, key))
        grouped_key = os.sep.join(key_values)
        if grouped_key not in grouped_infos:
            grouped_infos[grouped_key] = []
        grouped_infos[grouped_key].append(info)
    return grouped_infos


def _dedupe_files(summary, file_infos, max_result_size_KB):
    """Delete the given file and update the summary.

    @param summary: A ResultInfo object containing result summary.
    @param file_infos: A list of ResultInfo objects to be de-duplicated.
    @param max_result_size_KB: Maximum test result size in KB.
    """
    # Sort file infos based on the modify date of the file.
    file_infos.sort(
            key=lambda f: result_info_lib.get_last_modification_time(f.path))
    file_infos_to_delete = file_infos[
            OLDEST_FILES_TO_KEEP_COUNT:-NEWEST_FILES_TO_KEEP_COUNT]

    for file_info in file_infos_to_delete:
        if throttler_lib.try_delete_file_on_disk(file_info.path):
            file_info.trimmed_size = 0

            if throttler_lib.check_throttle_limit(summary, max_result_size_KB):
                return


def throttle(summary, max_result_size_KB):
    """Throttle the files in summary by de-duplicating files.

    Stop throttling until all files are processed or the result size is already
    reduced to be under the given max_result_size_KB.

    @param summary: A ResultInfo object containing result summary.
    @param max_result_size_KB: Maximum test result size in KB.
    """
    _, grouped_files = throttler_lib.sort_result_files(summary)
    for pattern in throttler_lib.RESULT_THROTTLE_PRIORITY:
        throttable_files = list(throttler_lib.get_throttleable_files(
                grouped_files[pattern], NO_DEDUPE_FILE_PATTERNS))

        for info in throttable_files:
            info.parent_dir = os.path.dirname(info.path)
            info.prefix = re.match(PREFIX_PATTERN, info.name).group(1)

        # Group files for each parent directory
        grouped_infos = _group_by(throttable_files, ['parent_dir', 'prefix'])

        for infos in grouped_infos.values():
            if (len(infos) <=
                OLDEST_FILES_TO_KEEP_COUNT + NEWEST_FILES_TO_KEEP_COUNT):
                # No need to dedupe if the count of file is too few.
                continue

            # Remove files can be deduped
            utils_lib.LOG('De-duplicating files in %s with the same prefix of '
                          '"%s"' % (infos[0].parent_dir, infos[0].prefix))
            #dedupe_file_infos = [i.result_info for i in infos]
            _dedupe_files(summary, infos, max_result_size_KB)

            if throttler_lib.check_throttle_limit(summary, max_result_size_KB):
                return
