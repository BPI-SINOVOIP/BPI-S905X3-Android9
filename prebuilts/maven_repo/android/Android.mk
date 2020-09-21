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

include $(CLEAR_VARS)
LOCAL_MODULE := apptoolkit-arch-core-common
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := android/arch/core/common/1.0.0/common-1.0.0.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
LOCAL_SDK_VERSION := 8
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := apptoolkit-arch-core-runtime
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := android/arch/core/runtime/1.0.0/runtime-1.0.0.aar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
LOCAL_SDK_VERSION := 8
LOCAL_USE_AAPT2 := true
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := apptoolkit-lifecycle-common
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := android/arch/lifecycle/common/1.0.3/common-1.0.3.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
LOCAL_SDK_VERSION := 8
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := apptoolkit-lifecycle-runtime
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := android/arch/lifecycle/runtime/1.0.3/runtime-1.0.3.aar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
LOCAL_SDK_VERSION := 8
LOCAL_USE_AAPT2 := true
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := apptoolkit-lifecycle-extensions
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := android/arch/lifecycle/extensions/1.0.0/extensions-1.0.0.aar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
LOCAL_SDK_VERSION := 8
LOCAL_USE_AAPT2 := true
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := apptoolkit-lifecycle-common-java8
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := android/arch/lifecycle/common-java8/1.0.0/common-java8-1.0.0.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
LOCAL_SDK_VERSION := 8
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)
