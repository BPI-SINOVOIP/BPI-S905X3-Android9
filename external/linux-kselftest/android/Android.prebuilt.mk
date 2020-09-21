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

include $(CLEAR_VARS)

module_name := kselftest_$(subst /,_,$(module_prebuilt))
module_stem := $(notdir $(module_src_files))
module_path := $(dir $(module_src_files))

LOCAL_MODULE := $(module_name)
LOCAL_INSTALLED_MODULE_STEM := $(module_stem)
LOCAL_PREBUILT_MODULE_FILE := $(LOCAL_PATH)/tools/testing/selftests/$(module_src_files)
LOCAL_MODULE_RELATIVE_PATH := linux-kselftest/$(module_path)
LOCAL_MODULE_CLASS := NATIVE_TESTS
LOCAL_MULTILIB := both

include $(BUILD_PREBUILT)

module_name :=
module_stem :=
module_path :=
