#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
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

"""This module holds constants used by the driver."""
BRANCH_PREFIX = "git_"
BUILD_TARGET_MAPPING = {
    "phone": "gce_x86_phone-userdebug",
    "tablet": "gce_x86_tablet-userdebug",
    "tablet_mobile": "gce_x86_tablet_mobile-userdebug",
    "aosp_phone": "aosp_gce_x86_phone-userdebug",
    "aosp_tablet": "aosp_gce_x86_tablet-userdebug",
}
SPEC_NAMES = {"nexus5", "nexus6", "nexus7_2012", "nexus7_2013", "nexus9",
              "nexus10"}

DEFAULT_SERIAL_PORT = 1
LOGCAT_SERIAL_PORT = 2
