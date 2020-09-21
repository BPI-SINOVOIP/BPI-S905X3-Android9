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

LOCAL_PATH := $(call my-dir)

# the cts-api-coverage script
# ============================================================
include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE := cts-api-coverage
LOCAL_SRC_FILES := etc/$(LOCAL_MODULE)
LOCAL_ADDITIONAL_DEPENDENCIES := $(HOST_OUT_JAVA_LIBRARIES)/$(LOCAL_MODULE)$(COMMON_JAVA_PACKAGE_SUFFIX)

include $(BUILD_PREBUILT)

# the ndk-api-report script
# ============================================================
include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE := ndk-api-report
LOCAL_SRC_FILES := etc/$(LOCAL_MODULE)

include $(BUILD_PREBUILT)

# cts-api-coverage java library
# ============================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    $(call all-subdir-java-files) \
    $(call all-proto-files-under, proto)

LOCAL_PROTOC_OPTIMIZE_TYPE := full
LOCAL_PROTOC_FLAGS := --proto_path=$(LOCAL_PATH)/proto/

LOCAL_JAVA_RESOURCE_DIRS := res
LOCAL_JAR_MANIFEST := MANIFEST.mf

LOCAL_STATIC_JAVA_LIBRARIES := \
  compatibility-host-util \
  dexlib2

LOCAL_MODULE := cts-api-coverage

# This tool is not checking any dependencies or metadata, so all of the
# dependencies of all of the tests must be on its classpath. This is
# super fragile.
LOCAL_STATIC_JAVA_LIBRARIES += \
  tradefed hosttestlib \
  platformprotos

include $(BUILD_HOST_JAVA_LIBRARY)
