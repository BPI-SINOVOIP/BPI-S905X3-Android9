#
# Copyright (C) 2018 The Android Open Source Project
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

from vts.utils.python.android import api

def getSupportedKernels(dut):
    """Returns a list of the supported kernel versions for this
    devices' advertised API level.

    Args:
        dut: The AndroidDevice object corresponding to the device
        under test.

    Returns:
        A list of supported kernel versions.
    """
    api_level = dut.getLaunchApiLevel(strict=False)
    if api_level > api.PLATFORM_API_LEVEL_O_MR1 or api_level == 0:
        return [[4, 4, 107], [4, 9, 84], [4, 14, 42]]
    else:
        return [[3, 18, 0], [4, 4, 0], [4, 9, 0]]
