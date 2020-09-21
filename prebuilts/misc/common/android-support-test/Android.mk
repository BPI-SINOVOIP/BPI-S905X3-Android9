# Copyright (C) 2016 The Android Open Source Project
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

# for Android JUnit runner, monitor and rules
include $(CLEAR_VARS)
LOCAL_MODULE := android-support-test
LOCAL_SDK_VERSION := 15
LOCAL_STATIC_JAVA_LIBRARIES := android-support-test-rules-nodep android-support-test-runner-nodep junit hamcrest hamcrest-library android-support-annotations android-support-test-monitor-nodep
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := android-support-test-nodep
LOCAL_SDK_VERSION := 23
LOCAL_STATIC_JAVA_LIBRARIES := android-support-test-rules-nodep android-support-test-runner-nodep android-support-test-monitor-nodep
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := android-support-test-rules-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := rules/rules_release_no_deps.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
LOCAL_SDK_VERSION := 15
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := android-support-test-runner-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := runner/runner_release_no_deps.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
LOCAL_SDK_VERSION := 15
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := android-support-test-monitor-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := monitor/monitor_release_no_deps.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
LOCAL_SDK_VERSION := 15
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

# for espresso-core
include $(CLEAR_VARS)
LOCAL_MODULE := espresso-core
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := 15
LOCAL_STATIC_JAVA_LIBRARIES := espresso-core-nodep espresso-idling-resource-nodep android-support-test-rules-nodep android-support-test-runner-nodep android-support-test-monitor-nodep junit hamcrest hamcrest-library android-support-annotations jsr330
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := espresso-core-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso_core_release_no_deps.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_SDK_VERSION := 15
include $(BUILD_PREBUILT)

# for espresso-contrib
include $(CLEAR_VARS)
LOCAL_MODULE := espresso-contrib
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := 15
LOCAL_STATIC_JAVA_LIBRARIES := espresso-core android-support-design android-support-v7-recyclerview android-support-v4
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := espresso-contrib-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso_contrib_release_no_deps.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_SDK_VERSION := 15
include $(BUILD_PREBUILT)

# for espresso-idling-resource
include $(CLEAR_VARS)
LOCAL_MODULE := espresso-idling-resource-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso_idling_resource_release_no_deps.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_SDK_VERSION := 15
include $(BUILD_PREBUILT)

# for espresso-intents
include $(CLEAR_VARS)
LOCAL_MODULE := espresso-intents
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := 15
LOCAL_STATIC_JAVA_LIBRARIES := espresso-intents-nodep espresso-core android-support-test-rules-nodep 
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := espresso-intents-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso_intents_release_no_deps.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_SDK_VERSION := 15
include $(BUILD_PREBUILT)

# for espresso-web
include $(CLEAR_VARS)
LOCAL_MODULE := espresso-web
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := 15
LOCAL_STATIC_JAVA_LIBRARIES := espresso-core android-support-annotations tagsoup
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := espresso-web-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso_web_release_no_deps.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_SDK_VERSION := 15
include $(BUILD_PREBUILT)
