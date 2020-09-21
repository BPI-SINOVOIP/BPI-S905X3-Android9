#
# Copyright (C) 2010 The Android Open Source Project
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

LOCAL_PATH:= $(call my-dir)

# $(1): sdk version
define declare_sdk_prebuilts

include $(CLEAR_VARS)
LOCAL_MODULE := sdk_v$(1)
LOCAL_SRC_FILES := $(1)/android.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
LOCAL_BUILT_MODULE_STEM := sdk_v$(1)$(COMMON_JAVA_PACKAGE_SUFFIX)
LOCAL_MIN_SDK_VERSION := $(if $(call math_is_number,$(strip $(1))),$(1),$(PLATFORM_JACK_MIN_SDK_VERSION))
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_SDK_VERSION := current
include $(BUILD_PREBUILT)

ifneq (,$(wildcard $(LOCAL_PATH)/$(1)/uiautomator.jar))
include $(CLEAR_VARS)
LOCAL_MODULE := uiautomator_sdk_v$(1)
LOCAL_SRC_FILES := $(1)/uiautomator.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
LOCAL_BUILT_MODULE_STEM := uiautomator_sdk_v$(1)$(COMMON_JAVA_PACKAGE_SUFFIX)
LOCAL_MIN_SDK_VERSION := $(if $(call math_is_number,$(strip $(1))),$(1),$(PLATFORM_JACK_MIN_SDK_VERSION))
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_SDK_VERSION := $(1)
include $(BUILD_PREBUILT)
endif

endef

$(foreach s,$(filter-out test_current core_current,$(TARGET_AVAILABLE_SDK_VERSIONS)),\
  $(eval $(call declare_sdk_prebuilts,$(s))))

include $(call all-makefiles-under,$(LOCAL_PATH))
