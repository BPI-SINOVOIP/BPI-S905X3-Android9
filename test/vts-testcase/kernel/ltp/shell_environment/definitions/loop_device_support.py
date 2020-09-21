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

from vts.runners.host import const
from vts.testcases.kernel.ltp.shell_environment.definitions.base_definitions import check_setup_cleanup


class LoopDeviceSupport(check_setup_cleanup.CheckSetupCleanup):
    """Class for checking loopback device support."""
    note = "Kernel does not have loop device support"

    def Check(self):
        return not self.ExecuteShellCommand("ls /dev/loop-control")[const.EXIT_CODE][0]
