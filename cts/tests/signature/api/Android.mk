# Copyright (C) 2015 The Android Open Source Project
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

# We define this in a subdir so that it won't pick up the parent's Android.xml by default.

LOCAL_PATH := $(call my-dir)

# $(1) name of the xml file to be created
# $(2) path to the api text file
define build_xml_api_file
include $(CLEAR_VARS)
LOCAL_MODULE := cts-$(subst .,-,$(1))
LOCAL_MODULE_STEM := $(1)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH = $(TARGET_OUT_DATA_ETC)
LOCAL_COMPATIBILITY_SUITE := arcts cts vts general-tests
include $(BUILD_SYSTEM)/base_rules.mk
$$(LOCAL_BUILT_MODULE): $(2) | $(APICHECK)
	@echo "Convert API file $$< -> $$@"
	@mkdir -p $$(dir $$@)
	$(hide) $(APICHECK_COMMAND) -convert2xmlnostrip $$< $$@
endef

# NOTE: the output XML file is also used
# in //cts/hostsidetests/devicepolicy/AndroidTest.xml
# by com.android.cts.managedprofile.CurrentApiHelper
# ============================================================
$(eval $(call build_xml_api_file,current.api,frameworks/base/api/current.txt))
$(eval $(call build_xml_api_file,system-current.api,frameworks/base/api/system-current.txt))
$(eval $(call build_xml_api_file,system-removed.api,frameworks/base/api/system-removed.txt))
$(eval $(call build_xml_api_file,android-test-base-current.api,frameworks/base/test-base/api/android-test-base-current.txt))
$(eval $(call build_xml_api_file,android-test-mock-current.api,frameworks/base/test-mock/api/android-test-mock-current.txt))
$(eval $(call build_xml_api_file,android-test-runner-current.api,frameworks/base/test-runner/api/android-test-runner-current.txt))
$(foreach ver,$(PLATFORM_SYSTEMSDK_VERSIONS),\
  $(if $(call math_is_number,$(ver)),\
    $(eval $(call build_xml_api_file,system-$(ver).api,prebuilts/sdk/system-api/$(ver).txt))\
  )\
)

# current apache-http-legacy minus current api, in text format.
# =============================================================
# Removes any classes from the org.apache.http.legacy API description that are
# also part of the Android API description.
include $(CLEAR_VARS)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_ETC)

# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests

LOCAL_MODULE := cts-apache-http-legacy-minus-current-api
LOCAL_MODULE_STEM := apache-http-legacy-minus-current.api

include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE) : \
        frameworks/base/api/current.txt \
        external/apache-http/api/apache-http-legacy-current.txt \
        | $(APICHECK)
	@echo "Generate unique Apache Http Legacy API file -> $@"
	@mkdir -p $(dir $@)
	$(hide) $(APICHECK_COMMAND) -new_api_no_strip \
	        frameworks/base/api/current.txt \
            external/apache-http/api/apache-http-legacy-current.txt \
            $@
