#
# Copyright 2017 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import posixpath


def JoinTargetPath(path, *paths):
    """Concatenates paths and inserts target path separators between them.

    Args:
        path: string, the first path to be concatenated.
        *paths: tuple of strings, the other paths to be concatenated.

    Returns:
        string, the concatenated path.
    """
    return posixpath.join(path, *paths)


def TargetBaseName(path):
    """Returns the base name of the path on target device.

    Args:
        path: string, the path on target device.

    Returns:
        string, the base name.
    """
    return posixpath.basename(path)


def TargetDirName(path):
    """Returns the directory name of the path on target device.

    Args:
        path: string, the path on target device.

    Returns:
        string, the directory name.
    """
    return posixpath.dirname(path)


def TargetNormPath(path):
    """Removes redundant separators and resolves relative path.

    Args:
        path: string, the path on target device.

    Returns:
        string, the normalized path.
    """
    return posixpath.normpath(path)

