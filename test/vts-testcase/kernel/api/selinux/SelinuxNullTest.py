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
from vts.testcases.kernel.api.selinux import KernelSelinuxFileTestBase
from vts.utils.python.file import target_file_utils


class SelinuxNull(KernelSelinuxFileTestBase.KernelSelinuxFileTestBase):
    """Validate /sys/fs/selinux/null permissions.

    The file should be read/write; there are no tests for content format.
    """

    def get_path(self):
        return "/sys/fs/selinux/null"

    def get_permission_checker(self):
        """Gets the function handle to validate r/w file permissions."""
        return target_file_utils.IsReadWrite
