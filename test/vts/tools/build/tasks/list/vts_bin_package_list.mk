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

vts_bin_packages := \
  vts_hal_agent \
  vts_hal_driver \
  vts_hal_replayer \
  vts_shell_driver \
  vts_profiling_configure \
  vts_coverage_configure \
  vts_testability_checker \

# Extra apk utils for VTS framework.
vts_bin_packages += \
    WifiUtil \
