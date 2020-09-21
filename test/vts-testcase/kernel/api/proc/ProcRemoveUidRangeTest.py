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

from vts.runners.host import const
from vts.testcases.kernel.api.proc import KernelProcFileTestBase
from vts.utils.python.file import target_file_utils


class ProcRemoveUidRangeTest(KernelProcFileTestBase.KernelProcFileTestBase):
    '''/proc/uid_cputime/remove_uid_range can be written in order to remove uids
    from being shown when reading show_uid_stat.

    Format is '[start uid]-[end uid]'

    This is an Android specific file.
    '''

    def test_format(self):
        return False

    def get_permission_checker(self):
        """Get write-only file permission checker.
        """
        return target_file_utils.IsWriteOnly

    def get_path(self):
        return '/proc/uid_cputime/remove_uid_range'
