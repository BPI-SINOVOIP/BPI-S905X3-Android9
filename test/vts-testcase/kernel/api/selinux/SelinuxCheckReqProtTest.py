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


class SelinuxCheckReqProt(KernelSelinuxFileTestBase.KernelSelinuxFileTestBase):
    """Validate /sys/fs/selinux/checkreqprot content and permissions.

    The contents are binary 0/1 and the file should be read/write.
    """

    def get_path(self):
        return "/sys/fs/selinux/checkreqprot"

    def result_correct(self, file_content):
        """Return True if the file contents are simply 0/1.

        Args:
            file_contents: String, the contents of the checkreqprot file

        Returns:
            True if the contents are 0/1, False otherwise.
        """
        return file_content == "0" or file_content == "1"

    def get_permission_checker(self):
        """Gets the function handle to validate r/w file permissions."""
        return target_file_utils.IsReadWrite
