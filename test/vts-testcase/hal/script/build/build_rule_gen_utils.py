#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Utility functions for build rule generator."""

import os

def HalNameDir(hal_name):
    """Returns directory name corresponding to hal name."""
    return hal_name.replace('.', '/')


def HalVerDir(hal_version):
    """Returns directory name corresponding to hal version."""
    return "V" + hal_version.replace('.', '_')


def WriteBuildRule(file_path, build_rule):
    """Writes the build rule into specified file.

    Opens file_path and writes build_rule into it. Creates intermediate
    directories if necessary.

    Args:
      file_path: string, path to file to which to write.
      build_rule: string, build rule to be written into file.
    """
    print 'Updating %s' % file_path
    dir_path = os.path.dirname(file_path)

    if not os.path.exists(dir_path):
        os.makedirs(dir_path)

    with open(file_path, 'w') as bp_file:
        bp_file.write(build_rule)


def OnlySubdirsBpRule(warning_header, subdirs):
    """Returns a .bp rule containing only subdirs field.

    For example, 'subdirs = ["*"]' bp rule tells soong to look in all
    sub-directories for Android.bp files.

    Args:
        subdirs: list of sub-directories.
    """
    result = warning_header

    result += 'subdirs = [\n'
    for subdir in subdirs:
        result += '    "%s",\n' % subdir
    result += ']\n'
    return result

def RemoveFilesInDirIf(dir_path, condition):
    """Removes all files under directory under given condition.

    Args:
        dir_path: string, path to directory
        condition: boolean function takes absolute file path,
            returns True iff file needs to be removed.
    """
    for base, _, files in os.walk(dir_path):
        for f in files:
            abs_path = os.path.join(base, f)
            if condition(abs_path):
                os.remove(abs_path)
