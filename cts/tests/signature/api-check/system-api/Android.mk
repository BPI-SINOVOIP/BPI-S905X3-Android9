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

LOCAL_PATH := $(call my-dir)

all_system_api_files := system-current.api system-removed.api
$(foreach ver,$(PLATFORM_SYSTEMSDK_VERSIONS),\
  $(if $(call math_is_number,$(ver)),\
    $(eval all_system_api_files += system-$(ver).api)\
  )\
)

include $(CLEAR_VARS)
LOCAL_MODULE := cts-system-all.api
LOCAL_MODULE_STEM := system-all.api.zip
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH = $(TARGET_OUT_DATA_ETC)
LOCAL_COMPATIBILITY_SUITE := arcts cts vts general-tests
include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE): $(addprefix $(COMPATIBILITY_TESTCASES_OUT_cts)/,$(all_system_api_files))
	@echo "Zip API files $^ -> $@"
	@mkdir -p $(dir $@)
	$(hide) rm -f $@
	$(hide) zip -q $@ $^

include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := CtsSystemApiSignatureTestCases

LOCAL_SIGNATURE_API_FILES := \
    current.api \
    android-test-mock-current.api \
    android-test-runner-current.api \
    $(all_sytem_api_files) \
    system-all.api.zip

include $(LOCAL_PATH)/../build_signature_apk.mk

all_system_api_files :=
