# Copyright 2008, The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

# Build the Email application itself, along with its tests and tests for the emailcommon
# static library.  All tests can be run via runtest email

include $(CLEAR_VARS)

unified_email_dir := ../UnifiedEmail
res_dir := res $(unified_email_dir)/res

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, $(unified_email_dir)/src)
LOCAL_SRC_FILES += $(call all-java-files-under, src/com/android)
LOCAL_SRC_FILES += $(call all-java-files-under, provider_src/com/android)
LOCAL_SRC_FILES += $(call all-java-files-under, src/com/beetstra)

LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, $(res_dir))
LOCAL_USE_AAPT2 := true

# Use assets dir from UnifiedEmail
# (the default package target doesn't seem to deal with multiple asset dirs)
LOCAL_ASSET_DIR := $(LOCAL_PATH)/$(unified_email_dir)/assets

LOCAL_AAPT_FLAGS := --auto-add-overlay
LOCAL_AAPT_FLAGS += --extra-packages com.android.mail:com.android.email:com.android.emailcommon

LOCAL_STATIC_ANDROID_LIBRARIES := \
    com.android.emailcommon \
    libchips \
    libphotoviewer_appcompat \
    android-opt-bitmap \
    android-opt-datetimepicker \
    android-support-compat \
    android-support-media-compat \
    android-support-core-utils \
    android-support-core-ui \
    android-support-fragment \
    android-support-v7-appcompat \
    android-support-v7-gridlayout \
    android-support-v13

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-annotations \
    android-common \
    guava \
    owasp-html-sanitizer

LOCAL_JAVA_LIBRARIES := org.apache.http.legacy

LOCAL_PACKAGE_NAME := Email

LOCAL_PROGUARD_FLAG_FILES := proguard.flags $(unified_email_dir)/proguard.flags
ifeq (eng,$(TARGET_BUILD_VARIANT))
  LOCAL_PROGUARD_FLAG_FILES += proguard-test.flags
endif

LOCAL_SDK_VERSION := current

include $(BUILD_PACKAGE)

# only include rules to build other stuff for the original package, not the derived package.
ifeq ($(strip $(LOCAL_PACKAGE_OVERRIDES)),)
# additionally, build unit tests in a separate .apk
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
