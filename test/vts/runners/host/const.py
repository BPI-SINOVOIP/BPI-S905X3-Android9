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
from vts.utils.python.common import cmd_utils

STDOUT = cmd_utils.STDOUT
STDERR = cmd_utils.STDERR
EXIT_CODE = cmd_utils.EXIT_CODE

# Note: filterOneTest method in base_test.py assumes SUFFIX_32BIT and SUFFIX_64BIT
# are in lower cases.
SUFFIX_32BIT = "32bit"
SUFFIX_64BIT = "64bit"

# for toggling hal hidl test passthrough mode
VTS_HAL_HIDL_GET_STUB = 'VTS_HAL_HIDL_GET_STUB'
