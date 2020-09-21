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

LOCAL_PATH:= $(call my-dir)

#-----------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := CtsAppWidgetProvider1

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

LOCAL_SRC_FILES := $(call all-java-files-under, ../src) \
        $(call all-java-files-under, ../../common/src)
LOCAL_FULL_LIBS_MANIFEST_FILES := \
    $(LOCAL_PATH)/AndroidManifestV1.xml

LOCAL_SDK_VERSION := current

# tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests cts_instant

include $(BUILD_CTS_PACKAGE)

#-----------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := CtsAppWidgetProvider2

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

LOCAL_SRC_FILES := $(call all-java-files-under, ../src) \
        $(call all-java-files-under, ../../common/src)
LOCAL_FULL_LIBS_MANIFEST_FILES := \
    $(LOCAL_PATH)/AndroidManifestV2.xml

LOCAL_SDK_VERSION := current

# tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests cts_instant

include $(BUILD_CTS_PACKAGE)

#-----------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := CtsAppWidgetProvider3

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

LOCAL_SRC_FILES := $(call all-java-files-under, ../src) \
        $(call all-java-files-under, ../../common/src)
LOCAL_FULL_LIBS_MANIFEST_FILES := \
    $(LOCAL_PATH)/AndroidManifestV3.xml

LOCAL_SDK_VERSION := current

# tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts vts general-tests cts_instant

include $(BUILD_CTS_PACKAGE)
