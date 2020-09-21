#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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
from abc import ABCMeta
from abc import abstractmethod

from vts.utils.python.file import target_file_utils


class KernelSelinuxFileTestBase(object):
    """Abstract test for the formatting of a selinux file.

    Individual files can inherit from this class and define the correct path,
    file content, and permissions.
    """
    __metaclass__ = ABCMeta

    @abstractmethod
    def get_path(self):
        """Return the full path of this selinux file."""
        pass

    def result_correct(self, file_contents):
        """Return True if the file contents are correct.

        Subclasses define the requirements for the selinux file and validate
        that the contents of a file are correct.

        Args:
            file_contents: String, the contents of an selinux file

        Returns:
            True if the contents are correct, False otherwise.
        """
        return True

    def get_permission_checker(self):
        """Gets the function handle to use for validating file permissions.

        Return the function that will check if the permissions are correct.
        By default, return the IsReadOnly function from target_file_utils.

        Returns:
            function which takes one argument (the unix file permission bits
            in octal format) and returns True if the permissions are correct,
            False otherwise.
        """
        return target_file_utils.IsReadOnly
