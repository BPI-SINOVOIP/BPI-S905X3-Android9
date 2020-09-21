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
include $(CLEAR_VARS)

# Intermediate target that produces classes-only jar

LOCAL_MODULE := robolectric_android-all-stub

LOCAL_STATIC_JAVA_LIBRARIES := \
    conscrypt \
    core-libart \
    ext \
    framework \
    icu4j-icudata-jarjar \
    icu4j-icutzdata-jarjar \
    ims-common \
    android.test.base \
    libphonenumber-platform \
    okhttp \
    services \
    services.accessibility \
    telephony-common

# include the uncompiled/raw resources in the jar
# Eventually these raw resources will be removed once the transition to
# binary/compiled resources is complete.
LOCAL_JAVA_RESOURCE_FILES := \
    frameworks/base/core/res/assets \
    frameworks/base/core/res/res

include $(BUILD_STATIC_JAVA_LIBRARY)

# Copy the tzdata, preserving its path.
$(LOCAL_INTERMEDIATE_TARGETS): $(call copy-many-files,\
    system/timezone/output_data/iana/tzdata:$(intermediates.COMMON)/usr/share/zoneinfo/tzdata \
    system/timezone/output_data/android/tzlookup.xml:$(intermediates.COMMON)/usr/share/zoneinfo/tzlookup.xml)
$(LOCAL_INTERMEDIATE_TARGETS): PRIVATE_EXTRA_JAR_ARGS += \
    -C "$(intermediates.COMMON)" "usr/share/zoneinfo"

# Copy the build.prop
$(LOCAL_INTERMEDIATE_TARGETS): $(call copy-many-files,\
    $(TARGET_OUT)/build.prop:$(intermediates.COMMON)/build.prop)
$(LOCAL_INTERMEDIATE_TARGETS): PRIVATE_EXTRA_JAR_ARGS += \
    -C "$(intermediates.COMMON)" "build.prop"

########################################

include $(CLEAR_VARS)

# Adds binary framework resources to the produced jar
robo_stub_module_name := robolectric_android-all-stub
include $(LOCAL_PATH)/include_framework_res.mk

# Distribute the android-all artifact with SDK artifacts.
$(call dist-for-goals,sdk win_sdk,\
    $(robo_full_target):android-all-$(PLATFORM_VERSION)-robolectric-$(FILE_NAME_TAG).jar)
