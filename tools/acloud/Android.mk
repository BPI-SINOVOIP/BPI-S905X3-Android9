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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := acloud.zip
LOCAL_MODULE_PATH := $(HOST_OUT)/tools
LOCAL_MODULE_CLASS := EXECUTABLE
LOCAL_IS_HOST_MODULE := true

include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE): $(SOONG_ZIP)
	$(hide) mkdir -p $(dir $@)
	@rm -f $@.list
        # ! used because setup.py needs to reside in the same directory
        # as acloud package for proper installation.
	$(hide) find tools/acloud ! -name '*setup.py' -name '*.py' -or -name \
	'*.config' -or -name 'LICENSE' -or -name '*.md' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C tools/ -l $@.list -C tools/acloud \
	-f tools/acloud/setup.py
	@rm -f $@.list

.PHONY: acloud
acloud: $(LOCAL_MODULE)
