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

cts_out_dir := $(HOST_OUT)/cts/android-cts/testcases
$(call dist-for-goals,cts,$(cts_out_dir)/CtsSecureElementAccessControlTestCases1.apk)
$(call dist-for-goals,cts,$(cts_out_dir)/CtsSecureElementAccessControlTestCases2.apk)
$(call dist-for-goals,cts,$(cts_out_dir)/CtsSecureElementAccessControlTestCases3.apk)
include $(call all-subdir-makefiles)

