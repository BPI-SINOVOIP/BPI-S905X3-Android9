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
LOCAL_MODULE := libctsgputools_jni
LOCAL_SRC_FILES := \
    jni/CtsGpuToolsJniOnLoad.cpp \
    jni/android_gputools_cts_RootlessGpuDebug.cpp
LOCAL_CFLAGS += -std=c++14 -Wall -Werror
LOCAL_SHARED_LIBRARIES := libandroid libvulkan liblog
LOCAL_NDK_STL_VARIANT := c++_static
LOCAL_SDK_VERSION := current
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_MODULE_TAGS := tests
LOCAL_PACKAGE_NAME := CtsGpuToolsRootlessGpuDebugApp-DEBUG
LOCAL_SDK_VERSION := current

# tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts

LOCAL_MULTILIB := both

LOCAL_JNI_SHARED_LIBRARIES := \
libctsgputools_jni

LOCAL_AAPT_FLAGS := \
--rename-manifest-package android.rootlessgpudebug.DEBUG.app \
--debug-mode

include $(call all-makefiles-under,$(LOCAL_PATH))
include $(BUILD_CTS_SUPPORT_PACKAGE)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_MODULE_TAGS := tests
LOCAL_PACKAGE_NAME := CtsGpuToolsRootlessGpuDebugApp-RELEASE
LOCAL_SDK_VERSION := current

# tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts

LOCAL_MULTILIB := both

LOCAL_JNI_SHARED_LIBRARIES := \
libctsgputools_jni \
libVkLayer_nullLayerC

LOCAL_AAPT_FLAGS := \
--rename-manifest-package android.rootlessgpudebug.RELEASE.app

include $(call all-makefiles-under,$(LOCAL_PATH))

include $(BUILD_CTS_SUPPORT_PACKAGE)
