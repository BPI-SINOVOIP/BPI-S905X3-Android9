# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for helper methods related to ResultInfo.
"""

import os


def _get_file_stat(path):
    """Get the os.stat of the file at the given path.

    @param path: Path to the file.
    @return: os.stat of the file. Return None if file doesn't exist.
    """
    try:
        return os.stat(path)
    except OSError:
        # File was deleted already.
        return None


def get_file_size(path):
    """Get the size of the file in bytes for the given path.

    @param path: Path to the file.
    @return: Size in bytes for the given file. Return 0 if file doesn't exist.
    """
    stat = _get_file_stat(path)
    return stat.st_size if stat else 0


def get_last_modification_time(path):
    """Get the last modification time for the given path.

    @param path: Path to the file.
    @return: The last modification time of the given file as a unix timestamp
            int, e.g., 1497896071
    """
    stat = _get_file_stat(path)
    return stat.st_mtime if stat else 0
