# Copyright 2011, The Android Open Source Project
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

LOCAL_PATH:= $(call my-dir)

##################################################
# Build APK
include $(CLEAR_VARS)

src_dirs := src unified_src
LOCAL_PACKAGE_NAME := UnifiedEmail

LOCAL_STATIC_ANDROID_LIBRARIES := \
    libchips \
    libphotoviewer_appcompat \
    android-support-compat \
    android-support-media-compat \
    android-support-core-utils \
    android-support-core-ui \
    android-support-fragment \
    android-support-v7-appcompat \
    android-support-v7-gridlayout \
    android-support-v13 \
    android-opt-bitmap \
    android-opt-datetimepicker

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-annotations \
    guava \
    android-common \
    owasp-html-sanitizer

LOCAL_SDK_VERSION := current

LOCAL_SRC_FILES := $(call all-java-files-under, $(src_dirs)) \
        $(call all-logtags-files-under, $(src_dirs))

LOCAL_USE_AAPT2 := true

LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res

LOCAL_PROGUARD_FLAG_FILES := proguard.flags
ifeq (eng,$(TARGET_BUILD_VARIANT))
  LOCAL_PROGUARD_FLAG_FILES += proguard-test.flags
endif

LOCAL_JACK_COVERAGE_INCLUDE_FILTER := com.android.mail.*,com.android.emailcommon.*,com.google.android.mail.*

include $(BUILD_PACKAGE)

##################################################
# Build all sub-directories

include $(call all-makefiles-under,$(LOCAL_PATH))
