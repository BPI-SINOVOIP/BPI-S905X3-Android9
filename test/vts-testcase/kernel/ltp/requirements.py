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

from vts.testcases.kernel.ltp import ltp_enums
from vts.testcases.kernel.ltp import ltp_configs
from vts.testcases.kernel.ltp.shell_environment.definitions import directory_exists
from vts.testcases.kernel.ltp.shell_environment.definitions import loop_device_support
from vts.testcases.kernel.ltp.shell_environment.definitions import path_permission
from vts.testcases.kernel.ltp.shell_environment.definitions import bin_in_path


def GetRequrementDefinitions():
    """Get requirement definition objects.

    Get a dictionary in which keys are requirement names and
    values are corresponding definition class object or a list
    of such objects.
    """
    return {
        ltp_enums.Requirements.LOOP_DEVICE_SUPPORT:
            loop_device_support.LoopDeviceSupport(),
        ltp_enums.Requirements.LTP_TMP_DIR: [
            directory_exists.DirectoryExists(
                paths=[ltp_configs.TMP, ltp_configs.TMPBASE,
                       ltp_configs.LTPTMP, ltp_configs.TMPDIR],
                to_check=False,
                to_setup=True,
                to_cleanup=True), path_permission.PathPermission(
                    paths=[ltp_configs.TMP, ltp_configs.TMPBASE,
                           ltp_configs.LTPTMP, ltp_configs.TMPDIR],
                    permissions=777,
                    to_check=False,
                    to_setup=True,
                    to_cleanup=False)
        ],
        ltp_enums.Requirements.BIN_IN_PATH_LDD:
            bin_in_path.BinInPath(paths='ldd'),
    }
